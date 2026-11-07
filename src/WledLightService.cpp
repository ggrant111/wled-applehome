#include "WledLightService.h"

#include <Arduino.h>

#include "BridgeMath.h"

WledLightService::WledLightService(WledClient &wled)
    : Service::LightBulb(), wled_(wled) {
  power_ = new Characteristic::On(false);
  brightness_ = new Characteristic::Brightness(50);
}

boolean WledLightService::update() {
  const bool changedPower = power_->updated();
  const bool changedBrightness = brightness_->updated();
  const int brightness = changedBrightness ? brightness_->getNewVal<int>()
                                           : brightness_->getVal<int>();
  bool power = changedPower ? power_->getNewVal<bool>()
                            : power_->getVal<bool>();

  // Brightness 0 is off in Home. WLED advises sending on:false, not bri:0.
  // If a Home client changes only brightness to a positive value, turn on.
  if (changedBrightness && !changedPower)
    power = brightness > 0;
  if (changedBrightness && brightness == 0)
    power = false;

  const bool sendPower = changedPower || changedBrightness;
  const bool sendBrightness = changedBrightness && brightness > 0;
  if (!wled_.writeState(sendPower, power, sendBrightness, brightness)) {
    Serial.println("[HomeKit] WLED write failed; rejecting change");
    return false;
  }

  if (power_->getVal<bool>() != power && !changedPower)
    power_->setVal(power);

  Serial.printf("[HomeKit] WLED power=%d brightness=%d\n", power, brightness);
  // Wait for poll to confirm the current state of WLED.
  lastPollMs_ = millis();
  return true;
}

void WledLightService::loop() {
  if (static_cast<uint32_t>(millis() - lastPollMs_) < kPollEveryMs)
    return;

  lastPollMs_ = millis();
  WledState state;
  if (!wled_.readState(state)) return;

  const bool previousPower = power_->getVal<bool>();
  const int previousBrightness = brightness_->getVal<int>();
  if (previousPower != state.on) power_->setVal(state.on);
  if (previousBrightness != state.brightness)
    brightness_->setVal(state.brightness);

  if (previousPower != state.on || previousBrightness != state.brightness)
    Serial.printf("[WLED] state sync power=%d brightness=%d\n",
                  state.on, state.brightness);
}
