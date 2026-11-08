#include "EffectsControllerRegistry.h"

namespace {
String cleanEffectsName(String name) {
  name.trim();
  if (name.isEmpty()) name = "WLED Preset";
  if (name.length() > 30) name = name.substring(0, 30);
  return name;
}
}

String EffectsControllerRegistry::field(unsigned slot, const char *prefix) const {
  return String(prefix) + String(slot - 1); // Modern slots 1..9 => keys "a0".."a8"
}
String EffectsControllerRegistry::inputKey(unsigned slot, unsigned input,
                                          const char *prefix) const {
  if (slot == 0) return String(prefix) + String(input);  // v0.8 key untouched
  return field(slot, prefix) + String("_") + String(input);
}

unsigned EffectsControllerRegistry::activeCount() const {
  unsigned count = 0;
  for (const auto &r : records_) if (r.aid) ++count;
  return count;
}
int EffectsControllerRegistry::indexByDeviceAid(uint32_t lightAid) const {
  if (!lightAid) return -1;
  for (unsigned i = 0; i < bridge::kEffectsSlots; ++i)
    if (records_[i].aid && records_[i].deviceAid == lightAid) return static_cast<int>(i);
  return -1;
}
int EffectsControllerRegistry::indexByAid(uint32_t aid) const {
  if (!aid) return -1;
  for (unsigned i = 0; i < bridge::kEffectsSlots; ++i)
    if (records_[i].aid == aid) return static_cast<int>(i);
  return -1;
}
int EffectsControllerRegistry::indexByPreset(unsigned slot, uint8_t preset) const {
  if (slot >= bridge::kEffectsSlots || !preset) return -1;
  for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i)
    if (records_[slot].inputs[i].presetId == preset) return static_cast<int>(i);
  return -1;
}

bool EffectsControllerRegistry::begin() {
  if (!legacy_.begin("wled-fx", false) || !modern_.begin("wled-fx9", false))
    return false;
  for (auto &r : records_) r = {};
  // Preserve original TV/grouped selector's AID, device AID, input order,
  // and exact NVS keys. Do NOT migrate or recreate its HomeKit accessory.
  const bool grouped = legacy_.getUInt("mode", 0) == 1;
  const uint32_t oldAid = grouped
    ? legacy_.getUInt("switchAid", 0) : legacy_.getUInt("aid", 0);
  const uint32_t oldDevice = legacy_.getUInt("dev", 0);
  if (bridge::validAid(oldAid) && oldAid > 2 && bridge::validAid(oldDevice)) {
    auto &r = records_[0];
    r.aid = oldAid;
    r.deviceAid = oldDevice;
    r.mode = grouped ? EffectsControllerMode::GroupedButtons
                     : EffectsControllerMode::Television;
    for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i) {
      const uint32_t preset = legacy_.getUInt(inputKey(0, i, "ps").c_str(), 0);
      if (bridge::validPresetId(preset) && indexByPreset(0, preset) < 0) {
        r.inputs[i] = {static_cast<uint8_t>(preset),
          legacy_.getString(inputKey(0, i, "nm").c_str(), "WLED Preset"),
          legacy_.getUInt(inputKey(0, i, "pl").c_str(), 0) == 1};
      }
    }
    if (!r.count()) r = {};
  }
  for (unsigned slot = 1; slot < bridge::kEffectsSlots; ++slot) {
    auto &r = records_[slot];
    const uint32_t aid = modern_.getUInt(field(slot, "a").c_str(), 0);
    const uint32_t lightAid = modern_.getUInt(field(slot, "d").c_str(), 0);
    if (!bridge::validAid(aid) || aid <= 2 || !bridge::validAid(lightAid))
      continue;
    r.aid = aid;
    r.deviceAid = lightAid;
    r.mode = EffectsControllerMode::GroupedButtons;
    for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i) {
      const uint32_t preset = modern_.getUInt(inputKey(slot, i, "p").c_str(), 0);
      if (bridge::validPresetId(preset) && indexByPreset(slot, preset) < 0) {
        r.inputs[i] = {static_cast<uint8_t>(preset),
          modern_.getString(inputKey(slot, i, "n").c_str(), "WLED Preset"),
          modern_.getUInt(inputKey(slot, i, "l").c_str(), 0) == 1};
      }
    }
    if (!r.count()) r = {};
  }

  // Recover an interrupted rebuild: a new AID was committed before the old
  // controller could be retired. Keep the highest AID for that WLED light.
  // The older record is retired only; no lights or unrelated data are altered.
  for (unsigned i = 0; i < bridge::kEffectsSlots; ++i) {
    if (!records_[i].aid) continue;
    for (unsigned j = i + 1; j < bridge::kEffectsSlots; ++j) {
      if (!records_[j].aid) continue;
      if (records_[i].deviceAid != records_[j].deviceAid
          && records_[i].aid != records_[j].aid) continue;
      const unsigned older = records_[i].aid < records_[j].aid ? i : j;
      Serial.printf("[Effects] WARNING duplicate light/controller identity, retire slot %u\n", older);
      if (!retire(older)) {
        Serial.println("[Effects] Could not retire duplicate; suppressing in RAM");
        records_[older] = {};
      }
    }
  }
  list();
  return true;
}

