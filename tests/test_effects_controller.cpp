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
  assert(afterReboot.remove(3));
  EffectsControllerRegistry removed;
  assert(removed.begin() && !removed.configured());
  assert(devices.allocateAid()==5); // TV AID 3 is never re-used.
  std::puts("Experimental HomeKit effects TV registry and AID persistence passed");
}
