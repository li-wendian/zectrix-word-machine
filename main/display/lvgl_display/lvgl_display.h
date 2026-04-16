#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include "display.h"
#include "lvgl_image.h"

#include <lvgl.h>

class LvglDisplay : public Display {
public:
    LvglDisplay();
    ~LvglDisplay() override;

    void SetStatus(const char* status) override;
    void ShowNotification(const char* notification, int duration_ms = 3000) override;
    void ShowNotification(const std::string& notification, int duration_ms = 3000) override;
    void SetPreviewImage(std::unique_ptr<LvglImage> image);
    void UpdateStatusBar(bool update_all = false) override;
    void SetPowerSaveMode(bool on) override;
    bool SnapshotToJpeg(std::string& jpeg_data, int quality = 80);

protected:
    lv_display_t* display_ = nullptr;

    friend class DisplayLockGuard;
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;
};

#endif
