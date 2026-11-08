#include <cassert>
#include <cstdio>
#include "EffectsControllerRegistry.h"

static void addLight(DeviceRegistry &devices, unsigned i, uint32_t *aid) {
  char mac[13];
  std::snprintf(mac, sizeof(mac), "AA00%08X", i);
  assert(devices.enroll(String("192.168.1.") + String(i + 20),
                        String(mac), String("Light ") + String(i), aid)
          == EnrollResult::Added);
}

int main() {
  // Simulate the ACTUAL v0.8 grouped accessory, including the retired
  // Television identity. v0.9 must retain AID=4 and original ps0/nm0 keys.
  Preferences::storage.clear();
  DeviceRegistry devices;
  assert(devices.begin());
  assert(devices.allocateAid() == 3); // old TV (retired)
  assert(devices.allocateAid() == 4); // existing working grouped Switch
  Preferences legacy;
  assert(legacy.begin("wled-fx", false));
  legacy.putUInt("aid", 3);
  legacy.putUInt("switchAid", 4);
  legacy.putUInt("mode", 1);
  legacy.putUInt("dev", 2);
  legacy.putString("nm0", "Mammoth");
  legacy.putUInt("ps0", 5);
  legacy.end();

  EffectsControllerRegistry fx;
  assert(fx.begin());
  assert(fx.activeCount() == 1);
  assert(fx.at(0).aid == 4);
  assert(fx.at(0).deviceAid == 2);
  assert(fx.at(0).mode == EffectsControllerMode::GroupedButtons);
  assert(fx.at(0).inputs[0].name == "Mammoth");
  uint32_t existingAid = 0;
  assert(fx.add(devices, 2, 8, "Wednesday", &existingAid)
         == EffectsAddResult::Added);
  assert(existingAid == 4); // No legacy AID change upon append.
  assert(fx.add(devices, 2, 8, "duplicate") == EffectsAddResult::Exists);
  assert(fx.rename(4, 8, "Wednesday Revised"));
  assert(!fx.rename(4, 99, "Missing"));

  uint32_t lightAids[7] = {};
  uint32_t effectAids[7] = {};
  for (unsigned i = 0; i < 7; ++i) {
    addLight(devices, i, &lightAids[i]);
    assert(fx.add(devices, lightAids[i], 10 + i,
                  String("Preset ") + String(i), &effectAids[i])
           == EffectsAddResult::Created);
    assert(effectAids[i] > 4);
    assert(fx.indexByDeviceAid(lightAids[i]) > 0);
    assert(fx.indexByAid(effectAids[i]) > 0);
    assert(fx.add(devices, lightAids[i], 10 + i, "duplicate")
           == EffectsAddResult::Exists);
  }
  assert(fx.activeCount() == 8);
  // A ninth WLED light cannot be enrolled; eight groups is the maximum.
  uint32_t unused = 0;
  assert(devices.enroll("192.168.1.99", "AA0000000099", "Ninth", &unused)
         == EnrollResult::Full);
  const uint32_t otherGroupAid = effectAids[1];

  EffectsControllerRegistry reboot;
  assert(reboot.begin());
  assert(reboot.activeCount() == 8);
  assert(reboot.at(0).aid == 4);
  assert(reboot.at(0).inputs[1].name == "Wednesday Revised");
  for (unsigned i = 0; i < 7; ++i)
    assert(reboot.at(reboot.indexByDeviceAid(lightAids[i])).aid == effectAids[i]);

  // Adding a second favorite to one new group does NOT replace its AID/IIDs.
  assert(reboot.add(devices, lightAids[0], 31, "Twinkle") == EffectsAddResult::Added);
  assert(reboot.at(reboot.indexByDeviceAid(lightAids[0])).aid == effectAids[0]);

  // Changing service order REQUIRES a new AID. No other group is disturbed.
  uint32_t reorderAid = 0;
  assert(reboot.editStructure(devices, effectAids[0], 31, -1, &reorderAid)
         == EffectsEditResult::Rebuilt);
  assert(reorderAid > effectAids[6]);
  assert(reboot.at(reboot.indexByDeviceAid(lightAids[0])).aid == reorderAid);
  assert(reboot.at(reboot.indexByDeviceAid(lightAids[0])).inputs[0].presetId == 31);
  assert(reboot.at(reboot.indexByDeviceAid(lightAids[0])).inputs[1].presetId == 10);
  assert(reboot.at(reboot.indexByDeviceAid(lightAids[1])).aid == otherGroupAid);
  assert(reboot.at(0).aid == 4); // v0.8 grouped controller survives unchanged.

  uint32_t removalAid = 0;
  assert(reboot.editStructure(devices, reorderAid, 10, 0, &removalAid)
         == EffectsEditResult::Rebuilt);
  assert(removalAid > reorderAid);
  assert(reboot.at(reboot.indexByDeviceAid(lightAids[0])).count() == 1);
  assert(reboot.editStructure(devices, removalAid, 31, 0)
         == EffectsEditResult::RemovedController);
  assert(reboot.indexByDeviceAid(lightAids[0]) == -1);
  assert(reboot.activeCount() == 7);

  EffectsControllerRegistry reboot2;
  assert(reboot2.begin());
  assert(reboot2.activeCount() == 7);
  assert(reboot2.at(0).aid == 4);
  assert(reboot2.at(reboot2.indexByDeviceAid(lightAids[1])).aid == otherGroupAid);

  // Deleting one light only deletes its associated effects group.
  assert(reboot2.removeForDevice(lightAids[1]) == 1);
  assert(reboot2.indexByDeviceAid(lightAids[1]) == -1);
  assert(reboot2.at(0).aid == 4);
  assert(reboot2.activeCount() == 6);

  // Simulate crash after committing a replacement's new AID but before
  // retiring the old one: newest AID wins; old legacy identity is retired.
  uint32_t recoveredAid = devices.allocateAid();
  assert(recoveredAid > 4);
  Preferences modern;
  assert(modern.begin("wled-fx9", false));
  modern.putUInt("d8", 2);
  modern.putString("n8_0", "Recovered Mammoth");
  modern.putUInt("p8_0", 5);
  modern.putUInt("a8", recoveredAid);
  modern.end();
  EffectsControllerRegistry recovered;
  assert(recovered.begin());
  assert(recovered.indexByDeviceAid(2) >= 0);
  assert(recovered.at(recovered.indexByDeviceAid(2)).aid == recoveredAid);
  assert(recovered.at(0).aid == 0); // Stale legacy group suppressed.
  Preferences verifyLegacy;
  assert(verifyLegacy.begin("wled-fx", true));
  assert(verifyLegacy.getUInt("switchAid", 0) == 0);
  verifyLegacy.end();

  // Also preserve the v0.7 TV-to-grouped conversion path after upgrade.
  Preferences::storage.clear();
  DeviceRegistry fresh;
  assert(fresh.begin());
  assert(fresh.allocateAid() == 3);
  Preferences tvPrefs;
  assert(tvPrefs.begin("wled-fx", false));
  tvPrefs.putUInt("aid", 3);
  tvPrefs.putUInt("dev", 2);
  tvPrefs.putString("nm0", "Rainbow");
  tvPrefs.putUInt("ps0", 7);
  tvPrefs.end();
  EffectsControllerRegistry tvFx;
  assert(tvFx.begin());
  assert(tvFx.at(0).mode == EffectsControllerMode::Television);
  assert(tvFx.convertToGroupedButtons(fresh, 3) == 4);
  assert(tvFx.at(0).aid == 4);
  assert(tvFx.at(0).mode == EffectsControllerMode::GroupedButtons);

  std::puts("v0.9 eight controllers, legacy AIDs, independent group rebuilds and crash recovery passed");
}
