# WLED Apple Home Bridge

A **local-only** Apple Home / Siri bridge for existing WLED controllers, running on an ESP32. The firmware exposes WLED controllers as independently dimmable HomeKit lights, without modifying their WLED firmware.

**Current version:** v0.9.1-dev — saved WLED playlists alongside presets with HomeKit active-playlist tracking (**CI passed; physical playlist test pending**). [Download firmware](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721). [Download firmware artifacts](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880).

## Hardware
- **Primary:** ESP32 D1 Mini (`wemos_d1_mini32`, not ESP8266).
- **Secondary:** ESP32-2432S028R CYD (headless for now; no touchscreen support yet).

## Current features

- HomeSpan bridge with **On, Brightness, Hue and Saturation** characteristics (color functionality added in v0.6 candidate; not yet verified on hardware).
- **User-confirmed:** Original firmware paired with Apple Home and changed brightness on a real WLED controller.
- v0.2: Up to eight persistent WLED accessory entries, stable AIDs, initial AID=2 migration.
- v0.2: On-demand mDNS WLED scan, `/json/info` validation, manual IP addition and explicit enrollment.
- v0.2: Serial add/list/remove commands, 8-second staggered polling with 30s offline backoff.
- v0.3: Saved WLED preset catalog (`@c<AID>`), selectively publish up to 12 momentary HomeKit preset switches (`@p<AID>:<presetId>`), list (`@s`) / remove (`@q<switchAID>`).
- v0.3: Stable shared AID allocator for lights and preset switches, and prompt state reconciliation following accepted WLED commands.
- v0.4: Local **password-gated web portal** (TCP 8080) with live health, light management/controls, opt-in preset switches, manual/restart operations and diagnostics.
- v0.4: **Native HomeSpan OTA** (no browser uploader) with user-provisioned admin password and 60-second deferred firmware validation/rollback safety, plus binary-size CI guard.
- v0.5: Background FreeRTOS worker moves routine WLED polling and light/preset POSTs off the HomeKit loop; queue depth and HTTP failure diagnostics, stale-response protection, backoff, and gzip-compressed dashboard stored in flash.
- Automated PlatformIO builds for D1 Mini/CYD and host-side brightness/registry policy tests.

**Build verified:** v0.3 passed native tests, D1 Mini and CYD firmware builds ([CI run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722)). **Physical tests remain pending** for Phases 2, 3 and 4. **Not yet hardware-verified:** v0.2 discovery, 8-light operation, and paired Apple Home database refresh after adding/removing devices. No presets, web setup UI, or CYD touchscreen interface yet.

## v0.9.1 — WLED saved playlists inside HomeKit Effects groups

**Saved WLED playlists** are stored in the same WLED `/presets.json` catalog as normal presets and launch through the exact same API call `{"ps":playlistId}`. The web dashboard now labels saved resources as **Preset** or **Playlist** and lets you add both to the same per-light Effects group (up to five total controls per WLED device). This does not create/modify playlists on WLED; create them in WLED first.

The bridge reads WLED's live `pl` (currently playing playlist ID) **as well as** `ps` (current preset step). A grouped playlist switch stays On while the playlist cycles through its constituent presets, and turns Off when playback stops or another selection is made. Ordinary preset buttons are not incorrectly highlighted as playlist steps. Toggling the active playlist button Off powers off the associated WLED light, matching the existing group's v0.8 On/Off semantics.

The resource kind is saved independently in the existing NVS `wled-fx` / `wled-fx9` entry; existing controls default to Preset and preserve their IDs, names, and order. If a playlist had previously been added as a preset, selecting the same entry again updates its classification **without changing its HomeKit accessory ID**, followed by a bridge restart. All grouped accessory AIDs and original light identities remain unchanged. [Playlist hardware test plan](docs/PHASE9_PLAYLIST_TEST_PLAN.md). **Both ESP32 firmware builds passed:** D1 Mini 1,576,656 bytes (454,960 free in OTA slot); CYD 1,576,640 bytes (454,976 free). [GitHub Actions CI](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721).

## v0.9 — Multiple WLED effects groups

Up to eight WLED light accessories can now have their own opt-in **grouped Effects accessory**, each with up to five named saved WLED presets. The original v0.8 Effects group keeps its existing pairing/AID, configured names, order and stateful On/Off behavior. New groups created from the authenticated web portal each get a separate, monotonic HomeKit accessory ID. Each group observes only its own WLED light's actual preset/power status.

The dashboard includes per-light preset discovery, adding effects, renaming controls, and removing/reordering controls. **Important:** removing/reordering an individual grouped control changes HomeKit service ordering, so the UI warns and recreates **that group only** with a new AID; scenes/automations using that one group may need updating. Appending or renaming presets leaves its existing AID unchanged. Use an OTA update **without erasing NVS**, then restart the bridge when the portal indicates HomeKit accessory changes are pending. **Build verification:** [run 42454610880](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880) passed host tests, D1 Mini firmware (1,574,848 bytes) and CYD firmware (1,574,832 bytes), leaving over 456KB available in either OTA slot. Hardware verification with multiple independent WLED groups remains pending. [Phase 9 hardware test checklist](docs/PHASE9_TEST_PLAN.md).