bool EffectsControllerRegistry::saveInput(unsigned slot, unsigned input,
                                           uint8_t preset, const String &name,
                                           bool isPlaylist) {
  if (slot >= bridge::kEffectsSlots || input >= bridge::kMaxEffectsInputs
      || !bridge::validPresetId(preset)) return false;
  Preferences &prefs = slot == 0 ? legacy_ : modern_;
  const String nameKey = inputKey(slot, input, slot == 0 ? "nm" : "n");
  const String typeKey = inputKey(slot, input, slot == 0 ? "pl" : "l");
  const String presetKey = inputKey(slot, input, slot == 0 ? "ps" : "p");
  prefs.putString(nameKey.c_str(), name);
  if (!prefs.isKey(nameKey.c_str()) || prefs.getString(nameKey.c_str()) != name)
    return false;
  if (prefs.putUInt(typeKey.c_str(), isPlaylist ? 1 : 0) != sizeof(uint32_t)
      || prefs.getUInt(typeKey.c_str(), 2) != (isPlaylist ? 1U : 0U))
    return false;
  // Preset ID remains the commit marker; protect against half-written input.
  return prefs.putUInt(presetKey.c_str(), preset) == sizeof(uint32_t);
}

bool EffectsControllerRegistry::saveModern(unsigned slot,
                                            const EffectsControllerRecord &r) {
  if (slot == 0 || slot >= bridge::kEffectsSlots || !r.aid || !r.count())
    return false;
  // Only write into a vacant slot. Identity AID is committed LAST, allowing
  // rollback to the previously active group on partial NVS failure.
  if (modern_.getUInt(field(slot, "a").c_str(), 0)) return false;
  for (unsigned i = 0; i < r.count(); ++i)
    if (!saveInput(slot, i, r.inputs[i].presetId, r.inputs[i].name,
                   r.inputs[i].isPlaylist))
      return false;
  // Clear any stale tail entries from a previously retired/reused slot.
  for (unsigned i = r.count(); i < bridge::kMaxEffectsInputs; ++i) {
    modern_.remove(inputKey(slot, i, "p").c_str());
    modern_.remove(inputKey(slot, i, "n").c_str());
    modern_.remove(inputKey(slot, i, "l").c_str());
  }
  if (modern_.putUInt(field(slot, "d").c_str(), r.deviceAid) != sizeof(uint32_t)
      || modern_.getUInt(field(slot, "d").c_str(), 0) != r.deviceAid)
    return false;
  return modern_.putUInt(field(slot, "a").c_str(), r.aid) == sizeof(uint32_t);
}

bool EffectsControllerRegistry::retire(unsigned slot) {
  if (slot >= bridge::kEffectsSlots || !records_[slot].aid) return false;
  if (slot == 0) {
    const char *identity = records_[0].mode == EffectsControllerMode::GroupedButtons
        ? "switchAid" : "aid";
    if (legacy_.putUInt(identity, 0) != sizeof(uint32_t)) return false;
    legacy_.putUInt("aid", 0);
    legacy_.remove("mode");
    legacy_.remove("dev");
    for (unsigned i = 0; i < bridge::kMaxEffectsInputs; ++i) {
      legacy_.remove(inputKey(0, i, "ps").c_str());
      legacy_.remove(inputKey(0, i, "nm").c_str());
      legacy_.remove(inputKey(0, i, "pl").c_str());
    }
  } else {
    if (modern_.putUInt(field(slot, "a").c_str(), 0) != sizeof(uint32_t))
      return false;
    modern_.remove(field(slot, "d").c_str());
    // Old names/presets are harmless; saveModern clears unused tail entries.
  }
  records_[slot] = {};
  return true;
}

