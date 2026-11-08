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
  // Saved segment presets without start/stop data cannot restore bounds.
  assert(bridge::presetHasCompleteBounds(1,1));
  assert(bridge::presetHasCompleteBounds(4,4));
  assert(!bridge::presetHasCompleteBounds(4,0));
  assert(!bridge::presetHasCompleteBounds(4,2));
  assert(!bridge::presetHasCompleteBounds(0,0)); // macro or non-segment preset
  std::puts("Stateful grouped effect power and segment bounds rules passed");
}
