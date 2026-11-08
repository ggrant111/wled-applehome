#pragma once
#include <cstdint>

namespace bridge {
// Keep HomeKit uncluttered and within a 4 MB classic ESP32's memory budget.
// This is a bridge-wide limit, not a limit on WLED's saved presets.
constexpr unsigned kMaxExposedPresets = 12;
constexpr unsigned kMaxWledPresetId = 250;

inline bool validPresetId(unsigned id) {
  return id >= 1 && id <= kMaxWledPresetId;
}
inline bool validPresetPair(uint32_t deviceAid, unsigned presetId) {
  return deviceAid >= 2 && validPresetId(presetId);
}
inline bool needsStateReconcile(uint32_t seenRevision, uint32_t currentRevision,
                                uint32_t millisSinceWrite) {
  return seenRevision != currentRevision && millisSinceWrite >= 450;
}
}
