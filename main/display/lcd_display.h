#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "lvgl_display.h"
#include "ui_page_registry.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

#include <memory>

class FactoryTestPageAdapter;
class VocabularyPageAdapter;
struct VocabularySnapshot;

class LcdDisplay : public LvglDisplay {
protected:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    lv_obj_t* factory_test_screen_ = nullptr;
    lv_obj_t* vocabulary_screen_ = nullptr;

    UiPageRegistry page_registry_;
    FactoryTestPageAdapter* factory_test_page_adapter_ = nullptr;
    VocabularyPageAdapter* vocabulary_page_adapter_ = nullptr;
    bool ui_setup_done_ = false;

    void ShowScreen(lv_obj_t* scr);
    bool RegisterPageLocked(std::unique_ptr<IUiPage> page);
    bool SwitchPageLocked(UiPageId id);
    void SetupUI();

    bool Lock(int timeout_ms = 0) override;
    void Unlock() override;

    friend class FactoryTestPageAdapter;
    friend class VocabularyPageAdapter;

    LcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height);

public:
    ~LcdDisplay() override;

    void SetEmotion(const char* emotion) override;
    void SetChatMessage(const char* role, const char* content) override;
    void SetPreviewImage(std::unique_ptr<LvglImage> image);
    void SetTheme(Theme* theme) override;

    bool RegisterPage(std::unique_ptr<IUiPage> page);
    bool SwitchPage(UiPageId id);
    UiPageId GetActivePageId() const;
    void DispatchPageEvent(const UiPageEvent& e, bool only_active = true);
    void ShowFactoryTestPage();
    bool IsFactoryTestPageActive();
    FactoryTestPageAdapter* GetFactoryTestPageAdapter() { return factory_test_page_adapter_; }
    void ShowVocabularyPage() override;
    void UpdateVocabularySnapshot(const VocabularySnapshot& snapshot) override;
    VocabularyPageAdapter* GetVocabularyPageAdapter() { return vocabulary_page_adapter_; }
};

#endif  // LCD_DISPLAY_H
