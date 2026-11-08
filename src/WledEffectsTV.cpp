#include "WledEffectsTV.h"
#include <Arduino.h>

WledEffectsTV::WledEffectsTV(WledClient &client, WledLightService &light,
                             const EffectsControllerRecord &record,
                             const char *displayName)
    : Service::Television(), client_(client), light_(light), record_(record) {
  active_ = new Characteristic::Active(1);
  input_ = new Characteristic::ActiveIdentifier(1);
  new Characteristic::ConfiguredName(displayName);
  // HomeSpan supports DisplayOrder TLV8 to prevent Home's selector shuffling.
  TLV8 order;
  for (unsigned i = 0; i < record.count(); ++i) {
    if (i) order.add(0);
    order.add(1, i + 1);
  }
  new Characteristic::DisplayOrder(order);
}

boolean WledEffectsTV::update() {
  const bool changedPower = active_->updated();
  const bool desiredOn = changedPower ? active_->getNewVal<uint8_t>() != 0
                                      : active_->getVal<uint8_t>() != 0;
  const bool selectedInput = input_->updated();
  const uint32_t id = selectedInput ? input_->getNewVal<uint32_t>()
                                    : input_->getVal<uint32_t>();
  if (!bridge::effectsInputIdentifierValid(id, record_.count())) {
    Serial.printf("[Effects] Rejected input index %lu\n",
                  static_cast<unsigned long>(id));
    return false;
  }
  // Unlike regular WLED light controls, TV power-on REAPPLIES the last
  // chosen preset; power-off actually turns WLED off.
  if (changedPower && !desiredOn) {
    if (!client_.writeState(true, false, false, 50)) return false;
    Serial.println("[Effects] Controller off -> WLED off queued");
    return true;
  }
  if (selectedInput || (changedPower && desiredOn)) {
    const uint8_t preset = record_.inputs[id - 1].presetId;
    if (!bridge::validPresetId(preset) || !client_.applyPreset(preset))
      return false;
    if (selectedInput && !desiredOn) active_->setVal(1);
    Serial.printf("[Effects] Input %lu -> WLED saved preset %u queued\n",
                  static_cast<unsigned long>(id), static_cast<unsigned>(preset));
  }
  return true;
}

void WledEffectsTV::loop() {
  if (!light_.lastSeenMs() || light_.pendingCommands()) return;
  const uint8_t actualOn = light_.observedOn() ? 1 : 0;
  if (active_->getVal<uint8_t>() != actualOn) active_->setVal(actualOn);
  // A preset changed in WLED or another client can also update the
  // Home input selector. If the WLED preset is not one of our chosen
  // entries, keep the last selection (do not fabricate a new input).
  const int preset = light_.observedPresetId();
  const int selected = preset > 0 ? [&]() {
    for (unsigned i = 0; i < record_.count(); ++i)
      if (record_.inputs[i].presetId == preset) return static_cast<int>(i);
    return -1;
  }() : -1;
  if (selected >= 0 && input_->getVal<uint32_t>() !=
                         static_cast<uint32_t>(selected + 1))
    input_->setVal(static_cast<uint32_t>(selected + 1));
}
