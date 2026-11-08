#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <cstring>
#include "HomeSpan.h"
#include "SpanRollback.h"
#include "AdminCredentials.h"
#include "PortalServer.h"

#include "DeviceRegistry.h"
#include "PresetRegistry.h"
#include "EffectsControllerRegistry.h"
#include "WledEffectsTV.h"
#include "PresetSwitchService.h"
#include "WledClient.h"
#include "WledNetworkWorker.h"
#include "WledDiscovery.h"
#include "WledLightService.h"

#ifndef WLED_APP_VERSION
#define WLED_APP_VERSION "0.7.1-dev"
#endif

namespace {
DeviceRegistry registry;
PresetRegistry presets;
EffectsControllerRegistry effects;
WledDiscovery discovery;
WledClient clients[bridge::kMaxDevices];
WledNetworkWorker networkWorker;
WledLightService *lightServices[bridge::kMaxDevices] = {};
AdminCredentials admin;
PortalServer portal(registry, presets, effects, discovery, clients, lightServices);
bool sketchValidated_ = false;

const char *args(const char *command, char commandLetter) {
  if (!command) return "";
  const char *p = command;
  if (*p == commandLetter || *p == (commandLetter - 32)) ++p;
  while (*p == ' ' || *p == '\t') ++p;
  return p;
}

bool parseIp(const char *text, String &address) {
  if (!text || *text == '\0') return false;
  IPAddress ip;
  if (!ip.fromString(text) || ip == IPAddress(0,0,0,0) ||
      ip == IPAddress(255,255,255,255))
    return false;
  address = ip.toString();
  return true;
}

uint32_t parseAid(const char *text) {
  if (!text || !*text) return 0;
  char *end = nullptr;
  const unsigned long n = strtoul(text, &end, 10);
  return end != text && *end == '\0' && n > 0 && n < 0xFFFFFFF0UL
             ? static_cast<uint32_t>(n) : 0;
}

void listCommand(const char *) { registry.list(); }
void listPresetsCommand(const char *) { presets.list(); }

void discoverCommand(const char *) {
  discovery.scan();
  Serial.println("[Discovery] Adding devices is opt-in; discovery does not change Apple Home.");
}

void announceEnroll(EnrollResult result, uint32_t aid) {
  switch (result) {
    case EnrollResult::Added:
      Serial.printf("[Registry] Added WLED as AID=%lu. Type R to reboot and expose in Apple Home.\n",
                    static_cast<unsigned long>(aid)); break;
    case EnrollResult::UpdatedIp:
      Serial.printf("[Registry] Updated existing AID=%lu without replacing its identity.\n",
                    static_cast<unsigned long>(aid)); break;
    case EnrollResult::AlreadyKnown:
      Serial.printf("[Registry] Already registered as AID=%lu.\n",
                    static_cast<unsigned long>(aid)); break;
    case EnrollResult::Full:
      Serial.println("[Registry] Bridge is full (8 lights)."); break;
    case EnrollResult::Invalid:
      Serial.println("[Registry] Refused duplicate IP, missing MAC, or invalid identity."); break;
    case EnrollResult::PersistenceFailed:
      Serial.println("[Registry] NVS write failed; configuration was not applied."); break;
  }
}

void enrollInfo(const WledInfo &info) {
  uint32_t aid = 0;
  const EnrollResult result = registry.enroll(info.ip, info.mac, info.name, &aid);
  announceEnroll(result, aid);
  if (result == EnrollResult::UpdatedIp || result == EnrollResult::AlreadyKnown) {
    const int index = registry.indexByAid(aid);
    if (index >= 0) clients[index].setHost(registry.at(index).ip);
  }
}

void addCommand(const char *command) {
  const uint32_t selected = parseAid(args(command, 'a'));
  if (selected < 1 || selected > discovery.count()) {
    Serial.println("[Discovery] Use @d to scan, then @a1, @a2, etc.");
    return;
  }
  const WledInfo *info = discovery.at(selected - 1);
  if (info) enrollInfo(*info);
}

void manualCommand(const char *command) {
  String ip;
  if (!parseIp(args(command, 'm'), ip)) {
    Serial.println("[Registry] Usage: @m192.168.1.50");
    return;
  }
  WledInfo info;
  if (!WledClient::probe(ip, info)) {
    Serial.println("[Registry] WLED /json/info probe failed; not added.");
    return;
  }
  if (info.mac.isEmpty()) {
    Serial.println("[Registry] Device MAC unavailable; stable identity required.");
    return;
  }
  enrollInfo(info);
}

void removeCommand(const char *command) {
  const uint32_t aid = parseAid(args(command, 'r'));
  const int index = registry.indexByAid(aid);
  if (index < 0) {
    Serial.println("[Registry] Usage: @r<light-AID> from @l; e.g. @r3");
    return;
  }
  if (registry.removeByAid(aid)) {
    clients[index].setHost("");
    const unsigned removed = presets.removeForDevice(aid);
    if (effects.configured() && effects.record().deviceAid == aid) {
      effects.remove(effects.record().aid);
      Serial.println("[Effects] Linked selector removed alongside WLED light.");
    }
    Serial.printf("[Registry] Removed light AID=%lu, %u preset switch(es).\n",
                  static_cast<unsigned long>(aid), removed);
    Serial.println("[Registry] Type R to reboot and update Apple Home.");
  } else {
    Serial.println("[Registry] NVS removal failed.");
  }
}

void wledCommand(const char *command) {
  const char *value = args(command, 'w');
  if (!*value) {
    const int idx = registry.indexByAid(2);
    Serial.printf("[Config] Original light (AID=2) IP: %s\n",
                  idx < 0 || registry.at(idx).ip.isEmpty()
                      ? "(not configured or removed)"
                      : registry.at(idx).ip.c_str());
    Serial.println("[Config] Usage: @w192.168.1.50");
    return;
  }
  String ip;
  if (!parseIp(value, ip)) {
    Serial.println("[Config] Invalid IPv4; use @w192.168.1.50");
    return;
  }
  WledInfo verified;
  const bool probed = WledClient::probe(ip, verified);
  if (!registry.updateLegacyIp(ip, probed ? verified.mac : "")) {
    Serial.println("[Config] AID=2 missing or NVS write failed.");
    return;
  }
  const int idx = registry.indexByAid(2);
  if (idx >= 0) clients[idx].setHost(ip);
  Serial.printf("[Config] Original AID=2 now controls %s%s\n", ip.c_str(),
                probed ? "" : " (not verified online)");
}

void adminCommand(const char *command) {
  // HomeSpan serial CLI: @k <strong-password> or @k -
  const char *p = command ? command : "";
  while (*p == ' ' || *p == '\t') ++p;
  if ((*p == 'k' || *p == 'K') && (p[1] == ' ' || p[1] == '\t')) ++p;
  while (*p == ' ' || *p == '\t') ++p;

  if (*p == '\0') {
    Serial.printf("[Admin] Portal and OTA: %s (password never displayed)\n",
                  admin.enabled() ? "configured" : "disabled");
    Serial.println("[Admin] @k <14-64 char ASCII password> | @k - to disable");
    return;
  }
  if (strcmp(p, "-") == 0) {
    Serial.println(admin.disable()
      ? "[Admin] Remote portal and OTA disabled after reboot; pairing remains"
      : "[Admin] Unable to remove saved admin password");
    return;
  }
  if (admin.setPassword(String(p)))
    Serial.println("[Admin] Password saved to device NVS. Reboot with R to enable digest-auth portal and password-protected OTA.");
  else
    Serial.println("[Admin] Rejected; use 14-64 printable ASCII characters, no spaces. Nothing changed.");
}

void catalogCommand(const char *command) {
  const uint32_t aid = parseAid(args(command, 'c'));
  const int idx = registry.indexByAid(aid);
  if (idx < 0) {
    Serial.println("[Presets] Usage: @c<light-AID>, e.g. @c2");
    return;
  }
  if (!clients[idx].printPresetCatalog())
    Serial.printf("[Presets] Cannot load catalog for light AID=%lu\n",
                  static_cast<unsigned long>(aid));
}

void presetCommand(const char *command) {
  const char *value = args(command, 'p');
  const char *separator = strchr(value, ':');
  if (!separator) {
    Serial.println("[Presets] Usage: @p<light-AID>:<preset-id>, e.g. @p2:5");
    return;
  }
  String left(value);
  left = left.substring(0, separator - value);
  const uint32_t lightAid = parseAid(left.c_str());
  const uint32_t preset = parseAid(separator + 1);
  const int idx = registry.indexByAid(lightAid);
  if (idx < 0 || !bridge::validPresetId(preset)) {
    Serial.println("[Presets] Unknown light AID or invalid preset ID (1..250).");
    return;
  }
  String name;
  if (!clients[idx].lookupPresetName(static_cast<uint8_t>(preset), name)) {
    Serial.println("[Presets] Saved preset not found; no switch created.");
    return;
  }
  String title = registry.at(idx).name + String(" ") + name;
  if (title.length() > 35) title = title.substring(0, 35);
  uint32_t switchAid = 0;
  const auto outcome = presets.add(registry, lightAid,
                                   static_cast<uint8_t>(preset),
                                   title, &switchAid);
  switch (outcome) {
    case PresetEnrollResult::Added:
      Serial.printf("[Presets] Switch AID=%lu created; type R to expose in Apple Home.\n",
                    static_cast<unsigned long>(switchAid)); break;
    case PresetEnrollResult::Exists:
      Serial.printf("[Presets] Already exposed as switch AID=%lu\n",
                    static_cast<unsigned long>(switchAid)); break;
    case PresetEnrollResult::Full:
      Serial.println("[Presets] Limit of 12 exposed preset switches reached."); break;
    case PresetEnrollResult::Invalid:
      Serial.println("[Presets] Invalid light or preset."); break;
    case PresetEnrollResult::PersistenceFailed:
      Serial.println("[Presets] Could not save switch to NVS."); break;
  }
}

void removePresetCommand(const char *command) {
  const uint32_t aid = parseAid(args(command, 'q'));
  if (!presets.remove(aid)) {
    Serial.println("[Presets] Usage: @q<switch-AID> from @s, e.g. @q9");
    return;
  }
  Serial.printf("[Presets] Removed switch AID=%lu; reboot with R to update Apple Home.\n",
                static_cast<unsigned long>(aid));
}

void addBridgeInformation(const char *name, const char *model) {
  new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name(name);
    new Characteristic::Manufacturer("GlowDeck");
    new Characteristic::Model(model);
    new Characteristic::FirmwareRevision(WLED_APP_VERSION);
}

void addDevices() {
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
    const DeviceRecord &record = registry.at(i);
    if (!record.aid) continue;
    clients[i].setHost(record.ip);
    clients[i].attachWorker(&networkWorker, i);
    new SpanAccessory(record.aid);
      addBridgeInformation(record.aid == 2 ? "WLED Light" : record.name.c_str(),
                           "WLED JSON Light");
      lightServices[i] = new WledLightService(clients[i], i, &networkWorker);
  }
}

