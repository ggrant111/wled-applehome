#pragma once
#include <cstddef>
#include <cstdint>

namespace bridge {
constexpr uint16_t kPortalPort = 8080;
constexpr size_t kMinAdminPassword = 14;
constexpr size_t kMaxAdminPassword = 64;
constexpr uint32_t kMinOtaAppHeadroom = 70000;

inline bool validAdminPassword(const char *value) {
  if (!value) return false;
  size_t count = 0;
  for (const unsigned char *p = reinterpret_cast<const unsigned char *>(value);
       *p; ++p) {
    if (*p < 33 || *p > 126 || ++count > kMaxAdminPassword) return false;
  }
  return count >= kMinAdminPassword;
}

inline bool validPositiveId(const char *text, uint32_t &out) {
  if (!text || !*text) return false;
  uint64_t v = 0;
  for (const char *p = text; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    v = v * 10 + static_cast<unsigned>(*p - '0');
    if (v >= 0xFFFFFFF0ULL) return false;
  }
  if (v == 0) return false;
  out = static_cast<uint32_t>(v);
  return true;
}
}
