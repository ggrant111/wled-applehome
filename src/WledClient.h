#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "PresetPolicy.h"

struct WledState {
  bool on = false;
  int brightness = 50;
  int presetId = -1;
  float hue = 0.0f;
  float saturation = 100.0f;
  bool colorValid = false;
  uint8_t mainSegmentId = 0;
};

struct WledInfo {
  String ip;
  String mac;
  String name;
};


class WledNetworkWorker;

class WledClient {
public:
  void setHost(const String &host);
  void attachWorker(WledNetworkWorker *worker, unsigned slot);
  bool backgroundActive() const { return worker_ != nullptr; }
  const String &host() const;
  bool configured() const;
  bool readState(WledState &out);
  bool writeState(bool changePower, bool power, bool changeBrightness, int brightnessPercent,
                  bool changeColor = false, float hue = 0.0f, float saturation = 100.0f,
                  int segmentId = -1);
  bool applyPreset(uint8_t presetId);
  bool printPresetCatalog();
  bool getPresetNames(JsonDocument &out) { return loadPresetNames(out, 0); }
  bool lookupPresetName(uint8_t presetId, String &name);
  uint32_t mutationRevision() const { return mutationRevision_; }
  uint32_t lastMutationMs() const { return lastMutationMs_; }
  static bool probe(const String &ip, WledInfo &out);
  static String normalizeMac(const String &input);
private:
  String host_;
  WledNetworkWorker *worker_ = nullptr;
  unsigned workerSlot_ = 0;
  uint32_t mutationRevision_ = 0;
  uint32_t lastMutationMs_ = 0;
  String url() const;
  bool sendJson(const String &body);
  bool loadPresetNames(JsonDocument &out, int onlyPresetId);
  void markMutation();
};
