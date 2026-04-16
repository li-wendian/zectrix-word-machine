#ifndef VOCABULARY_PROGRESS_STORE_H
#define VOCABULARY_PROGRESS_STORE_H

#include "vocabulary_types.h"

#include <cstddef>
#include <vector>

class ProgressStore {
public:
    bool Load(size_t word_count,
              std::vector<ReviewState>* states,
              VocabularyDailyStats* stats);
    bool Save(const std::vector<ReviewState>& states,
              const VocabularyDailyStats& stats);
    bool LoadSettings(VocabularySettings* settings);
    bool SaveSettings(const VocabularySettings& settings);
    void ResetDailyStatsIfNeeded(int32_t today,
                                 std::vector<ReviewState>* states,
                                 VocabularyDailyStats* stats);
};

#endif  // VOCABULARY_PROGRESS_STORE_H
