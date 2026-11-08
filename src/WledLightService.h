#pragma once
#include "HomeSpan.h"
#include "WledClient.h"
#include "WledNetworkWorker.h"

class WledLightService : public Service::LightBulb {
public:
  explicit WledLightService(WledClient &wled, unsigned position = 0, WledNetworkWorker *worker = nullptr);
  boolean update() override;
  void loop() override;
  bool online() const { return online_; }
  bool observedOn() const { return observedOn_; }
  int observedBrightness() const { return observedBrightness_; }
  int observedPresetId() const { return lastObservedPresetId_; }
  uint32_t lastSeenMs() const { return lastSeenMs_; }
  uint16_t pendingCommands() const { return pendingCommands_; }
  uint32_t networkFailures() const { return failures_; }
  uint32_t droppedCommands() const { return droppedCommands_; }
  bool lastCommandOK() const { return lastCommandOK_; }

private:
  WledClient &wled_;
  WledNetworkWorker *worker_ = nullptr;
  unsigned workerSlot_ = 0;
  uint32_t lastWorkerGeneration_ = 0;
  uint16_t pendingCommands_ = 0;
  uint32_t failures_ = 0;
  uint32_t droppedCommands_ = 0;
  bool lastCommandOK_ = true;
  SpanCharacteristic *power_;
  SpanCharacteristic *brightness_;
  SpanCharacteristic *hue_;
  SpanCharacteristic *saturation_;
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
