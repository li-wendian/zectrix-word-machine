#ifndef UI_PAGE_H
#define UI_PAGE_H

#include "display.h"

#include <cstdint>

#include <lvgl.h>

enum class UiPageId : uint8_t {
    FactoryTest = 0,
    Vocabulary,
};

enum class UiPageEventType : uint8_t {
    Custom = 0,
};

struct UiPageEvent {
    UiPageEventType type = UiPageEventType::Custom;
    int32_t i32 = 0;
    const char* text = nullptr;
    void* ptr = nullptr;
};

enum class UiRefreshHint : uint8_t {
    None = 0,
    Partial,
    UrgentFull,
};

class IUiPage {
public:
    virtual ~IUiPage() = default;
    virtual UiPageId Id() const = 0;
    virtual const char* Name() const = 0;
    virtual void Build() = 0;
    virtual lv_obj_t* Screen() const = 0;
    virtual void OnShow() {}
    virtual void OnHide() {}
    virtual bool HandleEvent(const UiPageEvent& event) {
        (void)event;
        return false;
    }
    virtual UiRefreshHint ConsumeRefreshHint() { return UiRefreshHint::None; }
    virtual void OnThemeChanged(Theme* theme) { (void)theme; }
};

#endif  // UI_PAGE_H