## v0.8 — Stateful preset power and segment inspection

Grouped WLED Effects accessory switches now act like persistent **power/selection controls** rather than 250ms momentary triggers: switch On activates that saved preset; another preset selected makes the first show Off after WLED state reconciliation; switch the active preset Off to turn WLED off. HomeKit's actual button appearance remains controlled by the Home app; standard standalone preset switches stay momentary, and existing LightBulb/pairing identities are unchanged.

The management portal gains a **read-only** "Inspect saved segments" action for a selected WLED preset. It reads only that preset's segment start/stop geometry from WLED's existing `/presets.json` with a filtered JSON stream, reports whether bounds were stored, and explains how to save them in WLED if absent. **The bridge continues using WLED's standard `{"ps":id}` API for full preset application and never writes segment geometry or overwrites user presets.** [Phase 8 test plan](docs/PHASE8_TEST_PLAN.md).

## v0.7.1 — Grouped effect buttons (alternative)

**Confirmed user feedback:** v0.7's bridged Television shows its four preset names in Settings, but **does not display a selectable input menu**. Update to v0.7.1, open the portal's Effects Controller, select **Convert to grouped buttons**, then reboot. This produces a new HomeKit accessory with multiple named **Switch** services, preserving the selected WLED presets, normal RGB light, Siri color commands, and any other preset switches. Converting is opt-in and uses a fresh stable accessory ID, never recycling the TV ID. [v0.7.1 CI run 42449994004](https://github.com/ggrant111/wled-applehome/actions/runs/42449994004) passed (both firmware.bin sizes 1,560,144 bytes; 471,472 bytes OTA headroom). Actual group appearance and actions still require a device test. [Phase 7B checklist](docs/PHASE7B_TEST_PLAN.md).

## v0.7 — Experimental effects selector

The management portal can optionally expose **one** HomeKit Television-style Effects Controller for **one** WLED light. Select up to **five existing saved WLED presets** in the portal. After a bridge restart, Apple Home should present these as named inputs under a separate accessory; each input invokes the corresponding WLED preset via the existing queued HTTP worker. This is NOT a native Lighting effects dropdown, and TV-style accessories may show a generic icon when bridged. **Both firmware targets and automated checks passed** in [CI run 42448093812](https://github.com/ggrant111/wled-applehome/actions/runs/42448093812) (1,557,792-byte images, 473,824 bytes free in each OTA slot); iPhone Home UI and WLED behavior have not been verified. The current full-color light stays intact. See [Phase 7 checklist](docs/PHASE7_TEST_PLAN.md).

## v0.6 — Apple Home RGB color wheel

Apple Home light accessories now expose **Hue** and **Saturation**, alongside the existing On/Brightness controls. Changing color selects **Solid** on WLED's main segment and sets RGB (white channel off), while WLED's other segments and saved presets remain untouched. Brightness remains independent; the main segment's primary color is polled back into Apple Home (animated pixel colors are not tracked). Existing paired bridge and stable AIDs are preserved; upgrade without erasing NVS. **CI builds passed** (D1 Mini 1,545,312 bytes; CYD 1,545,296 bytes with -Oz; both have >486KB OTA space available). **On-device validation still pending**, especially under aggressive size optimization. See [color test plan](docs/PHASE6_TEST_PLAN.md).

## v0.5 — Network reliability and flash budget

Routine WLED network operations are now queued to a background ESP32 task; HomeKit remains responsive while one WLED controller is offline. **Accepted commands mean queued, not guaranteed applied**. Check the dashboard's pending/failure counters, and verify WLED response on real hardware. `web/portal.html` is compressed before the firmware build. See [v0.5 test plan](docs/PHASE5_TEST_PLAN.md).

## v0.4.1 — Preset add 403 correction

If the ESP32 restarts while the management dashboard stays open, the browser tab may retain an expired per-boot CSRF token. In v0.4.0, clicking **Add to Home** for a preset can return `403 Missing or invalid CSRF token`. **Immediate workaround:** hard-refresh the portal tab after a bridge restart. Version 0.4.1 refreshes the token through authenticated status requests, sends it in a header as well as the form, and safely retries once if it changed. Digest login and CSRF checks remain enforced. **Update firmware without erasing NVS/HomeKit pairing.**

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

### v0.5 build verification

[CI run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) passed both builds and host tests. Full D1 Mini binary: 1,957,408 bytes (74,208 bytes remaining in each 2,031,616-byte OTA slot). CYD: 1,957,424 bytes (74,192 bytes remaining). **Only code/CI verified; physical Apple Home and WLED network-recovery tests are pending.**
