#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "DeviceRegistry.h"
#include "PresetRegistry.h"
#include "EffectsControllerRegistry.h"
#include "WledDiscovery.h"
#include "WledLightService.h"
#include "PortalValidation.h"

class PortalServer {
public:
  PortalServer(DeviceRegistry &devices, PresetRegistry &presets,
               EffectsControllerRegistry &effects,
               WledDiscovery &discovery, WledClient *clients,
               WledLightService **services);
  void configure(const String &adminPassword);
  void poll();

private:
  DeviceRegistry &devices_;
  PresetRegistry &presets_;
  EffectsControllerRegistry &effects_;
  uint32_t bootEffectsAid_ = 0;
  WledDiscovery &discovery_;
  WledClient *clients_;
  WledLightService **services_;
  WebServer http_{bridge::kPortalPort};
  String password_;
  String token_;
  uint32_t bootAids_[bridge::kMaxDevices] = {};
  uint32_t restartAtMs_ = 0;
  bool listening_ = false;

  void routes();
  bool authorize(bool mutation = false);
  void respond(int status, const String &error);
  void sendJSON(JsonDocument &doc);
  bool parseRequestId(const String &text, uint32_t &value) const;
  void handleIndex();
  void handleStatus();
  void handleCatalog();
  void handleScan();
  void handleEnroll();
  void handleControl();
  void handleRemoveDevice();
  void handleAddPreset();
  void handleRemovePreset();
  void handleAddEffect();
  void handleRemoveEffectsController();
  void handleConvertEffects();
  void handleRestart();
};
