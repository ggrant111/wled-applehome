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

## Next actions

1. Flash the ESP32 D1 Mini; pair using a unique code; test real WLED power/brightness and state reflection.
2. Run reboot, reconnection, and HomeKit state-notification tests.
3. Record real device results in this log and update the remaining Phase 1 acceptance boxes.
4. Begin Phase 2 discovery only once Phase 1 hardware tests are verified.
