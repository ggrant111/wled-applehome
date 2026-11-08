#include "EffectsControllerRegistry.h"

String EffectsControllerRegistry::key(unsigned position, const char *prefix) const {
  return String(prefix) + String(position);
}

bool EffectsControllerRegistry::begin() {
  if (!prefs_.begin("wled-fx", false)) return false;
  record_ = {};
  const uint32_t aid = prefs_.getUInt("aid", 0);
  const uint32_t deviceAid = prefs_.getUInt("dev", 0);
  if (!aid) return true;  // Opt-in: no new HomeKit accessory by default.
  if (!bridge::validAid(aid) || aid <= 2 || !bridge::validAid(deviceAid)) {
    Serial.println("[Effects] Invalid controller identity in NVS; ignoring");
    return true;
  }
  record_.aid = aid;
  record_.deviceAid = deviceAid;
  for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i) {
    const uint32_t preset = prefs_.getUInt(key(i, "ps").c_str(), 0);
    if (!bridge::validPresetId(preset) || hasInput(static_cast<uint8_t>(preset)))
      continue;
    record_.inputs[i] = {static_cast<uint8_t>(preset),
      prefs_.getString(key(i, "nm").c_str(), "WLED Preset")};
  }
  if (!record_.count()) {
    Serial.println("[Effects] No valid input sources; controller not published");
    record_ = {};
  }
  list();
  return true;
}

bool EffectsControllerRegistry::hasInput(uint8_t preset) const {
  return indexByPreset(preset) >= 0;
}

int EffectsControllerRegistry::indexByPreset(uint8_t preset) const {
  if (!preset) return -1;
  for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i)
    if (record_.inputs[i].presetId == preset) return static_cast<int>(i);
  return -1;
}

bool EffectsControllerRegistry::saveInput(unsigned index, uint8_t preset,
                                           const String &name) {
  if (index >= bridge::kMaxEffectsInputs || !bridge::validPresetId(preset))
    return false;
  const String nameKey = key(index, "nm");
  prefs_.putString(nameKey.c_str(), name);
  if (!prefs_.isKey(nameKey.c_str()) || prefs_.getString(nameKey.c_str()) != name)
    return false;
  // Persist the preset ID LAST. A reboot during NVS writes must not
  // publish an incompletely configured HomeKit input.
  return prefs_.putUInt(key(index, "ps").c_str(), preset) == sizeof(uint32_t);
}

EffectsAddResult EffectsControllerRegistry::add(DeviceRegistry &devices,
                                                  uint32_t deviceAid,
                                                  uint8_t presetId,
                                                  const String &name,
                                                  uint32_t *controllerAid) {
  if (devices.indexByAid(deviceAid) < 0 || !bridge::validPresetId(presetId))
    return EffectsAddResult::Invalid;
  if (configured() && record_.deviceAid != deviceAid)
    return EffectsAddResult::AssignedToAnotherLight;
  if (hasInput(presetId)) {
    if (controllerAid) *controllerAid = record_.aid;
    return EffectsAddResult::Exists;
  }
  const unsigned slot = record_.count(); // Slots are filled in order; no per-slot remove.
  if (slot >= bridge::kMaxEffectsInputs) return EffectsAddResult::Full;

  String clean = name;
  clean.trim();
  if (clean.isEmpty()) clean = "WLED Preset";
  if (clean.length() > 30) clean = clean.substring(0, 30);

  // An allocated AID is never recycled, even if NVS write subsequently fails.
  const bool creating = !configured();
  const uint32_t newAid = creating ? devices.allocateAid() : record_.aid;
  if (!newAid) return EffectsAddResult::PersistenceFailed;
  if (!saveInput(slot, presetId, clean)) return EffectsAddResult::PersistenceFailed;
  if (creating) {
    if (prefs_.putUInt("dev", deviceAid) != sizeof(uint32_t)
        || prefs_.putUInt("aid", newAid) != sizeof(uint32_t))
      return EffectsAddResult::PersistenceFailed;
    record_.aid = newAid;
    record_.deviceAid = deviceAid;
  }
  record_.inputs[slot] = {presetId, clean};
  if (controllerAid) *controllerAid = newAid;
  return creating ? EffectsAddResult::Created : EffectsAddResult::Added;
}

bool EffectsControllerRegistry::remove(uint32_t aid) {
  if (!configured() || record_.aid != aid) return false;
  // Deactivate AID first; reboot cannot revive a half-deleted controller.
  if (prefs_.putUInt("aid", 0) != sizeof(uint32_t)) return false;
  record_ = {};
  prefs_.remove("dev");
  for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i) {
    prefs_.remove(key(i, "ps").c_str());
    prefs_.remove(key(i, "nm").c_str());
  }
  return true;
}

void EffectsControllerRegistry::list() const {
  if (!configured()) {
    Serial.println("[Effects] No experimental TV controller enrolled");
    return;
  }
  Serial.printf("[Effects] TV AID=%lu for light AID=%lu: %u preset inputs\n",
                static_cast<unsigned long>(record_.aid),
                static_cast<unsigned long>(record_.deviceAid), record_.count());
  for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i) {
    const auto &entry = record_.inputs[i];
    if (entry.presetId)
      Serial.printf("  Input %u: WLED preset %u (%s)\n",
                    i+1, static_cast<unsigned>(entry.presetId), entry.name.c_str());
  }
}