int EffectsControllerRegistry::emptyModernSlot() const {
  for (unsigned slot = 1; slot < bridge::kEffectsSlots; ++slot)
    if (!records_[slot].aid) return static_cast<int>(slot);
  return -1;
}

EffectsAddResult EffectsControllerRegistry::add(DeviceRegistry &devices,
                                                  uint32_t deviceAid,
                                                  uint8_t presetId,
                                                  const String &name,
                                                  uint32_t *controllerAid,
                                                  bool isPlaylist) {
  if (devices.indexByAid(deviceAid) < 0 || !bridge::validPresetId(presetId))
    return EffectsAddResult::Invalid;
  const int existing = indexByDeviceAid(deviceAid);
  const String clean = cleanEffectsName(name);
  if (existing >= 0) {
    auto &r = records_[existing];
    if (controllerAid) *controllerAid = r.aid;
    const int present = indexByPreset(existing, presetId);
    if (present >= 0) {
      // Older firmware exposed playlists as plain presets with no pl metadata.
      // Re-selecting that WLED entry explicitly upgrades its classification,
      // without changing the existing HomeKit accessory/service IDs.
      if (r.inputs[present].isPlaylist == isPlaylist)
        return EffectsAddResult::Exists;
      Preferences &prefs = existing == 0 ? legacy_ : modern_;
      const String typeKey = inputKey(existing, static_cast<unsigned>(present),
                                     existing == 0 ? "pl" : "l");
      if (prefs.putUInt(typeKey.c_str(), isPlaylist ? 1 : 0) != sizeof(uint32_t)
          || prefs.getUInt(typeKey.c_str(), 2) != (isPlaylist ? 1U : 0U))
        return EffectsAddResult::PersistenceFailed;
      r.inputs[present].isPlaylist = isPlaylist;
      return EffectsAddResult::Reclassified;
    }
    const unsigned input = r.count();
    if (input >= bridge::kMaxEffectsInputs) return EffectsAddResult::Full;
    if (!saveInput(existing, input, presetId, clean, isPlaylist))
      return EffectsAddResult::PersistenceFailed;
    r.inputs[input] = {presetId, clean, isPlaylist};
    return EffectsAddResult::Added;
  }
  if (activeCount() >= bridge::kMaxEffectsControllers)
    return EffectsAddResult::ControllerLimit;
  const int slot = emptyModernSlot();
  if (slot < 0) return EffectsAddResult::PersistenceFailed;
  const uint32_t newAid = devices.allocateAid();
  if (!newAid) return EffectsAddResult::PersistenceFailed;
  EffectsControllerRecord created;
  created.aid = newAid;
  created.deviceAid = deviceAid;
  created.mode = EffectsControllerMode::GroupedButtons;
  created.inputs[0] = {presetId, clean, isPlaylist};
  if (!saveModern(slot, created)) return EffectsAddResult::PersistenceFailed;
  records_[slot] = created;
  if (controllerAid) *controllerAid = newAid;
  return EffectsAddResult::Created;
}

bool EffectsControllerRegistry::rename(uint32_t aid, uint8_t preset,
                                        const String &name) {
  const int slot = indexByAid(aid);
  if (slot < 0) return false;
  const int idx = indexByPreset(slot, preset);
  if (idx < 0) return false;
  const String cleaned = cleanEffectsName(name);
  // Keep the original service/IID; only update its label.
  Preferences &prefs = slot == 0 ? legacy_ : modern_;
  const String key = inputKey(slot, idx, slot == 0 ? "nm" : "n");
  prefs.putString(key.c_str(), cleaned);
  if (!prefs.isKey(key.c_str()) || prefs.getString(key.c_str()) != cleaned)
    return false;
  records_[slot].inputs[idx].name = cleaned;
  return true;
}

