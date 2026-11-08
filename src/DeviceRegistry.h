#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "RegistryPolicy.h"

struct DeviceRecord {
  uint32_t aid = 0;  // 0 = vacant
  String ip;
  String mac;        // normalized 12-digit uppercase hex device MAC
  String name;
};

enum class EnrollResult {
  Added,
  UpdatedIp,
  AlreadyKnown,
  Full,
  Invalid,
  PersistenceFailed,
};

class DeviceRegistry {
public:
  bool begin();
  const DeviceRecord &at(unsigned index) const { return records_[index]; }
  int indexByAid(uint32_t aid) const;
  int indexByMac(const String &mac) const;
  int indexByIp(const String &ip) const;
  unsigned activeCount() const;
  EnrollResult enroll(const String &ip, const String &mac, const String &name,
                      uint32_t *outAid = nullptr);
  bool updateLegacyIp(const String &ip, const String &mac = "");
  bool removeByAid(uint32_t aid);
  uint32_t allocateAid(); // Shared monotonic ID allocator for lights and preset switches.
  void list() const;

private:
  Preferences prefs_;
  DeviceRecord records_[bridge::kMaxDevices];
  uint32_t nextAid_ = 3;
  String key(unsigned slot, const char *field) const;
  bool save(unsigned slot);
};
