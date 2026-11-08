#include <cassert>
#include <cstdio>
#include "PortalValidation.h"

int main() {
  assert(!bridge::validAdminPassword(nullptr));
  assert(!bridge::validAdminPassword(""));
  assert(!bridge::validAdminPassword("short123"));
  assert(bridge::validAdminPassword("This-Is-Unique-2026!"));
  assert(!bridge::validAdminPassword("with spaces 123456789"));
  assert(!bridge::validAdminPassword("new\nline1234567890"));
  const char *longPass = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  assert(!bridge::validAdminPassword(longPass));
  uint32_t id = 0;
  assert(bridge::validPositiveId("2", id) && id == 2);
  assert(bridge::validPositiveId("429", id) && id == 429);
  assert(!bridge::validPositiveId("0", id));
  assert(!bridge::validPositiveId("-1", id));
  assert(!bridge::validPositiveId("12x", id));
  assert(!bridge::validPositiveId("4294967295", id));
  assert(bridge::kPortalPort != 80);
  puts("portal validation checks passed");
}
