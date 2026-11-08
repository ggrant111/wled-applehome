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

## ADR-010 — Read-only preset discovery and opt-in HomeKit triggers (2026-11-07)

WLED stores up to 250 saved presets at `/presets.json`; selected presets are activated by POST `{"ps":id}` to `/json/state`. Apple Home has no native WLED preset dropdown, so expose opt-in `Service::Switch` accessories that automatically reset to Off after a confirmed trigger. Limit to **12 bridge-wide HomeKit preset switches** initially. List stored presets read-only; never create, delete, save, or overwrite a WLED preset from the bridge.

## ADR-011 — Shared accessory identity namespace (2026-11-07)

Allocate all newly enrolled light and preset-switch HomeKit AIDs via the existing `DeviceRegistry` NVS `nextAid` high watermark, advance it in NVS **before** recording the new accessory and never reuse removed IDs. Original light remains AID=2. Preset entries in separate `wled-ps` NVS namespace refer to their parent light AIDs; missing parent prevents the switch appearing at boot. A reboot is still required after enrollment/removal.

## ADR-012 — Poll-based reconciliation during Phase 3 (2026-11-07)

On successful WLED POST (light/preset), increment a mutation revision and avoid querying the WLED state until ~450ms later to prevent stale state from replacing recently accepted HomeKit changes. After settling, read authoritative WLED power/brightness, notify Apple Home of changed values, and log observed `ps` preset ID. Keep 8-second staggered polling for external changes and existing offline backoff; implement WebSockets/nonblocking networking in a later reliability iteration. HTTP 2xx alone does not prove WLED physically applied a preset; hardware validation remains required.

## ADR-013 — Authenticated local web portal on TCP 8080 (2026-11-07)

HomeSpan owns HomeKit services and mDNS on its own ports. Use the Arduino-ESP32 synchronous `WebServer` on TCP **8080**, serviced after `homeSpan.poll()`, with no additional mDNS responder. Provide a compact local page for device health, WLED enrollment/controls, selected preset switches and explicit restarts. Read state from the existing WLED light services' **cached poll results**, avoiding additional HTTP calls every time the page refreshes. Discovery and catalog requests remain explicitly user-triggered and may block HomeKit briefly; nonblocking design is a later optimization.

The portal is **disabled by default** and is available only after a 14–64 character administrator secret is provisioned over the serial CLI (`@k <secret>`) and the board is rebooted. Browser authentication uses HTTP Digest (username `admin`), with per-boot random CSRF token required on POST changes. Password stored in a separate `wled-admin` NVS namespace, never shipped in firmware/git. HTTP Digest is not TLS and no port forwarding is supported. The secret is stored in plaintext in local unencrypted NVS, so anyone with physical flash access may extract it; hardware-protected storage/secure boot is out of MVP scope.

## ADR-014 — Native HomeSpan OTA with safe boot (2026-11-07)

Use `homeSpan.enableOTA(adminPassword)` **only after** serial provisioning; do not expose HomeSpan's known default OTA password. Reuse the admin password for ArduinoOTA. Preserve the existing dual `0x1F0000` app partitions and HomeSpan/NVS pairing. Include `SpanRollback.h` and call `homeSpan.markSketchOK()` only after 60 seconds of runtime with connected Wi-Fi to allow ESP-IDF bootloader rollback if a new OTA firmware crashes before it becomes healthy. This is **not an automatic browser update form**; use Arduino IDE network upload or compatible secure OTA client. On-device recovery testing is mandatory before release.

## ADR-015 — Explicit code and partition-size gates (2026-11-07)

GitHub Actions must compile both targets and run host validation tests. A dedicated post-build gate compares `firmware.bin` against the 2,031,616-byte OTA partition and requires at least 70,000 bytes spare, preventing oversized releases. Report size and note limits before adding UI/touchscreen assets.

## ADR-016 — Empty-string NVS migration must validate stored value, not putString count (2026-11-07)

The ESP32 Preferences API returns `strlen(value)` for `putString` (including a **successful zero** on `""`). The Phase 2 original-light migration intentionally writes an empty WLED MAC and sometimes an empty IP; treating a return of zero as failure could halt startup and leave the paired HomeKit bridge unreachable. `DeviceRegistry::save` now writes each string and verifies it via `isKey()` + `getString()` before persisting the AID. A native mock-Preferences integration test validates v0.1-to-v0.4 registry migration, stable original AID=2, IP/MAC updates, and never-reused IDs across reboot. Verified by CI run 42439628403. This fix has **not yet been validated on real hardware**.

## ADR-017 — Offload routine WLED HTTP from HomeSpan callbacks (2026-11-07)

HomeSpan callback methods must not block for slow/offline WLED controllers. Start a **single** FreeRTOS HTTP worker pinned to core 0, with a fixed-capacity 24-command queue covering all eight lights. HTTP polling and WLED JSON POST use an independent worker-owned `WledClient`; no network I/O or HomeKit characteristic writes occur across threads. The main loop enqueues commands immediately and consumes copies of worker snapshots through a short ESP32 critical section.

Use staggered polling, 8-second healthy poll cycles, ~30-second offline retry and a ~450ms reconciliation read after POST. Each queued command increments the slot's mutation epoch; an in-flight GET can publish only when its epoch and host generation remain unchanged and no commands are pending. Host changes invalidate stale snapshots. Keep queue insertion nonblocking; reject operations if full rather than freeze HomeSpan.

**Semantics:** A HomeKit write acknowledged by this asynchronous bridge is **queued, not confirmed by WLED**. Background HTTP errors are recorded and eventually corrected by state reconciliation. HomeKit cannot receive a deferred per-command error from this implementation; future versions may provide more explicit pending/failure notifications. Preset trigger switch still resets after activation attempt is queued.

User-initiated discovery, manual WLED probe and large saved-preset catalogs remain blocking on the main portal/serial path; this phase removes blocking from *routine* polling and light/preset commands rather than claiming full async networking.

## ADR-018 — Pre-gzip HTML to preserve 4MB dual OTA budget (2026-11-07)

The Phase 4 dashboard embedded ~13KB raw HTML in firmware and made a heap-allocating String copy for every request to interpolate a boot CSRF token. Move the editable page to `web/portal.html`; the PlatformIO `pre:tools/prepare_portal.py` script produces a gzip-compressed `include/PortalGzip.generated.h` at build time. Serve the bytes with `Content-Encoding: gzip` straight from PROGMEM, no runtime decompression or HTML String allocation.

The boot CSRF token is now obtained from the existing **Digest-authenticated, no-store, same-origin** `/api/status` header, rather than rendered into HTML. All state-changing requests still require exact CSRF validation. Browser regression tests are updated to read the editable HTML source.
