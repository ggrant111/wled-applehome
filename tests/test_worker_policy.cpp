#include <cassert>
#include <cstdio>
#include "WorkerPolicy.h"

int main() {
  static_assert(bridge::kWorkerMaxDevices == 8, "8 WLED controllers");
  assert(bridge::timeReached(100, 99));
  assert(bridge::timeReached(100, 100));
  assert(!bridge::timeReached(99, 100));
  assert(bridge::timeReached(3, 0xFFFFFFFDU)); // millis wrap
  assert(!bridge::timeReached(0xFFFFFFFDU, 3));
  assert(bridge::isFreshRead(10,10,3,3,0));
  assert(!bridge::isFreshRead(10,11,3,3,0));
  assert(!bridge::isFreshRead(10,10,3,4,0));
  assert(!bridge::isFreshRead(10,10,3,3,1));
  assert(bridge::workerRetryInterval(true) == 8000);
  assert(bridge::workerRetryInterval(false) == 30000);
  std::puts("background network worker scheduling and stale-read policy passed");
}
