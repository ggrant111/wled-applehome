# WLED Apple Home Bridge

A **local-only** Apple Home / Siri bridge for existing WLED controllers, running on an ESP32. The firmware exposes WLED controllers as independently dimmable HomeKit lights, without modifying their WLED firmware.

**Current version:** v0.3.0-dev — **Phase 3 CI PASS; physical validation pending**.

## Hardware
- **Primary:** ESP32 D1 Mini (`wemos_d1_mini32`, not ESP8266).
- **Secondary:** ESP32-2432S028R CYD (headless for now; no touchscreen support yet).

## Current features

- HomeSpan bridge with On and Brightness characteristics.
- **User-confirmed:** Original firmware paired with Apple Home and changed brightness on a real WLED controller.
- v0.2: Up to eight persistent WLED accessory entries, stable AIDs, initial AID=2 migration.
- v0.2: On-demand mDNS WLED scan, `/json/info` validation, manual IP addition and explicit enrollment.
- v0.2: Serial add/list/remove commands, 8-second staggered polling with 30s offline backoff.
- v0.3: Saved WLED preset catalog (`@c<AID>`), selectively publish up to 12 momentary HomeKit preset switches (`@p<AID>:<presetId>`), list (`@s`) / remove (`@q<switchAID>`).
- v0.3: Stable shared AID allocator for lights and preset switches, and prompt state reconciliation following accepted WLED commands.
- Automated PlatformIO builds for D1 Mini/CYD and host-side brightness/registry policy tests.

**Build verified:** v0.3 passed native tests, D1 Mini and CYD firmware builds ([CI run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722)). **Physical tests remain pending** for Phase 2 and Phase 3. **Not yet hardware-verified:** v0.2 discovery, 8-light operation, and paired Apple Home database refresh after adding/removing devices. No presets, web setup UI, or CYD touchscreen interface yet.

## Quick start

1. Read [setup and safety instructions](docs/SETUP.md).
2. Flash the same paired ESP32 normally (`pio run -e d1_mini_esp32 -t upload`). **Do not erase flash/NVS.**
3. Open serial at 115200 baud. Type `@l` to inspect the migrated original AID=2 light.
4. Type `@d` to discover WLED, then `@a1` to add a discovered controller, or `@m192.168.1.50` to add directly by IP (use your IP).
5. Type `R` to reboot and refresh the bridged accessories in Apple Home.

**Flash budget:** v0.3 uses ~86.6% of each 2,031,616-byte OTA app slot (about 273KB left) on 4MB hardware. Monitor size as the project grows.

## Project tracking

- **[Roadmap](docs/ROADMAP.md)** — milestone checklists, acceptance criteria, CI/hardware verification status.
- **[Progress log](docs/PROGRESS.md)** — work completed and verified, in chronological order.
- **[Phase 2 hardware test plan](docs/PHASE2_TEST_PLAN.md)** — safe test cases.
- **[Phase 3 preset test plan](docs/PHASE3_TEST_PLAN.md)** — pairing, preset switches, power/brightness sync.
- **[Architecture decisions](docs/DECISIONS.md)** — board, HomeKit, NVS and network choices.
- **[Setup](docs/SETUP.md)** — commands and deployment steps.

## References

- [HomeSpan](https://github.com/HomeSpan/HomeSpan)
- [WLED JSON API](https://kno.wled.ge/interfaces/json-api/)
- [PlatformIO Espressif32](https://github.com/platformio/platform-espressif32)
