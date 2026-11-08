#pragma once
#include <Arduino.h>
#include <Preferences.h>

class AdminCredentials {
public:
  bool begin();
  bool enabled() const { return !password_.isEmpty(); }
  const String &password() const { return password_; }
  // Only callable from the physical/USB HomeSpan serial CLI.
  bool setPassword(const String &newPassword);
  bool disable();
private:
  Preferences prefs_;
  String password_;
};
