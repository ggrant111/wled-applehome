#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include "Arduino.h"

class Preferences {
public:
  using Value = std::variant<std::string, uint32_t, bool>;
  static inline std::map<std::string, std::map<std::string, Value>> storage;

  bool begin(const char *name, bool readOnly = false) {
    ns_ = name ? name : "";
    readOnly_ = readOnly;
    open_ = !ns_.empty();
    return open_;
  }
  void end() { open_ = false; }

  size_t putString(const char *key, const String &value) {
    if (!open_ || readOnly_) return 0;
    storage[ns_][key] = std::string(value.c_str());
    // Real Arduino Preferences::putString returns strlen(value), even if EMPTY.
    return value.length();
  }
  String getString(const char *key, const char *fallback = "") const {
    if (!open_) return fallback;
    const auto foundNs = storage.find(ns_);
    if (foundNs == storage.end()) return fallback;
    const auto found = foundNs->second.find(key);
    if (found == foundNs->second.end()) return fallback;
    if (const auto *string = std::get_if<std::string>(&found->second))
      return *string;
    return fallback;
  }
  String getString(const char *key, const String &fallback) const {
    return getString(key, fallback.c_str());
  }
  bool isKey(const char *key) const {
    auto it = storage.find(ns_);
    return open_ && it != storage.end() && it->second.count(key) != 0;
  }
  size_t putUInt(const char *key, uint32_t value) {
    if (!open_ || readOnly_) return 0;
    storage[ns_][key] = value;
    return sizeof(value);
  }
  uint32_t getUInt(const char *key, uint32_t fallback = 0) const {
    auto it = storage.find(ns_);
    if (!open_ || it == storage.end() || !it->second.count(key)) return fallback;
    const auto *p = std::get_if<uint32_t>(&it->second.at(key));
    return p ? *p : fallback;
  }
  size_t putBool(const char *key, bool value) {
    if (!open_ || readOnly_) return 0;
    storage[ns_][key] = value;
    return 1;
  }
  bool getBool(const char *key, bool fallback = false) const {
    auto it = storage.find(ns_);
    if (!open_ || it == storage.end() || !it->second.count(key)) return fallback;
    const auto *p = std::get_if<bool>(&it->second.at(key));
    return p ? *p : fallback;
  }
  bool remove(const char *key) {
    if (!open_ || readOnly_) return false;
    return storage[ns_].erase(key) > 0;
  }

private:
  std::string ns_;
  bool readOnly_ = false;
  bool open_ = false;
};
