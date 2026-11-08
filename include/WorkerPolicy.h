#pragma once
#include <cstdint>

namespace bridge {
constexpr uint32_t kWorkerPollMs = 8000;
constexpr uint32_t kWorkerOfflineRetryMs = 30000;
constexpr uint32_t kWorkerWriteSettleMs = 450;
constexpr unsigned kWorkerQueueLength = 24;
constexpr unsigned kWorkerMaxDevices = 8;

// Correct across millis() wrap-around for durations under 2^31 ms.
inline bool timeReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

// A GET sent before a new command must not roll back HomeKit state.
inline bool isFreshRead(uint32_t startedEpoch, uint32_t currentEpoch,
                        uint32_t startedHostVersion, uint32_t currentHostVersion,
                        unsigned pendingCommands) {
  return startedEpoch == currentEpoch &&
         startedHostVersion == currentHostVersion &&
         pendingCommands == 0;
}

inline uint32_t workerRetryInterval(bool success) {
  return success ? kWorkerPollMs : kWorkerOfflineRetryMs;
}
} // namespace bridge
