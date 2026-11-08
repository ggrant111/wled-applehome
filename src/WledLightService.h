#pragma once
#include "HomeSpan.h"
#include "WledClient.h"

class WledLightService : public Service::LightBulb {
public:
  explicit WledLightService(WledClient &wled, unsigned position = 0);
  boolean update() override;
  void loop() override;
  bool online() const { return online_; }
  bool observedOn() const { return observedOn_; }
  int observedBrightness() const { return observedBrightness_; }
  uint32_t lastSeenMs() const { return lastSeenMs_; }

private:
  WledClient &wled_;
  SpanCharacteristic *power_;
  SpanCharacteristic *brightness_;
  uint32_t lastPollMs_ = 0;
  uint32_t retryIntervalMs_ = 8000;
  uint32_t acknowledgedRevision_ = 0;
  int lastObservedPresetId_ = -2;
  bool online_ = false;
  bool observedOn_ = false;
  int observedBrightness_ = 50;
  uint32_t lastSeenMs_ = 0;
  static constexpr uint32_t kPollEveryMs = 8000;
  static constexpr uint32_t kOfflineBackoffMs = 30000;
};
