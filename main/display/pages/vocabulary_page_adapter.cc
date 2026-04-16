#include "pages/vocabulary_page_adapter.h"

#include "lcd_display.h"
#include "lvgl_theme.h"

#include <cstdio>

LV_FONT_DECLARE(DejaVuSans_Phonetic_16);
LV_FONT_DECLARE(SourceHanSansSC_Vocab_18);

namespace {

constexpr lv_coord_t kPageWidth = 400;
constexpr lv_coord_t kPageHeight = 300;
constexpr lv_coord_t kHeaderHeight = 42;
constexpr lv_coord_t kFooterHeight = 0;

void StyleScreen(lv_obj_t* obj) {
    lv_obj_set_size(obj, kPageWidth, kPageHeight);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

void StylePanel(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_black(), 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_left(obj, 10, 0);
    lv_obj_set_style_pad_right(obj, 10, 0);
}

void StyleWordLabel(lv_obj_t* obj) {
    lv_obj_set_width(obj, LV_PCT(100));
    lv_obj_set_height(obj, 40);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(obj, 0, 0);
}

void StyleContentLabel(lv_obj_t* obj) {
    lv_obj_set_width(obj, LV_PCT(100));
    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(obj, 1, 0);
}

void SetLabel(lv_obj_t* label, const char* text) {
    if (label != nullptr) {
        lv_label_set_text(label, text != nullptr ? text : "");
    }
}

void SetVisible(lv_obj_t* obj, bool visible) {
    if (obj == nullptr) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace

VocabularyPageAdapter::VocabularyPageAdapter(LcdDisplay* host)
    : host_(host) {}

UiPageId VocabularyPageAdapter::Id() const {
    return UiPageId::Vocabulary;
}

const char* VocabularyPageAdapter::Name() const {
    return "Vocabulary";
}

void VocabularyPageAdapter::Build() {
    if (built_ || host_ == nullptr) {
        built_ = true;
        return;
    }
    if (host_->vocabulary_screen_ != nullptr) {
        screen_ = host_->vocabulary_screen_;
        built_ = true;
        return;
    }

    auto* lvgl_theme = static_cast<LvglTheme*>(host_->current_theme_);
    const lv_font_t* text_font = lvgl_theme->text_font() ? lvgl_theme->text_font()->font() : nullptr;
    const lv_font_t* body_font = lvgl_theme->reminder_text_font()
        ? lvgl_theme->reminder_text_font()->font()
        : text_font;
    const lv_font_t* word_font = body_font;
#if LV_FONT_MONTSERRAT_28
    word_font = &lv_font_montserrat_28;
#endif
    const lv_font_t* content_font = &SourceHanSansSC_Vocab_18;
    const lv_font_t* phonetic_font = &DejaVuSans_Phonetic_16;

    screen_ = lv_obj_create(nullptr);
    host_->vocabulary_screen_ = screen_;
    StyleScreen(screen_);

    lv_obj_t* header = lv_obj_create(screen_);
    StylePanel(header);
    lv_obj_set_size(header, kPageWidth, kHeaderHeight);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    header_label_ = lv_label_create(header);
    if (text_font) {
        lv_obj_set_style_text_font(header_label_, text_font, 0);
    }
    lv_obj_set_width(header_label_, 190);
    lv_label_set_long_mode(header_label_, LV_LABEL_LONG_CLIP);
    lv_obj_align(header_label_, LV_ALIGN_LEFT_MID, 0, 0);

    progress_label_ = lv_label_create(header);
    if (text_font) {
        lv_obj_set_style_text_font(progress_label_, text_font, 0);
    }
    lv_obj_set_width(progress_label_, 180);
    lv_label_set_long_mode(progress_label_, LV_LABEL_LONG_CLIP);
    lv_obj_align(progress_label_, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t* card = lv_obj_create(screen_);
    StylePanel(card);
    lv_obj_set_size(card, kPageWidth, kPageHeight - kHeaderHeight - kFooterHeight);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, kHeaderHeight);
    lv_obj_set_style_pad_top(card, 8, 0);
    lv_obj_set_style_pad_bottom(card, 6, 0);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    word_label_ = lv_label_create(card);
    if (word_font) {
        lv_obj_set_style_text_font(word_label_, word_font, 0);
    }
    StyleWordLabel(word_label_);

    phonetic_label_ = lv_label_create(card);
    if (phonetic_font) {
        lv_obj_set_style_text_font(phonetic_label_, phonetic_font, 0);
    }
    lv_obj_set_width(phonetic_label_, LV_PCT(100));
    lv_obj_set_style_text_align(phonetic_label_, LV_TEXT_ALIGN_CENTER, 0);

    meaning_label_ = lv_label_create(card);
    if (content_font) {
        lv_obj_set_style_text_font(meaning_label_, content_font, 0);
    }
    StyleContentLabel(meaning_label_);

    example_label_ = lv_label_create(card);
    if (content_font) {
        lv_obj_set_style_text_font(example_label_, content_font, 0);
    }
    StyleContentLabel(example_label_);

    translation_label_ = lv_label_create(card);
    if (content_font) {
        lv_obj_set_style_text_font(translation_label_, content_font, 0);
    }
    StyleContentLabel(translation_label_);

    status_label_ = lv_label_create(card);
    if (content_font) {
        lv_obj_set_style_text_font(status_label_, content_font, 0);
    }
    StyleContentLabel(status_label_);

    hint_label_ = lv_label_create(card);
    if (content_font) {
        lv_obj_set_style_text_font(hint_label_, content_font, 0);
    }
    StyleContentLabel(hint_label_);

    built_ = true;
    UpdateSnapshot(snapshot_);
}

lv_obj_t* VocabularyPageAdapter::Screen() const {
    return screen_;
}

void VocabularyPageAdapter::OnShow() {
    UpdateSnapshot(snapshot_);
}

bool VocabularyPageAdapter::HandleEvent(const UiPageEvent& event) {
    (void)event;
    return false;
}

void VocabularyPageAdapter::UpdateSnapshot(const VocabularySnapshot& snapshot) {
    snapshot_ = snapshot;
    if (!built_ || screen_ == nullptr) {
        return;
    }

    SetLabel(header_label_, snapshot_.header);
    SetLabel(progress_label_, snapshot_.progress);
    SetLabel(footer_label_, snapshot_.footer);

    switch (snapshot_.screen) {
        case VocabularyScreen::kCardFront:
            ApplyCardFront();
            break;
        case VocabularyScreen::kCardBack:
            ApplyCardBack();
            break;
        case VocabularyScreen::kDailySummary:
            ApplySummary();
            break;
        case VocabularyScreen::kSettings:
            ApplySettings();
            break;
    }
}

void VocabularyPageAdapter::ApplyCardFront() {
    SetVisible(phonetic_label_, true);
    SetVisible(meaning_label_, false);
    SetVisible(example_label_, false);
    SetVisible(translation_label_, false);
    SetVisible(status_label_, false);
    SetVisible(hint_label_, false);

    SetLabel(word_label_, snapshot_.word);
    SetLabel(phonetic_label_, snapshot_.phonetic);
    SetLabel(status_label_, snapshot_.status_line);
    SetLabel(hint_label_, snapshot_.hint);
}

void VocabularyPageAdapter::ApplyCardBack() {
    SetVisible(phonetic_label_, true);
    SetVisible(meaning_label_, true);
    SetVisible(example_label_, true);
    SetVisible(translation_label_, snapshot_.example_translation[0] != '\0');
    SetVisible(status_label_, false);
    SetVisible(hint_label_, false);

    SetLabel(word_label_, snapshot_.word);
    SetLabel(phonetic_label_, snapshot_.phonetic);
    SetLabel(meaning_label_, snapshot_.meaning);
    SetLabel(example_label_, snapshot_.example);
    SetLabel(translation_label_, snapshot_.example_translation);
    SetLabel(status_label_, snapshot_.status_line);
    SetLabel(hint_label_, snapshot_.hint);
}

void VocabularyPageAdapter::ApplySummary() {
    SetVisible(phonetic_label_, false);
    SetVisible(meaning_label_, true);
    SetVisible(example_label_, true);
    SetVisible(translation_label_, false);
    SetVisible(status_label_, false);
    SetVisible(hint_label_, true);

    SetLabel(word_label_, snapshot_.word);
    SetLabel(meaning_label_, snapshot_.meaning);
    SetLabel(example_label_, snapshot_.example);
    SetLabel(hint_label_, snapshot_.hint);
}

void VocabularyPageAdapter::ApplySettings() {
    SetVisible(phonetic_label_, false);
    SetVisible(meaning_label_, true);
    SetVisible(example_label_, true);
    SetVisible(translation_label_, true);
    SetVisible(status_label_, false);
    SetVisible(hint_label_, true);

    SetLabel(word_label_, snapshot_.word);
    SetLabel(meaning_label_, snapshot_.meaning);
    SetLabel(example_label_, snapshot_.example);
    SetLabel(translation_label_, snapshot_.example_translation);
    SetLabel(hint_label_, snapshot_.hint);
}
