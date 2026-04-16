#ifndef VOCABULARY_DECK_STORE_H
#define VOCABULARY_DECK_STORE_H

#include "vocabulary_types.h"

#include <cstddef>

class DeckStore {
public:
    DeckStore();

    const DeckManifest& Manifest() const;
    size_t Count() const;
    const WordCard* Get(size_t index) const;

private:
    DeckManifest manifest_;
};

#endif  // VOCABULARY_DECK_STORE_H
