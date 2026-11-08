# WLED Apple Home Bridge

A **local-only** Apple Home / Siri bridge for existing WLED controllers, running on an ESP32. The firmware exposes WLED controllers as independently dimmable HomeKit lights, without modifying their WLED firmware.

**Current version:** v0.4.0-dev — **Phase 4A CI passed, hardware verification pending** ([CI run 42439628403](https://github.com/ggrant111/wled-applehome/actions/runs/42439628403)).

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
- v0.4: Local **password-gated web portal** (TCP 8080) with live health, light management/controls, opt-in preset switches, manual/restart operations and diagnostics.
- v0.4: **Native HomeSpan OTA** (no browser uploader) with user-provisioned admin password and 60-second deferred firmware validation/rollback safety, plus binary-size CI guard.
- Automated PlatformIO builds for D1 Mini/CYD and host-side brightness/registry policy tests.

**Build verified:** v0.3 passed native tests, D1 Mini and CYD firmware builds ([CI run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722)). **Physical tests remain pending** for Phases 2, 3 and 4. **Not yet hardware-verified:** v0.2 discovery, 8-light operation, and paired Apple Home database refresh after adding/removing devices. No presets, web setup UI, or CYD touchscreen interface yet.

## Phase 4A first-run setup

1. Keep your paired ESP32 and flash **without erasing NVS**.
2. On the serial CLI (115200 baud), enter `@k <YOUR-OWN-UNIQUE-ADMIN-PASSWORD>` using 14–64 printable non-space ASCII characters. Never post this password to GitHub.
3. Reboot with `R`. The portal opens at `http://<ESP32-IP>:8080/` on the trusted local network. Login: `admin` + your password.
4. Portal supports reading light status, device discovery, enrolling/removing lights, selected preset switches, and control. Accessory-list changes become effective after reboot.
5. The same password enables HomeSpan's authenticated ArduinoOTA upload service; update via Arduino IDE's network port or a compatible uploader. **No default OTA password is exposed.**

**Security:** Local HTTP Digest is not HTTPS. Never port-forward the portal or OTA ports. The password lives in device NVS, not in git. To disable the portal and OTA: `@k -` then `R`. See [setup](docs/SETUP.md).

## Quick start

1. Read [setup and safety instructions](docs/SETUP.md).
2. Flash the same paired ESP32 normally (`pio run -e d1_mini_esp32 -t upload`). **Do not erase flash/NVS.**
3. Open serial at 115200 baud. Type `@l` to inspect the migrated original AID=2 light.
4. Type `@d` to discover WLED, then `@a1` to add a discovered controller, or `@m192.168.1.50` to add directly by IP (use your IP).
5. Type `R` to reboot and refresh the bridged accessories in Apple Home.

**Flash budget:** v0.4 full firmware.bin consumes **96.2%** of each 2,031,616-byte OTA app slot, leaving **~76KB**. CI enforces at least 70KB spare. Significant additional features will likely require optimizing firmware size or moving to larger-flash ESP32 hardware. Monitor size as the project grows.

## Project tracking

- **[Roadmap](docs/ROADMAP.md)** — milestone checklists, acceptance criteria, CI/hardware verification status.
- **[Progress log](docs/PROGRESS.md)** — work completed and verified, in chronological order.
- **[Phase 2 hardware test plan](docs/PHASE2_TEST_PLAN.md)** — safe test cases.
- **[Phase 3 preset test plan](docs/PHASE3_TEST_PLAN.md)** — pairing, preset switches, power/brightness sync.
- **[Phase 4 web/OTA test plan](docs/PHASE4_TEST_PLAN.md)** — browser authentication, OTA, recovery and soak checks. — pairing, preset switches, power/brightness sync.
- **[Architecture decisions](docs/DECISIONS.md)** — board, HomeKit, NVS and network choices.
- **[Setup](docs/SETUP.md)** — commands and deployment steps.

## References

- [HomeSpan](https://github.com/HomeSpan/HomeSpan)
- [WLED JSON API](https://kno.wled.ge/interfaces/json-api/)
- [PlatformIO Espressif32](https://github.com/platformio/platform-espressif32)
