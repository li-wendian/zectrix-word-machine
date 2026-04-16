#include "vocabulary_app.h"

#include "display.h"
#include "audio_codec.h"
#include "board.h"
#include "boards/zectrix/zectrix_nfc.h"
#include "boards/zectrix-s3-epaper-4.2/rtc_pcf8563.h"

#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" RtcPcf8563* ZectrixGetRtc();
extern "C" ZectrixNfc* ZectrixGetNfc();

namespace {

constexpr char kTag[] = "VocabularyApp";
constexpr char kNfcEntryUrl[] = "https://www.zectrix.com/vocab";
constexpr int64_t kComboLongPressWindowMs = 1400;
constexpr uint8_t kSettingsItemCount = 2;
constexpr size_t kAgainDelayCards = 3;

int32_t DaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int32_t>(era * 146097 + static_cast<int>(doe) - 719468);
}

const char* StatusText(ReviewStatus status) {
    switch (status) {
        case ReviewStatus::kNew:
            return "new";
        case ReviewStatus::kLearning:
            return "learning";
        case ReviewStatus::kReview:
            return "review";
        case ReviewStatus::kMastered:
            return "mastered";
        default:
            return "new";
    }
}

const char* OrderModeText(VocabularyOrderMode mode) {
    switch (mode) {
        case VocabularyOrderMode::kSequential:
            return "Sequential";
        case VocabularyOrderMode::kRandom:
            return "Random";
        default:
            return "Sequential";
    }
}

void CopyText(char* out, size_t out_size, const char* text) {
    if (out == nullptr || out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", text != nullptr ? text : "");
}

}  // namespace

void VocabularyApp::Initialize(Display* display) {
    std::lock_guard<std::mutex> lock(mutex_);
    display_ = display;
    today_ = TodayDayIndex();

    const size_t word_count = deck_store_.Count();
    if (!progress_store_.Load(word_count, &states_, &stats_)) {
        states_.assign(word_count, ReviewState{});
        stats_ = VocabularyDailyStats{};
    }
    (void)progress_store_.LoadSettings(&settings_);
    progress_store_.ResetDailyStatsIfNeeded(today_, &states_, &stats_);
    RebuildQueueLocked();
    WriteNfcEntryLink();
    PublishSnapshotLocked(true);
}

void VocabularyApp::HandleButton(VocabularyButton button) {
    std::lock_guard<std::mutex> lock(mutex_);
    const int64_t now_ms = esp_timer_get_time() / 1000;

    if (settings_open_) {
        HandleSettingsButtonLocked(button);
        PublishSnapshotLocked(false);
        return;
    }

    switch (button) {
        case VocabularyButton::kConfirmClick:
            if (queue_.empty()) {
                RebuildQueueLocked();
            } else if (!showing_back_) {
                showing_back_ = true;
            } else {
                ScoreCurrentLocked(true);
            }
            break;
        case VocabularyButton::kConfirmLongPress:
            if (!queue_.empty()) {
                DeferCurrentLocked();
            }
            break;
        case VocabularyButton::kUpClick:
            if (showing_back_) {
                MissCurrentLocked();
            } else {
                MoveCardLocked(-1);
            }
            break;
        case VocabularyButton::kDownClick:
            if (showing_back_) {
                DeferCurrentLocked();
            } else {
                MoveCardLocked(1);
            }
            break;
        case VocabularyButton::kUpLongPress:
            last_up_long_ms_ = now_ms;
            if ((now_ms - last_down_long_ms_) <= kComboLongPressWindowMs) {
                EnterSettingsLocked();
            }
            break;
        case VocabularyButton::kDownLongPress:
            last_down_long_ms_ = now_ms;
            if ((now_ms - last_up_long_ms_) <= kComboLongPressWindowMs) {
                EnterSettingsLocked();
            }
            break;
    }

    PublishSnapshotLocked(false);
}

VocabularySnapshot VocabularyApp::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

