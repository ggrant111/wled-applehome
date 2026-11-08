#include <cassert>
#include <cstdio>
#include "PresetPolicy.h"

int main() {
  static_assert(bridge::kMaxExposedPresets == 12, "Bridge preset budget");
  assert(!bridge::validPresetId(0));
  assert(bridge::validPresetId(1));
  assert(bridge::validPresetId(250));
  assert(!bridge::validPresetId(251));
  assert(!bridge::validPresetPair(1, 20));
  assert(bridge::validPresetPair(2, 20));
  assert(!bridge::needsStateReconcile(0, 0, 1000));
  assert(!bridge::needsStateReconcile(1, 2, 449));
  assert(bridge::needsStateReconcile(1, 2, 450));
  assert(bridge::needsStateReconcile(1, 2, 1000));
  puts("preset and reconciliation policy tests passed");
}
