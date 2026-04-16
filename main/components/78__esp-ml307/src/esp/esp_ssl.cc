#include "esp_ssl.h"
#include <esp_log.h>
#include <esp_crt_bundle.h>
#include <esp_tls_errors.h>
#include <cstring>
#include <unistd.h>
#include <mutex>
#include <vector>
#include <sys/socket.h>

#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#endif

static const char *TAG = "EspSsl";
static constexpr int kReceivePollTimeoutMs = 1000;

#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
namespace {
std::mutex g_tls_session_mutex;
esp_tls_client_session_t* g_tls_session = nullptr;
std::vector<uint8_t> g_tls_session_blob;

bool SerializeSession(const mbedtls_ssl_session& session, std::vector<uint8_t>* out) {
    if (out == nullptr) {
        return false;
    }
    size_t olen = 0;
    int ret = mbedtls_ssl_session_save(&session, nullptr, 0, &olen);
    if (ret != MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL || olen == 0) {
        return false;
    }
    out->assign(olen, 0);
    ret = mbedtls_ssl_session_save(&session, out->data(), out->size(), &olen);
    if (ret != 0) {
        return false;
    }
    out->resize(olen);
    return true;
}
}  // namespace
#endif

EspSsl::EspSsl() {
    event_group_ = xEventGroupCreate();
}

EspSsl::~EspSsl() {
    Disconnect();

    if (event_group_ != nullptr) {
        vEventGroupDelete(event_group_);
        event_group_ = nullptr;
    }
}

bool EspSsl::Connect(const std::string& host, int port) {
    if (tls_client_ != nullptr) {
        ESP_LOGE(TAG, "tls client has been initialized");
        return false;
    }

    tls_client_ = esp_tls_init();
    if (tls_client_ == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize TLS");
        return false;
    }

    esp_tls_cfg_t cfg = {};
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.esp_tls_dyn_buf_strategy = ESP_TLS_DYN_BUF_RX_STATIC;
    cfg.timeout_ms = kReceivePollTimeoutMs;
#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
    bool resume_attempt = false;
    {
        std::lock_guard<std::mutex> lock(g_tls_session_mutex);
        if (g_tls_session != nullptr) {
            cfg.client_session = g_tls_session;
            resume_attempt = true;
        }
    }
    ESP_LOGI(TAG, "stage=tls event=session_resume_attempt enabled=1 attempt=%d", resume_attempt ? 1 : 0);
#else
    ESP_LOGI(TAG, "stage=tls event=session_resume_attempt enabled=0 attempt=0");
#endif

    int ret = esp_tls_conn_new_sync(host.c_str(), host.length(), port, &cfg, tls_client_);
    if (ret != 1) {
        esp_tls_error_handle_t last_error;
        if (esp_tls_get_error_handle(tls_client_, &last_error) == ESP_OK) {
            int error_code, error_flags;
            esp_err_t err = esp_tls_get_and_clear_last_error(last_error, &error_code, &error_flags);
            last_error_ = err;
            ESP_LOGE(TAG, "Failed to connect to %s:%d, code=0x%x", host.c_str(), port, err);
        } else {
            last_error_ = -1;
            ESP_LOGE(TAG, "Failed to get error handle");
        }
        esp_tls_conn_destroy(tls_client_);
        tls_client_ = nullptr;
        return false;
    }

#if CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS
    bool resume_hit = false;
    bool resume_known = false;
    std::vector<uint8_t> new_blob;
    esp_tls_client_session_t* new_session = esp_tls_get_client_session(tls_client_);
    if (new_session != nullptr) {
        bool serialized = SerializeSession(new_session->saved_session, &new_blob);
        std::lock_guard<std::mutex> lock(g_tls_session_mutex);
        if (serialized && !g_tls_session_blob.empty() && !new_blob.empty()) {
            resume_known = true;
            resume_hit = (g_tls_session_blob == new_blob);
        }
        if (g_tls_session != nullptr) {
            esp_tls_free_client_session(g_tls_session);
        }
        g_tls_session = new_session;
        if (serialized) {
            g_tls_session_blob = std::move(new_blob);
        } else {
            g_tls_session_blob.clear();
        }
    }
    ESP_LOGI(TAG, "stage=tls event=session_resume_result attempt=%d known=%d hit=%d",
             resume_attempt ? 1 : 0, resume_known ? 1 : 0, resume_hit ? 1 : 0);
#endif

    connected_ = true;

    xEventGroupClearBits(event_group_, ESP_SSL_EVENT_RECEIVE_TASK_EXIT);
    xTaskCreate([](void* arg) {
        EspSsl* ssl = (EspSsl*)arg;
        ssl->ReceiveTask();
        xEventGroupSetBits(ssl->event_group_, ESP_SSL_EVENT_RECEIVE_TASK_EXIT);
        vTaskDelete(NULL);
    }, "ssl_receive", 4096, this, 1, &receive_task_handle_);
    return true;
}

void EspSsl::Disconnect() {
    connected_ = false;
    
    // Close socket if it is open
    if (tls_client_ != nullptr) {
        int sockfd;
        ESP_ERROR_CHECK(esp_tls_get_conn_sockfd(tls_client_, &sockfd));
        if (sockfd >= 0) {
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
        }
    
        auto bits = xEventGroupWaitBits(event_group_, ESP_SSL_EVENT_RECEIVE_TASK_EXIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(10000));
        if (!(bits & ESP_SSL_EVENT_RECEIVE_TASK_EXIT)) {
            ESP_LOGE(TAG, "Failed to wait for receive task exit");
        }

        esp_tls_conn_destroy(tls_client_);
        tls_client_ = nullptr;
    }
}

/* CONFIG_MBEDTLS_SSL_RENEGOTIATION should be disabled in sdkconfig.
 * Otherwise, invalid memory access may be triggered.
 */
int EspSsl::Send(const std::string& data) {
    if (!connected_) {
        ESP_LOGE(TAG, "Not connected");
        return -1;
    }

    size_t total_sent = 0;
    size_t data_size = data.size();
    const char* data_ptr = data.data();
    
    while (total_sent < data_size) {
        int ret = esp_tls_conn_write(tls_client_, data_ptr + total_sent, data_size - total_sent);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE) {
            continue;
        }

        if (ret <= 0) {
            ESP_LOGE(TAG, "SSL send failed: ret=%d, errno=%d", ret, errno);
            return ret;
        }
        
        total_sent += ret;
    }
    
    return total_sent;
}

void EspSsl::ReceiveTask() {
    std::string data;
    while (connected_) {
        data.resize(1500);
        int ret = esp_tls_conn_read(tls_client_, data.data(), data.size());

        if (ret == ESP_TLS_ERR_SSL_WANT_READ ||
            ret == ESP_TLS_ERR_SSL_WANT_WRITE ||
            ret == ESP_TLS_ERR_SSL_TIMEOUT) {
            continue;
        }

        if (ret <= 0) {
            if (ret < 0) {
                ESP_LOGE(TAG, "SSL receive failed: %d", ret);
            }
            connected_ = false;
            // 接收失败或连接断开时调用断连回调
            if (disconnect_callback_) {
                disconnect_callback_();
            }
            break;
        }
        
        if (stream_callback_) {
            data.resize(ret);
            stream_callback_(data);
        }
    }
}

int EspSsl::GetLastError() {
    return last_error_;
}
