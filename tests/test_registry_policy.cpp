#include <cassert>
#include <cstdio>
#include "RegistryPolicy.h"

int main() {
  static_assert(bridge::kMaxDevices == 8, "Bridge capacity must remain 8");
  assert(bridge::safeNextAid(0, 0) == 3);
  assert(bridge::safeNextAid(3, 2) == 3);
  assert(bridge::safeNextAid(3, 9) == 10);
  assert(bridge::safeNextAid(99, 4) == 99);
  assert(bridge::validAid(2));
  assert(bridge::validAid(65536));
  assert(!bridge::validAid(0));
  assert(!bridge::validAid(1));
  puts("registry policy checks passed");
}
