#include "PresetSwitchService.h"
#include <Arduino.h>

PresetSwitchService::PresetSwitchService(WledClient &client,
                                         uint32_t lightAid, uint8_t presetId,
                                         const char *serviceName)
    : Service::Switch(), client_(client), lightAid_(lightAid),
      presetId_(presetId) {
  on_ = new Characteristic::On(false);
  if (serviceName && *serviceName)
    new Characteristic::ConfiguredName(serviceName);
}

boolean PresetSwitchService::update() {
  // A preset is an *action*, not a persistent state. Ignore "Off" requests.
  if (!on_->getNewVal<bool>()) return true;

  if (!client_.applyPreset(presetId_)) {
    Serial.printf("[Preset] FAILED light AID=%lu preset=%u\n",
                  static_cast<unsigned long>(lightAid_),
                  static_cast<unsigned>(presetId_));
    return false; // Rejected by the local queue or failed synchronously.
  }

  triggeredAtMs_ = millis();
  resetPending_ = true;
  Serial.printf("[Preset] %s light AID=%lu preset=%u\n",
                client_.backgroundActive() ? "Queued" : "Triggered",
                static_cast<unsigned long>(lightAid_),
                static_cast<unsigned>(presetId_));
  return true;
}

void PresetSwitchService::loop() {
  // Reset AFTER HomeSpan commits its update() to avoid characteristic races.
  if (resetPending_ && static_cast<uint32_t>(millis() - triggeredAtMs_) >= 250) {
    resetPending_ = false;
    on_->setVal(false); // send event so the switch can be pressed again.
  }
}
