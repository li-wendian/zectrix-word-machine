#ifndef VOCABULARY_TYPES_H
#define VOCABULARY_TYPES_H

#include <cstddef>
#include <cstdint>

enum class ReviewStatus : uint8_t {
    kNew = 0,
    kLearning,
    kReview,
    kMastered,
};

enum class VocabularyScreen : uint8_t {
    kCardFront = 0,
    kCardBack,
    kDailySummary,
    kSettings,
};

enum class VocabularyButton : uint8_t {
    kConfirmClick = 0,
    kConfirmLongPress,
    kUpClick,
    kDownClick,
    kUpLongPress,
    kDownLongPress,
};

enum class VocabularyOrderMode : uint8_t {
    kSequential = 0,
    kRandom,
};

struct WordCard {
    uint16_t id = 0;
    const char* deck_id = nullptr;
    const char* word = nullptr;
    const char* phonetic = nullptr;
    const char* meaning = nullptr;
    const char* example = nullptr;
    const char* example_translation = nullptr;
};

struct ReviewState {
    ReviewStatus status = ReviewStatus::kNew;
    int32_t due_day = 0;
    int32_t last_review_day = 0;
    uint16_t interval_days = 0;
    uint8_t correct_streak = 0;
    uint16_t lapses = 0;
};

struct DeckManifest {
    const char* deck_id = nullptr;
    const char* name = nullptr;
    const char* language = nullptr;
    uint16_t version = 1;
    size_t word_count = 0;
};

struct VocabularySettings {
    VocabularyOrderMode order_mode = VocabularyOrderMode::kSequential;
    uint16_t daily_target = 20;
};

struct VocabularyDailyStats {
    int32_t day = 0;
    uint16_t reviewed = 0;
    uint16_t correct = 0;
    uint16_t streak_days = 0;
};

struct VocabularySnapshot {
    VocabularyScreen screen = VocabularyScreen::kCardFront;
    bool has_card = false;
    bool showing_back = false;
    ReviewStatus status = ReviewStatus::kNew;
    uint16_t interval_days = 0;
    uint16_t lapses = 0;
    uint16_t reviewed_today = 0;
    uint16_t correct_today = 0;
    uint16_t streak_days = 0;
    uint16_t due_count = 0;
    uint16_t total_count = 0;
    uint16_t card_position = 0;
    uint16_t card_count = 0;
    char header[64] = {0};
    char word[48] = {0};
    char phonetic[64] = {0};
    char meaning[192] = {0};
    char example[448] = {0};
    char example_translation[256] = {0};
    char progress[96] = {0};
    char hint[128] = {0};
    char footer[128] = {0};
    char status_line[96] = {0};
};

#endif  // VOCABULARY_TYPES_H
