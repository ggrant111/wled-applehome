#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "DeviceRegistry.h"
#include "PresetPolicy.h"

// Experimental opt-in: one effects controller for one existing WLED light.
 // Conversion from the v0.7 bridged TV to grouped named Switch services is
 // explicit and retains its selected WLED preset list.
// All IDs are persistent; unrelated HomeKit lights/switches never change.
namespace bridge { constexpr unsigned kMaxEffectsInputs = 5; }

struct EffectsInput {
  uint8_t presetId = 0;   // 0 = unconfigured input
  String name;
};

enum class EffectsControllerMode : uint8_t { Television = 0, GroupedButtons = 1 };

struct EffectsControllerRecord {
  EffectsControllerMode mode = EffectsControllerMode::Television;
  uint32_t aid = 0;       // TV-style HomeKit AID
  uint32_t deviceAid = 0; // Existing LightBulb AID
  EffectsInput inputs[bridge::kMaxEffectsInputs];
  unsigned count() const {
    unsigned n = 0;
    for (const auto &input : inputs) if (input.presetId) ++n;
    return n;
  }
};

enum class EffectsAddResult {
  Created, Added, Exists, Full, AssignedToAnotherLight,
  Invalid, PersistenceFailed
};

class EffectsControllerRegistry {
public:
  bool begin();
  const EffectsControllerRecord &record() const { return record_; }
  bool configured() const { return record_.aid != 0; }
  bool hasInput(uint8_t preset) const;
  int indexByPreset(uint8_t preset) const;
  EffectsAddResult add(DeviceRegistry &devices, uint32_t deviceAid,
                       uint8_t presetId, const String &name,
                       uint32_t *controllerAid = nullptr);
  // Returns a new, reserved HomeKit AID on success, zero on failure.
  // The TV AID is not reused; switch accessory is activated on reboot.
  uint32_t convertToGroupedButtons(DeviceRegistry &devices);
  bool remove(uint32_t aid);
  void list() const;
private:
  Preferences prefs_;
  EffectsControllerRecord record_;
  String key(unsigned position, const char *prefix) const;
  bool saveInput(unsigned position, uint8_t presetId, const String &name);
};
