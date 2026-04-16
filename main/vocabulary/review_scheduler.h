#ifndef VOCABULARY_REVIEW_SCHEDULER_H
#define VOCABULARY_REVIEW_SCHEDULER_H

#include "vocabulary_types.h"

#include <cstddef>
#include <vector>

class ReviewScheduler {
public:
    static constexpr uint16_t kDefaultDailyTarget = 20;
    static constexpr uint16_t kMinDailyTarget = 1;
    static constexpr uint16_t kMaxDailyTarget = 200;

    std::vector<size_t> BuildQueue(const std::vector<ReviewState>& states,
                                   int32_t today,
                                   const VocabularySettings& settings,
                                   uint16_t reviewed_today) const;
    void ApplyAnswer(ReviewState* state, bool known, int32_t today) const;
    uint16_t CountDue(const std::vector<ReviewState>& states, int32_t today) const;
};

#endif  // VOCABULARY_REVIEW_SCHEDULER_H
