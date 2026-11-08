#include "PortalServer.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include "PortalPage.h"
#include "PresetPolicy.h"
#include "CsrfPolicy.h"

namespace {
constexpr const char *kRealm = "WLED Apple Home";
const char *csrfHeaderNames[] = {"X-Bridge-Token"};

bool parseIpv4(String text, String &normalized) {
  text.trim();
  if (text.isEmpty()) return false;
  IPAddress ip;
  if (!ip.fromString(text) || ip == IPAddress(0, 0, 0, 0)
       || ip == IPAddress(255, 255, 255, 255)) return false;
  normalized = ip.toString();
  return true;
}
}

PortalServer::PortalServer(DeviceRegistry &devices, PresetRegistry &presets,
                           WledDiscovery &discovery, WledClient *clients,
                           WledLightService **services)
    : devices_(devices), presets_(presets), discovery_(discovery),
      clients_(clients), services_(services) {}

void PortalServer::configure(const String &adminPassword) {
  password_ = adminPassword;
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i)
    bootAids_[i] = devices_.at(i).aid;

  char token[25];
  snprintf(token, sizeof(token), "%08lX%08lX%08lX",
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()));
  token_ = token;
}

bool PortalServer::authorize(bool mutation) {
  if (password_.isEmpty()) {
    respond(503, "Admin credentials not configured");
    return false;
  }
  if (!http_.authenticate("admin", password_.c_str())) {
    http_.requestAuthentication(DIGEST_AUTH, kRealm);
    return false;
  }
  // Accept the same boot-scoped token in a custom header or legacy form field.
  // Never permit an unauthenticated/CSRF-less POST.
  if (mutation &&
      !bridge::authorizedCsrf(token_.c_str(),
                              http_.header("X-Bridge-Token").c_str(),
                              http_.arg("token").c_str())) {
    Serial.println("[Portal] Rejected stale or missing CSRF token");
    respond(403, "Missing or invalid CSRF token");
    return false;
  }
  return true;
}

void PortalServer::respond(int status, const String &error) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = error;
  String payload;
  serializeJson(doc, payload);
  http_.sendHeader("Cache-Control", "no-store");
  http_.send(status, "application/json", payload);
}

void PortalServer::sendJSON(JsonDocument &doc) {
  String payload;
  serializeJson(doc, payload);
  http_.sendHeader("Cache-Control", "no-store");
  http_.send(200, "application/json", payload);
}

bool PortalServer::parseRequestId(const String &text, uint32_t &out) const {
  return bridge::validPositiveId(text.c_str(), out);
}

void PortalServer::handleIndex() {
  if (!authorize()) return;
  // Compressed read-only page is served directly from PROGMEM, eliminating
  // the 13 KB per-request String allocation and saving OTA flash headroom.
  // The per-boot CSRF secret is fetched via Digest-authenticated /api/status.
  http_.sendHeader("Cache-Control", "no-store");
  http_.sendHeader("Content-Encoding", "gzip");
  http_.sendHeader("X-Content-Type-Options", "nosniff");
  http_.send_P(200, "text/html; charset=utf-8",
               reinterpret_cast<const char *>(kPortalGzip),
               kPortalGzipLength);
}

