#pragma once
#include "HomeSpan.h"
#include "WledClient.h"

class PresetSwitchService : public Service::Switch {
public:
  PresetSwitchService(WledClient &client, uint32_t lightAid, uint8_t presetId,
                      const char *serviceName = nullptr);
  boolean update() override;
  void loop() override;

private:
  WledClient &client_;
  uint32_t lightAid_;
  uint8_t presetId_;
  SpanCharacteristic *on_;
  bool resetPending_ = false;
  uint32_t triggeredAtMs_ = 0;
};
