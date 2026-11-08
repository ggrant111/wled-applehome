#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "HomeSpan.h"

#include "DeviceRegistry.h"
#include "WledClient.h"
#include "WledDiscovery.h"
#include "WledLightService.h"

#ifndef WLED_APP_VERSION
#define WLED_APP_VERSION "0.2.0-dev"
#endif

namespace {
DeviceRegistry registry;
WledDiscovery discovery;
WledClient clients[bridge::kMaxDevices];

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
    Serial.println("[Registry] Device MAC not provided by WLED; can't safely assign stable identity.");
    return;
  }
  enrollInfo(info);
}

void removeCommand(const char *command) {
  const uint32_t aid = parseAid(args(command, 'r'));
  const int index = registry.indexByAid(aid);
  if (index < 0) {
    Serial.println("[Registry] Usage: @r<accessory-id> from @l; e.g. @r3");
    return;
  }
  Serial.printf("[Registry] Removing AID=%lu (user requested).\n",
                static_cast<unsigned long>(aid));
  if (registry.removeByAid(aid)) {
    clients[index].setHost(""); // stop writes until reboot
    Serial.println("[Registry] Stored. Type R to reboot and update Apple Home.");
    Serial.println("[Registry] Removed AIDs are NEVER reused for other WLED devices.");
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

void addBridgeInformation(const char *name, const char *model) {
  new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name(name);
    new Characteristic::Manufacturer("GlowDeck");
    new Characteristic::Model(model);
    new Characteristic::FirmwareRevision(WLED_APP_VERSION);
}

void addDevices() {
  // Stable AID=2 for the original paired light; new AIDs allocated monotonically.
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
    const DeviceRecord &record = registry.at(i);
    if (!record.aid) continue;
    clients[i].setHost(record.ip);
    new SpanAccessory(record.aid);
      addBridgeInformation(record.aid == 2 ? "WLED Light" : record.name.c_str(),
                           record.aid == 2 ? "WLED JSON Light" : "WLED JSON Light");
      new WledLightService(clients[i], i);
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[Boot] WLED Apple Home Bridge v" WLED_APP_VERSION);
  if (!registry.begin()) {
    Serial.println("[Boot] NVS registry failed; startup halted to prevent identity loss.");
    while (true) delay(1000);
  }

  homeSpan.setSketchVersion(WLED_APP_VERSION);
  homeSpan.begin(Category::Bridges, "WLED Apple Home Bridge", "wled-applehome");

  new SpanAccessory(1);
    addBridgeInformation("WLED Apple Home Bridge", "ESP32 HomeKit Bridge");
  addDevices();

  new SpanUserCommand('w', " <IPv4> set existing AID=2 IP", wledCommand);
  new SpanUserCommand('l', " list registered WLED lights", listCommand);
  new SpanUserCommand('d', " discover WLED controllers by mDNS", discoverCommand);
  new SpanUserCommand('a', " <number> enroll a discovered WLED light", addCommand);
  new SpanUserCommand('m', " <IPv4> manually enroll a verified WLED light", manualCommand);
  new SpanUserCommand('r', " <AID> remove a registered light", removeCommand);

  Serial.println("[Setup] @l list, @d discover, @a1 add, @m<IP> manual add, @r<AID> remove.");
  Serial.println("[Setup] @w<IP> retains the original light. Reboot with R after add/remove.");
  Serial.println("[Setup] HomeSpan commands: ? help, W Wi-Fi, S pairing code, R reboot.");
}

void loop() {
  homeSpan.poll();
}
