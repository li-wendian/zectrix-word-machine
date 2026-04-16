#ifndef ZECTRIX_FACTORY_TEST_ACOUSTIC_SELFTEST_H_
#define ZECTRIX_FACTORY_TEST_ACOUSTIC_SELFTEST_H_

#include <array>
#include <cstdint>

class AudioCodec;

enum class AcousticSelftestFailureReason : uint8_t {
    kNone = 0,
    kBusy,
    kUnsupportedCodec,
    kCaptureTimeout,
    kDecodeNoSync,
    kDecodeCrcMismatch,
    kDecodeMacMismatch,
    kInternalError,
};

struct AcousticSelftestSummary {
    bool pass = false;
    int round = -1;
    int fc = 0;
    AcousticSelftestFailureReason reason = AcousticSelftestFailureReason::kInternalError;
    std::array<uint8_t, 3> payload = {0, 0, 0};
    bool has_payload = false;
};

class AcousticSelftest {
public:
    AcousticSelftestSummary Run(AudioCodec* codec, const std::array<uint8_t, 6>& mac) const;

    static const char* FailureReasonToString(AcousticSelftestFailureReason reason);
};

#endif  // ZECTRIX_FACTORY_TEST_ACOUSTIC_SELFTEST_H_
