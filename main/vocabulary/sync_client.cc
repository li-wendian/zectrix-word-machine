#include "sync_client.h"

void SyncClient::MarkPending() {
    pending_ = true;
    last_status_ = "pending";
}

bool SyncClient::HasPending() const {
    return pending_;
}

bool SyncClient::TrySync(const DeckManifest& manifest, const VocabularyDailyStats& stats) {
    (void)manifest;
    (void)stats;
    last_status_ = pending_ ? "pending wifi" : "offline";
    return false;
}

const char* SyncClient::LastStatus() const {
    return last_status_;
}