EffectsEditResult EffectsControllerRegistry::editStructure(
    DeviceRegistry &devices, uint32_t aid, uint8_t preset, int direction,
    uint32_t *newAid) {
  const int slot = indexByAid(aid);
  if (slot < 0) return EffectsEditResult::NotFound;
  if (direction < -1 || direction > 1) return EffectsEditResult::Invalid;
  const int inputIndex = indexByPreset(slot, preset);
  if (inputIndex < 0) return EffectsEditResult::NotFound;
  const auto &previous = records_[slot];
  if (direction == 0 && previous.count() == 1) {
    return remove(aid) ? EffectsEditResult::RemovedController
                       : EffectsEditResult::PersistenceFailed;
  }
  if (direction && (inputIndex + direction < 0
      || inputIndex + direction >= static_cast<int>(previous.count())))
    return EffectsEditResult::NoChange;
  const int freeSlot = emptyModernSlot();
  if (freeSlot < 0) return EffectsEditResult::PersistenceFailed;
  const uint32_t nextAid = devices.allocateAid();
  if (!nextAid) return EffectsEditResult::PersistenceFailed;
  EffectsControllerRecord updated;
  updated.aid = nextAid;
  updated.deviceAid = previous.deviceAid;
  updated.mode = EffectsControllerMode::GroupedButtons;
  unsigned n = 0;
  for (unsigned i = 0; i < previous.count(); ++i)
    if (direction != 0 || static_cast<int>(i) != inputIndex)
      updated.inputs[n++] = previous.inputs[i];
  if (direction != 0) {
    const auto temp = updated.inputs[inputIndex];
    updated.inputs[inputIndex] = updated.inputs[inputIndex + direction];
    updated.inputs[inputIndex + direction] = temp;
  }
  if (!saveModern(freeSlot, updated)) return EffectsEditResult::PersistenceFailed;
  // The new identity is now safe. A power loss before the old retirement
  // is resolved by begin() keeping the higher AID for the same WLED device.
  records_[freeSlot] = updated;
  if (!retire(slot))
    Serial.println("[Effects] Old accessory retirement incomplete; next boot will reconcile");
  if (newAid) *newAid = nextAid;
  return EffectsEditResult::Rebuilt;
}

uint32_t EffectsControllerRegistry::convertToGroupedButtons(
    DeviceRegistry &devices, uint32_t aid) {
  const int slot = indexByAid(aid);
  if (slot != 0 || records_[0].mode != EffectsControllerMode::Television)
    return 0;
  const uint32_t newAid = devices.allocateAid();
  if (!newAid) return 0;
  if (legacy_.putUInt("switchAid", newAid) != sizeof(uint32_t)
      || legacy_.getUInt("switchAid", 0) != newAid
      || legacy_.putUInt("mode", 1) != sizeof(uint32_t))
    return 0;
  records_[0].aid = newAid;
  records_[0].mode = EffectsControllerMode::GroupedButtons;
  return newAid;
}
uint32_t EffectsControllerRegistry::convertToGroupedButtons(DeviceRegistry &devices) {
  return convertToGroupedButtons(devices, records_[0].aid);
}
bool EffectsControllerRegistry::remove(uint32_t aid) {
  const int slot = indexByAid(aid);
  return slot >= 0 && retire(static_cast<unsigned>(slot));
}
unsigned EffectsControllerRegistry::removeForDevice(uint32_t aid) {
  unsigned removed = 0;
  for (unsigned i = 0; i < bridge::kEffectsSlots; ++i)
    if (records_[i].aid && records_[i].deviceAid == aid && retire(i))
      ++removed;
  return removed;
}
void EffectsControllerRegistry::list() const {
  Serial.printf("[Effects] %u/%u grouped controllers\n",
                activeCount(), bridge::kMaxEffectsControllers);
  for (unsigned slot = 0; slot < bridge::kEffectsSlots; ++slot) {
    const auto &r = records_[slot];
    if (!r.aid) continue;
    Serial.printf("  Slot=%u AID=%lu Light=%lu Mode=%s inputs=%u\n", slot,
      static_cast<unsigned long>(r.aid), static_cast<unsigned long>(r.deviceAid),
      r.mode == EffectsControllerMode::GroupedButtons ? "Grouped" : "TV", r.count());
  }
}
