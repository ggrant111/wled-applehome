#include <cassert>
#include <cstdio>
#include "EffectsControllerRegistry.h"
#include "EffectsInputPolicy.h"

int main() {
  Preferences::storage.clear();
  DeviceRegistry devices;
  assert(devices.begin()); // Existing legacy AID=2 survives.
  EffectsControllerRegistry notEnabled;
  assert(notEnabled.begin());
  assert(!notEnabled.configured());
  assert(devices.indexByAid(2) >= 0);

  auto bad = notEnabled.add(devices, 400, 4, "No light");
  assert(bad == EffectsAddResult::Invalid);
  uint32_t tvAid = 0;
  assert(notEnabled.add(devices, 2, 5, "Rainbow", &tvAid)
         == EffectsAddResult::Created);
  assert(tvAid == 3); // Monotonic allocator; original light keeps AID=2.
  assert(notEnabled.record().count() == 1);
  assert(notEnabled.add(devices, 2, 5, "duplicate", &tvAid)
         == EffectsAddResult::Exists);
  assert(notEnabled.add(devices, 2, 6, "Fire", &tvAid)
         == EffectsAddResult::Added);
  assert(notEnabled.add(devices, 2, 7, "Twinkle", &tvAid)
         == EffectsAddResult::Added);
  assert(notEnabled.add(devices, 2, 8, "Breathe", &tvAid)
         == EffectsAddResult::Added);
  assert(notEnabled.add(devices, 2, 9, "Solid", &tvAid)
         == EffectsAddResult::Added);
  assert(notEnabled.add(devices, 2, 10, "Over capacity", &tvAid)
         == EffectsAddResult::Full);
  assert(!bridge::effectsInputIdentifierValid(0, 5));
  assert(bridge::effectsInputIdentifierValid(1, 5));
  assert(bridge::effectsInputIdentifierValid(5, 5));
  assert(!bridge::effectsInputIdentifierValid(6, 5));

  uint32_t otherAid=0;
  assert(devices.enroll("192.168.1.40","BB1122334455",
                        "Other light",&otherAid)==EnrollResult::Added);
  assert(otherAid==4); // TV ID 3 is reserved in shared global allocation.
  assert(notEnabled.add(devices,otherAid,12,"Fire")
         ==EffectsAddResult::AssignedToAnotherLight);

  EffectsControllerRegistry afterReboot;
  assert(afterReboot.begin());
  assert(afterReboot.configured());
  assert(afterReboot.record().aid==3);
  assert(afterReboot.record().deviceAid==2);
  assert(afterReboot.record().count()==5);
  assert(afterReboot.record().inputs[1].name=="Fire");

  // Switch away from Apple's generic bridged-TV tile. The light and saved
  // preset mapping survive, but the different HomeKit service gets a NEW AID.
  assert(afterReboot.convertToGroupedButtons(devices)==5);
  assert(afterReboot.record().mode==EffectsControllerMode::GroupedButtons);
  assert(afterReboot.record().aid==5);
  EffectsControllerRegistry groupedReboot;
  assert(groupedReboot.begin());
  assert(groupedReboot.record().aid==5);
  assert(groupedReboot.record().deviceAid==2);
  assert(groupedReboot.record().mode==EffectsControllerMode::GroupedButtons);
  assert(groupedReboot.record().count()==5);
  assert(groupedReboot.record().inputs[0].presetId==5);
  assert(groupedReboot.record().inputs[1].name=="Fire");
  assert(groupedReboot.convertToGroupedButtons(devices)==0);
  assert(groupedReboot.remove(5));
  EffectsControllerRegistry removed;
  assert(removed.begin() && !removed.configured());
  assert(devices.allocateAid()==6); // Neither TV AID 3 nor grouped AID 5 reused.
  std::puts("HomeKit effects TV-to-grouped conversion, NVS identity and AIDs passed");
}
