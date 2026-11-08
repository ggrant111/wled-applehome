#include <cassert>
#include <cstdio>
#include "PresetPowerPolicy.h"

int main() {
  // Only the current, powered WLED saved preset should show On.
  assert(bridge::presetPowerOn(true, 5, 5));
  assert(!bridge::presetPowerOn(false, 5, 5));
  assert(!bridge::presetPowerOn(true, 4, 5));
  assert(!bridge::presetPowerOn(true, -1, 5));
  assert(!bridge::presetPowerOn(true, 5, 0));
  // WLED playlist uses pl=playlist resource ID while ps changes to each
  // internal step. Playlist button stays On throughout all steps.
  assert(bridge::presetOrPlaylistPowerOn(true, 11, 37, 37, true));
  assert(bridge::presetOrPlaylistPowerOn(true, 12, 37, 37, true));
  assert(bridge::presetOrPlaylistPowerOn(true, 13, 37, 37, true));
  assert(!bridge::presetOrPlaylistPowerOn(true, 13, -1, 37, true));
  assert(!bridge::presetOrPlaylistPowerOn(false, 11, 37, 37, true));
  assert(!bridge::presetOrPlaylistPowerOn(true, 11, 42, 37, true));
  // A step preset should NOT be displayed as independently selected while a
  // playlist is playing; single-preset controls recover when pl returns -1.
  assert(!bridge::presetOrPlaylistPowerOn(true, 11, 37, 11, false));
  assert(bridge::presetOrPlaylistPowerOn(true, 11, -1, 11, false));
  assert(!bridge::presetOrPlaylistPowerOn(true, -1, -1, 11, false));
  // Finite playlists stop reporting pl when complete, so their button goes Off.
  assert(!bridge::presetOrPlaylistPowerOn(true, 11, -1, 37, true));
  // Saved segment presets without start/stop data cannot restore bounds.
  assert(bridge::presetHasCompleteBounds(1,1));
  assert(bridge::presetHasCompleteBounds(4,4));
  assert(!bridge::presetHasCompleteBounds(4,0));
  assert(!bridge::presetHasCompleteBounds(4,2));
  assert(!bridge::presetHasCompleteBounds(0,0)); // macro or non-segment preset
  std::puts("Preset/playlist power synchronization and segment bounds rules passed");
}
