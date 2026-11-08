# Development progress / verification log

## 2026-11-07 — Project start

- Created tracked `docs/ROADMAP.md` (phases v0.1 → v2.0, gates, checklists).
- Chosen ESP32 D1 Mini first; ESP32-2432S028R CYD headless build second.
- Designed Phase 1 HomeSpan bridge with one light, WLED JSON GET/POST, NVS WLED IPv4 setting, and serial setup command `@w`.
- Added separate PlatformIO targets, two 1.94MiB app partitions, host-side brightness checks, and GitHub Actions builds.
- **CI verification (passed):** [GitHub Actions run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) — D1 Mini firmware, CYD firmware, and host tests all passed. **Hardware remains untested.** No claim of working Apple Home pairing until verified with an iPhone and WLED.

## 2026-11-07 — User physical validation and Phase 2 start

- **User-confirmed:** Initial firmware flashed, paired in Apple Home, and Apple Home brightness control works against a real WLED controller.
- **Not yet confirmed:** On/off actuation, two-way synchronization, reboot/pairing preservation. Leave P1.8/P1.9 open until tested.
- Beginning v0.2 increment: mDNS WLED discovery, manual approval, multi-device NVS registry with stable accessory IDs and backward-compatible AID=2.

## 2026-11-07 — Phase 2 candidate implementation

- Implemented opt-in `_wled._tcp` mDNS scan; probe discovered devices at `/json/info` and verify WLED version, LED data, and MAC.
- Serial CLI: `@l` list, `@d` discover, `@aN` add discovered device, `@m<IPv4>` verified manual enrollment, `@r<AID>` remove, `@w<IPv4>` original light IP.
- NVS registry persists up to 8 devices, allocates monotonic AIDs and preserves original AID=2 (same service/characteristics and name).
- Enrollment is explicit; adding/removing devices requires reboot `R` to refresh HomeKit accessories.
- Staggered WLED state polling, bounded HTTP timeouts and a 30-second retry backoff for offline devices.
- **CI PASSED:** [Phase 2 run 42430738405](https://github.com/ggrant111/wled-applehome/actions/runs/42430738405) — D1 Mini, CYD and host-side tests successful.
- **Flash budget:** D1 Mini firmware 1,733,463 / 2,031,616 bytes (85.3% app slot); CYD 1,733,479 / 2,031,616. Keep expansion size-constrained.
- **Not yet verified on hardware:** mDNS results, multiple bridged Home lights, Home app refresh following add/remove, reboot pairing preservation. See `docs/PHASE2_TEST_PLAN.md`.

## 2026-11-07 — Phase 3 work started while user away from hardware

- The user requested progression without physical testing until returning home. Phase 2's discovery, multi-light and pairing regression gates stay open.
- Implemented filtered `/presets.json` catalog reader with on-demand `@c<light-AID>`.
- Implemented explicit, persistent selection of up to 12 HomeKit momentary preset switches using `@p<light-AID>:<preset-ID>`, `@s` to list, and `@q<switch-AID>` to remove. Each switch sends only `{"ps":id}`; no WLED preset modification.
- Introduced shared NVS-backed AID allocation for WLED lights and preset switches, maintaining original AID=2 and preventing AID collisions/reuse.
- Enhanced local POST acknowledgement with a ~450ms settle window and prompt WLED power/brightness reconcile (periodic external-change polling remains).
- **Build VERIFIED:** [Phase 3 GitHub Actions run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722) passed all three jobs (host tests, D1 Mini build, CYD build). D1 Mini flash 1,758,847/2,031,616 (86.6%); CYD 1,758,863/2,031,616 (86.6%).
- **Merged to main** via [PR #1](https://github.com/ggrant111/wled-applehome/pull/1). **All v0.3 physical behavior remains unverified** until user returns to hardware.

## 2026-11-07 — Phase 4A web manager, OTA + safety implementation

- User advanced to Phase 4 while away from hardware; **do not interpret the new work as physical verification of v0.2/v0.3**.
- Implemented mobile-friendly local management web app on port 8080, with live cached WLED health, device actions, mDNS discovery, manual add/remove, selected preset catalog/enrollment, confirmation-based reboot, and diagnostics.
- Kept the portal inactive until a strong 14–64 character administrator password is set at the physical serial console via `@k <password>` and rebooted. Digest login and a boot-local CSRF token protect POST mutations. No default login/OTA secret.
- Enabled HomeSpan's **native authenticated OTA** only after the admin password is provisioned; same secret is used for OTA, and `SpanRollback.h` defers validating pending OTA firmware until stable operation (~60s with Wi-Fi).
- Added host-side admin password/ID validation tests and CI guard that fails a firmware image with <70KB spare room in the 2,031,616-byte OTA app slot.
- **Verification — CI PASSED:** [GitHub Actions run 42439628403](https://github.com/ggrant111/wled-applehome/actions/runs/42439628403): all host checks including migration regression, D1 Mini and CYD firmware builds, and binary size guard passed. D1 Mini full binary 1,955,232 bytes (76,384 free in OTA slot); CYD 1,955,248 (76,368 free). **No physical web/OTA/Apple Home regression tests have been done.**
- **Migration bug corrected and regression-tested:** ESP32 Preferences::putString returns 0 for successfully stored empty strings; Phase 2's original AID=2 migration saved a blank MAC and could falsely halt startup. Fixed by readback validation and added a mock-Preferences integration test for initial migration, AID stability and reboot.
- **Merged to main:** [Phase 4 PR #2](https://github.com/ggrant111/wled-applehome/pull/2). Phase 1 on/off/reboot, Phase 2 multi-device, Phase 3 preset behavior and Phase 4 portal/OTA acceptance tests remain pending.

## Next actions

1. **Done:** v0.4 host tests, migration regression, D1 Mini/CYD builds and OTA size guard passed.
2. Preserve existing NVS/HomeSpan pairing: upload on the **same paired ESP32** without erase when home.
3. Verify pending Phase 2 identity/discovery/multi-device acceptance tests before relying on preset switches.
4. Test a named preset: `@c2` -> `@p2:5` (use actual saved preset ID) -> `R` -> trigger in Apple Home -> verify switch resets and WLED responds.
5. Record observations in progress log, update phase checkboxes, and plan asynchronous networking / Phase 4 portal.
