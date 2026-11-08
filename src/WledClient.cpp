#include "WledClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ctype.h>
#include "BridgeMath.h"
#include "ColorMath.h"
#include "WledNetworkWorker.h"

namespace {
constexpr uint16_t kStateTimeoutMs = 650;
constexpr uint16_t kCatalogTimeoutMs = 4000;
bool ready() { return WiFi.status() == WL_CONNECTED; }
bool ok(int status) { return status >= 200 && status < 300; }
}

void WledClient::markMutation() {
  ++mutationRevision_;
  lastMutationMs_ = millis();
}
void WledClient::setHost(const String &host) {
  if (host_ != host) {
    host_ = host;
    markMutation();
    if (worker_) worker_->setHost(workerSlot_, host);
  }
}

void WledClient::attachWorker(WledNetworkWorker *worker, unsigned slot) {
  worker_ = worker;
  workerSlot_ = slot;
  if (worker_) worker_->setHost(slot, host_);
}
const String &WledClient::host() const { return host_; }
bool WledClient::configured() const { return host_.length() > 0; }
String WledClient::url() const { return String("http://") + host_ + "/json/state"; }

String WledClient::normalizeMac(const String &input) {
  String normalized;
  normalized.reserve(12);
  for (unsigned i = 0; i < input.length(); ++i) {
    const unsigned char c = static_cast<unsigned char>(input[i]);
    if (isxdigit(c))
      normalized += static_cast<char>(toupper(c));
  }
  return normalized.length() == 12 ? normalized : String("");
}

bool WledClient::probe(const String &ip, WledInfo &out) {
  if (!ready() || ip.isEmpty()) return false;
  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kStateTimeoutMs);
  http.setTimeout(kStateTimeoutMs);
  if (!http.begin(transport, String("http://") + ip + "/json/info"))
    return false;
  const int status = http.GET();
  if (!ok(status)) {
    Serial.printf("[Probe] %s HTTP %d\n", ip.c_str(), status);
    http.end();
    return false;
  }
  JsonDocument data;
  const DeserializationError error = deserializeJson(data, http.getStream());
  http.end();
  if (error || data["ver"].isNull() || data["leds"].isNull()) {
    Serial.printf("[Probe] %s is not a verified WLED controller\n", ip.c_str());
    return false;
  }
  WledInfo verified;
  verified.ip = ip;
  verified.mac = normalizeMac(String(data["mac"] | ""));
  verified.name = String(data["name"] | "WLED Light");
  verified.name.trim();
  if (verified.name.isEmpty()) verified.name = "WLED Light";
  if (verified.name.length() > 32) verified.name = verified.name.substring(0, 32);
  out = verified;
  return true;
}

bool WledClient::readState(WledState &out) {
  if (!configured() || !ready()) return false;
  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kStateTimeoutMs);
  http.setTimeout(kStateTimeoutMs);
  if (!http.begin(transport, url())) return false;
  const int status = http.GET();
  if (!ok(status)) {
    Serial.printf("[WLED] GET failed HTTP %d (%s)\n", status, host_.c_str());
    http.end();
    return false;
  }
  JsonDocument data;
  const DeserializationError error = deserializeJson(data, http.getStream());
  http.end();
  if (error || data["on"].isNull() || data["bri"].isNull()) {
    Serial.println("[WLED] Invalid JSON state response");
    return false;
  }
  out.on = data["on"].as<bool>();
  out.brightness = bridge::wledToHomeBrightness(data["bri"].as<int>());
  out.presetId = data["ps"] | -1;
  out.colorValid = false;
  // WLED's main segment represents the color shown in its own controls.
  // Do not invent a color if the segment has no RGB primary color.
  const int rawMainSegment = data["mainseg"] | 0;
  const int mainSegment = rawMainSegment >= 0 && rawMainSegment <= 255
      ? rawMainSegment : 0;
  out.mainSegmentId = static_cast<uint8_t>(mainSegment);
  JsonArray segments = data["seg"].as<JsonArray>();
  for (JsonObject segment : segments) {
    if (static_cast<int>(segment["id"] | -1) != mainSegment) continue;
    JsonArray primary = segment["col"][0].as<JsonArray>();
    if (primary.size() < 3 || !primary[0].is<int>() ||
        !primary[1].is<int>() || !primary[2].is<int>()) break;
    const int r = primary[0].as<int>(), g = primary[1].as<int>(),
              b = primary[2].as<int>();
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
      break;
    const bridge::HsvColor hsv = bridge::wledRgbToHomeColor(
        {static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
    out.hue = hsv.hue;
    out.saturation = hsv.saturation;
    out.colorValid = true;
    break;
  }
  return true;
}

bool WledClient::sendJson(const String &body) {
  if (!configured() || !ready()) return false;
  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kStateTimeoutMs);
  http.setTimeout(kStateTimeoutMs);
  if (!http.begin(transport, url())) return false;
  http.addHeader("Content-Type", "application/json");
  const int status = http.POST(body);
  http.end();
  if (!ok(status)) {
    Serial.printf("[WLED] POST failed HTTP %d (%s)\n", status, host_.c_str());
    return false;
  }
  markMutation();
  Serial.printf("[WLED] POST %s -> HTTP %d (%s)\n", body.c_str(),
                status, host_.c_str());
  return true;
}

