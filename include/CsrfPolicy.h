#pragma once
#include <cstring>

namespace bridge {
// True only if a supplied token exactly matches the current 24-digit secret.
inline bool equalCsrfToken(const char *expected, const char *provided) {
  if (!expected || !provided) return false;
  const size_t len = std::strlen(expected);
  if (len != 24 || std::strlen(provided) != len) return false;
  unsigned char diff = 0;
  for (size_t i = 0; i < len; ++i)
    diff |= static_cast<unsigned char>(expected[i] ^ provided[i]);
  return diff == 0;
}
inline bool authorizedCsrf(const char *expected, const char *header,
                           const char *form) {
  return equalCsrfToken(expected, header) || equalCsrfToken(expected, form);
}
} // namespace bridge
