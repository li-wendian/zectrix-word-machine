#include "ssid_manager.h"
#include "wifi_manager.h"

#include <algorithm>
#include <esp_log.h>
#include <nvs_flash.h>

#define TAG "SsidManager"
#define NVS_NAMESPACE "wifi"
#define MAX_WIFI_SSID_COUNT 10

SsidManager::SsidManager() {
    LoadFromNvs();
}

SsidManager::~SsidManager() {
}

void SsidManager::Clear() {
    ssid_list_.clear();
    SaveToNvs();
    WifiManager::GetInstance().ClearFastReconnectCache("ssid_changed");
    WifiManager::GetInstance().ClearIpFastCache("ssid_changed");
}

void SsidManager::LoadFromNvs() {
    ssid_list_.clear();

    // Load ssid and password from NVS from namespace "wifi"
    // ssid, ssid1, ssid2, ... ssid9
    // password, password1, password2, ... password9
    nvs_handle_t nvs_handle;
    auto ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        for (int i = 0; i < MAX_WIFI_SSID_COUNT; i++) {
            std::string ssid_key = "ssid";
            if (i > 0) {
                ssid_key += std::to_string(i);
            }
            std::string password_key = "password";
            if (i > 0) {
                password_key += std::to_string(i);
            }

            char ssid[33];
            char password[65];
            size_t length = sizeof(ssid);
            if (nvs_get_str(nvs_handle, ssid_key.c_str(), ssid, &length) != ESP_OK) {
                continue;
            }
            length = sizeof(password);
            if (nvs_get_str(nvs_handle, password_key.c_str(), password, &length) != ESP_OK) {
                continue;
            }
            ssid_list_.push_back({ssid, password});
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "NVS namespace %s doesn't exist", NVS_NAMESPACE);
    }

    // 编译时预设WiFi：NVS为空时自动添加
#ifdef CONFIG_DEFAULT_WIFI_SSID
    if (ssid_list_.empty() && CONFIG_DEFAULT_WIFI_SSID[0] != '\0') {
        ESP_LOGI(TAG, "No saved WiFi, using preset SSID: %s", CONFIG_DEFAULT_WIFI_SSID);
        ssid_list_.push_back({CONFIG_DEFAULT_WIFI_SSID, CONFIG_DEFAULT_WIFI_PASSWORD});
    }
#endif
}

void SsidManager::SaveToNvs() {
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle));
    for (int i = 0; i < MAX_WIFI_SSID_COUNT; i++) {
        std::string ssid_key = "ssid";
        if (i > 0) {
            ssid_key += std::to_string(i);
        }
        std::string password_key = "password";
        if (i > 0) {
            password_key += std::to_string(i);
        }

        if (i < ssid_list_.size()) {
            nvs_set_str(nvs_handle, ssid_key.c_str(), ssid_list_[i].ssid.c_str());
            nvs_set_str(nvs_handle, password_key.c_str(), ssid_list_[i].password.c_str());
        } else {
            nvs_erase_key(nvs_handle, ssid_key.c_str());
            nvs_erase_key(nvs_handle, password_key.c_str());
        }
    }
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

void SsidManager::AddSsid(const std::string& ssid, const std::string& password) {
    for (auto& item : ssid_list_) {
        ESP_LOGI(TAG, "compare [%s:%d] [%s:%d]", item.ssid.c_str(), item.ssid.size(), ssid.c_str(), ssid.size());
        if (item.ssid == ssid) {
            ESP_LOGW(TAG, "SSID %s already exists, overwrite it", ssid.c_str());
            bool changed = item.password != password;
            item.password = password;
            SaveToNvs();
            if (changed) {
                WifiManager::GetInstance().ClearFastReconnectCache("ssid_changed");
                WifiManager::GetInstance().ClearIpFastCache("ssid_changed");
            }
            return;
        }
    }

    if (ssid_list_.size() >= MAX_WIFI_SSID_COUNT) {
        ESP_LOGW(TAG, "SSID list is full, pop one");
        ssid_list_.pop_back();
    }
    // Add the new ssid to the front of the list
    ssid_list_.insert(ssid_list_.begin(), {ssid, password});
    SaveToNvs();
    WifiManager::GetInstance().ClearFastReconnectCache("ssid_changed");
    WifiManager::GetInstance().ClearIpFastCache("ssid_changed");
}

void SsidManager::RemoveSsid(int index) {
    if (index < 0 || index >= ssid_list_.size()) {
        ESP_LOGW(TAG, "Invalid index %d", index);
        return;
    }
    ssid_list_.erase(ssid_list_.begin() + index);
    SaveToNvs();
}

void SsidManager::SetDefaultSsid(int index) {
    if (index < 0 || index >= ssid_list_.size()) {
        ESP_LOGW(TAG, "Invalid index %d", index);
        return;
    }
    // Move the ssid at index to the front of the list
    auto item = ssid_list_[index];
    ssid_list_.erase(ssid_list_.begin() + index);
    ssid_list_.insert(ssid_list_.begin(), item);
    SaveToNvs();
}