bool WledClient::writeState(bool changePower, bool power,
                            bool changeBrightness, int brightnessPercent,
                            bool changeColor, float hue, float saturation, int segmentId) {
  if (!changePower && !changeBrightness && !changeColor) return true;
  if (worker_) {
    const bool queued = worker_->queueWrite(workerSlot_, changePower, power,
                                           changeBrightness, brightnessPercent,
                                           changeColor, hue, saturation);
    if (queued) markMutation();
    return queued;
  }
  JsonDocument data;
  if (changePower) data["on"] = power;
  if (changeBrightness)
    data["bri"] = bridge::homeToWledBrightness(brightnessPercent);
  if (changeColor) {
    const bridge::RgbColor rgb = bridge::homeColorToRgb(hue, saturation);
    // An omitted ID can affect selected segments other than the main one.
    // Target the main segment explicitly; read its ID first if unknown.
    if (segmentId < 0 || segmentId > 255) {
      WledState current;
      if (!readState(current)) return false;
      segmentId = current.mainSegmentId;
    }
    JsonObject segment = data["seg"].to<JsonObject>();
    segment["id"] = segmentId;
    segment["fx"] = 0;
    JsonArray colors = segment["col"].to<JsonArray>();
    JsonArray primary = colors.add<JsonArray>();
    primary.add(rgb.r);
    primary.add(rgb.g);
    primary.add(rgb.b);
    primary.add(0);  // Explicit RGBW white=0 to avoid a stale white channel.
  }
  String payload;
  serializeJson(data, payload);
  return sendJson(payload);
}

bool WledClient::applyPreset(uint8_t presetId) {
  if (!bridge::validPresetId(presetId)) return false;
  if (worker_) {
    const bool queued = worker_->queuePreset(workerSlot_, presetId);
    if (queued) markMutation();
    return queued;
  }
  return sendJson(String("{\"ps\":") + presetId + "}");
}

bool WledClient::loadPresetNames(JsonDocument &out, int onlyPresetId) {
  if (!configured() || !ready()) return false;
  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kStateTimeoutMs);
  http.setTimeout(kCatalogTimeoutMs);
  if (!http.begin(transport, String("http://") + host_ + "/presets.json"))
    return false;
  const int status = http.GET();
  if (!ok(status) || http.getSize() > 524288) {
    Serial.printf("[Presets] GET /presets.json failed, status=%d size=%d (%s)\n",
                  status, http.getSize(), host_.c_str());
    http.end();
    return false;
  }
  // Filtering retains ONLY preset names, not entire LED/segment arrays.
  JsonDocument filter;
  if (onlyPresetId > 0) {
    filter[String(onlyPresetId)]["n"] = true;
  } else {
    for (unsigned id = 1; id <= bridge::kMaxWledPresetId; ++id)
      filter[String(id)]["n"] = true;
  }
  const DeserializationError error = deserializeJson(
      out, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (error || !out.is<JsonObject>()) {
    Serial.printf("[Presets] JSON parsing failed (%s): %s\n",
                  host_.c_str(), error.c_str());
    return false;
  }
  return true;
}

