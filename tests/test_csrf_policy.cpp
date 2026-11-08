#include <cassert>
#include <cstdio>
#include "CsrfPolicy.h"

int main() {
  constexpr char good[] = "ABCDEF0123456789ABCDEF01";
  constexpr char stale[] = "ABCDEF0123456789ABCDEF02";
  assert(bridge::authorizedCsrf(good, good, nullptr));
  assert(bridge::authorizedCsrf(good, nullptr, good)); // Legacy form field
  assert(bridge::authorizedCsrf(good, stale, good));   // Fallback
  assert(!bridge::authorizedCsrf(good, stale, stale));
  assert(!bridge::authorizedCsrf(good, "", nullptr));
  assert(!bridge::authorizedCsrf(good, nullptr, ""));
  assert(!bridge::authorizedCsrf("", "", ""));
  assert(!bridge::authorizedCsrf(good, "ABCDEF0123456789ABCDEF0", nullptr));
  assert(!bridge::authorizedCsrf(good, "ABCDEF0123456789ABCDEF010", nullptr));
  assert(!bridge::authorizedCsrf(good, "ABCDEF0123456789ABCDEFAA", nullptr));
  std::puts("CSRF policy tests passed");
}
