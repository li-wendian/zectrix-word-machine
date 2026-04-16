#include "acoustic_selftest.h"

#include "audio_codec.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

const char* const TAG = "AcousticSelftest";

constexpr int kSampleRate = 16000;
constexpr int kBitDurationMs = 12;
constexpr int kBitSamples = (kSampleRate * kBitDurationMs) / 1000;
constexpr int kPacketBits = 48;
constexpr int kPacketBytes = kPacketBits / 8;
constexpr int kPacketSamples = kPacketBits * kBitSamples;
constexpr int kToneDeltaHz = 200;
constexpr int kOutputAmplitude = 8191;
constexpr int kMaxRounds = 3;
constexpr int kScanStepSamples = kSampleRate / 1000;
constexpr int kCaptureChunkSamples = 160;
constexpr int kCapturePrerollSamples = (kSampleRate * 60) / 1000;
constexpr int kCaptureTailSamples = (kSampleRate * 200) / 1000;
constexpr int kWarmupDiscardSamples = (kSampleRate * 250) / 1000;
constexpr double kPi = 3.14159265358979323846;
constexpr TickType_t kPlaybackJoinTimeoutTicks = pdMS_TO_TICKS(1500);
constexpr TickType_t kCodecWarmupDelayTicks = pdMS_TO_TICKS(250);

constexpr uint8_t kPreamble = 0x55;
constexpr uint8_t kSync = 0xD3;

constexpr std::array<int, 6> kFreqTable = {1400, 1800, 2200, 2600, 3000, 3400};

struct AcousticSelftestRoundResult {
    bool pass = false;
    int round = -1;
    int fc = 0;
    AcousticSelftestFailureReason reason = AcousticSelftestFailureReason::kInternalError;
    std::array<uint8_t, 3> payload = {0, 0, 0};
    bool has_payload = false;
};

struct PlaybackContext {
    AudioCodec* codec = nullptr;
    TaskHandle_t owner_task = nullptr;
    const std::vector<int16_t>* pcm = nullptr;
};

uint8_t Crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

void AppendByteBits(uint8_t value, std::vector<uint8_t>& bits) {
    for (int shift = 7; shift >= 0; --shift) {
        bits.push_back((value >> shift) & 0x01);
    }
}

std::string PayloadToHex(const std::array<uint8_t, 3>& payload) {
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%02X%02X%02X", payload[0], payload[1], payload[2]);
    return std::string(buffer);
}

std::array<uint8_t, kPacketBytes> BitsToBytes(const std::array<uint8_t, kPacketBits>& bits) {
    std::array<uint8_t, kPacketBytes> bytes = {0};
    for (int byte_index = 0; byte_index < kPacketBytes; ++byte_index) {
        uint8_t value = 0;
        for (int bit = 0; bit < 8; ++bit) {
            value = static_cast<uint8_t>((value << 1) | bits[byte_index * 8 + bit]);
        }
        bytes[byte_index] = value;
    }
    return bytes;
}

double GoertzelPower(const int16_t* samples, int sample_count, int frequency_hz) {
    const double omega = (2.0 * kPi * static_cast<double>(frequency_hz)) / static_cast<double>(kSampleRate);
    const double coeff = 2.0 * std::cos(omega);
    double s_prev = 0.0;
    double s_prev2 = 0.0;
    for (int i = 0; i < sample_count; ++i) {
        const double s = static_cast<double>(samples[i]) + coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
    }
    return s_prev2 * s_prev2 + s_prev * s_prev - coeff * s_prev * s_prev2;
}

std::vector<int16_t> GeneratePacketPcm(const std::array<uint8_t, 3>& payload, int fc) {
    const uint8_t crc = Crc8(payload.data(), payload.size());
    std::vector<uint8_t> bits;
    bits.reserve(kPacketBits);
    AppendByteBits(kPreamble, bits);
    AppendByteBits(kSync, bits);
    for (uint8_t byte : payload) {
        AppendByteBits(byte, bits);
    }
    AppendByteBits(crc, bits);

    std::vector<int16_t> pcm;
    pcm.reserve(kPacketSamples);

    double phase = 0.0;
    for (uint8_t bit : bits) {
        const int frequency = bit == 0 ? (fc - kToneDeltaHz) : (fc + kToneDeltaHz);
        const double phase_step = (2.0 * kPi * static_cast<double>(frequency)) / static_cast<double>(kSampleRate);
        for (int i = 0; i < kBitSamples; ++i) {
            const int sample = static_cast<int>(std::sin(phase) * static_cast<double>(kOutputAmplitude));
            pcm.push_back(static_cast<int16_t>(sample));
            phase += phase_step;
            if (phase >= 2.0 * kPi) {
                phase -= 2.0 * kPi;
            }
        }
    }

    return pcm;
}