int32_t VocabularyApp::TodayDayIndex() const {
    RtcPcf8563* rtc = ZectrixGetRtc();
    tm local_tm = {};
    if (rtc != nullptr && rtc->GetTime(local_tm)) {
        return DaysFromCivil(local_tm.tm_year + 1900,
                             static_cast<unsigned>(local_tm.tm_mon + 1),
                             static_cast<unsigned>(local_tm.tm_mday));
    }

    const int32_t fallback_start = DaysFromCivil(2026, 1, 1);
    return fallback_start + static_cast<int32_t>(esp_timer_get_time() / 1000000LL / 86400LL);
}

void VocabularyApp::RebuildQueueLocked() {
    queue_ = scheduler_.BuildQueue(states_, today_, settings_, stats_.reviewed);
    queue_pos_ = 0;
    showing_back_ = false;
}

void VocabularyApp::PublishSnapshotLocked(bool full_refresh) {
    snapshot_ = BuildSnapshotLocked();
    if (display_ == nullptr) {
        return;
    }
    display_->UpdateVocabularySnapshot(snapshot_);
    if (full_refresh) {
        display_->RequestUrgentFullRefresh();
    } else {
        display_->RequestUrgentRefresh();
    }
}

VocabularySnapshot VocabularyApp::BuildSnapshotLocked() const {
    VocabularySnapshot snapshot;
    snapshot.total_count = static_cast<uint16_t>(std::min<size_t>(deck_store_.Count(), UINT16_MAX));
    snapshot.due_count = scheduler_.CountDue(states_, today_);
    snapshot.reviewed_today = stats_.reviewed;
    snapshot.correct_today = stats_.correct;
    snapshot.streak_days = static_cast<uint16_t>(
        std::min<uint32_t>(static_cast<uint32_t>(stats_.streak_days) +
                               (stats_.reviewed > 0 ? 1U : 0U),
                           UINT16_MAX));
    snapshot.card_count = static_cast<uint16_t>(std::min<size_t>(queue_.size(), UINT16_MAX));
    snapshot.card_position = queue_.empty()
        ? 0
        : static_cast<uint16_t>(std::min<size_t>(queue_pos_ + 1, UINT16_MAX));

    const DeckManifest& manifest = deck_store_.Manifest();
    if (manifest.version > 0) {
        snprintf(snapshot.header, sizeof(snapshot.header), "%s v%u",
                 manifest.name, static_cast<unsigned>(manifest.version));
    } else {
        snprintf(snapshot.header, sizeof(snapshot.header), "%s", manifest.name);
    }
    snprintf(snapshot.progress, sizeof(snapshot.progress), "%u/%u  OK %u",
             static_cast<unsigned>(stats_.reviewed),
             static_cast<unsigned>(settings_.daily_target),
             static_cast<unsigned>(stats_.correct));

    if (settings_open_) {
        const bool order_selected = settings_selection_ == 0;
        const bool target_selected = settings_selection_ == 1;
        snapshot.screen = VocabularyScreen::kSettings;
        snprintf(snapshot.word, sizeof(snapshot.word), "Settings");
        snprintf(snapshot.meaning, sizeof(snapshot.meaning), "Deck: %s (%u words)",
                 manifest.name, static_cast<unsigned>(snapshot.total_count));
        snprintf(snapshot.example, sizeof(snapshot.example), "%c Order: %s",
                 order_selected ? '>' : ' ', OrderModeText(settings_.order_mode));
        snprintf(snapshot.example_translation, sizeof(snapshot.example_translation),
                 "%c Daily target: %u words",
                 target_selected ? '>' : ' ', static_cast<unsigned>(settings_.daily_target));
        snapshot.hint[0] = '\0';
        snprintf(snapshot.footer, sizeof(snapshot.footer), "Sync: %s", sync_client_.LastStatus());
        return snapshot;
    }

    if (queue_.empty()) {
        snapshot.screen = VocabularyScreen::kDailySummary;
        snprintf(snapshot.word, sizeof(snapshot.word), "Done");
        snprintf(snapshot.meaning, sizeof(snapshot.meaning), "Today's target is complete.");
        snprintf(snapshot.example, sizeof(snapshot.example), "Reviewed %u, correct %u, streak %u day(s).",
                 static_cast<unsigned>(stats_.reviewed),
                 static_cast<unsigned>(stats_.correct),
                 static_cast<unsigned>(snapshot.streak_days));
        snapshot.hint[0] = '\0';
        snapshot.footer[0] = '\0';
        return snapshot;
    }

    const size_t word_index = queue_[std::min(queue_pos_, queue_.size() - 1)];
    const WordCard* card = deck_store_.Get(word_index);
    if (card == nullptr || word_index >= states_.size()) {
        snapshot.screen = VocabularyScreen::kDailySummary;
        snprintf(snapshot.word, sizeof(snapshot.word), "Deck error");
        snprintf(snapshot.meaning, sizeof(snapshot.meaning), "Fallback queue is empty.");
        snprintf(snapshot.footer, sizeof(snapshot.footer), "Restart or reflash the deck.");
        return snapshot;
    }

    const ReviewState& state = states_[word_index];
    snapshot.has_card = true;
    snapshot.showing_back = showing_back_;
    snapshot.status = state.status;
    snapshot.interval_days = state.interval_days;
    snapshot.lapses = state.lapses;
    snapshot.screen = showing_back_ ? VocabularyScreen::kCardBack : VocabularyScreen::kCardFront;
    CopyText(snapshot.word, sizeof(snapshot.word), card->word);
    CopyText(snapshot.phonetic, sizeof(snapshot.phonetic), card->phonetic);
    CopyText(snapshot.meaning, sizeof(snapshot.meaning), card->meaning);
    CopyText(snapshot.example, sizeof(snapshot.example), card->example);
    CopyText(snapshot.example_translation, sizeof(snapshot.example_translation), card->example_translation);
    snprintf(snapshot.status_line, sizeof(snapshot.status_line), "%s %ud %u",
             StatusText(state.status),
             static_cast<unsigned>(state.interval_days),
             static_cast<unsigned>(state.lapses));
    snapshot.hint[0] = '\0';
    snapshot.footer[0] = '\0';
    return snapshot;
}

