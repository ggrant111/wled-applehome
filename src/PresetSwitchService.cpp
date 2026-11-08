#include "PresetSwitchService.h"
#include <Arduino.h>
#include "PresetPowerPolicy.h"

PresetSwitchService::PresetSwitchService(WledClient &client,
                                         uint32_t lightAid, uint8_t presetId,
                                         const char *serviceName,
                                         WledLightService *trackedLight,
                                         bool isPlaylist)
    : Service::Switch(), client_(client), lightAid_(lightAid),
      presetId_(presetId), trackedLight_(trackedLight),
      isPlaylist_(isPlaylist) {
  on_ = new Characteristic::On(false);
  if (serviceName && *serviceName)
    new Characteristic::ConfiguredName(serviceName);
}

boolean PresetSwitchService::update() {
  const bool nextOn = on_->getNewVal<bool>();
  if (!nextOn) {
    // Grouped effects are SELECTABLE POWER buttons: Off turns the associated
    // WLED light off. Existing standalone preset triggers remain momentary.
    if (!trackedLight_) return true;
    if (!client_.writeState(true, false, false, 50)) {
      Serial.printf("[EffectPower] Cannot queue off: light AID=%lu\n",
                    static_cast<unsigned long>(lightAid_));
      return false;
    }
    lastActionMs_ = millis();
    Serial.printf("[EffectPower] light AID=%lu preset=%u OFF queued\n",
                  static_cast<unsigned long>(lightAid_), unsigned(presetId_));
    return true;
  }

  if (!client_.applyPreset(presetId_)) {
    Serial.printf("[Preset] FAILED light AID=%lu preset=%u\n",
                  static_cast<unsigned long>(lightAid_),
                  static_cast<unsigned>(presetId_));
    return false; // Rejected by the local queue or failed synchronously.
  }

  lastActionMs_ = millis();
  if (!trackedLight_) {
    triggeredAtMs_ = lastActionMs_;
    resetPending_ = true;
  }
  Serial.printf("[Preset] %s light AID=%lu preset=%u\n",
                client_.backgroundActive() ? "Queued" : "Triggered",
                static_cast<unsigned long>(lightAid_),
                static_cast<unsigned>(presetId_));
  return true;
}

void PresetSwitchService::loop() {
  if (trackedLight_) {
    // Only one grouped effect stays On, matching the selected WLED preset.
    // Never infer failure from stale readings during or right after a command.
    if (!trackedLight_->online() || !trackedLight_->lastSeenMs()
        || trackedLight_->pendingCommands()
        || static_cast<uint32_t>(millis() - lastActionMs_) < 1400)
      return;
    const bool active = bridge::presetOrPlaylistPowerOn(
        trackedLight_->observedOn(), trackedLight_->observedPresetId(),
        trackedLight_->observedPlaylistId(), presetId_, isPlaylist_);
    if (on_->getVal<bool>() != active) on_->setVal(active);
    return;
  }
  // Standalone preset switches retain the original momentary action behavior.
  if (resetPending_ && static_cast<uint32_t>(millis() - triggeredAtMs_) >= 250) {
    resetPending_ = false;
    on_->setVal(false);
  }
}
