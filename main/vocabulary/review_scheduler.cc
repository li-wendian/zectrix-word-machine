#include "review_scheduler.h"

#include <algorithm>

namespace {

bool IsDue(const ReviewState& state, int32_t today) {
    if (state.status == ReviewStatus::kNew) {
        return true;
    }
    return state.due_day <= today;
}

uint16_t ClampInterval(uint32_t days) {
    return static_cast<uint16_t>(std::min<uint32_t>(days, 180));
}

uint16_t ClampDailyTarget(uint16_t target) {
    if (target < ReviewScheduler::kMinDailyTarget) {
        return ReviewScheduler::kMinDailyTarget;
    }
    if (target > ReviewScheduler::kMaxDailyTarget) {
        return ReviewScheduler::kMaxDailyTarget;
    }
    return target;
}

uint32_t NextRandom(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x == 0 ? 0x6D2B79F5U : x;
    return *state;
}

void Shuffle(std::vector<size_t>* items, uint32_t seed) {
    if (items == nullptr || items->size() < 2) {
        return;
    }
    uint32_t state = seed == 0 ? 0x9E3779B9U : seed;
    for (size_t i = items->size() - 1; i > 0; --i) {
        const size_t j = NextRandom(&state) % (i + 1);
        std::swap((*items)[i], (*items)[j]);
    }
}

}  // namespace

std::vector<size_t> ReviewScheduler::BuildQueue(const std::vector<ReviewState>& states,
                                                int32_t today,
                                                const VocabularySettings& settings,
                                                uint16_t reviewed_today) const {
    std::vector<size_t> queue;
    const uint16_t daily_target = ClampDailyTarget(settings.daily_target);
    if (reviewed_today >= daily_target) {
        return queue;
    }

    const size_t remaining_today = static_cast<size_t>(daily_target - reviewed_today);
    queue.reserve(remaining_today);

    std::vector<size_t> due_reviews;
    std::vector<size_t> new_words;
    due_reviews.reserve(remaining_today);
    new_words.reserve(remaining_today);

    for (size_t i = 0; i < states.size(); ++i) {
        const ReviewState& state = states[i];
        if (state.status != ReviewStatus::kNew && state.due_day <= today) {
            due_reviews.push_back(i);
        } else if (state.status == ReviewStatus::kNew) {
            new_words.push_back(i);
        }
    }

    if (settings.order_mode == VocabularyOrderMode::kRandom) {
        const uint32_t seed = 0xA511E9B3U ^
            static_cast<uint32_t>(today) ^
            (static_cast<uint32_t>(states.size()) << 1);
        Shuffle(&due_reviews, seed ^ 0xB5297A4DU);
        Shuffle(&new_words, seed ^ 0x68E31DA4U);
    }

    for (size_t index : due_reviews) {
        if (queue.size() >= remaining_today) {
            return queue;
        }
        queue.push_back(index);
    }

    for (size_t index : new_words) {
        if (queue.size() >= remaining_today) {
            break;
        }
        queue.push_back(index);
    }

    return queue;
}

void ReviewScheduler::ApplyAnswer(ReviewState* state, bool known, int32_t today) const {
    if (state == nullptr) {
        return;
    }

    state->last_review_day = today;
    if (!known) {
        state->status = ReviewStatus::kLearning;
        state->correct_streak = 0;
        state->lapses = ClampInterval(static_cast<uint32_t>(state->lapses) + 1);
        state->interval_days = 0;
        state->due_day = today;
        return;
    }

    state->correct_streak = static_cast<uint8_t>(
        std::min<uint32_t>(static_cast<uint32_t>(state->correct_streak) + 1, UINT8_MAX));

    switch (state->status) {
        case ReviewStatus::kNew:
            state->status = ReviewStatus::kLearning;
            state->interval_days = 1;
            break;
        case ReviewStatus::kLearning:
            state->status = state->correct_streak >= 2 ? ReviewStatus::kReview : ReviewStatus::kLearning;
            state->interval_days = state->correct_streak >= 2 ? 3 : 1;
            break;
        case ReviewStatus::kReview:
            state->interval_days = ClampInterval(std::max<uint32_t>(3, state->interval_days * 2U));
            if (state->correct_streak >= 5 && state->interval_days >= 30) {
                state->status = ReviewStatus::kMastered;
            }
            break;
        case ReviewStatus::kMastered:
            state->interval_days = 90;
            break;
    }

    state->due_day = today + state->interval_days;
}

uint16_t ReviewScheduler::CountDue(const std::vector<ReviewState>& states, int32_t today) const {
    uint32_t due = 0;
    for (const auto& state : states) {
        if (IsDue(state, today)) {
            ++due;
        }
    }
    return static_cast<uint16_t>(std::min<uint32_t>(due, UINT16_MAX));
}
