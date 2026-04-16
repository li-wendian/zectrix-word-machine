#include "progress_store.h"

#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>

#include <algorithm>
#include <cstring>

namespace {

constexpr char kTag[] = "ProgressStore";
constexpr char kNamespace[] = "vocab";
constexpr char kBlobKey[] = "progress";
constexpr char kSettingsOrderKey[] = "order";
constexpr char kSettingsDailyTargetKey[] = "daily_target";
constexpr uint32_t kMagic = 0x56434152;  // VCAR
constexpr uint16_t kVersion = 4;

struct ProgressHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    int32_t stats_day;
    uint16_t reviewed_today;
    uint16_t correct_today;
    uint16_t streak_days;
    uint16_t reserved;
};

struct PackedReviewState {
    int32_t due_day;
    int32_t last_review_day;
    uint16_t interval_days;
    uint8_t status;
    uint8_t correct_streak;
    uint16_t lapses;
    uint16_t reserved;
};

ReviewStatus ClampStatus(uint8_t raw) {
    switch (static_cast<ReviewStatus>(raw)) {
        case ReviewStatus::kNew:
        case ReviewStatus::kLearning:
        case ReviewStatus::kReview:
        case ReviewStatus::kMastered:
            return static_cast<ReviewStatus>(raw);
        default:
            return ReviewStatus::kNew;
    }
}

ReviewState DefaultState() {
    ReviewState state;
    state.status = ReviewStatus::kNew;
    state.due_day = 0;
    return state;
}

VocabularyOrderMode ClampOrderMode(uint8_t raw) {
    switch (static_cast<VocabularyOrderMode>(raw)) {
        case VocabularyOrderMode::kSequential:
        case VocabularyOrderMode::kRandom:
            return static_cast<VocabularyOrderMode>(raw);
        default:
            return VocabularyOrderMode::kSequential;
    }
}

uint16_t ClampDailyTarget(uint16_t target) {
    if (target < 1) {
        return 1;
    }
    if (target > 200) {
        return 200;
    }
    return target;
}

}  // namespace

bool ProgressStore::Load(size_t word_count,
                         std::vector<ReviewState>* states,
                         VocabularyDailyStats* stats) {
    if (states == nullptr || stats == nullptr) {
        return false;
    }

    states->assign(word_count, DefaultState());
    *stats = VocabularyDailyStats{};

    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "open failed: %s", esp_err_to_name(ret));
        return false;
    }

    size_t blob_size = 0;
    ret = nvs_get_blob(handle, kBlobKey, nullptr, &blob_size);
    if (ret != ESP_OK || blob_size < sizeof(ProgressHeader)) {
        nvs_close(handle);
        return true;
    }

    std::vector<uint8_t> blob(blob_size);
    ret = nvs_get_blob(handle, kBlobKey, blob.data(), &blob_size);
    nvs_close(handle);
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "read failed: %s", esp_err_to_name(ret));
        return false;
    }

    ProgressHeader header = {};
    memcpy(&header, blob.data(), sizeof(header));
    if (header.magic != kMagic || header.version != kVersion) {
        ESP_LOGW(kTag, "ignore incompatible progress blob");
        return true;
    }

    stats->day = header.stats_day;
    stats->reviewed = header.reviewed_today;
    stats->correct = header.correct_today;
    stats->streak_days = header.streak_days;

    const size_t available_count =
        (blob.size() - sizeof(ProgressHeader)) / sizeof(PackedReviewState);
    const size_t copy_count =
        std::min({word_count, static_cast<size_t>(header.count), available_count});
    for (size_t i = 0; i < copy_count; ++i) {
        PackedReviewState packed = {};
        memcpy(&packed,
               blob.data() + sizeof(ProgressHeader) + i * sizeof(PackedReviewState),
               sizeof(packed));
        ReviewState state;
        state.status = ClampStatus(packed.status);
        state.due_day = packed.due_day;
        state.last_review_day = packed.last_review_day;
        state.interval_days = packed.interval_days;
        state.correct_streak = packed.correct_streak;
        state.lapses = packed.lapses;
        (*states)[i] = state;
    }

    return true;
}

bool ProgressStore::Save(const std::vector<ReviewState>& states,
                         const VocabularyDailyStats& stats) {
    ProgressHeader header = {};
    header.magic = kMagic;
    header.version = kVersion;
    header.count = static_cast<uint16_t>(std::min<size_t>(states.size(), UINT16_MAX));
    header.stats_day = stats.day;
    header.reviewed_today = stats.reviewed;
    header.correct_today = stats.correct;
    header.streak_days = stats.streak_days;

    const size_t packed_count = header.count;
    std::vector<uint8_t> blob(sizeof(ProgressHeader) + packed_count * sizeof(PackedReviewState));
    memcpy(blob.data(), &header, sizeof(header));
    for (size_t i = 0; i < packed_count; ++i) {
        PackedReviewState packed = {};
        packed.due_day = states[i].due_day;
        packed.last_review_day = states[i].last_review_day;
        packed.interval_days = states[i].interval_days;
        packed.status = static_cast<uint8_t>(states[i].status);
        packed.correct_streak = states[i].correct_streak;
        packed.lapses = states[i].lapses;
        memcpy(blob.data() + sizeof(ProgressHeader) + i * sizeof(PackedReviewState),
               &packed,
               sizeof(packed));
    }

    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "open write failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = nvs_set_blob(handle, kBlobKey, blob.data(), blob.size());
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "save failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool ProgressStore::LoadSettings(VocabularySettings* settings) {
    if (settings == nullptr) {
        return false;
    }

    *settings = VocabularySettings{};

    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "open settings failed: %s", esp_err_to_name(ret));
        return false;
    }

    uint8_t order = static_cast<uint8_t>(settings->order_mode);
    ret = nvs_get_u8(handle, kSettingsOrderKey, &order);
    if (ret == ESP_OK) {
        settings->order_mode = ClampOrderMode(order);
    } else if (ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(kTag, "read order failed: %s", esp_err_to_name(ret));
    }

    uint16_t daily_target = settings->daily_target;
    ret = nvs_get_u16(handle, kSettingsDailyTargetKey, &daily_target);
    if (ret == ESP_OK) {
        settings->daily_target = ClampDailyTarget(daily_target);
    } else if (ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(kTag, "read daily target failed: %s", esp_err_to_name(ret));
    }

    nvs_close(handle);
    return true;
}

bool ProgressStore::SaveSettings(const VocabularySettings& settings) {
    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "open settings write failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = nvs_set_u8(handle, kSettingsOrderKey, static_cast<uint8_t>(settings.order_mode));
    if (ret == ESP_OK) {
        ret = nvs_set_u16(handle, kSettingsDailyTargetKey, ClampDailyTarget(settings.daily_target));
    }
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);

    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "save settings failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

void ProgressStore::ResetDailyStatsIfNeeded(int32_t today,
                                            std::vector<ReviewState>* states,
                                            VocabularyDailyStats* stats) {
    if (states == nullptr || stats == nullptr) {
        return;
    }
    if (stats->day == today) {
        return;
    }

    if (stats->day != 0 && stats->reviewed > 0) {
        stats->streak_days = static_cast<uint16_t>(
            std::min<int>(static_cast<int>(stats->streak_days) + 1, UINT16_MAX));
    }
    stats->day = today;
    stats->reviewed = 0;
    stats->correct = 0;

    for (auto& state : *states) {
        if (state.due_day < 0) {
            state.due_day = today;
        }
    }
}