void VocabularyApp::ScoreCurrentLocked(bool known) {
    if (queue_.empty() || queue_pos_ >= queue_.size()) {
        return;
    }

    const size_t word_index = queue_[queue_pos_];
    if (word_index >= states_.size()) {
        return;
    }

    scheduler_.ApplyAnswer(&states_[word_index], known, today_);
    stats_.reviewed = static_cast<uint16_t>(
        std::min<uint32_t>(static_cast<uint32_t>(stats_.reviewed) + 1, UINT16_MAX));
    if (known) {
        stats_.correct = static_cast<uint16_t>(
            std::min<uint32_t>(static_cast<uint32_t>(stats_.correct) + 1, UINT16_MAX));
    }
    sync_client_.MarkPending();
    (void)progress_store_.Save(states_, stats_);

    queue_.erase(queue_.begin() + static_cast<std::ptrdiff_t>(queue_pos_));
    if (!queue_.empty() && queue_pos_ >= queue_.size()) {
        queue_pos_ = queue_.size() - 1;
    }
    PlayCueLocked(known, queue_.empty());
    showing_back_ = false;
}

void VocabularyApp::MissCurrentLocked() {
    if (queue_.empty() || queue_pos_ >= queue_.size()) {
        return;
    }

    const size_t word_index = queue_[queue_pos_];
    if (word_index >= states_.size()) {
        return;
    }

    scheduler_.ApplyAnswer(&states_[word_index], false, today_);
    sync_client_.MarkPending();
    (void)progress_store_.Save(states_, stats_);

    RequeueCurrentIndexLocked(word_index);
    PlayCueLocked(false, false);
}

void VocabularyApp::DeferCurrentLocked() {
    if (queue_.empty() || queue_pos_ >= queue_.size()) {
        return;
    }

    const size_t word_index = queue_[queue_pos_];
    RequeueCurrentIndexLocked(word_index);
}

void VocabularyApp::RequeueCurrentIndexLocked(size_t word_index) {
    queue_.erase(queue_.begin() + static_cast<std::ptrdiff_t>(queue_pos_));
    if (queue_.empty()) {
        queue_.push_back(word_index);
        queue_pos_ = 0;
        showing_back_ = false;
        return;
    }

    if (queue_pos_ >= queue_.size()) {
        queue_pos_ = 0;
    }
    const size_t insert_pos = std::min(queue_pos_ + kAgainDelayCards, queue_.size());
    queue_.insert(queue_.begin() + static_cast<std::ptrdiff_t>(insert_pos), word_index);
    showing_back_ = false;
}

