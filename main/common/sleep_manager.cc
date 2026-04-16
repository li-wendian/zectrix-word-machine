#include "sleep_manager.h"

#include <atomic>
#include <mutex>

#include <esp_timer.h>

#include "application.h"

namespace {
int64_t NowMs() {
    return esp_timer_get_time() / 1000;
}
}  // namespace

class SleepManagerImpl {
public:
    std::atomic<uint32_t> busy_mask{0};
    std::atomic<int> hold_count{0};
    std::atomic<int64_t> deadline_ms{0};

    std::mutex hook_mutex;
    std::function<bool()> pre_sleep_hook;
};

static SleepManagerImpl g_sm;

SleepManager& SleepManager::GetInstance() {
    static SleepManager instance;
    return instance;
}

void SleepManager::SetBusy(SleepBusySrc src, bool busy) {
    const uint32_t bit = static_cast<uint32_t>(src);
    if (busy) {
        g_sm.busy_mask.fetch_or(bit, std::memory_order_acq_rel);
    } else {
        g_sm.busy_mask.fetch_and(~bit, std::memory_order_acq_rel);
    }
}

void SleepManager::Kick(uint32_t delay_ms, const char* /*reason*/) {
    const int64_t new_deadline = NowMs() + static_cast<int64_t>(delay_ms);
    int64_t cur = g_sm.deadline_ms.load(std::memory_order_acquire);
    while (cur < new_deadline &&
           !g_sm.deadline_ms.compare_exchange_weak(cur, new_deadline,
                                                   std::memory_order_acq_rel,
                                                   std::memory_order_acquire)) {
    }
}

void SleepManager::Hold(const char* /*reason*/) {
    g_sm.hold_count.fetch_add(1, std::memory_order_acq_rel);
}

void SleepManager::Release(const char* /*reason*/) {
    int cur = g_sm.hold_count.load(std::memory_order_acquire);
    while (true) {
        if (cur <= 0) {
            g_sm.hold_count.store(0, std::memory_order_release);
            return;
        }
        if (g_sm.hold_count.compare_exchange_weak(cur, cur - 1,
                                                  std::memory_order_acq_rel,
                                                  std::memory_order_acquire)) {
            return;
        }
    }
}

bool SleepManager::CanSleepNow() const {
    if (!Application::GetInstance().CanEnterSleepMode()) {
        return false;
    }

    if (g_sm.busy_mask.load(std::memory_order_acquire) != 0) {
        return false;
    }

    if (g_sm.hold_count.load(std::memory_order_acquire) > 0) {
        return false;
    }

    const int64_t now = NowMs();
    const int64_t deadline = g_sm.deadline_ms.load(std::memory_order_acquire);
    if (now < deadline) {
        return false;
    }

    return true;
}

bool SleepManager::PrepareForLightSleep() {
    if (!CanSleepNow()) {
        return false;
    }

    std::function<bool()> hook;
    {
        std::lock_guard<std::mutex> lock(g_sm.hook_mutex);
        hook = g_sm.pre_sleep_hook;
    }
    if (hook) {
        if (!hook()) {
            return false;
        }
    }
    return true;
}

void SleepManager::SetPreSleepHook(std::function<bool()> hook) {
    std::lock_guard<std::mutex> lock(g_sm.hook_mutex);
    g_sm.pre_sleep_hook = std::move(hook);
}

uint32_t SleepManager::GetBusyMask() const {
    return g_sm.busy_mask.load(std::memory_order_acquire);
}

int SleepManager::GetHoldCount() const {
    return g_sm.hold_count.load(std::memory_order_acquire);
}

int64_t SleepManager::GetDeadlineMs() const {
    return g_sm.deadline_ms.load(std::memory_order_acquire);
}
