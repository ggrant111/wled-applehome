#include "PresetRegistry.h"
#include "DeviceRegistry.h"

namespace {
String cleanPresetName(String name) {
  name.trim();
  if (name.length() > 35) name = name.substring(0, 35);
  return name.isEmpty() ? String("WLED Preset") : name;
}
}

String PresetRegistry::key(unsigned slot, const char *field) const {
  return String(field) + String(slot);
}

bool PresetRegistry::save(unsigned slot) {
  const PresetRecord &r = records_[slot];
  // Write AID last. Missing AID means no registered switch.
  return prefs_.putUInt(key(slot, "dev").c_str(), r.deviceAid) == sizeof(uint32_t)
      && prefs_.putUChar(key(slot, "ps").c_str(), r.presetId) == 1
      && prefs_.putString(key(slot, "nm").c_str(), r.name) > 0
      && prefs_.putUInt(key(slot, "aid").c_str(), r.aid) == sizeof(uint32_t);
}

bool PresetRegistry::begin() {
  if (!prefs_.begin("wled-ps", false)) return false;
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i) {
    const uint32_t aid = prefs_.getUInt(key(i, "aid").c_str(), 0);
    if (!aid) continue;
    const uint32_t deviceAid = prefs_.getUInt(key(i, "dev").c_str(), 0);
    const uint8_t presetId = prefs_.getUChar(key(i, "ps").c_str(), 0);
    if (aid <= 2 || !bridge::validPresetPair(deviceAid, presetId)
        || indexByAid(aid) >= 0 || indexByPair(deviceAid, presetId) >= 0) {
      Serial.printf("[Presets] Invalid/duplicate record in slot %u; ignored\n", i);
      continue;
    }
    records_[i] = {aid, deviceAid, presetId,
                   prefs_.getString(key(i, "nm").c_str(), "WLED Preset")};
  }
  list();
  return true;
}

unsigned PresetRegistry::count() const {
  unsigned found = 0;
  for (const auto &r : records_) if (r.aid) ++found;
  return found;
}

int PresetRegistry::indexByAid(uint32_t aid) const {
  if (!aid) return -1;
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i)
    if (records_[i].aid == aid) return static_cast<int>(i);
  return -1;
}

int PresetRegistry::indexByPair(uint32_t deviceAid, uint8_t presetId) const {
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i)
    if (records_[i].aid && records_[i].deviceAid == deviceAid
        && records_[i].presetId == presetId) return static_cast<int>(i);
  return -1;
}

PresetEnrollResult PresetRegistry::add(DeviceRegistry &devices,
                                       uint32_t deviceAid, uint8_t presetId,
                                       const String &name, uint32_t *outAid) {
  if (!bridge::validPresetPair(deviceAid, presetId) || devices.indexByAid(deviceAid) < 0)
    return PresetEnrollResult::Invalid;
  const int old = indexByPair(deviceAid, presetId);
  if (old >= 0) {
    if (outAid) *outAid = records_[old].aid;
    return PresetEnrollResult::Exists;
  }
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i) {
    if (records_[i].aid) continue;
    const uint32_t aid = devices.allocateAid(); // shared non-recycled AID allocator
    if (!aid) return PresetEnrollResult::PersistenceFailed;
    records_[i] = {aid, deviceAid, presetId, cleanPresetName(name)};
    if (!save(i)) {
      records_[i] = {}; // AID may be skipped; never reuse it.
      return PresetEnrollResult::PersistenceFailed;
    }
    if (outAid) *outAid = aid;
    return PresetEnrollResult::Added;
  }
  return PresetEnrollResult::Full;
}

bool PresetRegistry::remove(uint32_t accessoryAid) {
  const int idx = indexByAid(accessoryAid);
  if (idx < 0) return false;
  if (prefs_.putUInt(key(idx, "aid").c_str(), 0) != sizeof(uint32_t))
    return false;
  records_[idx] = {};
  prefs_.remove(key(idx, "dev").c_str());
  prefs_.remove(key(idx, "ps").c_str());
  prefs_.remove(key(idx, "nm").c_str());
  return true;
}

unsigned PresetRegistry::removeForDevice(uint32_t deviceAid) {
  unsigned removed = 0;
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i) {
    if (records_[i].aid && records_[i].deviceAid == deviceAid
        && remove(records_[i].aid)) ++removed;
  }
  return removed;
}

void PresetRegistry::list() const {
  Serial.printf("[Presets] %u/%u HomeKit preset switches:\n",
                count(), bridge::kMaxExposedPresets);
  for (const auto &p : records_) {
    if (!p.aid) continue;
    Serial.printf("  switch AID=%lu | light AID=%lu | preset=%u | %s\n",
                  static_cast<unsigned long>(p.aid),
                  static_cast<unsigned long>(p.deviceAid),
                  static_cast<unsigned>(p.presetId),
                  p.name.c_str());
  }
}