void addEffectsController() {
  if (!effects.configured()) return;
  const EffectsControllerRecord &record = effects.record();
  const int index = registry.indexByAid(record.deviceAid);
  if (index < 0 || !lightServices[index]) {
    Serial.println("[Effects] Controller ignored because its light is missing");
    return;
  }
  String name = registry.at(index).name + " Effects";
  if (name.length() > 35) name = name.substring(0, 35);
  new SpanAccessory(record.aid);
    addBridgeInformation(name.c_str(),
      record.mode == EffectsControllerMode::GroupedButtons
          ? "WLED Grouped Effects Buttons" : "WLED Effects Input Selector");
    if (record.mode == EffectsControllerMode::GroupedButtons) {
      // Grouped Switch services display multiple named, tappable controls
      // inside one HomeKit accessory. No TV power semantics apply here.
      for (unsigned i = 0; i < record.count(); ++i)
        new PresetSwitchService(clients[index], record.deviceAid,
                                record.inputs[i].presetId,
                                record.inputs[i].name.c_str());
    } else {
      WledEffectsTV *tv = new WledEffectsTV(clients[index], *lightServices[index],
                                            record, name.c_str());
      for (unsigned i = 0; i < record.count(); ++i) {
        SpanService *input = new Service::InputSource();
          new Characteristic::ConfiguredName(record.inputs[i].name.c_str());
          new Characteristic::Identifier(i + 1);
          new Characteristic::IsConfigured(1);
        tv->addLink(input);
      }
    }
  Serial.printf("[Effects] Added %s AID=%lu for WLED AID=%lu\n",
                record.mode == EffectsControllerMode::GroupedButtons
                    ? "grouped buttons" : "TV selector",
                static_cast<unsigned long>(record.aid),
                static_cast<unsigned long>(record.deviceAid));
}