void VocabularyApp::MoveCardLocked(int delta) {
    if (queue_.empty()) {
        return;
    }
    if (delta < 0) {
        queue_pos_ = (queue_pos_ == 0) ? queue_.size() - 1 : queue_pos_ - 1;
    } else if (delta > 0) {
        queue_pos_ = (queue_pos_ + 1) % queue_.size();
    }
    showing_back_ = false;
}

void VocabularyApp::EnterSettingsLocked() {
    settings_open_ = true;
    settings_selection_ = 0;
    showing_back_ = false;
    (void)sync_client_.TrySync(deck_store_.Manifest(), stats_);
}

void VocabularyApp::HandleSettingsButtonLocked(VocabularyButton button) {
    switch (button) {
        case VocabularyButton::kConfirmClick:
            settings_selection_ = static_cast<uint8_t>((settings_selection_ + 1) % kSettingsItemCount);
            break;
        case VocabularyButton::kConfirmLongPress:
            settings_open_ = false;
            showing_back_ = false;
            RebuildQueueLocked();
            break;
        case VocabularyButton::kUpClick:
            ChangeSettingsValueLocked(-1);
            break;
        case VocabularyButton::kDownClick:
            ChangeSettingsValueLocked(1);
            break;
        case VocabularyButton::kUpLongPress:
            ChangeSettingsValueLocked(settings_selection_ == 1 ? -10 : -1);
            break;
        case VocabularyButton::kDownLongPress:
            ChangeSettingsValueLocked(settings_selection_ == 1 ? 10 : 1);
            break;
    }
}

void VocabularyApp::ChangeSettingsValueLocked(int delta) {
    if (settings_selection_ == 0) {
        settings_.order_mode = settings_.order_mode == VocabularyOrderMode::kSequential
            ? VocabularyOrderMode::kRandom
            : VocabularyOrderMode::kSequential;
    } else {
        const int next_target = static_cast<int>(settings_.daily_target) + delta;
        settings_.daily_target = static_cast<uint16_t>(
            std::min<int>(ReviewScheduler::kMaxDailyTarget,
                          std::max<int>(ReviewScheduler::kMinDailyTarget, next_target)));
    }
    SaveSettingsAndRebuildQueueLocked();
}

void VocabularyApp::SaveSettingsAndRebuildQueueLocked() {
    (void)progress_store_.SaveSettings(settings_);
    RebuildQueueLocked();
}

void VocabularyApp::WriteNfcEntryLink() {
    ZectrixNfc* nfc = ZectrixGetNfc();
    if (nfc == nullptr) {
        ESP_LOGW(kTag, "NFC unavailable for entry link");
        return;
    }
    if (!nfc->IsPowered() && !nfc->PowerOn()) {
        ESP_LOGW(kTag, "NFC power on failed");
        return;
    }
    const esp_err_t ret = nfc->WriteUriNdef(kNfcEntryUrl);
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "NFC entry write failed: %s", esp_err_to_name(ret));
    }
}

void VocabularyApp::PlayCueLocked(bool known, bool complete) {
    AudioCodec* codec = Board::GetInstance().GetAudioCodec();
    if (codec == nullptr) {
        return;
    }

    const int sample_rate = codec->output_sample_rate() > 0 ? codec->output_sample_rate() : 16000;
    const int duration_ms = complete ? 180 : (known ? 70 : 120);
    const int frequency = complete ? 1040 : (known ? 880 : 220);
    const int samples = sample_rate * duration_ms / 1000;
    const int half_period = std::max(1, sample_rate / (frequency * 2));

    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        const bool high = ((i / half_period) % 2) == 0;
        const int amplitude = complete ? 5000 : (known ? 3800 : 3200);
        pcm[i] = static_cast<int16_t>(high ? amplitude : -amplitude);
    }

    const bool output_was_enabled = codec->output_enabled();
    if (!output_was_enabled) {
        codec->EnableOutput(true);
    }
    codec->OutputData(pcm);
    if (!output_was_enabled) {
        codec->EnableOutput(false);
    }
}
