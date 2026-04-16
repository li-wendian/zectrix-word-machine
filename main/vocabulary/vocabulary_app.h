#ifndef VOCABULARY_APP_H
#define VOCABULARY_APP_H

#include "deck_store.h"
#include "progress_store.h"
#include "review_scheduler.h"
#include "sync_client.h"
#include "vocabulary_types.h"

#include <mutex>
#include <vector>

class Display;

class VocabularyApp {
public:
    void Initialize(Display* display);
    void HandleButton(VocabularyButton button);
    VocabularySnapshot GetSnapshot() const;

private:
    int32_t TodayDayIndex() const;
    void RebuildQueueLocked();
    void PublishSnapshotLocked(bool full_refresh);
    VocabularySnapshot BuildSnapshotLocked() const;
    void ScoreCurrentLocked(bool known);
    void DeferCurrentLocked();
    void MoveCardLocked(int delta);
    void EnterSettingsLocked();
    void HandleSettingsButtonLocked(VocabularyButton button);
    void ChangeSettingsValueLocked(int delta);
    void SaveSettingsAndRebuildQueueLocked();
    void WriteNfcEntryLink();
    void PlayCueLocked(bool known, bool complete);

    Display* display_ = nullptr;
    DeckStore deck_store_;
    ProgressStore progress_store_;
    ReviewScheduler scheduler_;
    SyncClient sync_client_;
    std::vector<ReviewState> states_;
    std::vector<size_t> queue_;
    VocabularySettings settings_;
    VocabularyDailyStats stats_;
    size_t queue_pos_ = 0;
    bool showing_back_ = false;
    bool settings_open_ = false;
    uint8_t settings_selection_ = 0;
    int32_t today_ = 0;
    int64_t last_up_long_ms_ = 0;
    int64_t last_down_long_ms_ = 0;
    mutable std::mutex mutex_;
    VocabularySnapshot snapshot_;
};

#endif  // VOCABULARY_APP_H
