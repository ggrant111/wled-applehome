#include "WledDiscovery.h"

#include <ESPmDNS.h>
#include <WiFi.h>

bool WledDiscovery::exists(const WledInfo &info) const {
  for (unsigned i = 0; i < count_; ++i) {
    if ((!info.mac.isEmpty() && candidates_[i].mac == info.mac) ||
        candidates_[i].ip == info.ip)
      return true;
  }
  return false;
}

void WledDiscovery::scan() {
  count_ = 0;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Discovery] Wi-Fi offline; cannot scan");
    return;
  }

  Serial.println("[Discovery] Scanning _wled._tcp via mDNS...");
  // HomeSpan owns the mDNS responder. Do not call MDNS.begin()/MDNS.end(),
  // which would replace its advertised HomeKit service.
  const int total = MDNS.queryService("wled", "tcp");
  Serial.printf("[Discovery] mDNS records: %d\n", total);
  for (int i = 0; i < total && count_ < kMaxCandidates; ++i) {
    if (MDNS.port(i) != 80) {
      Serial.printf("[Discovery] Skipping nonstandard port %u\n", MDNS.port(i));
      continue; // Phase 2a supports default WLED port 80
    }
    const IPAddress ip = MDNS.address(i);
    if (ip == IPAddress(0, 0, 0, 0)) continue;
    WledInfo info;
    if (!WledClient::probe(ip.toString(), info)) continue;
    if (exists(info)) continue;
    // mDNS TXT record may advertise the MAC if JSON info lacks it.
    if (info.mac.isEmpty() && MDNS.hasTxt(i, "mac"))
      info.mac = WledClient::normalizeMac(MDNS.txt(i, "mac"));
    if (info.mac.length() != 12) {
      Serial.printf("[Discovery] Ignoring %s: stable MAC unavailable\n", info.ip.c_str());
      continue;
    }
    candidates_[count_++] = info;
  }
  list();
}

void WledDiscovery::list() const {
  Serial.printf("[Discovery] %u verified WLED device(s) found\n", count_);
  for (unsigned i = 0; i < count_; ++i) {
    const auto &d = candidates_[i];
    Serial.printf("  %u) %s  %s  MAC=%s\n",
                  i + 1, d.name.c_str(), d.ip.c_str(), d.mac.c_str());
  }
  if (count_)
    Serial.println("[Discovery] Type @a1 (or another list number) to add.");
  else
    Serial.println("[Discovery] If a device is missing, add manually via @m192.168.1.50.");
}
