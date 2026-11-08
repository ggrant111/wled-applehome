# WLED Apple Home Bridge — Roadmap

> **Source of truth:** Update this file whenever a milestone changes, and record decisions in `docs/DECISIONS.md`. Do not mark hardware or HomeKit pairing tests complete from a successful compile alone.
>
> **Updated:** 2026-11-07 · **Active milestone:** v0.2 / Phase 2 (in progress; Phase 1 reboot and on/off gate pending) · **Primary target:** ESP32 D1 Mini · **Secondary target:** ESP32-2432S028R CYD (headless mode first).

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

### Phase 2 — v0.2: Discovery and stable multi-device bridge — **IN PROGRESS**

- [ ] Discover WLED using mDNS/service discovery and verify candidates via `/json/info`.
- [ ] Manual device add, rename, remove; de-duplicate by persistent identity.
- [ ] Assign stable HomeKit accessory IDs saved in NVS, **never** based on scan order/IP.
- [ ] Device health and offline states; nonblocking network I/O and bounded retries.
- [ ] Support 8 WLED devices by default, test up to capacity limits.
- [ ] Ensure addition/removal updates the HomeKit database without accidental re-pairing.
- [ ] Test reboot, IP changes, reordering, and missing devices.

### Phase 3 — v0.3: Presets and state sync — **PLANNED**

- [ ] Discover saved presets from each WLED controller.
- [ ] User-selected presets as momentary trigger switches or HomeKit scene helpers.
- [ ] Add bidirectional state sync with conflict protection and command acknowledgement.
- [ ] Observe changes made via WLED, GlowDeck, automation, or other controllers.
- [ ] Optional polling/WebSocket transport where supported.

### Phase 4 — v1.0 MVP: Web setup, reliability, and release — **PLANNED**

- [ ] Local configuration portal with device list, pairing/setup instructions, and diagnostics.
- [ ] OTA firmware with configuration/paired-identity preservation and recovery plan.
- [ ] Network reconnection/backoff, watchdog checks, reboot tests.
- [ ] Local-network-only security posture; protect setup mode and dangerous operations.
- [ ] Config export/import, release changelog, binary artifacts, and user documentation.
- [ ] Final 8-device and 24-hour run verification.

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

## Non-goals / guardrails

1. Do **not** flash or change firmware on any WLED controller.
2. Do **not** change WLED presets, segments, or running effects unless commanded.
3. Do **not** put SSIDs, passwords, or HomeKit codes in git.
4. Do **not** assign HomeKit accessory IDs from discovery order. Once assigned, preserve IDs.
5. Home app does **not** provide a native WLED preset dropdown; use optional trigger accessories.
6. **CYD v0.1 is headless**, even though it builds for the touchscreen hardware.
7. The D1 Mini must be **ESP32**, not ESP8266.
8. Keep HomeKit/UDP/mDNS traffic on the same permitted LAN; no cloud required.

## Build/test tracking

| Check | Status | Evidence |
| --- | --- | --- |
| Host-side logic checks | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| D1 Mini PlatformIO build | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| CYD PlatformIO build | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| User hardware Apple Home pairing and brightness | **Verified by user (2026-11-07)** | First firmware installed, paired and changed brightness of real WLED |
| D1 Mini physical on/off and reboot persistence | Not yet reported | Requires physical test |
| CYD physical pairing / WLED actuation | Not tested | User hardware |

## Release decision process

Each phase changes status from **PLANNED → IN PROGRESS → CODE COMPLETE → HARDWARE VERIFIED** only after the relevant evidence exists. Add a dated note in `docs/PROGRESS.md` for every milestone, plus CI run URLs, hardware observations, and known blockers.

## References

- [HomeSpan](https://github.com/HomeSpan/HomeSpan) — HomeKit HAP bridge; requires Arduino-ESP32 3.3.0+ and at least ~1.9 MiB app partition.
- [WLED JSON API](https://kno.wled.ge/interfaces/json-api/) — state/power/brightness/presets.
- [PlatformIO Espressif32 8.0.0](https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0) — Arduino-ESP32 3.3.12.
