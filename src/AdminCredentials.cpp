#include "AdminCredentials.h"
#include "PortalValidation.h"

bool AdminCredentials::begin() {
  if (!prefs_.begin("wled-admin", false)) return false;
  password_ = prefs_.getString("password", "");
  if (!password_.isEmpty() && !bridge::validAdminPassword(password_.c_str())) {
    Serial.println("[Admin] Invalid saved password; remote management disabled.");
    password_ = "";
  }
  return true;
}

bool AdminCredentials::setPassword(const String &newPassword) {
  if (!bridge::validAdminPassword(newPassword.c_str())) return false;
  if (prefs_.putString("password", newPassword) != newPassword.length())
    return false;
  password_ = newPassword;
  return true;
}

bool AdminCredentials::disable() {
  if (!prefs_.remove("password")) return false;
  password_ = "";
  return true;
}
