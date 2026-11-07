# Development progress / verification log

## 2026-11-07 — Project start

- Created tracked `docs/ROADMAP.md` (phases v0.1 → v2.0, gates, checklists).
- Chosen ESP32 D1 Mini first; ESP32-2432S028R CYD headless build second.
- Designed Phase 1 HomeSpan bridge with one light, WLED JSON GET/POST, NVS WLED IPv4 setting, and serial setup command `@w`.
- Added separate PlatformIO targets, two 1.94MiB app partitions, host-side brightness checks, and GitHub Actions builds.
- **Verification status: Not hardware tested.** CI status must be checked after push. No claim of working Apple Home pairing until verified on an iPhone and WLED device.

## Next actions

1. Run/inspect GitHub Actions host checks, D1 Mini build, and CYD build.
2. Fix any compiler or partition errors reported by CI.
3. Flash the ESP32 D1 Mini; pair using a unique code; test real WLED power/brightness and state reflection.
4. Update checklist/status in `docs/ROADMAP.md` with verified evidence, then plan Phase 2 discovery.
