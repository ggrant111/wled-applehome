#include "WledLightService.h"
#include <Arduino.h>
#include "RegistryPolicy.h"
#include "PresetPolicy.h"

WledLightService::WledLightService(WledClient &wled, unsigned position)
    : Service::LightBulb(), wled_(wled) {
  power_ = new Characteristic::On(false);
  brightness_ = new Characteristic::Brightness(50);
  const uint32_t delayMs = 450 + (position % bridge::kMaxDevices) * 850;
  lastPollMs_ = millis() - (kPollEveryMs - delayMs);
}

boolean WledLightService::update() {
  const bool changedPower = power_->updated();
  const bool changedBrightness = brightness_->updated();
  const int brightness = changedBrightness ? brightness_->getNewVal<int>()
                                           : brightness_->getVal<int>();
  bool power = changedPower ? power_->getNewVal<bool>() : power_->getVal<bool>();

  if (changedBrightness && !changedPower) power = brightness > 0;
  if (changedBrightness && brightness == 0) power = false;

  const bool sendPower = changedPower || changedBrightness;
  const bool sendBrightness = changedBrightness && brightness > 0;
  if (!wled_.writeState(sendPower, power, sendBrightness, brightness)) {
    Serial.printf("[HomeKit] WLED write rejected (%s)\n", wled_.host().c_str());
    retryIntervalMs_ = kOfflineBackoffMs;
    return false;
  }

  if (power_->getVal<bool>() != power && !changedPower) power_->setVal(power);
  retryIntervalMs_ = kPollEveryMs;
  lastPollMs_ = millis();
  Serial.printf("[HomeKit] %s power=%d brightness=%d\n",
                wled_.host().c_str(), power, brightness);
  return true;
}

void WledLightService::loop() {
  const uint32_t now = millis();
  const uint32_t elapsed = now - lastPollMs_;
  const uint32_t currentRevision = wled_.mutationRevision();
  const uint32_t mutationAge = now - wled_.lastMutationMs();
  const bool needsAck = bridge::needsStateReconcile(
      acknowledgedRevision_, currentRevision, mutationAge);

  // Never fetch stale state immediately after an accepted Home command.
  if (currentRevision != acknowledgedRevision_ && mutationAge < 450) return;

  // Reconcile after local WLED POST; otherwise poll for external changes.
  const uint32_t minDelay =
      retryIntervalMs_ == kOfflineBackoffMs ? kOfflineBackoffMs : 450;
  if (!needsAck && elapsed < retryIntervalMs_) return;
  if (needsAck && elapsed < minDelay) return;

  lastPollMs_ = now;
  if (!wled_.configured()) return;
  WledState state;
  if (!wled_.readState(state)) {
    retryIntervalMs_ = kOfflineBackoffMs;
    return;
  }

  retryIntervalMs_ = kPollEveryMs;
  acknowledgedRevision_ = currentRevision;

  const bool previousPower = power_->getVal<bool>();
  const int previousBrightness = brightness_->getVal<int>();
  if (previousPower != state.on) power_->setVal(state.on);
  if (previousBrightness != state.brightness)
    brightness_->setVal(state.brightness);

  if (previousPower != state.on || previousBrightness != state.brightness)
    Serial.printf("[WLED] %s state sync on=%d brightness=%d\n",
                  wled_.host().c_str(), state.on, state.brightness);
  if (lastObservedPresetId_ != state.presetId) {
    lastObservedPresetId_ = state.presetId;
    Serial.printf("[WLED] %s active preset=%d\n",
                  wled_.host().c_str(), state.presetId);
  }
}
