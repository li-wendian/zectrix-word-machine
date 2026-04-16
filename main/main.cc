#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "application.h"
#include "system_info.h"

#define TAG "main"

static void LogNvsStats() {
    nvs_stats_t stats = {};
    esp_err_t ret = nvs_get_stats(nullptr, &stats);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "NVS stats read failed: %s", esp_err_to_name(ret));
        return;
    }
    uint32_t used = stats.used_entries;
    uint32_t total = stats.total_entries;
    uint32_t free_entries = stats.free_entries;
    uint32_t percent = (total > 0) ? (used * 100U / total) : 0;
    ESP_LOGI(TAG, "NVS stats: used=%u free=%u total=%u (%u%%) namespaces=%u",
             used, free_entries, total, percent, stats.namespace_count);
}

extern "C" void app_main(void)
{
    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    LogNvsStats();

    // Initialize and run the application
    auto& app = Application::GetInstance();
    app.Initialize();
    app.Run();  // This function runs the main event loop and never returns
}
