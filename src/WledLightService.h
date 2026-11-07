#pragma once
#include "HomeSpan.h"
#include "WledClient.h"

class WledLightService : public Service::LightBulb {
public:
  explicit WledLightService(WledClient &wled);
  boolean update() override;
  void loop() override;

private:
  WledClient &wled_;
  SpanCharacteristic *power_;
  SpanCharacteristic *brightness_;
  uint32_t lastPollMs_ = 0;
  static constexpr uint32_t kPollEveryMs = 5000;
};