bool WledClient::inspectPresetSegments(uint8_t presetId, WledPresetSegments &out) {
  out = {};
  if (!bridge::validPresetId(presetId) || !configured() || !ready()) return false;
  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kStateTimeoutMs);
  http.setTimeout(kCatalogTimeoutMs);
  if (!http.begin(transport, String("http://") + host_ + "/presets.json"))
    return false;
  const int status = http.GET();
  if (!ok(status) || http.getSize() > 524288) {
    Serial.printf("[Presets] Inspection GET failed HTTP %d (%s)\n",
                  status, host_.c_str());
    http.end();
    return false;
  }
  // The WLED preset file can be very large. ArduinoJson filters to only
  // the selected preset and only segment BOUNDS, never copying palettes,
  // effects, segment color arrays or other presets into ESP32 heap.
  JsonDocument filter;
  JsonObject itemFilter = filter[String(presetId)].to<JsonObject>();
  itemFilter["n"] = true;
  itemFilter["win"] = true;
  itemFilter["o"] = true;
  itemFilter["playlist"] = true;
  JsonArray segmentFilters = itemFilter["seg"].to<JsonArray>();
  JsonObject segmentFilter = segmentFilters.add<JsonObject>();
  segmentFilter["id"] = true;
  segmentFilter["start"] = true;
  segmentFilter["stop"] = true;
  JsonDocument doc;
  const DeserializationError error = deserializeJson(
      doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (error) {
    Serial.printf("[Presets] Inspection JSON failed (%s): %s\n",
                  host_.c_str(), error.c_str());
    return false;
  }
  JsonVariant item = doc[String(presetId)];
  if (item.isNull()) return false;
  out.apiMacro = !item["win"].isNull() || !item["o"].isNull()
      || !item["playlist"].isNull();
  JsonArray segments = item["seg"].as<JsonArray>();
  out.hasSegmentArray = !segments.isNull();
  for (JsonObject segment : segments) {
    ++out.savedSegments;
    const bool hasStart = !segment["start"].isNull();
    const bool hasStop = !segment["stop"].isNull();
    // WLED may save a stop:0 tombstone to remove a missing segment.
    if (hasStop && (hasStart || segment["stop"].as<int>() == 0))
      ++out.segmentsWithBounds;
  }
  Serial.printf("[Presets] inspected %u: segment items=%u bounds=%u macro=%u\n",
                unsigned(presetId), out.savedSegments,
                out.segmentsWithBounds, out.apiMacro);
  return true;
}

bool WledClient::printPresetCatalog() {
  JsonDocument doc;
  if (!loadPresetNames(doc, 0)) return false;
  unsigned found = 0;
  Serial.printf("[Presets] Saved presets for %s:\n", host_.c_str());
  for (unsigned id = 1; id <= bridge::kMaxWledPresetId; ++id) {
    JsonVariant item = doc[String(id)];
    if (item.isNull()) continue;
    ++found;
    if (found <= 80) {
      const char *name = item["n"] | "(unnamed preset)";
      Serial.printf("  %u: %s\n", id, name);
    }
  }
  if (found > 80) Serial.printf("  ... %u more hidden; enroll by ID\n", found - 80);
  Serial.printf("[Presets] %u entries. Use @p<lightAID>:<presetId> to expose one.\n", found);
  return true;
}

bool WledClient::lookupPresetName(uint8_t presetId, String &name) {
  if (!bridge::validPresetId(presetId)) return false;
  JsonDocument doc;
  if (!loadPresetNames(doc, presetId)) return false;
  JsonVariant item = doc[String(presetId)];
  if (item.isNull()) {
    Serial.printf("[Presets] %u is not a saved preset on %s\n",
                  presetId, host_.c_str());
    return false;
  }
  name = String(item["n"] | "Preset");
  name.trim();
  if (name.isEmpty()) name = "Preset";
  if (name.length() > 34) name = name.substring(0, 34);
  return true;
}
