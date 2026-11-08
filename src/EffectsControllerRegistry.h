#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "DeviceRegistry.h"
#include "PresetPolicy.h"

// Experimental opt-in: ONE bridged TV-style effects selector for ONE WLED light.
// Up to five saved WLED presets appear as labeled HomeKit input sources.
// All IDs are persistent; unrelated HomeKit lights/switches never change.
namespace bridge { constexpr unsigned kMaxEffectsInputs = 5; }

struct EffectsInput {
  uint8_t presetId = 0;   // 0 = unconfigured input
  String name;
};

struct EffectsControllerRecord {
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
  bool remove(uint32_t aid);
  void list() const;
private:
  Preferences prefs_;
  EffectsControllerRecord record_;
  String key(unsigned position, const char *prefix) const;
  bool saveInput(unsigned position, uint8_t presetId, const String &name);
};
