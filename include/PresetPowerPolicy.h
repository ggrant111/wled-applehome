#pragma once

// Pure HomeKit/segment readback rules; no device or network dependency.
namespace bridge {
inline bool presetPowerOn(bool wledOn, int observedPreset, int selectedPreset) {
  return wledOn && selectedPreset > 0 && observedPreset == selectedPreset;
}
inline bool presetHasCompleteBounds(unsigned segmentCount, unsigned withBounds) {
  return segmentCount > 0 && segmentCount == withBounds;
}
}
