#pragma once
#include <Arduino.h>

struct WledState {
  bool on = false;
  int brightness = 50; // Apple Home scale 0..100
};

struct WledInfo {
  String ip;
  String mac;
  String name;
};

// Local-only HTTP client. POST only requested state keys.
class WledClient {
public:
  void setHost(const String &host);
  const String &host() const;
  bool configured() const;
  bool readState(WledState &out);
  bool writeState(bool changePower, bool power,
                  bool changeBrightness, int brightnessPercent);

  // GET /json/info identifies real WLED devices before enrollment.
  static bool probe(const String &ip, WledInfo &out);
  static String normalizeMac(const String &input);

private:
  String host_;
  String url() const;
};