int FailureReasonPriority(AcousticSelftestFailureReason reason) {
    switch (reason) {
        case AcousticSelftestFailureReason::kDecodeMacMismatch:
            return 5;
        case AcousticSelftestFailureReason::kDecodeCrcMismatch:
            return 4;
        case AcousticSelftestFailureReason::kDecodeNoSync:
            return 3;
        case AcousticSelftestFailureReason::kCaptureTimeout:
            return 2;
        case AcousticSelftestFailureReason::kInternalError:
            return 1;
        case AcousticSelftestFailureReason::kUnsupportedCodec:
            return 6;
        case AcousticSelftestFailureReason::kBusy:
            return 7;
        case AcousticSelftestFailureReason::kNone:
        default:
            return 0;
    }
}

AcousticSelftestFailureReason ChoosePreferredReason(AcousticSelftestFailureReason current,
                                                    AcousticSelftestFailureReason candidate) {
    return FailureReasonPriority(candidate) > FailureReasonPriority(current) ? candidate : current;
}

AcousticSelftestRoundResult DecodeCapture(const std::vector<int16_t>& capture,
                                          const std::array<uint8_t, 3>& expected_payload,
                                          int round,
                                          int fc) {
    AcousticSelftestRoundResult result;
    result.round = round;
    result.fc = fc;
    result.reason = AcousticSelftestFailureReason::kDecodeNoSync;

    if (capture.size() < static_cast<size_t>(kPacketSamples)) {
        result.reason = AcousticSelftestFailureReason::kCaptureTimeout;
        return result;
    }

    bool saw_crc_mismatch = false;
    bool saw_mac_mismatch = false;

    for (size_t start = 0; start + kPacketSamples <= capture.size(); start += kScanStepSamples) {
        std::array<uint8_t, kPacketBits> bits = {0};
        for (int bit_index = 0; bit_index < kPacketBits; ++bit_index) {
            const int16_t* window = capture.data() + start + bit_index * kBitSamples;
            const double power_f0 = GoertzelPower(window, kBitSamples, fc - kToneDeltaHz);
            const double power_f1 = GoertzelPower(window, kBitSamples, fc + kToneDeltaHz);
            bits[bit_index] = power_f1 > power_f0 ? 1 : 0;
        }

        const auto bytes = BitsToBytes(bits);
        if (bytes[0] != kPreamble || bytes[1] != kSync) {
            continue;
        }

        const std::array<uint8_t, 3> payload = {bytes[2], bytes[3], bytes[4]};
        const uint8_t expected_crc = Crc8(payload.data(), payload.size());
        if (bytes[5] != expected_crc) {
            saw_crc_mismatch = true;
            continue;
        }

        if (payload != expected_payload) {
            saw_mac_mismatch = true;
            result.payload = payload;
            result.has_payload = true;
            continue;
        }

        result.pass = true;
        result.reason = AcousticSelftestFailureReason::kNone;
        result.payload = payload;
        result.has_payload = true;
        return result;
    }

    if (saw_mac_mismatch) {
        result.reason = AcousticSelftestFailureReason::kDecodeMacMismatch;
    } else if (saw_crc_mismatch) {
        result.reason = AcousticSelftestFailureReason::kDecodeCrcMismatch;
    }
    return result;
}

bool CaptureSamples(AudioCodec* codec, size_t target_samples, std::vector<int16_t>& capture) {
    capture.clear();
    capture.reserve(target_samples);

    std::vector<int16_t> chunk(kCaptureChunkSamples);
    while (capture.size() < target_samples) {
        if (!codec->InputData(chunk)) {
            return false;
        }
        const size_t remaining = target_samples - capture.size();
        const size_t append_count = std::min(remaining, chunk.size());
        capture.insert(capture.end(), chunk.begin(), chunk.begin() + append_count);
        if (append_count == 0) {
            break;
        }
    }

    return capture.size() >= target_samples;
}

bool WarmupCodec(AudioCodec* codec) {
    codec->EnableInput(true);
    codec->EnableOutput(true);
    vTaskDelay(kCodecWarmupDelayTicks);

    std::vector<int16_t> discard;
    return CaptureSamples(codec, static_cast<size_t>(kWarmupDiscardSamples), discard);
}

void PlaybackTask(void* arg) {
    auto* ctx = static_cast<PlaybackContext*>(arg);
    if (ctx->codec != nullptr && ctx->pcm != nullptr) {
        auto pcm_copy = *ctx->pcm;
        ctx->codec->OutputData(pcm_copy);
    }
    xTaskNotifyGive(ctx->owner_task);
    vTaskDelete(NULL);
}

