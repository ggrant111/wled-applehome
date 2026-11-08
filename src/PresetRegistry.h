#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "PresetPolicy.h"

class DeviceRegistry;

struct PresetRecord {
  uint32_t aid = 0;       // Stable HomeKit accessory ID; 0 = empty slot.
  uint32_t deviceAid = 0; // WLED bridged light HomeKit AID.
  uint8_t presetId = 0;   // WLED preset 1..250
  String name;
};

enum class PresetEnrollResult {
  Added,
  Exists,
  Full,
  Invalid,
  PersistenceFailed
};

class PresetRegistry {
public:
  bool begin();
  unsigned count() const;
  const PresetRecord &at(unsigned index) const { return records_[index]; }
  int indexByAid(uint32_t aid) const;
  int indexByPair(uint32_t deviceAid, uint8_t presetId) const;

  PresetEnrollResult add(DeviceRegistry &devices, uint32_t deviceAid,
                         uint8_t presetId, const String &name,
                         uint32_t *outAid = nullptr);
  bool remove(uint32_t accessoryAid);
  unsigned removeForDevice(uint32_t deviceAid);
  void list() const;

private:
  Preferences prefs_;
  PresetRecord records_[bridge::kMaxExposedPresets];
  String key(unsigned slot, const char *field) const;
  bool save(unsigned slot);
};
