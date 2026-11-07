#include "WledClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "BridgeMath.h"

namespace {
constexpr uint16_t kRequestTimeoutMs = 900;
bool ready() { return WiFi.status() == WL_CONNECTED; }
bool ok(int status) { return status >= 200 && status < 300; }
}

void WledClient::setHost(const String &host) { host_ = host; }
const String &WledClient::host() const { return host_; }
bool WledClient::configured() const { return host_.length() > 0; }
String WledClient::url() const { return String("http://") + host_ + "/json/state"; }

bool WledClient::readState(WledState &out) {
  if (!configured() || !ready()) return false;

  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kRequestTimeoutMs);
  http.setTimeout(kRequestTimeoutMs);
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

  // Only commit state once a complete valid response is available.
  out.on = data["on"].as<bool>();
  out.brightness = bridge::wledToHomeBrightness(data["bri"].as<int>());
  return true;
}

bool WledClient::writeState(bool changePower, bool power,
                            bool changeBrightness, int brightnessPercent) {
  if (!configured() || !ready()) return false;
  if (!changePower && !changeBrightness) return true;

  JsonDocument data;
  if (changePower) data["on"] = power;
  if (changeBrightness) {
    data["bri"] = bridge::homeToWledBrightness(brightnessPercent);
  }

  String payload;
  serializeJson(data, payload);

  WiFiClient transport;
  HTTPClient http;
  http.setConnectTimeout(kRequestTimeoutMs);
  http.setTimeout(kRequestTimeoutMs);
  if (!http.begin(transport, url())) return false;

  http.addHeader("Content-Type", "application/json");
  const int status = http.POST(payload);
  http.end();
  if (!ok(status)) {
    Serial.printf("[WLED] POST failed HTTP %d (%s)\n", status, host_.c_str());
    return false;
  }

  Serial.printf("[WLED] POST %s -> HTTP %d\n", payload.c_str(), status);
  return true;
}
