#pragma once

#include <cstdint>
#include <functional>

// Busy sources must include at least NET/AUDIO/DISPLAY/UI/NVS/TODO.
enum class SleepBusySrc : uint32_t {
    Net     = 1u << 0,
    Audio   = 1u << 1,
    Display = 1u << 2,
    Ui      = 1u << 3,
    Nvs     = 1u << 4,
    Todo    = 1u << 5,
    Protocol = 1u << 6,
};

class SleepManager {
public:
    static SleepManager& GetInstance();

    // Busy votes
    void SetBusy(SleepBusySrc src, bool busy);

    // Sleep delay deadline: extend to max(now + delay_ms).
    void Kick(uint32_t delay_ms, const char* reason = nullptr);

    // Hold/Release: link-level blocking (press-to-talk, wake key chains, etc.)
    void Hold(const char* reason = nullptr);
    void Release(const char* reason = nullptr);

    // Gate: busy == 0 && hold == 0 && now >= deadline
    bool CanSleepNow() const;

    // Called right before light sleep. No business logic here.
    // Returns false if sleep should be aborted.
    bool PrepareForLightSleep();

    // Optional hook for pre-sleep actions (e.g., wifi stop).
    // Return false to abort sleep.
    void SetPreSleepHook(std::function<bool()> hook);

    // Debug/inspection helpers
    uint32_t GetBusyMask() const;
    int GetHoldCount() const;
    int64_t GetDeadlineMs() const;

private:
    SleepManager() = default;
};

// C-style wrappers (requested sm_* API)
inline void sm_set_busy(SleepBusySrc src, bool busy) {
    SleepManager::GetInstance().SetBusy(src, busy);
}

inline void sm_kick(uint32_t delay_ms, const char* reason = nullptr) {
    SleepManager::GetInstance().Kick(delay_ms, reason);
}

inline void sm_hold(const char* reason = nullptr) {
    SleepManager::GetInstance().Hold(reason);
}

inline void sm_release(const char* reason = nullptr) {
    SleepManager::GetInstance().Release(reason);
}

// Alias for a typo in requirements (sg_release).
inline void sg_release(const char* reason = nullptr) {
    sm_release(reason);
}

inline bool sm_can_sleep_now() {
    return SleepManager::GetInstance().CanSleepNow();
}

inline bool sm_prepare_for_light_sleep() {
    return SleepManager::GetInstance().PrepareForLightSleep();
}
