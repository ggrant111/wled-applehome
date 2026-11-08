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

## 2026-11-07 — User hardware verification: web portal and Apple Home enrollment

- **User-confirmed:** v0.4 web portal is working on their ESP32, WLED controllers were discovered from the portal and new lights were successfully added to Apple Home.
- This is real hardware validation of the portal discovery/enrollment path, not proof that all eight lights, independent power/brightness, WLED preset switches, remote OTA, reboot recovery, credential controls or 24-hour uptime have passed.
- Phase 4B priority order: preset switch end-to-end test; safe OTA and pairing/NVS retention; nonblocking WLED traffic and offline recovery; flash-size optimization; extended stability tests.
- Existing v0.4 build uses about 96.2% OTA app slot; substantial code/UI expansion on this 4 MB board should be avoided until footprint is reduced or hardware capacity increased.

## 2026-11-07 — v0.4.1 preset-add CSRF 403 correction

- User reported HTTP 403 `{"ok":false,"error":"Missing or invalid CSRF token"}` when adding a WLED preset through the working portal.
- **Probable cause from code:** Per-boot random token is replaced when the bridge restarts after newly added lights. The browser tab keeps the prior token until reloaded; other read-only GETs remain functional. Confirm on hardware after update.
- Immediate workaround: hard-refresh the portal tab; this does not delete any paired HomeKit devices or WLED configuration.
- Code fix: return token from Digest-authenticated same-origin status GET, refresh JavaScript token on status updates, and on CSRF 403 fetch a new token + retry the mutation only once if the token changed. Send same token in request header and form field; server validates either with exact-match policy.
- Added tests for C++ CSRF validation and for the **actual** portal JavaScript request code simulating reboot, retry bounds, invalid token and real application errors.
- **CI VERIFIED:** [run 42442460633](https://github.com/ggrant111/wled-applehome/actions/runs/42442460633): all host tests passed (including actual-browser-helper reboot simulation), firmware builds passed on D1 Mini and CYD, and 70KB minimum OTA space guard passed. Both full binaries are 1,956,928 bytes, with 74,688 bytes free in each OTA app slot. **Hardware test pending.**
- **Merged to main:** [PR #3](https://github.com/ggrant111/wled-applehome/pull/3). No user HomeKit pairing/WLED NVS data changed.

## 2026-11-07 — v0.5 background HTTP worker candidate

- Implemented a single FreeRTOS network worker on ESP32 core 0. Routine WLED GETs and HomeKit/web/preset POSTs run off the main HomeSpan/Arduino loop, avoiding up-to-650ms synchronous WLED delays there.
- Fixed-length 24-command queue sends immediately from HAP/web callbacks, protects WLED identity with per-slot host-version checks, discards stale GETs when a newer command arrives, and reconciles state using a 450ms post-write read. Worker polls every ~8 seconds and backs off ~30 seconds on failures.
- Per-light health metrics now include pending commands, HTTP failures and dropped commands. A queued write is **not** a confirmed WLED execution; expect eventual state reconciliation and inspect diagnostics for errors.
- Added build-time gzip compression of the HTML portal: `web/portal.html` is the editable source, `tools/prepare_portal.py` generates `include/PortalGzip.generated.h`. Remove HTML String allocation at page request time and preserve reboot-safe CSRF recovery using authenticated status header.
- No HomeKit/NVS registry/administrator secret/partition changes.
- **CI PASSED:** [run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) — all host tests (including browser security and worker freshness), both D1 Mini and CYD firmware builds, and binary headroom gate passed. HTML compresses **13,425 → 5,001 bytes**. D1 Mini firmware.bin **1,957,408 bytes (74,208 free)**; CYD **1,957,424 (74,192 free)**. **Hardware remains unverified;** consult [Phase 5 test plan](PHASE5_TEST_PLAN.md).
- **Merged into main:** [PR #4](https://github.com/ggrant111/wled-applehome/pull/4). All previous NVS/HomeKit identities and admin configuration should persist, but no physical v0.5 upgrade has yet been tested.

## 2026-11-07 — v0.6 Apple Home Hue/Saturation code candidate

- Implemented HomeSpan Hue and Saturation on existing WLED bridged LightBulb services while keeping original AIDs and On/Brightness characteristic order.
- HomeKit hue/saturation converted to WLED RGB via portable pure C++ code, with explicit RGBW white=0 and targeted WLED main segment Solid effect only when user changes color; includes an explicit main segment ID to protect other selected segments. No segment arrangement, other segments, NVS, or preset switch definitions changed.
- WLED main-segment primary RGB read back during routine background polls and converted to HomeKit color characteristics; no attempt to show rapidly changing animation colors.
- Reused v0.5 nonblocking queue, combined HomeKit color/power/brightness updates and existing stale-response safeguards.
- Added native conversion/round-trip tests. **CI passed:** [run 42445302149](https://github.com/ggrant111/wled-applehome/actions/runs/42445302149) — HSV↔RGB tests, existing host/browser/security checks, D1 Mini/CYD firmware and OTA image-size check. D1 Mini image 1,545,312 / 2,031,616 bytes (486,304 free); CYD image 1,545,296 bytes (486,320 free). **On-device Apple Home color control, pairing persistence and OTA still require testing.** [Phase 6 test plan](PHASE6_TEST_PLAN.md).
- **Merged to main:** [PR #5](https://github.com/ggrant111/wled-applehome/pull/5). Keep the prior v0.5 binary available for rollback until the -Oz-optimized image is tested on hardware.

## Next actions

1. **Done:** v0.4 host tests, migration regression, D1 Mini/CYD builds and OTA size guard passed.
2. **Done (user verified):** v0.4 web portal works and discovers/enrolls WLED controllers in Apple Home.
3. **Next user test:** Verify v0.4.1 preset-add success and trigger before upgrading to v0.5; then run the Phase 5 background worker hardware test plan without clearing NVS.
4. Test independent on/off/brightness on multiple devices and restart/power-cycle retention of all existing pairing/IDs.
5. Validate OTA safely, then prioritize nonblocking WLED networking, monitoring, flash-budget optimization and 24h soak.
