#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "DeviceRegistry.h"
#include "PresetPolicy.h"

// Up to eight WLED lights may each have their own opt-in Effects accessory.
// Slot zero reads/writes the original v0.7/v0.8 "wled-fx" NVS untouched.
// Nine additional slots in "wled-fx9" allow eight new controllers PLUS
// a spare for atomic structural edits (new HomeKit AID before old is retired).
namespace bridge {
constexpr unsigned kMaxEffectsInputs = 5;
constexpr unsigned kMaxEffectsControllers = 8;
constexpr unsigned kEffectsSlots = kMaxEffectsControllers + 2;
}

struct EffectsInput {
  uint8_t presetId = 0;
  String name;
};

enum class EffectsControllerMode : uint8_t { Television = 0, GroupedButtons = 1 };

struct EffectsControllerRecord {
  EffectsControllerMode mode = EffectsControllerMode::GroupedButtons;
  uint32_t aid = 0;
  uint32_t deviceAid = 0;
  EffectsInput inputs[bridge::kMaxEffectsInputs];
  unsigned count() const {
    unsigned n = 0;
    for (const auto &input : inputs) if (input.presetId) ++n;
    return n;
  }
};

enum class EffectsAddResult {
  Created, Added, Exists, Full, ControllerLimit,
  AssignedToAnotherLight, Invalid, PersistenceFailed
};

enum class EffectsEditResult {
  Rebuilt, RemovedController, NoChange, NotFound, Invalid, PersistenceFailed
};

class EffectsControllerRegistry {
public:
  bool begin();
  unsigned activeCount() const;
  bool configured() const { return activeCount() != 0; }
  const EffectsControllerRecord &at(unsigned slot) const { return records_[slot]; }
  // Legacy compatibility helper for native tests; modern code uses at().
  const EffectsControllerRecord &record() const { return records_[0]; }
  int indexByDeviceAid(uint32_t deviceAid) const;
  int indexByAid(uint32_t controllerAid) const;
  int indexByPreset(unsigned slot, uint8_t preset) const;
  bool hasInput(uint8_t preset) const { return indexByPreset(0, preset) >= 0; }
  int indexByPreset(uint8_t preset) const { return indexByPreset(0, preset); }
  EffectsAddResult add(DeviceRegistry &devices, uint32_t deviceAid,
                       uint8_t presetId, const String &name,
                       uint32_t *controllerAid = nullptr);
  // Renaming changes only ConfiguredName, not HomeKit service topology.
  bool rename(uint32_t controllerAid, uint8_t preset, const String &name);
  // Removal or changing the order changes service IIDs. Explicitly rebuild
  // under a NEW AID, leaving every unrelated light/group/scene intact.
  // direction=0 removes the preset, -1 moves earlier, +1 moves later.
  EffectsEditResult editStructure(DeviceRegistry &devices, uint32_t controllerAid,
                                  uint8_t preset, int direction,
                                  uint32_t *newAid = nullptr);
  uint32_t convertToGroupedButtons(DeviceRegistry &devices, uint32_t aid);
  uint32_t convertToGroupedButtons(DeviceRegistry &devices); // old API
  bool remove(uint32_t controllerAid);
  unsigned removeForDevice(uint32_t deviceAid);
  void list() const;
private:
  Preferences legacy_;
  Preferences modern_;
  EffectsControllerRecord records_[bridge::kEffectsSlots];
  String field(unsigned slot, const char *prefix) const;
  String inputKey(unsigned slot, unsigned input, const char *prefix) const;
  bool saveInput(unsigned slot, unsigned input, uint8_t preset, const String &name);
  bool saveModern(unsigned slot, const EffectsControllerRecord &record);
  bool retire(unsigned slot);
  int emptyModernSlot() const;
};
