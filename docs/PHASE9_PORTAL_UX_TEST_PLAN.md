# v0.9.2 — Local bridge dashboard UX hardware checklist

**CI:** [run 42506904065](https://github.com/ggrant111/wled-applehome/actions/runs/42506904065) passed all host/browser/CSRF/gzip checks and both ESP32 firmware builds; OTA free space 447,072 bytes per image. This does **not** verify actual iPhone/desktop UI behavior.

## Upgrade safely
- [ ] Record current paired HomeKit light AIDs, existing Effects group names/preset assignments, and last-good v0.9.1 OTA firmware.
- [ ] OTA-install the correct **v0.9.2** D1 Mini or CYD firmware, retaining NVS, existing pairing and partition table.
- [ ] Open the local password-protected portal at `http://<ESP32-IP>:8080/`. Confirm all five navigation sections appear and the current firmware version/connection status updates.

## Navigation & responsive UX
- [ ] Open portal on desktop and iPhone. Check narrow screen navigation and all five sections: Overview, WLED Lights, Effects & Playlists, Preset Switches, Bridge Settings.
- [ ] Confirm Overview shows both WLED lights, current saved preset/playlist (if active), group counts, online status and quick actions.
- [ ] Switch between the two WLED lights on the Effects page. The correct grouped HomeKit favorites remain attached to their light; active preset/playlist highlights correctly.
- [ ] Select **Load saved presets & playlists** then add one favorite: dashboard signals a restart is required; after restart the chosen control appears in Apple Home without re-pairing.
- [ ] In Effects, choose a saved preset or playlist's **Run** button: only its associated WLED controller responds. It should NOT require a HomeKit restart; WLED source presets remain unchanged.
- [ ] Test Rename (name dialog), Move and Remove. Move/Remove must warn before reallocating the affected group's HomeKit accessory ID, which may require updating Home scenes. Do not actually perform a destructive edit during a normal smoke test.
- [ ] Use Lights for On/Off and valid brightness 1–100, and open Discover/scan or manual IP form. Review real device state and network failure counts.
- [ ] Visit Preset Switches, confirm previously enrolled momentary switches, use the existing saved segment inspector.
- [ ] Confirm Settings shows version, bridge IP/RSSI/uptime/heap and OTA instructions, and Restart only runs with confirmation.
- [ ] While editing brightness, wait for an automatic refresh: focus and typed numeric value should not be erased. Verify cached page recovers CSRF authorization after reboot.
- [ ] Validate controls keyboard-accessible, user-defined WLED names HTML-escaped, no missing text/overflow on narrow viewport and no browser JS errors.
- [ ] Repeat two-WLED, two-Effects-group switching in Apple Home and Siri. Ensure paired AIDs and actual WLED preset/playlist contents are unchanged.

## CI validation (separate from physical)
- [ ] Browser VM/in-page script syntax/nav and security contract tests; gzip portal size test.
- [ ] Firmware compilation for both targets plus OTA headroom >=70KB.
- [ ] User hardware/UI sign-off on at least phone and desktop. **Do not infer success from CI alone.**
