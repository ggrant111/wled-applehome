# Phase 4A — Web management and OTA hardware acceptance plan

> **Not yet tested on physical hardware.** Phase 1 reboot/on-off, Phase 2 8-light multi-device behavior and Phase 3 preset-switch behavior remain pending as well. Never claim production-ready solely from GitHub Actions.

## 1. Safe migration and pairing preservation

- [ ] Record existing Apple Home WLED bridge, accessory names and original AID=2.
- [ ] Flash v0.4 without erase/NVS reset (existing 4MB dual-OTA partition layout).
- [ ] Confirm original HomeKit bridge is still paired and all WLED lights/switches still appear.
- [ ] Confirm `@l` and `@s` list identical AIDs and IPs as before flashing.
- [ ] Confirm HomeKit control and WLED-originated state sync still work.

## 2. Administrator security / portal availability

- [ ] Without an administrator password, TCP 8080 is not listening and HomeSpan OTA is not enabled.
- [ ] Serial `@k` shows configured/disabled status without printing any secret.
- [ ] Reject weak (under 14 chars) passwords or whitespace; password is never logged.
- [ ] Set a unique 14–64 character password via `@k <secret>`, reboot `R`.
- [ ] Dashboard loads on `http://<ESP32-LAN-IP>:8080/`, asks for `admin` credentials.
- [ ] Wrong password denies access to status, device list and mutations.
- [ ] Missing/invalid CSRF token blocks POST additions/removals/controls/restart.
- [ ] `@k -` then reboot disables portal and OTA without affecting HomeKit pairing or WLED registry.
- [ ] From a remote/external network, neither portal nor OTA is publicly reachable (no router forwards).

## 3. Web operations

- [ ] Status dashboard shows correct IP, uptime, heap and firmware version.
- [ ] Each light shows correct AID, WLED name, online status and observed brightness.
- [ ] Browser On/Off/brightness controls only the selected WLED light and syncs HomeKit.
- [ ] mDNS discovery returns real WLED devices; non-WLED endpoints are rejected.
- [ ] Add light by scan or IP and reboot; HomeKit adds it without re-pairing.
- [ ] Remove a light and its preset switches; reboot and verify correct HomeKit topology.
- [ ] Load a large `/presets.json` from a WLED device and select a saved preset.
- [ ] Chosen preset appears as an independently triggerable momentary HomeKit switch; removing it is safe.
- [ ] Reboot button requires confirmation, preserves state and resumes operations.

## 4. OTA & rollback (important)

- [ ] Obtain correct board `firmware.bin` from successful CI run and confirm size guard passed.
- [ ] Confirm authenticated HomeSpan OTA uses the configured secret, **not** the public default.
- [ ] Try a normal OTA update; retain all HomeKit pairing and NVS configuration.
- [ ] Confirm log shows sketch marked valid after ~60s with Wi-Fi.
- [ ] Test an intentionally faulty OTA candidate in a safe development environment; verify rollback to last known-good image, not a boot loop.
- [ ] Test Wi-Fi absent during initial OTA stability window; document fallback behavior.
- [ ] Verify administrator password change and portal disablement don't reset lights.

## 5. Soak / resource checks

- [ ] Test 1, 3 and 8 WLED devices with refresh every 10 seconds for an hour.
- [ ] Observe free/minimum heap, flash headroom and HomeKit latency while browser is connected.
- [ ] WLED device disconnect/reconnect doesn't crash HomeSpan or block other lights.
- [ ] Run for 24h, power cycle, and verify stable state.
- [ ] Review whether synchronous `WebServer` + WLED HTTP is sufficiently responsive; otherwise prioritize nonblocking I/O.

## Not in Phase 4A

- Browser-based firmware binary upload, TLS/HTTPS, encrypted NVS storage, automatic cloud connectivity.
- Configuration import/export, automated remote firmware downloading, custom event rules.
- These remain candidates for further Phase 4 iterations once physical tests and security review pass.
