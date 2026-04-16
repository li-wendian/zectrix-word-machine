#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef CONFIG_USE_EMOTE_MESSAGE_STYLE
#define HAVE_LVGL 1
#include <lvgl.h>
#endif

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_pm.h>

#include <string>
#include <vector>
#include <chrono>

struct VocabularySnapshot;

class Theme {
public:
    Theme(const std::string& name) : name_(name) {}
    virtual ~Theme() = default;

    inline std::string name() const { return name_; }
private:
    std::string name_;
};

class Display {
public:
    Display();
    virtual ~Display();

    virtual void SetStatus(const char* status);
    virtual void ShowNotification(const char* notification, int duration_ms = 3000);
    virtual void ShowNotification(const std::string &notification, int duration_ms = 3000);
    virtual void SetEmotion(const char* emotion);
    virtual void SetChatMessage(const char* role, const char* content);
    virtual void SetTheme(Theme* theme);
    virtual Theme* GetTheme() { return current_theme_; }
    virtual void UpdateStatusBar(bool update_all = false);
    virtual void SetPowerSaveMode(bool on);
    virtual void ShowVocabularyPage() {}
    virtual void UpdateVocabularySnapshot(const VocabularySnapshot& snapshot) { (void)snapshot; }
    virtual void RequestUrgentRefresh() {}
    virtual void RequestUrgentFullRefresh() {}

    // 写入原始 1bpp 位图数据到帧缓冲区（由子类实现）
    // data 中 bit=1 表示黑色像素，bit=0 表示白色像素
    virtual void WriteRaw1bpp(int x, int y, int w, int h, const uint8_t* data, size_t len) { (void)x; (void)y; (void)w; (void)h; (void)data; (void)len; }

    // 文本渲染项
    struct TextItem {
        std::string content;
        int x = 0;
        int y = 0;
        int size = 24;  // 16 or 24
    };

    // 直接在设备端渲染文本到帧缓冲区（由子类实现）
    virtual void DrawTexts(const std::vector<TextItem>& texts, bool clear) { (void)texts; (void)clear; }

    // 更新图片页缓存（默认无实现）
    virtual void UpdatePicRegion(int x, int y, int w, int h, const uint8_t* data, size_t len) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)data;
        (void)len;
    }

    // 图片页是否存在有效内容（默认无）
    virtual bool HasPicContent() const { return false; }

    inline int width() const { return width_; }
    inline int height() const { return height_; }

protected:
    int width_ = 0;
    int height_ = 0;

    Theme* current_theme_ = nullptr;

    friend class DisplayLockGuard;
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;
};


class DisplayLockGuard {
public:
    DisplayLockGuard(Display *display) : display_(display), locked_(false) {
        locked_ = display_->Lock(30000);
        if (!locked_) {
            ESP_LOGE("Display", "Failed to lock display");
        }
    }
    ~DisplayLockGuard() {
        if (locked_) {
            display_->Unlock();
        }
    }

private:
    Display *display_;
    bool locked_;
};

class NoDisplay : public Display {
private:
    virtual bool Lock(int timeout_ms = 0) override {
        return true;
    }
    virtual void Unlock() override {}
};

#endif
