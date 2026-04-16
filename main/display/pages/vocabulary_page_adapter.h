#ifndef VOCABULARY_PAGE_ADAPTER_H
#define VOCABULARY_PAGE_ADAPTER_H

#include "ui_page.h"
#include "vocabulary/vocabulary_types.h"

class LcdDisplay;

class VocabularyPageAdapter : public IUiPage {
public:
    explicit VocabularyPageAdapter(LcdDisplay* host);

    UiPageId Id() const override;
    const char* Name() const override;
    void Build() override;
    lv_obj_t* Screen() const override;
    void OnShow() override;
    bool HandleEvent(const UiPageEvent& event) override;

    void UpdateSnapshot(const VocabularySnapshot& snapshot);

private:
    void ApplyCardFront();
    void ApplyCardBack();
    void ApplySummary();
    void ApplySettings();

    LcdDisplay* host_ = nullptr;
    bool built_ = false;
    VocabularySnapshot snapshot_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* header_label_ = nullptr;
    lv_obj_t* progress_label_ = nullptr;
    lv_obj_t* word_label_ = nullptr;
    lv_obj_t* phonetic_label_ = nullptr;
    lv_obj_t* meaning_label_ = nullptr;
    lv_obj_t* example_label_ = nullptr;
    lv_obj_t* translation_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* hint_label_ = nullptr;
    lv_obj_t* footer_label_ = nullptr;
};

#endif  // VOCABULARY_PAGE_ADAPTER_H