void addPresetSwitches() {
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i) {
    const PresetRecord &p = presets.at(i);
    if (!p.aid) continue;
    const int lightIndex = registry.indexByAid(p.deviceAid);
    if (lightIndex < 0) {
      Serial.printf("[Presets] Orphan switch AID=%lu skipped; light missing.\n",
                    static_cast<unsigned long>(p.aid));
      continue;
    }
    new SpanAccessory(p.aid);
      addBridgeInformation(p.name.c_str(), "WLED Preset Switch");
      new PresetSwitchService(clients[lightIndex], p.deviceAid, p.presetId);
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[Boot] WLED Apple Home Bridge v" WLED_APP_VERSION);
  if (!registry.begin() || !presets.begin() || !effects.begin() || !admin.begin()) {
    Serial.println("[Boot] NVS registry failed; startup halted to prevent identity loss.");
    while (true) delay(1000);
  }

  // Never enable the publicly known default HomeSpan OTA password.
  if (admin.enabled()) homeSpan.enableOTA(admin.password().c_str());
  homeSpan.setSketchVersion(WLED_APP_VERSION);
  homeSpan.begin(Category::Bridges, "WLED Apple Home Bridge", "wled-applehome");

  new SpanAccessory(1);
    addBridgeInformation("WLED Apple Home Bridge", "ESP32 HomeKit Bridge");
  addDevices();
  addPresetSwitches();
  addEffectsController();
  if (!networkWorker.start()) {
    Serial.println("[Boot] Background WLED HTTP worker failed; stopping to protect HomeKit responsiveness.");
    while (true) delay(1000);
  }

  new SpanUserCommand('w', " <IPv4> set existing AID=2 IP", wledCommand);
  new SpanUserCommand('l', " list registered WLED lights", listCommand);
  new SpanUserCommand('d', " discover WLED controllers by mDNS", discoverCommand);
  new SpanUserCommand('a', " <number> enroll a discovered WLED light", addCommand);
  new SpanUserCommand('m', " <IPv4> manually enroll verified WLED light", manualCommand);
  new SpanUserCommand('r', " <AID> remove a registered light", removeCommand);
  new SpanUserCommand('c', " <light-AID> list WLED saved presets", catalogCommand);
  new SpanUserCommand('p', " <light-AID>:<preset-ID> expose preset as switch", presetCommand);
  new SpanUserCommand('s', " list exposed HomeKit preset switches", listPresetsCommand);
  new SpanUserCommand('q', " <switch-AID> remove HomeKit preset switch", removePresetCommand);
  new SpanUserCommand('k', " <password> enable LAN dashboard and OTA (R reboot)", adminCommand);

  portal.configure(admin.password());
  Serial.printf("[Admin] Management is %s; HTTP port 8080, authenticated on LAN only.\n",
                admin.enabled() ? "enabled" : "disabled");
  if (!admin.enabled())
    Serial.println("[Admin] To enable: @k <unique-password-at-least-14-characters> then R. Never commit passwords.");

  Serial.println("[Setup] WLED: @l list, @d discover, @a1 add, @m<IP> manual add.");
  Serial.println("[Setup] Presets: @c2 catalog, @p2:5 add preset #5, @s list, @q<switch-AID> remove.");
  Serial.println("[Setup] Effects: add up to 5 saved WLED presets in the web portal; R exposes selector.");
  Serial.println("[Setup] Reboot with R after add/remove; keep original AID=2.");
  Serial.println("[Setup] HomeSpan commands: ? help, W Wi-Fi, S pairing code, R reboot.");
}

void loop() {
  homeSpan.poll();
  portal.poll();

  // Keep the OTA partition pending until startup and Wi-Fi stabilize.
  if (!sketchValidated_ && millis() >= 60000 && WiFi.status() == WL_CONNECTED) {
    homeSpan.markSketchOK();
    sketchValidated_ = true;
    Serial.println("[OTA] Sketch marked valid after startup stability window");
  }
}
