#include <cassert>
#include <cstdio>
#include "DeviceRegistry.h"

int main() {
  // Simulate real v0.1 migration: its IP is present but it has NO MAC.
  // The ESP32 Preferences::putString() returns 0 for a SUCCESSFUL empty string.
  Preferences::storage.clear();
  Preferences legacy;
  assert(legacy.begin("wled-home", false));
  assert(legacy.putString("ip", "192.168.1.22") != 0);
  legacy.end();

  DeviceRegistry registry;
  assert(registry.begin());
  assert(registry.activeCount() == 1);
  assert(registry.indexByAid(2) == 0);
  assert(registry.at(0).ip == "192.168.1.22");
  assert(registry.at(0).mac.isEmpty());

  Preferences inspect;
  assert(inspect.begin("wled-reg", true));
  assert(inspect.isKey("mac0"));
  assert(inspect.getString("mac0") == "");
  inspect.end();

  // Changing the original IP must work while its MAC is still empty.
  assert(registry.updateLegacyIp("192.168.1.23"));
  assert(registry.at(0).ip == "192.168.1.23");

  uint32_t aid = 0;
  auto result = registry.enroll("192.168.1.23", "AA1122334455", "Main Light", &aid);
  assert(result == EnrollResult::UpdatedIp && aid == 2);
  assert(registry.activeCount() == 1);

  result = registry.enroll("192.168.1.40", "BB1122334455", "Second Light", &aid);
  assert(result == EnrollResult::Added && aid == 3);
  result = registry.enroll("192.168.1.41", "BB1122334455", "Second Light", &aid);
  assert(result == EnrollResult::UpdatedIp && aid == 3);

  // Simulate a reboot: IDs, endpoints and allocator high watermark persist.
  DeviceRegistry afterReboot;
  assert(afterReboot.begin());
  assert(afterReboot.indexByAid(2) >= 0);
  assert(afterReboot.indexByAid(3) >= 0);
  assert(afterReboot.indexByIp("192.168.1.41") >= 0);
  assert(afterReboot.allocateAid() == 4);

  result = afterReboot.enroll("192.168.1.50", "CC1122334455", "Third Light", &aid);
  assert(result == EnrollResult::Added && aid == 5);
  assert(afterReboot.removeByAid(3));
  result = afterReboot.enroll("192.168.1.60", "DD1122334455", "New Light", &aid);
  assert(result == EnrollResult::Added && aid == 6);
  assert(afterReboot.indexByAid(3) == -1);
  std::puts("DeviceRegistry v0.1 migration, reboot, dedupe and stable IDs passed");
}
