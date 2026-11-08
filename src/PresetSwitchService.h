#pragma once
#include "HomeSpan.h"
#include "WledClient.h"
#include "WledLightService.h"

class PresetSwitchService : public Service::Switch {
public:
  PresetSwitchService(WledClient &client, uint32_t lightAid, uint8_t presetId,
                      const char *serviceName = nullptr,
                      WledLightService *trackedLight = nullptr);
  boolean update() override;
  void loop() override;

private:
  WledClient &client_;
  uint32_t lightAid_;
  uint8_t presetId_;
  WledLightService *trackedLight_ = nullptr;
  uint32_t lastActionMs_ = 0;
  SpanCharacteristic *on_;
  bool resetPending_ = false;
  uint32_t triggeredAtMs_ = 0;
};
