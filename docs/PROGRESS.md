# Development progress / verification log

## 2026-11-07 — Project start

- Created tracked `docs/ROADMAP.md` (phases v0.1 → v2.0, gates, checklists).
- Chosen ESP32 D1 Mini first; ESP32-2432S028R CYD headless build second.
- Designed Phase 1 HomeSpan bridge with one light, WLED JSON GET/POST, NVS WLED IPv4 setting, and serial setup command `@w`.
- Added separate PlatformIO targets, two 1.94MiB app partitions, host-side brightness checks, and GitHub Actions builds.
- **CI verification (passed):** [GitHub Actions run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) — D1 Mini firmware, CYD firmware, and host tests all passed. **Hardware remains untested.** No claim of working Apple Home pairing until verified with an iPhone and WLED.

## Next actions

1. Flash the ESP32 D1 Mini; pair using a unique code; test real WLED power/brightness and state reflection.
2. Run reboot, reconnection, and HomeKit state-notification tests.
3. Record real device results in this log and update the remaining Phase 1 acceptance boxes.
4. Begin Phase 2 discovery only once Phase 1 hardware tests are verified.
