#pragma once
#include <Arduino.h>

struct WledState {
  bool on = false;
  int brightness = 50;  // Apple Home scale 0..100
};

// Local-only HTTP client; no WLED firmware changes required.
class WledClient {
public:
  void setHost(const String &host);
  const String &host() const;
  bool configured() const;
  bool readState(WledState &out);
  bool writeState(bool changePower, bool power, bool changeBrightness, int brightnessPercent);

private:
  String host_;
  String url() const;
};
