# WLED Apple Home Bridge — Roadmap

> **Source of truth:** Update this file whenever a milestone changes, and record decisions in `docs/DECISIONS.md`. Do not mark hardware or HomeKit pairing tests complete from a successful compile alone.
>
> **Updated:** 2026-11-07 · **Active milestone:** v0.5 background WLED networking and compressed portal (CI passed; hardware verification pending) · **Primary target:** ESP32 D1 Mini · **Secondary target:** ESP32-2432S028R CYD (headless mode first).

## Goal

An always-on, **local-only** ESP32 bridge that makes existing WLED devices appear as independent lights in Apple Home, without changing their WLED firmware. Power, brightness, and selected presets must be controllable from Apple Home/Siri while preserving WLED device settings and supporting other WLED clients.

## Definition of MVP (v1.0)

- [ ] Bridge pairs with Apple Home using a device-specific setup code.
- [ ] At least **8 WLED devices** can be configured and exposed as **separate, stable bridged light accessories**.
- [ ] Per-device on/off and 1–100% brightness work from Apple Home.
- [ ] Selected WLED presets are exposed through **optional trigger switches** or scene helpers (not every preset).
- [ ] State changes originating in WLED/other clients are reflected back in Home.
- [ ] Network dropouts, device reboot, and ESP32 restart recover automatically.
- [ ] WLED IP/hostname changes can be corrected without losing HomeKit pairings.
- [ ] Local configuration UI, meaningful logs, and repeatable setup instructions.
- [ ] OTA update path that preserves pairing, stable accessory IDs, and device config.
- [ ] Successful PlatformIO firmware build and on-device smoke tests for supported hardware.

## Phases

### Phase 1 — v0.1: HomeKit + one WLED proof of concept — **CODE COMPLETE / HARDWARE PENDING**

- [x] P1.1 PlatformIO firmware for ESP32 D1 Mini on Arduino-ESP32 3.3+ with HomeSpan 2.1.8.
- [x] P1.2 CYD ESP32-2432S028R board build, initially without display/touch drivers.
- [x] P1.3 One HomeKit *bridged* LightBulb, containing On and Brightness characteristics.
- [x] P1.4 WLED JSON API GET state and POST on/off/brightness, with status checks.
- [x] P1.5 Configure a WLED IPv4 address over serial CLI and persist it in ESP32 NVS.
- [x] P1.6 Poll WLED for external changes; notify HomeKit only on changes.
- [x] P1.7 GitHub Actions build both firmware configurations and run host-only checks.
- [ ] P1.8 **Physical verification:** iPhone pairs with bridge; on/off and brightness control a real WLED device.
- [ ] P1.9 **Physical verification:** power-cycle the ESP32 and verify the pairing remains.

**Acceptance gate:** P1.1–P1.9 passed, with actual device evidence. Compiling is not sufficient.

### Phase 2 — v0.2: Discovery and stable multi-device bridge — **PORTAL DISCOVERY / ADD-TO-HOME HARDWARE-VERIFIED; FULL TESTS PENDING**

