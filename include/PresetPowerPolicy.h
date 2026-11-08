#pragma once

// WLED has two independent IDs: ps = current applied preset (can be a step
// inside a playlist); pl = active playlist resource ID (while cycling).
namespace bridge {
inline bool presetOrPlaylistPowerOn(bool wledOn, int currentPreset,
                                    int currentPlaylist, int selectedId,
                                    bool isPlaylist) {
  if (!wledOn || selectedId <= 0) return false;
  if (isPlaylist) return currentPlaylist == selectedId;
  // Never highlight an individual playlist step as if selected separately.
  return currentPlaylist < 0 && currentPreset == selectedId;
}
inline bool presetPowerOn(bool wledOn, int observedPreset, int selectedPreset) {
  return presetOrPlaylistPowerOn(wledOn, observedPreset, -1, selectedPreset, false);
}
inline bool presetHasCompleteBounds(unsigned segmentCount, unsigned withBounds) {
  return segmentCount > 0 && segmentCount == withBounds;
}
}