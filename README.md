# WLED Apple Home Bridge

**Current release:** v0.1 development / Phase 1. This project runs a local HomeKit bridge on an ESP32 and maps WLED's JSON API to Apple Home light controls. **One WLED light** is implemented in the initial firmware; 8-device discovery, presets, web UI, and touchscreen control are roadmap items, not yet delivered.

- **Primary board:** WEMOS/LOLIN ESP32 D1 Mini (`wemos_d1_mini32`; **not** ESP8266).
- **Secondary board:** ESP32-2432S028R ("CYD"; headless firmware initially).
- **HomeKit layer:** HomeSpan 2.1.8.
- **WLED transport:** HTTP JSON API (`/json/state`).
- **Build:** PlatformIO Espressif32 8.0.0 / Arduino-ESP32 3.3.12.

## Project documents

- [ROADMAP.md](docs/ROADMAP.md) — primary milestone tracker, acceptance gates, and future features.
- [SETUP.md](docs/SETUP.md) — flash, Wi-Fi, WLED configuration, Apple Home pairing, and testing.
- [DECISIONS.md](docs/DECISIONS.md) — architectural choices and their rationale.
- [PROGRESS.md](docs/PROGRESS.md) — chronological development and verification record.

## Current functionality (Phase 1 firmware)

- A real HomeSpan bridge with **one bridged LightBulb accessory**.
- Apple Home **On** and **Brightness** mapped to WLED JSON POST.
- WLED GET polling to reflect changes made in other apps.
- Store/change a WLED IPv4 address in ESP32 NVS using HomeSpan's serial CLI.
- Boot-safe configuration, network timeouts, separate hardware build environments.
- Automated native brightness math tests and firmware-build workflow.

**No device has yet been physically paired or tested.** GitHub Actions builds need to complete before binary downloads are considered verified.

### Quick start

1. Install [PlatformIO](https://platformio.org/); confirm that your D1 Mini uses **ESP32**, not ESP8266.
2. Plug it into USB and run `pio run -e d1_mini_esp32 -t upload` (or CYD: `-e cyd_2432s028r`).
3. Open `pio device monitor -b 115200`. Use `W` to set Wi-Fi, `S` to set a **unique HomeKit code**, and `@w192.168.1.50` to set your WLED address (replace example IP).
4. Pair the **WLED Apple Home Bridge** from Apple Home → Add Accessory → More Options.
5. Verify On, Brightness, and reflection of WLED changes.

Read [full setup instructions](docs/SETUP.md) before flashing.

## Technical constraints

HomeSpan 2.1.8 needs Arduino-ESP32 >=3.3.0 and a >=1.9MiB app partition. We use two 0x1F0000 OTA partitions on a 4MB flash device, reserving an upgrade path. The official Espressif32 8.0.0 platform provides Arduino-ESP32 3.3.12. Apple Home may identify a DIY accessory as uncertified. A HomePod or Apple TV home hub is recommended/required by HomeSpan for supported use.

This bridge communicates over the LAN; it does not alter WLED firmware. Its configuration and pairing credentials stay on the ESP32, **not in git**.

## References

- https://github.com/HomeSpan/HomeSpan
- https://kno.wled.ge/interfaces/json-api/
- https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0
