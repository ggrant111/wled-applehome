#pragma once
#include <cstdint>

namespace bridge {
constexpr unsigned kMaxDevices = 8;

inline uint32_t safeNextAid(uint32_t persisted, uint32_t highestExisting) {
  const uint32_t fromExisting = highestExisting < 2 ? 3 : highestExisting + 1;
  if (persisted < 3) persisted = 3;
  return persisted > fromExisting ? persisted : fromExisting;
}

inline bool validAid(uint32_t aid) { return aid >= 2 && aid < 0xFFFFFFF0UL; }
}
