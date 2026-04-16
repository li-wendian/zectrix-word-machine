#ifndef VOCABULARY_SYNC_CLIENT_H
#define VOCABULARY_SYNC_CLIENT_H

#include "vocabulary_types.h"

class SyncClient {
public:
    void MarkPending();
    bool HasPending() const;
    bool TrySync(const DeckManifest& manifest, const VocabularyDailyStats& stats);
    const char* LastStatus() const;

private:
    bool pending_ = false;
    const char* last_status_ = "offline";
};

#endif  // VOCABULARY_SYNC_CLIENT_H