AcousticSelftestRoundResult RunRound(AudioCodec* codec,
                                     const std::array<uint8_t, 3>& expected_payload,
                                     int round) {
    AcousticSelftestRoundResult result;
    result.round = round;
    result.fc = kFreqTable[static_cast<size_t>((round + expected_payload[2]) % static_cast<int>(kFreqTable.size()))];
    const size_t target_samples = static_cast<size_t>(kCapturePrerollSamples + kPacketSamples + kCaptureTailSamples);
    const size_t post_tx_samples = static_cast<size_t>(kPacketSamples + kCaptureTailSamples);

    codec->EnableInput(true);
    codec->EnableOutput(true);

    std::vector<int16_t> capture;
    capture.reserve(target_samples);
    if (!CaptureSamples(codec, static_cast<size_t>(kCapturePrerollSamples), capture)) {
        result.reason = AcousticSelftestFailureReason::kCaptureTimeout;
        return result;
    }

    std::vector<int16_t> tx_pcm = GeneratePacketPcm(expected_payload, result.fc);
    PlaybackContext playback_ctx;
    playback_ctx.codec = codec;
    playback_ctx.owner_task = xTaskGetCurrentTaskHandle();
    playback_ctx.pcm = &tx_pcm;
    ulTaskNotifyTake(pdTRUE, 0);
    if (xTaskCreate(PlaybackTask, "ft_audio_tx", 4096, &playback_ctx, 4, nullptr) != pdPASS) {
        result.reason = AcousticSelftestFailureReason::kInternalError;
        return result;
    }

    std::vector<int16_t> post_tx_capture;
    if (!CaptureSamples(codec, post_tx_samples, post_tx_capture)) {
        result.reason = AcousticSelftestFailureReason::kCaptureTimeout;
        return result;
    }
    if (ulTaskNotifyTake(pdTRUE, kPlaybackJoinTimeoutTicks) == 0) {
        result.reason = AcousticSelftestFailureReason::kInternalError;
        return result;
    }
    capture.insert(capture.end(), post_tx_capture.begin(), post_tx_capture.end());

    return DecodeCapture(capture, expected_payload, round, result.fc);
}

}  // namespace

AcousticSelftestSummary AcousticSelftest::Run(AudioCodec* codec, const std::array<uint8_t, 6>& mac) const {
    AcousticSelftestSummary summary;
    if (codec == nullptr || !codec->duplex() || codec->input_channels() != 1 || codec->output_channels() != 1 ||
        codec->input_sample_rate() != kSampleRate || codec->output_sample_rate() != kSampleRate) {
        summary.reason = AcousticSelftestFailureReason::kUnsupportedCodec;
        return summary;
    }

    if (!WarmupCodec(codec)) {
        summary.reason = AcousticSelftestFailureReason::kCaptureTimeout;
        ESP_LOGW(TAG, "factory_test type=audio_path warmup result=FAIL reason=%s",
                 FailureReasonToString(summary.reason));
        return summary;
    }

    const std::array<uint8_t, 3> expected_payload = {mac[3], mac[4], mac[5]};
    AcousticSelftestFailureReason best_reason = AcousticSelftestFailureReason::kInternalError;

    for (int round = 0; round < kMaxRounds; ++round) {
        const AcousticSelftestRoundResult round_result = RunRound(codec, expected_payload, round);
        if (round_result.pass) {
            summary.pass = true;
            summary.round = round_result.round + 1;
            summary.fc = round_result.fc;
            summary.reason = AcousticSelftestFailureReason::kNone;
            summary.payload = round_result.payload;
            summary.has_payload = round_result.has_payload;
            ESP_LOGI(TAG, "factory_test type=audio_path round=%d fc=%d result=PASS payload=%s",
                     summary.round, summary.fc, PayloadToHex(summary.payload).c_str());
            return summary;
        }

        best_reason = ChoosePreferredReason(best_reason, round_result.reason);
        summary.payload = round_result.payload;
        summary.has_payload = round_result.has_payload;
        if (round_result.has_payload) {
            ESP_LOGW(TAG,
                     "factory_test type=audio_path round=%d fc=%d result=FAIL reason=%s payload=%s expected=%s",
                     round_result.round + 1, round_result.fc, FailureReasonToString(round_result.reason),
                     PayloadToHex(round_result.payload).c_str(), PayloadToHex(expected_payload).c_str());
        } else {
            ESP_LOGW(TAG, "factory_test type=audio_path round=%d fc=%d result=FAIL reason=%s",
                     round_result.round + 1, round_result.fc, FailureReasonToString(round_result.reason));
        }
    }

    summary.reason = best_reason;
    return summary;
}

const char* AcousticSelftest::FailureReasonToString(AcousticSelftestFailureReason reason) {
    switch (reason) {
        case AcousticSelftestFailureReason::kNone:
            return "none";
        case AcousticSelftestFailureReason::kBusy:
            return "busy";
        case AcousticSelftestFailureReason::kUnsupportedCodec:
            return "unsupported_codec";
        case AcousticSelftestFailureReason::kCaptureTimeout:
            return "capture_timeout";
        case AcousticSelftestFailureReason::kDecodeNoSync:
            return "decode_no_sync";
        case AcousticSelftestFailureReason::kDecodeCrcMismatch:
            return "decode_crc_mismatch";
        case AcousticSelftestFailureReason::kDecodeMacMismatch:
            return "decode_mac_mismatch";
        case AcousticSelftestFailureReason::kInternalError:
        default:
            return "internal_error";
    }
}
