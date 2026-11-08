#pragma once
#include "HomeSpan.h"
#include "WledClient.h"
#include "WledLightService.h"
#include "EffectsControllerRegistry.h"
#include "EffectsInputPolicy.h"

// Experimental HomeKit TV-style InputSource selector for up to five WLED
// SAVED presets. Separate accessory: never replaces the working RGB light.
class WledEffectsTV : public Service::Television {
public:
  WledEffectsTV(WledClient &client, WledLightService &light,
                const EffectsControllerRecord &record,
                const char *displayName);
  boolean update() override;
  void loop() override;
private:
  WledClient &client_;
  WledLightService &light_;
  const EffectsControllerRecord &record_;
  SpanCharacteristic *active_;
  SpanCharacteristic *input_;
};
