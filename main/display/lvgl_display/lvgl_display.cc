#include "lvgl_display.h"

LvglDisplay::LvglDisplay() = default;

LvglDisplay::~LvglDisplay() = default;

void LvglDisplay::SetStatus(const char* status) {
    (void)status;
}

void LvglDisplay::ShowNotification(const char* notification, int duration_ms) {
    (void)notification;
    (void)duration_ms;
}

void LvglDisplay::ShowNotification(const std::string& notification, int duration_ms) {
    ShowNotification(notification.c_str(), duration_ms);
}

void LvglDisplay::SetPreviewImage(std::unique_ptr<LvglImage> image) {
    (void)image;
}

void LvglDisplay::UpdateStatusBar(bool update_all) {
    (void)update_all;
}

void LvglDisplay::SetPowerSaveMode(bool on) {
    (void)on;
}

bool LvglDisplay::SnapshotToJpeg(std::string& jpeg_data, int quality) {
    (void)jpeg_data;
    (void)quality;
    return false;
}
