#pragma once
#include "HomeSpan.h"
#include "WledClient.h"

class WledLightService : public Service::LightBulb {
public:
  explicit WledLightService(WledClient &wled, unsigned position = 0);
  boolean update() override;
  void loop() override;

private:
  WledClient &wled_;
  SpanCharacteristic *power_;
  SpanCharacteristic *brightness_;
  uint32_t lastPollMs_ = 0;
  uint32_t retryIntervalMs_ = 8000;
  static constexpr uint32_t kPollEveryMs = 8000;
  static constexpr uint32_t kOfflineBackoffMs = 30000;
};
