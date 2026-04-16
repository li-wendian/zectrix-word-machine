#include "deck_store.h"

namespace {

constexpr const char* kDeckId = "kaoyan-5047-local";

const WordCard kKaoyanFullDeck[] = {
#include "kaoyan_full_deck.inc"
};

}  // namespace

DeckStore::DeckStore() {
    manifest_.deck_id = kDeckId;
    manifest_.name = "Kaoyan 5047 Local";
    manifest_.language = "en-zh";
    manifest_.version = 0;
    manifest_.word_count = Count();
}

const DeckManifest& DeckStore::Manifest() const {
    return manifest_;
}

size_t DeckStore::Count() const {
    return sizeof(kKaoyanFullDeck) / sizeof(kKaoyanFullDeck[0]);
}

const WordCard* DeckStore::Get(size_t index) const {
    if (index >= Count()) {
        return nullptr;
    }
    return &kKaoyanFullDeck[index];
}
