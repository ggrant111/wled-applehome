# Architecture decision record

## ADR-001 — D1 Mini ESP32 primary, CYD secondary (2026-11-07)
Choose the compact **ESP32** D1 Mini for the always-on headless bridge. CYD is an optional alternative with touchscreen UX scheduled after MVP. Keep hardware-independent HomeKit/WLED code shared. CYD v0.1 builds but does not drive the display.

## ADR-002 — HAP/HomeSpan before Matter (2026-11-07)
Use HomeSpan 2.1.8 for independent bridged LightBulb accessories in Apple Home. Matter bridging is a separate research milestone after a proven MVP. Don't imply the Home app has a native preset picker.

## ADR-003 — Official PlatformIO 8.0.0 Arduino core (2026-11-07)
Pin `espressif32@8.0.0` (Arduino 3.3.12) because HomeSpan 2.1.8 requires Arduino 3.3.0+. Earlier `espressif32@6.10.0`/7.1.x core combinations use Arduino 2.x and are insufficient. Validate in CI.

## ADR-004 — Local WLED JSON API (2026-11-07)
Treat WLED as the state authority and use GET/POST `/json/state` for MVP. No modifications to WLED firmware. Write **only requested keys** to avoid overwriting segments, colors, effects, or presets. HTTP calls have timeouts; later introduce asynchronous networking/WebSockets when scaling to 8 devices.

## ADR-005 — Identity & data persistence (2026-11-07)
Assign AID=1 for bridge and AID=2 for Phase 1 light. Future discovered devices must get stable IDs in NVS independent of network address, name, or discovery order. Credentials, pairing keys, and WLED address are never committed to git. Use NVS for WLED IPv4.

## ADR-006 — Flash/OTA budget (2026-11-07)
HomeSpan requires about 1.9MiB app partition. On 4MB ESP32, use two `0x1F0000` (~1.94MiB) OTA slots, retaining NVS and OTA metadata. OTA transport/UI remains Phase 4. Confirm partition sizes with the actual firmware build.

## ADR-007 — Opt-in discovery and cold-boot HomeKit database changes (2026-11-07)

Probe devices advertised as `_wled._tcp` by mDNS, confirm `/json/info` before showing them, and require explicit enrollment (`@aN` or `@m<IP>`). Persist registry entries and change bridged-accessory topology only on the next reboot to avoid mutation during normal HomeKit operations. HomeSpan recomputes the database configuration on startup. Do **not** initialize a second mDNS responder: HomeSpan owns that responder. This is a deliberate safe-first v0.2 choice; background discovery/web enrollment is later.

## ADR-008 — Preserve original pairing and stable MAC/AID identity (2026-11-07)

Migrate the original `wled-home/ip` into slot 0 with **AID=2**, the existing HomeKit services, model and name; keep HomeSpan's NVS credentials untouched. Future devices are validated via `/json/info` and keyed by normalized hardware MAC, with never-reused AIDs allocated monotonically and persisted across reboots. An IP change for a previously identified MAC updates its endpoint without replacing its AID. Deleting devices is explicit and completed by reboot. WLED devices missing valid MACs aren't enrolled until supported stable identity fallback is designed.

## ADR-009 — Transport performance compromise in v0.2 (2026-11-07)

Use staggered 8-second polling and 650ms request timeouts; back off failed polling to 30 seconds. The HTTP client and on-demand mDNS scan are still blocking. This is a constrained intermediate implementation; moving WLED traffic off the HomeKit poll loop must precede claiming production-grade 8-device responsiveness.
