# Phase 9 — Multi-WLED effects groups hardware acceptance

**CI passed:** [run 42454610880](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880) — all native/JS tests, D1 Mini/CYD builds, OTA slot size safety. Images: D1 Mini 1,574,848 bytes (456,768 free); CYD 1,574,832 (456,784 free). **CI/build evidence is not physical verification.** Preserve HomeKit pairing; never erase flash/NVS or change partition table.

## Upgrade / migration
- [ ] Before update, record the existing v0.8 Effects group AID, saved preset names/order and working WLED light. Keep a copy of the previous OTA firmware.
- [ ] OTA update to v0.9.0-dev on the same paired ESP32, without NVS erase or re-pairing. Verify the old group's AID, lights, grouped controls and Siri commands remain unchanged.
- [ ] Portal on port 8080 → WLED effects groups: see the original group and correct associated WLED light.
- [ ] On a SECOND enrolled WLED device, load its saved presets into the effects-specific dropdown and add 2–3 favorites. Restart bridge. Confirm a SECOND, independently named Effects accessory appears in Apple Home.

## Independent state and commands
- [ ] In Apple Home choose a preset in Group A; verify only WLED A responds. Select in Group B; verify only WLED B responds and both group indicators remain correct.
- [ ] Change WLED B's active saved preset directly in WLED; after ~8s the correct Group B button becomes active while Group A remains unchanged.
- [ ] Turn active preset Off in Group A; only WLED A turns off. Siri color and brightness controls for BOTH normal LightBulb accessories still work.
- [ ] Disconnect WLED B from Wi-Fi and command WLED A; A still responds promptly. Reconnect B and verify automatic active-preset reconciliation.
- [ ] Restart ESP32; pairing, old group ID, new group ID, all light AIDs and mappings remain identical.

## Editing and protection
- [ ] Add a third selected preset to Group B: group AID remains unchanged, existing buttons retain labels/order, newly appended button appears after reboot.
- [ ] Rename one Group B preset: group AID unchanged; restart bridge; verify label is updated in Apple Home.
- [ ] Remove one selected preset from Group B. UI must explicitly warn that group AID is replaced and scenes/automations may need reconfiguration. Confirm, restart, verify original WLED lights and Group A unchanged.
- [ ] Reorder another selected preset with the same explicit warning and AID change. Verify actual Home app control order; if Home app caches the former group, reopen the Home app.
- [ ] Remove Group B entirely from portal, restart; only Group B disappears. Original light and original Effects group remain.
- [ ] Confirm a light removed from the bridge also loses only its linked effects group and standalone preset switches.

## Not automatic / release gates
- [ ] Eight-group and five-preset software capacity tests pass; physical tests with all eight WLED controllers remain a separate high-load acceptance gate.
- [ ] At least two physical WLEDs and two Effects groups pass all isolation tests before calling v0.9 hardware-verified.
- [ ] OTA upload, NVS preservation and bridge operation continue through a later reboot; watch free heap, queue depth and HTTP failures.
- [ ] Never alter WLED presets or segment geometry through the bridge registry/UI.
