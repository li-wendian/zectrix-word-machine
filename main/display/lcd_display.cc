#include "lcd_display.h"

#include "lvgl_theme.h"
#include "pages/factory_test_page_adapter.h"
#include "pages/vocabulary_page_adapter.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <font_zectrix.h>

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);
LV_FONT_DECLARE(BUILTIN_ICON_FONT);
LV_FONT_DECLARE(font_zectrix_48_1);
LV_FONT_DECLARE(SourceHanSansSC_Medium_slim);

namespace {

constexpr char kTag[] = "LcdDisplay";

void InitializeLcdThemes() {
    static bool initialized = false;
    if (initialized) {
        return;
    }

    auto text_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_TEXT_FONT);
    auto icon_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_ICON_FONT);
    auto large_icon_font = std::make_shared<LvglBuiltInFont>(&font_zectrix_48_1);
    auto reminder_font = std::make_shared<LvglBuiltInFont>(&SourceHanSansSC_Medium_slim);

    auto* light_theme = new LvglTheme("light");
    light_theme->set_background_color(lv_color_hex(0xFFFFFF));
    light_theme->set_text_color(lv_color_hex(0x000000));
    light_theme->set_border_color(lv_color_hex(0x000000));
    light_theme->set_low_battery_color(lv_color_hex(0x000000));
    light_theme->set_text_font(text_font);
    light_theme->set_reminder_text_font(reminder_font);
    light_theme->set_icon_font(icon_font);
    light_theme->set_large_icon_font(large_icon_font);

    auto* dark_theme = new LvglTheme("dark");
    dark_theme->set_background_color(lv_color_hex(0x000000));
    dark_theme->set_text_color(lv_color_hex(0xFFFFFF));
    dark_theme->set_border_color(lv_color_hex(0xFFFFFF));
    dark_theme->set_low_battery_color(lv_color_hex(0xFFFFFF));
    dark_theme->set_text_font(text_font);
    dark_theme->set_reminder_text_font(reminder_font);
    dark_theme->set_icon_font(icon_font);
    dark_theme->set_large_icon_font(large_icon_font);

    auto& theme_manager = LvglThemeManager::GetInstance();
    theme_manager.RegisterTheme("light", light_theme);
    theme_manager.RegisterTheme("dark", dark_theme);
    initialized = true;
}

}  // namespace

LcdDisplay::LcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                       esp_lcd_panel_handle_t panel,
                       int width,
                       int height)
    : panel_io_(panel_io), panel_(panel) {
    width_ = width;
    height_ = height;

    InitializeLcdThemes();

    Settings settings("display", false);
    std::string theme_name = settings.GetString("theme", "light");
    current_theme_ = LvglThemeManager::GetInstance().GetTheme(theme_name);
    if (current_theme_ == nullptr) {
        current_theme_ = LvglThemeManager::GetInstance().GetTheme("light");
    }
}

LcdDisplay::~LcdDisplay() {
    {
        DisplayLockGuard lock(this);
        page_registry_.Reset();
        factory_test_page_adapter_ = nullptr;
        vocabulary_page_adapter_ = nullptr;
        ui_setup_done_ = false;
        if (factory_test_screen_ != nullptr) {
            lv_obj_del(factory_test_screen_);
            factory_test_screen_ = nullptr;
        }
        if (vocabulary_screen_ != nullptr) {
            lv_obj_del(vocabulary_screen_);
            vocabulary_screen_ = nullptr;
        }
    }

    if (display_ != nullptr) {
        lv_display_delete(display_);
        display_ = nullptr;
    }
    if (panel_ != nullptr) {
        esp_lcd_panel_del(panel_);
        panel_ = nullptr;
    }
    if (panel_io_ != nullptr) {
        esp_lcd_panel_io_del(panel_io_);
        panel_io_ = nullptr;
    }
}

bool LcdDisplay::Lock(int timeout_ms) {
    return lvgl_port_lock(timeout_ms);
}

void LcdDisplay::Unlock() {
    lvgl_port_unlock();
}

void LcdDisplay::ShowScreen(lv_obj_t* scr) {
    if (scr != nullptr) {
        lv_screen_load(scr);
    }
}

bool LcdDisplay::RegisterPageLocked(std::unique_ptr<IUiPage> page) {
    return page_registry_.Register(std::move(page));
}

bool LcdDisplay::RegisterPage(std::unique_ptr<IUiPage> page) {
    DisplayLockGuard lock(this);
    return RegisterPageLocked(std::move(page));
}

bool LcdDisplay::SwitchPageLocked(UiPageId id) {
    return page_registry_.SwitchTo(id);
}

bool LcdDisplay::SwitchPage(UiPageId id) {
    DisplayLockGuard lock(this);
    return SwitchPageLocked(id);
}

UiPageId LcdDisplay::GetActivePageId() const {
    DisplayLockGuard lock(const_cast<LcdDisplay*>(this));
    return page_registry_.ActiveId();
}

void LcdDisplay::DispatchPageEvent(const UiPageEvent& e, bool only_active) {
    DisplayLockGuard lock(this);
    page_registry_.Dispatch(e, only_active);
}

void LcdDisplay::ShowFactoryTestPage() {
    (void)SwitchPage(UiPageId::FactoryTest);
}

bool LcdDisplay::IsFactoryTestPageActive() {
    DisplayLockGuard lock(this);
    return page_registry_.HasActive() && page_registry_.ActiveId() == UiPageId::FactoryTest;
}

void LcdDisplay::ShowVocabularyPage() {
    (void)SwitchPage(UiPageId::Vocabulary);
}

void LcdDisplay::UpdateVocabularySnapshot(const VocabularySnapshot& snapshot) {
    DisplayLockGuard lock(this);
    if (vocabulary_page_adapter_ != nullptr) {
        vocabulary_page_adapter_->UpdateSnapshot(snapshot);
    }
}

void LcdDisplay::SetupUI() {
    DisplayLockGuard lock(this);
    if (ui_setup_done_) {
        return;
    }

    auto factory_test_page = std::make_unique<FactoryTestPageAdapter>(this);
    factory_test_page_adapter_ = factory_test_page.get();
    if (!RegisterPageLocked(std::move(factory_test_page))) {
        factory_test_page_adapter_ = nullptr;
        return;
    }

    auto vocabulary_page = std::make_unique<VocabularyPageAdapter>(this);
    vocabulary_page_adapter_ = vocabulary_page.get();
    if (!RegisterPageLocked(std::move(vocabulary_page))) {
        vocabulary_page_adapter_ = nullptr;
        return;
    }

    if (!SwitchPageLocked(UiPageId::Vocabulary)) {
        ESP_LOGW(kTag, "Failed to switch to vocabulary page");
        return;
    }

    ui_setup_done_ = true;
}

void LcdDisplay::SetEmotion(const char* emotion) {
    (void)emotion;
}

void LcdDisplay::SetChatMessage(const char* role, const char* content) {
    (void)role;
    (void)content;
}

void LcdDisplay::SetPreviewImage(std::unique_ptr<LvglImage> image) {
    (void)image;
}

void LcdDisplay::SetTheme(Theme* theme) {
    Display::SetTheme(theme);
}
