#include <cassert>
#include <cstdio>
#include "BridgeMath.h"

int main() {
  using bridge::homeToWledBrightness;
  using bridge::wledToHomeBrightness;
  assert(homeToWledBrightness(0) == 1);    // off is separate from brightness
  assert(homeToWledBrightness(1) == 3);
  assert(homeToWledBrightness(50) == 128);
  assert(homeToWledBrightness(100) == 255);
  assert(homeToWledBrightness(500) == 255);
  assert(wledToHomeBrightness(-1) == 0);
  assert(wledToHomeBrightness(128) == 50);
  assert(wledToHomeBrightness(255) == 100);
  int previous = 0;
  for (int percent = 1; percent <= 100; ++percent) {
    const int mapped = homeToWledBrightness(percent);
    assert(mapped >= previous && mapped >= 1 && mapped <= 255);
    previous = mapped;
    const int roundTrip = wledToHomeBrightness(mapped);
    assert(roundTrip >= percent - 1 && roundTrip <= percent + 1);
  }
  puts("brightness mapping checks passed");
}
