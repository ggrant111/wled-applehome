#include "DeviceRegistry.h"

namespace {
const char *kNamespace = "wled-reg";

String cleanName(String name) {
  name.trim();
  if (name.isEmpty()) name = "WLED Light";
  if (name.length() > 32) name = name.substring(0, 32);
  return name;
}
}

String DeviceRegistry::key(unsigned slot, const char *field) const {
  return String(field) + String(slot);
}

bool DeviceRegistry::save(unsigned slot) {
  const DeviceRecord &record = records_[slot];
  // Write the identity last. Records with an absent AID are ignored at boot.
  // ESP32 Preferences::putString returns strlen(value).  A successful write
  // of an empty IPv4 or MAC string therefore returns ZERO, not an error!
  // The original v0.1 light is intentionally migrated with mac="".
  // Validate persisted values by reading them back before committing AID.
  const String ipKey = key(slot, "ip");
  const String macKey = key(slot, "mac");
  const String nameKey = key(slot, "nm");
  prefs_.putString(ipKey.c_str(), record.ip);
  prefs_.putString(macKey.c_str(), record.mac);
  prefs_.putString(nameKey.c_str(), record.name);
  const bool metadataOK =
      prefs_.isKey(ipKey.c_str()) && prefs_.getString(ipKey.c_str()) == record.ip
      && prefs_.isKey(macKey.c_str()) && prefs_.getString(macKey.c_str()) == record.mac
      && prefs_.isKey(nameKey.c_str()) && prefs_.getString(nameKey.c_str()) == record.name;
  if (!metadataOK) return false;
  return prefs_.putUInt(key(slot, "aid").c_str(), record.aid) == sizeof(uint32_t);
}

bool DeviceRegistry::begin() {
  if (!prefs_.begin(kNamespace, false)) {
    Serial.println("[Registry] Could not open NVS");
    return false;
  }

  if (!prefs_.getBool("initialized", false)) {
    // Exactly preserve existing paired AID=2 and its original Name/Services.
    Preferences legacy;
    String ip;
    if (legacy.begin("wled-home", true)) {
      ip = legacy.getString("ip", "");
      legacy.end();
    }
    records_[0] = {2, ip, "", "WLED Light"};
    nextAid_ = 3;
    if (!save(0)) {
      Serial.println("[Registry] WARNING migration save failed");
      return false;
    }
    prefs_.putUInt("nextAid", nextAid_);
    prefs_.putBool("initialized", true);
    Serial.printf("[Registry] Migrated original AID=2, WLED target %s\n",
                  ip.isEmpty() ? "(unconfigured)" : ip.c_str());
  }

  uint32_t highest = 2;
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
    const uint32_t aid = prefs_.getUInt(key(i, "aid").c_str(), 0);
    if (!aid) continue;
    if (!bridge::validAid(aid)) {
      Serial.printf("[Registry] WARNING invalid AID in slot %u\n", i);
      continue;
    }
    DeviceRecord candidate;
    candidate.aid = aid;
    candidate.ip = prefs_.getString(key(i, "ip").c_str(), "");
    candidate.mac = prefs_.getString(key(i, "mac").c_str(), "");
    candidate.name = prefs_.getString(key(i, "nm").c_str(), "WLED Light");
    if (indexByAid(aid) >= 0) {
      Serial.printf("[Registry] WARNING duplicate AID %lu\n",
                    static_cast<unsigned long>(aid));
      continue;
    }
    records_[i] = candidate;
    if (aid > highest) highest = aid;
  }
  nextAid_ = bridge::safeNextAid(prefs_.getUInt("nextAid", 3), highest);
  prefs_.putUInt("nextAid", nextAid_);
  list();
  return true;
}

int DeviceRegistry::indexByAid(uint32_t aid) const {
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i)
    if (records_[i].aid == aid && aid != 0) return static_cast<int>(i);
  return -1;
}

int DeviceRegistry::indexByMac(const String &mac) const {
  if (mac.isEmpty()) return -1;
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i)
    if (records_[i].aid && records_[i].mac.equalsIgnoreCase(mac))
      return static_cast<int>(i);
  return -1;
}

