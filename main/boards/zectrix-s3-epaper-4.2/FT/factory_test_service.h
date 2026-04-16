#ifndef ZECTRIX_FACTORY_TEST_SERVICE_H_
#define ZECTRIX_FACTORY_TEST_SERVICE_H_

#include <atomic>
#include <array>
#include <cstdint>
#include <functional>
#include <mutex>

enum class FactoryTestStep : uint8_t {
    kRf = 0,
    kAudio,
    kRtc,
    kCharge,
    kLed,
    kKeys,
    kNfc,
    kComplete,
    kFailed,
};

enum class FactoryTestStepState : uint8_t {
    kWait = 0,
    kRunning,
    kPass,
    kFail,
};

enum class FactoryTestButton : uint8_t {
    kConfirmClick = 0,
    kConfirmLongPress,
    kUpClick,
    kDownClick,
};

struct FactoryTestSnapshot {
    bool active = false;
    bool terminal_failure = false;
    FactoryTestStep current_step = FactoryTestStep::kRf;
    FactoryTestStepState current_state = FactoryTestStepState::kWait;
    std::array<FactoryTestStepState, 7> step_states = {
        FactoryTestStepState::kWait,
        FactoryTestStepState::kWait,
        FactoryTestStepState::kWait,
        FactoryTestStepState::kWait,
        FactoryTestStepState::kWait,
        FactoryTestStepState::kWait,
        FactoryTestStepState::kWait,
    };
    char title[32] = {0};
    char hint[96] = {0};
    char detail1[96] = {0};
    char detail2[96] = {0};
    char detail3[96] = {0};
    char detail4[96] = {0};
    char footer[96] = {0};
};

class FactoryTestService {
public:
    static FactoryTestService& Instance();

    using SnapshotCallback = std::function<void(const FactoryTestSnapshot&)>;
    using VoidCallback = std::function<void()>;

    bool IsRunning() const;
    FactoryTestSnapshot GetSnapshot() const;
    void SetSnapshotCallback(SnapshotCallback callback);
    void SetShutdownCallback(VoidCallback callback);
    void StartFlow();
    void HandleButton(FactoryTestButton button);

private:
    FactoryTestService() = default;

    static void FlowTaskEntry(void* arg);
    void FlowTask();
    void PublishSnapshotLocked() const;

    mutable std::mutex mutex_;
    FactoryTestSnapshot snapshot_;
    SnapshotCallback snapshot_callback_;
    VoidCallback shutdown_callback_;
    std::atomic<bool> running_{false};
};

#endif  // ZECTRIX_FACTORY_TEST_SERVICE_H_
