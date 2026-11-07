#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "HomeSpan.h"

#include "WledClient.h"
#include "WledLightService.h"

#ifndef WLED_APP_VERSION
#define WLED_APP_VERSION "0.1.0"
#endif

namespace {
Preferences prefs;
WledClient wled;

void wledCommand(const char *command) {
  // HomeSpan user commands arrive as "w<arguments>" via "@w...".
  const char *address = command;
  if (*address == 'w' || *address == 'W') ++address;
  while (*address == ' ' || *address == '\t') ++address;

  if (*address == '\0') {
    Serial.printf("[Config] WLED IP: %s\n",
                  wled.configured() ? wled.host().c_str() : "(not configured)");
    Serial.println("[Config] Usage: @w192.168.1.50");
    return;
  }

  IPAddress ip;
  if (!ip.fromString(address) || ip == IPAddress(0,0,0,0) ||
      ip == IPAddress(255,255,255,255)) {
    Serial.println("[Config] Invalid IPv4 address; no changes saved");
    return;
  }

  const String normalized = ip.toString();
  prefs.putString("ip", normalized);
  wled.setHost(normalized);
  Serial.printf("[Config] WLED IP saved to NVS: %s\n", normalized.c_str());
}

void addBridgeInformation(const char *name, const char *model) {
  new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name(name);
    new Characteristic::Manufacturer("GlowDeck");
    new Characteristic::Model(model);
    new Characteristic::FirmwareRevision(WLED_APP_VERSION);
}
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[Boot] WLED Apple Home Bridge v" WLED_APP_VERSION);

  prefs.begin("wled-home", false);
  wled.setHost(prefs.getString("ip", ""));
  Serial.printf("[Boot] WLED target: %s\n",
                wled.configured() ? wled.host().c_str() : "(unconfigured)");

  // Wi-Fi and pairing secrets are provisioned using HomeSpan's CLI or setup AP;
  // NEVER hardcode credentials or a shared pairing code in firmware.
  homeSpan.setSketchVersion(WLED_APP_VERSION);
  homeSpan.begin(Category::Bridges, "WLED Apple Home Bridge", "wled-applehome");

  new SpanAccessory(1);               // AID 1: bridge, no LightBulb service
    addBridgeInformation("WLED Apple Home Bridge", "ESP32 HomeKit Bridge");

  new SpanAccessory(2);               // AID 2: stable first WLED accessory
    addBridgeInformation("WLED Light", "WLED JSON Light");
    new WledLightService(wled);

  new SpanUserCommand('w', " <IPv4> configure WLED controller IP; empty to show",
                      wledCommand);

  Serial.println("[Setup] Type ? for HomeSpan help, W for Wi-Fi, S for pairing code.");
  Serial.println("[Setup] Type @w192.168.1.50 to configure the first WLED light.");
}

void loop() {
  homeSpan.poll();
}