int DeviceRegistry::indexByIp(const String &ip) const {
  if (ip.isEmpty()) return -1;
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i)
    if (records_[i].aid && records_[i].ip == ip)
      return static_cast<int>(i);
  return -1;
}

unsigned DeviceRegistry::activeCount() const {
  unsigned count = 0;
  for (const auto &record : records_) if (record.aid) ++count;
  return count;
}

EnrollResult DeviceRegistry::enroll(const String &ip, const String &mac,
                                    const String &name, uint32_t *outAid) {
  if (ip.isEmpty() || mac.length() != 12) return EnrollResult::Invalid;
  int existing = indexByMac(mac);
  // The first v0.1 light had no stored MAC. Claim it by its original IP.
  if (existing < 0) {
    const int ipMatch = indexByIp(ip);
    if (ipMatch >= 0 && records_[ipMatch].mac.isEmpty())
      existing = ipMatch;
    else if (ipMatch >= 0)
      return EnrollResult::Invalid;  // same IP, conflicting identity
  }

  if (existing >= 0) {
    DeviceRecord &record = records_[existing];
    if (outAid) *outAid = record.aid;
    if (record.ip == ip && record.mac == mac) return EnrollResult::AlreadyKnown;
    const DeviceRecord backup = record;
    record.ip = ip;
    record.mac = mac;
    if (!save(existing)) {
      record = backup;
      return EnrollResult::PersistenceFailed;
    }
    return EnrollResult::UpdatedIp;
  }

  if (activeCount() >= bridge::kMaxDevices) return EnrollResult::Full;
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
    if (records_[i].aid) continue;
    const uint32_t aid = allocateAid();
    if (!aid) return EnrollResult::PersistenceFailed;
    const DeviceRecord newRecord = {aid, ip, mac, cleanName(name)};
    records_[i] = newRecord;
    if (!save(i)) {
      records_[i] = {};
      return EnrollResult::PersistenceFailed;
    }
    if (outAid) *outAid = aid;
    return EnrollResult::Added;
  }
  return EnrollResult::Full;
}

bool DeviceRegistry::updateLegacyIp(const String &ip, const String &mac) {
  const int idx = indexByAid(2);
  if (idx < 0) return false;
  DeviceRecord &record = records_[idx];
  const DeviceRecord old = record;
  record.ip = ip;
  if (mac.length() == 12) record.mac = mac;
  if (!save(idx)) {
    record = old;
    return false;
  }
  return true;
}

bool DeviceRegistry::removeByAid(uint32_t aid) {
  const int idx = indexByAid(aid);
  if (idx < 0) return false;
  // Mark deleted before deleting supplementary fields so a power failure
  // cannot resurrect a partially removed accessory.
  if (prefs_.putUInt(key(idx, "aid").c_str(), 0) != sizeof(uint32_t))
    return false;
  records_[idx] = {};
  prefs_.remove(key(idx, "ip").c_str());
  prefs_.remove(key(idx, "mac").c_str());
  prefs_.remove(key(idx, "nm").c_str());
  return true;
}

void DeviceRegistry::list() const {
  Serial.printf("[Registry] %u/%u enabled lights, next AID=%lu\n",
                activeCount(), bridge::kMaxDevices,
                static_cast<unsigned long>(nextAid_));
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
    const auto &r = records_[i];
    if (!r.aid) continue;
    Serial.printf("  AID=%lu IP=%s MAC=%s NAME=%s%s\n",
                  static_cast<unsigned long>(r.aid),
                  r.ip.isEmpty() ? "(unconfigured)" : r.ip.c_str(),
                  r.mac.isEmpty() ? "(unverified)" : r.mac.c_str(),
                  r.name.c_str(), r.aid == 2 ? " [original light]" : "");
  }
}

uint32_t DeviceRegistry::allocateAid() {
  if (nextAid_ < 3 || nextAid_ >= 0xFFFFFFEFUL) return 0;
  const uint32_t reserved = nextAid_;
  // Reserve the next ID BEFORE exposing this ID, never recycle an allocated AID.
  if (prefs_.putUInt("nextAid", reserved + 1) != sizeof(uint32_t)) return 0;
  nextAid_ = reserved + 1;
  return reserved;
}