- [x] **Code delivered:** On-demand mDNS scan of `_wled._tcp` and HTTP `/json/info` verification. **Web portal discovery user-verified (2026-11-07); serial CLI scan remains unverified.**
- [x] **Code delivered:** CLI manual add/remove and de-duplicate by normalized hardware MAC; rename remains planned.
- [ ] Device renaming (separate from WLED's own name) and polished web configuration.
- [x] **Code delivered:** NVS-backed monotonic HomeKit AIDs; original light retains AID=2, without reallocating removed AIDs.
- [x] **Code delivered:** Per-light WLED polling stagger, timeout cap, and 30s offline backoff.
- [ ] Nonblocking WLED networking, exposed device-health UI, and advanced reconnection behavior.
- [x] **Code delivered:** Eight WLED registry slots and bridged accessory creation.
- [ ] Hardware test with eight WLED devices and evaluate memory/latency.
- [ ] Hardware verify both add **and removal**, reboot, and HomeKit pairing continuity. **User confirmed adding new WLED devices via portal into Apple Home (2026-11-07); removal and identity persistence not yet reported.**
- [ ] Test reboot, IP changes, reordering, and missing devices.

### Phase 3 — v0.3: Presets and state sync — **CI PASSED / HARDWARE TESTS PENDING**

- [x] **Code implemented:** On-demand read of `/presets.json` by light AID; streaming filtered JSON retains only names (avoids allocating full WLED preset configurations). Hardware discovery test pending.
- [x] **Code implemented:** Explicitly enroll up to 12 preset triggers (`@p<lightAID>:<presetId>`) as HomeKit momentary Switch accessories with stable, never-reused AIDs. Reboot to advertise. Hardware behavior pending.
- [x] **Code implemented:** After successful HTTP POST, refresh WLED power/brightness after a 450ms settle delay; no state overwrite during that delay, and normal periodic external-change polling retained. Hardware timing/recovery pending.
- [ ] Hardware verify observation of WLED/GlowDeck/external state and selected preset changes on multiple lights; polling-based sync code is in place.
- [ ] WebSocket-assisted live updates / fully nonblocking transport (deferred; 8s periodic polling retained as fallback).

### Phase 4 — v0.4/1.0: Web setup, OTA, reliability and release — **WEB DISCOVERY / HOME ENROLLMENT VERIFIED; REMAINING GATES OPEN**

- [x] **Code delivered:** Password-gated local web manager on port 8080 (device list, status, 8-light discovery/add/remove, preset selection, controls, diagnostics and reboot). **Portal loads and discovers/enrolls WLED devices into Apple Home on user hardware (2026-11-07); the rest of its functions remain unverified.**
- [ ] Test real browser Digest login, CSRF protections, reconnection and Apple Home coexistence; optimize any WebServer-induced HomeKit latency.
- [x] **Code delivered:** HomeSpan built-in password-protected ArduinoOTA enabled only after serial-set administrator password (no default OTA password); OTA partition validation delayed 60s with `SpanRollback.h`. **OTA hardware recovery/pairing test pending.**
- [ ] Validate OTA with firmware image and ensure pairing, Wi-Fi and WLED registry survive; test rollback after invalid/new image.
- [x] **Code delivered:** Existing per-device 30s offline backoff; portal begins only once Wi-Fi becomes connected; read-only cached health endpoint, no additional WLED polling on dashboard refresh.
- [ ] Nonblocking WLED polling, improved network recovery and 24h soak / reboot/power loss tests.
- [x] **Code delivered:** Serial-only admin-secret provisioning, Digest login, CSRF tokens for POST mutations; portal and OTA both disabled until provisioned. No default credentials or embedded secrets.
- [ ] Hardware security review, no open router forwarding, authentication rate-limit/timeouts, threat-model/credentials lifecycle review. Note local HTTP is not TLS.
- [x] **Code delivered:** Release documentation, web/OTA setup guide, GitHub CI firmware artifacts and binary OTA-partition headroom check.
- [ ] Configuration export/import with secret redaction, release changelog and versioned release tagging.
- [ ] Final **8-device and 24-hour run verification**; phase must remain open until real hardware evidence.

### Phase 4B — next release priorities (acceptance gated)

1. **Preset controls / end-to-end state:** User selects a real saved WLED preset in the portal; it appears in Apple Home after bridge restart, activates the correct WLED preset, resets for another trigger, and power/brightness stay synchronized. Confirm independent power/brightness changes for at least two enrolled lights.
2. **Safe OTA:** Upload a compatible OTA image using HomeSpan's authenticated OTA service; demonstrate that HomeKit pairing, device identities (including AID=2), WLED IPs, preset switches and admin settings survive. Confirm the delayed sketch-valid marker and test rollback only on a safe spare/test device.
3. **Networking and stability:** Replace/limit synchronous WLED HTTP calls so an offline light cannot impair all Apple Home lights; test missing Wi-Fi, disconnected WLED, state-update conflicts, discovery latency and 24-hour continuous uptime.
4. **Capacity management:** Optimize firmware footprint and quantify RAM/flash at 1, 3, and 8 WLED devices; preserve a substantial safety margin for OTA. On classic ESP32 4MB the current 76KB spare is a blocker for major added features.
5. **Production polish:** Security review, diagnostic logs, backup/export with sensitive data redacted, versioned release, and documented recovery instructions.

**User-verified hardware evidence:** Web manager operational; discovered WLED controllers using portal and successfully added lights to Apple Home. Do not infer tests of every light's independent power/brightness, pairing across reboot, preset activation, removal, or OTA.

### Phase 5 / v0.5 — Background WLED networking & firmware footprint — **CI PASSED / HARDWARE VERIFICATION PENDING**

- [x] **Code:** Moved routine WLED polling and HomeKit/portal light + preset JSON POSTs into one bounded FreeRTOS task on ESP32 core 0. The HomeSpan main loop now queues commands without synchronously waiting on WLED HTTP.
- [x] **Code:** Fixed-length command queue (24 commands), enqueue rejection on saturation, per-light queue depth, HTTP failures, drop counts, online state and last-success timestamps surfaced in cached portal status.
- [x] **Code:** Queue commands ahead of polling; stagger periodic GETs by device, use 8-second healthy poll, 30-second offline backoff, and 450ms post-write reconciliation.
- [x] **Code:** In-flight GETs are invalidated when a newer command or host/IP update arrives; only the main loop updates HomeKit characteristics from worker snapshots.
- [x] **Code:** Portal page now precompressed to gzip at build time, served directly from flash with no 13KB HTML String allocation; authenticated status endpoint supplies the live CSRF token.
- [x] **Code:** Added native freshness/scheduler checks, compressed asset tests and existing browser CSRF regression to CI.
- [x] **CI PASSED:** [run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) — host C++/browser/asset tests, both firmware builds, 70KB OTA headroom guard.
- [ ] **Hardware:** Validate multiple connected WLED lights, offline device isolation, commands queued/failed, preset switch timing, HomeKit state reconciliation, Wi-Fi disconnect/reconnect, 24-hour soak and stability.
- [ ] On-demand discovery, WLED preset catalog and manual IP verification still use bounded **blocking** calls in the portal/serial action path; make these async later if real latency tests justify.
- [ ] Continue measuring heap and browser performance, and evaluate queuing/coalescing latest brightness changes if heavy automation fills the queue.

**Behavior change:** A successful HomeKit/portal write means **accepted by the bridge command queue**, *not* confirmed executed by WLED. Failed WLED POSTs appear as per-light errors/health diagnostics and are reconciled on the next successful poll. Queue saturation rejects a new command rather than blocking HomeKit.

**Preservation:** No NVS schema, HomeKit AID, password, preset registry, partition table or pairing change in v0.5. Upgrade without erase.

## After MVP

| Version | Focus | Status |
| --- | --- | --- |
| v1.1 | Polished web dashboard, log viewer, per-device diagnostics | Planned |
| v1.2 | Multi-WLED scenes, virtual groups, master brightness/on-off | Planned |
| v1.3 | CYD touchscreen dashboard with light tiles and controls | Planned |
| v1.4 | WLED segments, RGB, palettes, effect parameters | Planned |
| v1.5 | WebSocket-led event sync and reliability/performance improvements | Planned |
| v1.6 | Health monitoring, configuration backup and safe rollback | Planned |
| v2.0 | Research Matter bridge for Apple Home/Google Home/Alexa | Research |

### Phase 3 feature boundaries

- The bridge does **not** create or edit any WLED presets; `@c<AID>` reads names and `@p<AID>:<ID>` exposes a selected saved preset as a HomeKit switch. Only `{"ps":<ID>}` is sent when invoked.
- Up to **12 selected preset switches total**, independent of the 8 WLED-light limit; no preset tiles are added automatically.
- Existing light AIDs, HomeKit pairing, and WLED NVS registry are preserved; **new switches get fresh AIDs from the same allocator used for lights**.
- Instant WebSocket state sync is **still open** (a local portal now exists in Phase 4A). Large WLED catalog reads are on-demand and may temporarily block HomeKit while the request finishes.
- Portal-based WLED discovery and Apple Home enrollment are user-verified; other v0.2 and v0.3 tests remain open. Do not mark those phases released solely from CI.

## v0.4.1 — Portal 403 CSRF reboot recovery fix

- [x] Root cause identified: the portal generates a new random CSRF token on each ESP32 boot, while an already-open browser tab keeps the old one after users restart to publish newly added Apple Home accessories.
- [x] Dashboard now refreshes a boot token from authenticated same-origin `/api/status` responses and retries **one** rejected POST after a 403 only when a different valid token is returned.
- [x] Device accepts the exact current token through either the collected `X-Bridge-Token` request header (reliable modern browsers) or legacy form field (backward compatibility); no authentication or CSRF bypass.
- [x] C++ CSRF policy and real browser JS regression tests passed, including token refresh after restart, one safe retry, invalid tokens and unrelated errors. [CI run 42442460633](https://github.com/ggrant111/wled-applehome/actions/runs/42442460633).
- [ ] **Pending hardware:** Update via normal upload/OTA without erasing NVS; reboot with browser tab open; add preset; validate Apple Home preset switch appears and triggers WLED.

**Immediate user workaround on v0.4.0:** Reload the browser dashboard after an ESP32 reboot, then add the preset again. Do not reset Apple Home pairings.

## Phase 4 safety & deployment notes

- This is **v0.4.0-dev with Phase 4B next**, not a finished v1.0 release. Preserve outstanding Phase 1–4 hardware gates.
- **The portal is OFF by default**. On USB serial console, enter `@k <strong-unique-password>` (14–64 printable non-space ASCII characters) and reboot with `R`; then visit `http://<bridge-ip>:8080/`, sign in with username `admin`.
- The administrator password is stored in local ESP32 NVS for web Digest login and reused to activate HomeSpan's password-protected ArduinoOTA. It is **not hardcoded in firmware or written to git**, but plaintext NVS is not encryption-at-rest—physical flash access remains a security boundary.
- Disable portal + OTA via `@k -` followed by `R`; HomeKit/WLED configuration must remain unchanged.
- **No web firmware upload form in Phase 4A.** Use HomeSpan's OTA workflow with Arduino IDE / compatible uploader. No remote flashing at all until unique secret is configured. `SpanRollback.h` marks pending OTA firmware valid after ~60 seconds of running with Wi-Fi.
- Portal uses HTTP Digest authentication and CSRF tokens; **it does not use HTTPS**. Trusted LAN only; never expose port 8080 or OTA publicly.
- Network operations are still partially blocking. Portal is polled after HomeSpan's loop and starts only with Wi-Fi. Multi-device latency and 8-light capacity remain to test.
- Firmware CI now rejects builds if OTA app partition headroom falls below 70,000 bytes.

## Non-goals / guardrails

1. Do **not** flash or change firmware on any WLED controller.
2. Do **not** change WLED presets, segments, or running effects unless commanded.
3. Do **not** put SSIDs, passwords, or HomeKit codes in git.
4. Do **not** assign HomeKit accessory IDs from discovery order. Once assigned, preserve IDs.
5. Home app does **not** provide a native WLED preset dropdown; use optional trigger accessories.
6. **CYD v0.1 is headless**, even though it builds for the touchscreen hardware.
7. The D1 Mini must be **ESP32**, not ESP8266.
8. Keep HomeKit/UDP/mDNS traffic on the same permitted LAN; no cloud required.

## Phase 2 deployment notes

1. Use the **existing paired ESP32**. Flash without erasing flash/NVS (do not run `pio run -t erase`).
2. v0.2 migrates the existing `wled-home` IP to AID=2 before creating any new accessories.
3. On the serial CLI: `@l` list lights, `@d` discover, `@a1` enroll discovery result 1, `@m192.168.1.50` manually enroll an IP, and `@r3` remove AID=3.
4. New additions/removals take effect in Apple Home on the next `R` reboot. Discovery alone never enrolls devices.
5. Restore stable IP identity via matching WLED MAC. If mDNS fails, use manual verified IP enrollment.
6. v0.2 intentionally remains serial-managed; web UI is still a Phase 4 goal.

## Build/test tracking

| Check | Status | Evidence |
| --- | --- | --- |
| Host-side logic checks | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| D1 Mini PlatformIO build | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| CYD PlatformIO build | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| User hardware Apple Home pairing and brightness | **Verified by user (2026-11-07)** | First firmware installed, paired and changed brightness of real WLED |
| D1 Mini physical on/off and reboot persistence | Not yet reported | Requires physical test |
| CYD physical pairing / WLED actuation | Not tested | User hardware |
| v0.2 D1 Mini firmware, CYD firmware, host tests | **Passed** | [CI run 42430738405](https://github.com/ggrant111/wled-applehome/actions/runs/42430738405) |
| v0.2/v0.4 portal discovery and Apple Home enrollment | **User-verified (2026-11-07)** | Portal discovers devices and adds lights in Apple Home; other Phase 2 checks pending |
| v0.2 full 8-light / remove-reboot / identity recovery | Pending hardware | [Phase 2 test plan](PHASE2_TEST_PLAN.md) |

| v0.3 D1 Mini firmware, CYD firmware and host tests | **Passed** | [Phase 3 CI run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722) |
| v0.3 preset control / switch-reset / state reconciliation | Pending hardware | [Phase 3 test plan](PHASE3_TEST_PLAN.md) |

| v0.4 D1 Mini, CYD firmware builds + host/NVS migration tests | **Passed** | [CI run 42439628403](https://github.com/ggrant111/wled-applehome/actions/runs/42439628403) |
| v0.4 portal load / discovery / add to Home | **User-verified (2026-11-07)** | Web portal works; WLED devices discovered and added to Apple Home |
| v0.4 web authentication hardening / OTA recovery | **Not yet verified** | [Phase 4 test plan](PHASE4_TEST_PLAN.md) |

| v0.4.1 CSRF fix, browser regression, D1 Mini/CYD firmware & OTA size | **Passed** | [CI run 42442460633](https://github.com/ggrant111/wled-applehome/actions/runs/42442460633) |
| v0.4.1 live preset-add / stale-tab recovery | **Hardware pending** | User to verify without NVS erase |

| v0.5 worker / gzip portal / D1 Mini + CYD builds | **Passed** | [CI run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) |
| v0.5 background worker behavior with offline controllers | **Pending hardware** | [v0.5 test plan](PHASE5_TEST_PLAN.md) |

## Engineering risk / budget

- **4MB OTA flash budget is tight.** v0.2 compiled binary uses ~1,733,463 bytes (85.3%) of each 2,031,616-byte app slot; approximately 298KB remains. Major additions (especially touchscreen/web assets) will need explicit size-budget tests, optimization, larger flash hardware, or an altered OTA strategy.
- v0.3 linked flash use was **1,758,847 bytes (86.6%)** of 2,031,616 per OTA application slot.
- **v0.5 full firmware.bin:** D1 Mini **1,957,408** / 2,031,616 bytes (**74,208 free**); CYD **1,957,424** bytes (**74,192 free**) — **96.3%** of OTA slot. Portal raw HTML 13,425 bytes compresses to 5,001 bytes at build time. CI 42443428109 passed all gates.
- **v0.4.1 full firmware.bin:** 1,956,928 / 2,031,616 bytes (96.3% OTA slot); **74,688 bytes remaining** on both D1 Mini and CYD.
- **v0.4 linked flash:** D1 Mini **1,876,139 bytes (92.3%)**, CYD 1,876,155 (92.3%). **Full firmware.bin** (includes headers/padding): D1 Mini **1,955,232** bytes, CYD **1,955,248**; **96.2% OTA slot** and only **76,384 / 76,368 bytes spare** respectively. Do not plan significant more UI code on 4MB boards without moving to larger-flash hardware, compression, or optimizing flash use. CI requires at least 70KB spare.
- **Important migration fix:** Empty WLED MAC and IP strings are valid on the v0.1 original light, but ESP32 Preferences::putString returns 0 for a successful empty write. DeviceRegistry now verifies NVS string writes by readback; a native mock-Preferences regression test checks AID=2 migration and stable IDs across reboots.
- Current network operations use a bounded blocking HTTP client; eight offline lights and mDNS scans may temporarily delay HomeKit servicing. Treat nonblocking IO as a priority before closing Phase 2.

## Release decision process

Each phase changes status from **PLANNED → IN PROGRESS → CODE COMPLETE → HARDWARE VERIFIED** only after the relevant evidence exists. Add a dated note in `docs/PROGRESS.md` for every milestone, plus CI run URLs, hardware observations, and known blockers.

## References

- [HomeSpan](https://github.com/HomeSpan/HomeSpan) — HomeKit HAP bridge; requires Arduino-ESP32 3.3.0+ and at least ~1.9 MiB app partition.
- [WLED JSON API](https://kno.wled.ge/interfaces/json-api/) — state/power/brightness/presets.
- [PlatformIO Espressif32 8.0.0](https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0) — Arduino-ESP32 3.3.12.
