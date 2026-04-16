#ifndef FACTORY_TEST_PAGE_ADAPTER_H
#define FACTORY_TEST_PAGE_ADAPTER_H

#include "boards/zectrix-s3-epaper-4.2/FT/factory_test_service.h"
#include "ui_page.h"

class LcdDisplay;

class FactoryTestPageAdapter : public IUiPage {
public:
    explicit FactoryTestPageAdapter(LcdDisplay* host);

    UiPageId Id() const override;
    const char* Name() const override;
    void Build() override;
    lv_obj_t* Screen() const override;
    void OnShow() override;
    bool HandleEvent(const UiPageEvent& event) override;

    void UpdateSnapshot(const FactoryTestSnapshot& snapshot);

private:
    void RefreshStepStateLocked(int index, FactoryTestStepState state);

    LcdDisplay* host_ = nullptr;
    bool built_ = false;
    FactoryTestSnapshot snapshot_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* header_step_label_ = nullptr;
    lv_obj_t* header_state_label_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* hint_label_ = nullptr;
    lv_obj_t* detail1_label_ = nullptr;
    lv_obj_t* detail2_label_ = nullptr;
    lv_obj_t* detail3_label_ = nullptr;
    lv_obj_t* detail4_label_ = nullptr;
    lv_obj_t* footer_label_ = nullptr;
    lv_obj_t* step_rows_[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    lv_obj_t* step_name_labels_[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    lv_obj_t* step_state_labels_[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

#endif  // FACTORY_TEST_PAGE_ADAPTER_H
