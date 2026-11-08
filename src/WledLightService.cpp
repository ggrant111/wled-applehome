#include "WledLightService.h"
#include <Arduino.h>
#include "RegistryPolicy.h"
#include "PresetPolicy.h"

WledLightService::WledLightService(WledClient &wled, unsigned position,
                                     WledNetworkWorker *worker)
    : Service::LightBulb(), wled_(wled), worker_(worker), workerSlot_(position) {
  power_ = new Characteristic::On(false);
  brightness_ = new Characteristic::Brightness(50);
  // Append new characteristics AFTER the originals to preserve their IIDs.
  hue_ = new Characteristic::Hue(0.0f);
  saturation_ = new Characteristic::Saturation(100.0f);
  const uint32_t delayMs = 450 + (position % bridge::kMaxDevices) * 850;
  lastPollMs_ = millis() - (kPollEveryMs - delayMs);
}

boolean WledLightService::update() {
  const bool changedPower = power_->updated();
  const bool changedBrightness = brightness_->updated();
  const bool changedColor = hue_->updated() || saturation_->updated();
  const float hue = hue_->updated() ? hue_->getNewVal<float>()
                                   : hue_->getVal<float>();
  const float saturation = saturation_->updated()
      ? saturation_->getNewVal<float>() : saturation_->getVal<float>();
  const int brightness = changedBrightness ? brightness_->getNewVal<int>()
                                           : brightness_->getVal<int>();
  bool power = changedPower ? power_->getNewVal<bool>() : power_->getVal<bool>();

  if ((changedBrightness || changedColor) && !changedPower)
    power = changedColor || brightness > 0;
  if (changedBrightness && brightness == 0) power = false;

  const bool sendPower = changedPower || changedBrightness || changedColor;
  const bool sendBrightness = changedBrightness && brightness > 0;
  if (!wled_.writeState(sendPower, power, sendBrightness, brightness,
                        changedColor, hue, saturation)) {
    Serial.printf("[HomeKit] WLED command could not be queued (%s)\n", wled_.host().c_str());
    retryIntervalMs_ = kOfflineBackoffMs;
    return false;
  }

  if (power_->getVal<bool>() != power && !changedPower) power_->setVal(power);
  retryIntervalMs_ = kPollEveryMs;
  lastPollMs_ = millis();
  // Avoid float printf formatting; it pulls a large formatter into our
  // extremely tight 4MB dual-OTA image. Rounded degrees/percent suffice.
  Serial.printf("[HomeKit] %s %s power=%d brightness=%d color=%d hue=%u sat=%u\n",
                wled_.host().c_str(), worker_ ? "queued" : "accepted",
                power, brightness, changedColor,
                static_cast<unsigned>(hue + 0.5f),
                static_cast<unsigned>(saturation + 0.5f));
  return true;
}

void WledLightService::loop() {
  if (worker_) {
    WledNetworkWorker::Snapshot result;
    if (!worker_->snapshot(workerSlot_, result)) return;
    online_ = result.online;
    pendingCommands_ = result.pendingCommands;
    failures_ = result.totalFailures;
    droppedCommands_ = result.queueDrops;
    lastCommandOK_ = result.lastCommandOK;
    lastSeenMs_ = result.lastSeenMs;

    // Only the main HomeSpan loop modifies HAP characteristics. A stale
    // worker GET or an in-flight command must never roll a new setting back.
    if (!result.valid || result.pendingCommands
        || result.generation == lastWorkerGeneration_) return;
    lastWorkerGeneration_ = result.generation;
    observedOn_ = result.state.on;
    observedBrightness_ = result.state.brightness;
    if (power_->getVal<bool>() != result.state.on)
      power_->setVal(result.state.on);
    if (brightness_->getVal<int>() != result.state.brightness)
      brightness_->setVal(result.state.brightness);
    if (result.state.colorValid) {
      // Grey/white has no meaningful hue; retain the user's last wheel angle.
      if (result.state.saturation > 0.4f &&
          (hue_->getVal<float>() < result.state.hue - 0.4f ||
           hue_->getVal<float>() > result.state.hue + 0.4f))
        hue_->setVal(result.state.hue);
      if (saturation_->getVal<float>() < result.state.saturation - 0.4f ||
          saturation_->getVal<float>() > result.state.saturation + 0.4f)
        saturation_->setVal(result.state.saturation);
    }
    if (lastObservedPresetId_ != result.state.presetId
        || lastObservedPlaylistId_ != result.state.playlistId) {
      lastObservedPresetId_ = result.state.presetId;
      lastObservedPlaylistId_ = result.state.playlistId;
      Serial.printf("[WLED] %s active preset=%d playlist=%d\n",
                    wled_.host().c_str(), result.state.presetId,
                    result.state.playlistId);
    }
    return;
  }
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
    online_ = false;
    retryIntervalMs_ = kOfflineBackoffMs;
    return;
  }

  retryIntervalMs_ = kPollEveryMs;
  online_ = true;
  observedOn_ = state.on;
  observedBrightness_ = state.brightness;
  lastSeenMs_ = now;
  acknowledgedRevision_ = currentRevision;

  const bool previousPower = power_->getVal<bool>();
  const int previousBrightness = brightness_->getVal<int>();
  if (previousPower != state.on) power_->setVal(state.on);
  if (previousBrightness != state.brightness)
    brightness_->setVal(state.brightness);

  if (previousPower != state.on || previousBrightness != state.brightness)
    Serial.printf("[WLED] %s state sync on=%d brightness=%d\n",
                  wled_.host().c_str(), state.on, state.brightness);
  if (state.colorValid) {
    if (state.saturation > 0.4f &&
        (hue_->getVal<float>() < state.hue - 0.4f ||
         hue_->getVal<float>() > state.hue + 0.4f))
      hue_->setVal(state.hue);
    if (saturation_->getVal<float>() < state.saturation - 0.4f ||
        saturation_->getVal<float>() > state.saturation + 0.4f)
      saturation_->setVal(state.saturation);
  }
  if (lastObservedPresetId_ != state.presetId
      || lastObservedPlaylistId_ != state.playlistId) {
    lastObservedPresetId_ = state.presetId;
    lastObservedPlaylistId_ = state.playlistId;
    Serial.printf("[WLED] %s active preset=%d playlist=%d\n",
                  wled_.host().c_str(), state.presetId, state.playlistId);
  }
}
