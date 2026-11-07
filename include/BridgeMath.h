#pragma once

// Plain C++ helpers; platform-independent so they can be exercised with g++.
namespace bridge {
inline int clampPercent(int value) {
  return value < 0 ? 0 : (value > 100 ? 100 : value);
}

// WLED brightness uses 1..255 when on; use On=false for off.
inline int homeToWledBrightness(int percent) {
  percent = clampPercent(percent);
  if (percent <= 0) return 1;
  return (percent * 255 + 50) / 100;
}

inline int wledToHomeBrightness(int raw) {
  if (raw <= 0) return 0;
  if (raw > 255) raw = 255;
  return (raw * 100 + 127) / 255;
}
}