void PortalServer::handleStatus() {
  if (!authorize()) return;
  // Same-origin authenticated status GET allows a stale tab to re-sync
  // after an ESP32 reboot; no CORS access or cache is permitted.
  http_.sendHeader("X-Bridge-Token", token_);
  JsonDocument doc;
  doc["ok"] = true;
  doc["version"] = "0.5.0-dev";
  doc["wifi"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["uptimeSec"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["minFreeHeap"] = ESP.getMinFreeHeap();
  doc["freeSketchSpace"] = ESP.getFreeSketchSpace();

  JsonArray lights = doc["devices"].to<JsonArray>();
  for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
    const DeviceRecord &d = devices_.at(i);
    if (!d.aid) continue;
    JsonObject row = lights.add<JsonObject>();
    row["aid"] = d.aid;
    row["name"] = d.name;
    row["ip"] = d.ip;
    row["mac"] = d.mac;
    const bool booted = bootAids_[i] == d.aid && services_[i] != nullptr;
    row["pending"] = !booted;
    row["online"] = booted && services_[i]->online();
    if (booted) {
      row["on"] = services_[i]->observedOn();
      row["brightness"] = services_[i]->observedBrightness();
      row["lastSeenSec"] = services_[i]->lastSeenMs() == 0
          ? -1 : static_cast<int32_t>((millis() - services_[i]->lastSeenMs()) / 1000);
      row["pendingCommands"] = services_[i]->pendingCommands();
      row["networkFailures"] = services_[i]->networkFailures();
      row["droppedCommands"] = services_[i]->droppedCommands();
      row["lastCommandOK"] = services_[i]->lastCommandOK();
    }
  }

  JsonArray switches = doc["presets"].to<JsonArray>();
  for (unsigned i = 0; i < bridge::kMaxExposedPresets; ++i) {
    const PresetRecord &p = presets_.at(i);
    if (!p.aid) continue;
    JsonObject row = switches.add<JsonObject>();
    row["aid"] = p.aid;
    row["deviceAid"] = p.deviceAid;
    row["presetId"] = p.presetId;
    row["name"] = p.name;
  }
  sendJSON(doc);
}

void PortalServer::handleCatalog() {
  if (!authorize()) return;
  uint32_t aid = 0;
  if (!parseRequestId(http_.arg("aid"), aid)) return respond(400, "Invalid light AID");
  const int index = devices_.indexByAid(aid);
  if (index < 0) return respond(404, "Light not registered");

  JsonDocument catalog;
  if (!clients_[index].getPresetNames(catalog))
    return respond(502, "WLED preset catalog unavailable");

  JsonDocument response;
  response["ok"] = true;
  JsonArray items = response["presets"].to<JsonArray>();
  for (unsigned id = 1; id <= bridge::kMaxWledPresetId; ++id) {
    JsonVariant preset = catalog[String(id)];
    if (preset.isNull()) continue;
    JsonObject row = items.add<JsonObject>();
    row["id"] = id;
    row["name"] = String(preset["n"] | "Preset");
  }
  sendJSON(response);
}

void PortalServer::handleScan() {
  if (!authorize(true)) return;
  discovery_.scan(); // on-demand and potentially blocking; not a background scan.
  JsonDocument response;
  response["ok"] = true;
  JsonArray items = response["devices"].to<JsonArray>();
  for (unsigned i = 0; i < discovery_.count(); ++i) {
    const WledInfo *d = discovery_.at(i);
    if (!d) continue;
    JsonObject row = items.add<JsonObject>();
    row["name"] = d->name;
    row["ip"] = d->ip;
    row["mac"] = d->mac;
  }
  sendJSON(response);
}

void PortalServer::handleEnroll() {
  if (!authorize(true)) return;
  WledInfo info;
  uint32_t candidateIndex = 0;
  if (parseRequestId(http_.arg("index"), candidateIndex)) {
    if (candidateIndex > discovery_.count())
      return respond(400, "Discovery selection is no longer valid");
    const WledInfo *d = discovery_.at(candidateIndex - 1);
    if (!d) return respond(404, "Device disappeared from scan");
    info = *d;
  } else {
    String ip;
    if (!parseIpv4(http_.arg("ip"), ip))
      return respond(400, "Invalid WLED IPv4 address");
    if (!WledClient::probe(ip, info))
      return respond(502, "Address is not responding as a WLED controller");
  }
  if (info.mac.length() != 12) return respond(409, "WLED hardware MAC required");

  uint32_t aid = 0;
  const EnrollResult result = devices_.enroll(info.ip, info.mac, info.name, &aid);
  if (result == EnrollResult::Full) return respond(409, "Limit of 8 WLED lights reached");
  if (result == EnrollResult::Invalid) return respond(409, "Duplicate or conflicting device identity");
  if (result == EnrollResult::PersistenceFailed) return respond(500, "NVS registry write failed");
  if (result == EnrollResult::UpdatedIp || result == EnrollResult::AlreadyKnown) {
    const int index = devices_.indexByAid(aid);
    if (index >= 0) clients_[index].setHost(devices_.at(index).ip);
  }
  JsonDocument response;
  response["ok"] = true;
  response["aid"] = aid;
  response["requiresReboot"] = result == EnrollResult::Added;
  response["message"] = result == EnrollResult::Added
      ? "WLED saved. Restart bridge to add it to Apple Home."
      : "Existing WLED device updated; accessory identity preserved.";
  sendJSON(response);
}

void PortalServer::handleControl() {
  if (!authorize(true)) return;
  uint32_t aid = 0;
  if (!parseRequestId(http_.arg("aid"), aid))
    return respond(400, "Invalid WLED accessory ID");
  const int index = devices_.indexByAid(aid);
  if (index < 0) return respond(404, "Light not registered");
  // Only control devices that were already in the HomeKit accessory database.
  if (bootAids_[index] != aid || !services_[index])
    return respond(409, "Restart to activate this new light");

  bool succeeded = false;
  const String action = http_.arg("action");
  if (action == "on") succeeded = clients_[index].writeState(true, true, false, 50);
  else if (action == "off") succeeded = clients_[index].writeState(true, false, false, 50);
  else if (action == "brightness") {
    uint32_t level = 0;
    if (!parseRequestId(http_.arg("brightness"), level) || level > 100)
      return respond(400, "Brightness must be 1–100%");
    succeeded = clients_[index].writeState(true, true, true, static_cast<int>(level));
  } else return respond(400, "Unknown control action");

  if (!succeeded) return respond(503, "WLED command queue unavailable or full");
  JsonDocument response;
  response["ok"] = true;
  response["message"] = "Command queued. WLED acknowledgement and state will update asynchronously.";
  sendJSON(response);
}

void PortalServer::handleRemoveDevice() {
  if (!authorize(true)) return;
  uint32_t aid = 0;
  if (!parseRequestId(http_.arg("aid"), aid)) return respond(400, "Invalid light AID");
  const int index = devices_.indexByAid(aid);
  if (index < 0) return respond(404, "Light not registered");
  if (!devices_.removeByAid(aid))
    return respond(500, "Could not persist removal");
  clients_[index].setHost("");
  const unsigned removed = presets_.removeForDevice(aid);
  JsonDocument response;
  response["ok"] = true;
  response["requiresReboot"] = true;
  response["removedPresetSwitches"] = removed;
  response["message"] = "Light removed from registry. Restart bridge to update Apple Home.";
  sendJSON(response);
}

void PortalServer::handleAddPreset() {
  if (!authorize(true)) return;
  uint32_t aid = 0, id = 0;
  if (!parseRequestId(http_.arg("aid"), aid)
      || !parseRequestId(http_.arg("preset"), id)
      || !bridge::validPresetId(id))
    return respond(400, "Valid light AID and saved preset (1–250) required");
  const int index = devices_.indexByAid(aid);
  if (index < 0) return respond(404, "Light not registered");
  String name;
  if (!clients_[index].lookupPresetName(static_cast<uint8_t>(id), name))
    return respond(502, "Saved WLED preset not found or controller offline");

  String title = devices_.at(index).name + String(" ") + name;
  if (title.length() > 35) title = title.substring(0, 35);
  uint32_t switchAid = 0;
  const PresetEnrollResult result = presets_.add(devices_, aid, static_cast<uint8_t>(id),
                                                  title, &switchAid);
  if (result == PresetEnrollResult::Full) return respond(409, "12 selected preset switch limit reached");
  if (result == PresetEnrollResult::Invalid) return respond(400, "Invalid preset/device pairing");
  if (result == PresetEnrollResult::PersistenceFailed) return respond(500, "Could not persist switch");
  JsonDocument response;
  response["ok"] = true;
  response["aid"] = switchAid;
  response["requiresReboot"] = result == PresetEnrollResult::Added;
  response["message"] = result == PresetEnrollResult::Added
      ? "Preset switch saved. Restart bridge to add it to Apple Home."
      : "Preset switch already exists; no duplicate added.";
  sendJSON(response);
}

void PortalServer::handleRemovePreset() {
  if (!authorize(true)) return;
  uint32_t aid = 0;
  if (!parseRequestId(http_.arg("aid"), aid))
    return respond(400, "Invalid preset switch AID");
  if (!presets_.remove(aid)) return respond(404, "Preset switch not found");
  JsonDocument response;
  response["ok"] = true;
  response["requiresReboot"] = true;
  response["message"] = "Preset switch removed. Restart bridge to update Apple Home.";
  sendJSON(response);
}

void PortalServer::handleRestart() {
  if (!authorize(true)) return;
  if (http_.arg("confirm") != "REBOOT")
    return respond(400, "Explicit restart confirmation required");
  JsonDocument response;
  response["ok"] = true;
  response["message"] = "Restart scheduled.";
  sendJSON(response);
  restartAtMs_ = millis() + 1200; // Let the HTTP response leave first.
}

void PortalServer::routes() {
  http_.collectHeaders(csrfHeaderNames, 1);
  http_.on("/", HTTP_GET, [this]() { handleIndex(); });
  http_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  http_.on("/api/catalog", HTTP_GET, [this]() { handleCatalog(); });
  http_.on("/api/scan", HTTP_POST, [this]() { handleScan(); });
  http_.on("/api/enroll", HTTP_POST, [this]() { handleEnroll(); });
  http_.on("/api/control", HTTP_POST, [this]() { handleControl(); });
  http_.on("/api/devices/remove", HTTP_POST, [this]() { handleRemoveDevice(); });
  http_.on("/api/presets/add", HTTP_POST, [this]() { handleAddPreset(); });
  http_.on("/api/presets/remove", HTTP_POST, [this]() { handleRemovePreset(); });
  http_.on("/api/restart", HTTP_POST, [this]() { handleRestart(); });
  http_.onNotFound([this]() {
    if (authorize()) respond(404, "Endpoint not found");
  });
}

void PortalServer::poll() {
  if (password_.isEmpty()) return; // Opt-in; closed until serial provisioning.
  if (!listening_) {
    if (WiFi.status() != WL_CONNECTED) return;
    routes();
    http_.begin();
    listening_ = true;
    Serial.printf("[Portal] Authenticated web manager on http://%s:%u/\n",
                  WiFi.localIP().toString().c_str(), bridge::kPortalPort);
  }

  http_.handleClient();
  if (restartAtMs_ && static_cast<int32_t>(millis() - restartAtMs_) >= 0) {
    Serial.println("[Portal] Restart requested by authenticated administrator");
    Serial.flush();
    ESP.restart();
  }
}
