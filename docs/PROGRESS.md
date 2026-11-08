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

## 2026-11-07 — Phase 3 work started while user away from hardware

- The user requested progression without physical testing until returning home. Phase 2's discovery, multi-light and pairing regression gates stay open.
- Implemented filtered `/presets.json` catalog reader with on-demand `@c<light-AID>`.
- Implemented explicit, persistent selection of up to 12 HomeKit momentary preset switches using `@p<light-AID>:<preset-ID>`, `@s` to list, and `@q<switch-AID>` to remove. Each switch sends only `{"ps":id}`; no WLED preset modification.
- Introduced shared NVS-backed AID allocation for WLED lights and preset switches, maintaining original AID=2 and preventing AID collisions/reuse.
- Enhanced local POST acknowledgement with a ~450ms settle window and prompt WLED power/brightness reconcile (periodic external-change polling remains).
- **Build VERIFIED:** [Phase 3 GitHub Actions run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722) passed all three jobs (host tests, D1 Mini build, CYD build). D1 Mini flash 1,758,847/2,031,616 (86.6%); CYD 1,758,863/2,031,616 (86.6%).
- **Merged to main** via [PR #1](https://github.com/ggrant111/wled-applehome/pull/1). **All v0.3 physical behavior remains unverified** until user returns to hardware.

## 2026-11-07 — Phase 4A web manager, OTA + safety implementation

- User advanced to Phase 4 while away from hardware; **do not interpret the new work as physical verification of v0.2/v0.3**.
- Implemented mobile-friendly local management web app on port 8080, with live cached WLED health, device actions, mDNS discovery, manual add/remove, selected preset catalog/enrollment, confirmation-based reboot, and diagnostics.
- Kept the portal inactive until a strong 14–64 character administrator password is set at the physical serial console via `@k <password>` and rebooted. Digest login and a boot-local CSRF token protect POST mutations. No default login/OTA secret.
- Enabled HomeSpan's **native authenticated OTA** only after the admin password is provisioned; same secret is used for OTA, and `SpanRollback.h` defers validating pending OTA firmware until stable operation (~60s with Wi-Fi).
- Added host-side admin password/ID validation tests and CI guard that fails a firmware image with <70KB spare room in the 2,031,616-byte OTA app slot.
- **Verification — CI PASSED:** [GitHub Actions run 42439628403](https://github.com/ggrant111/wled-applehome/actions/runs/42439628403): all host checks including migration regression, D1 Mini and CYD firmware builds, and binary size guard passed. D1 Mini full binary 1,955,232 bytes (76,384 free in OTA slot); CYD 1,955,248 (76,368 free). **No physical web/OTA/Apple Home regression tests have been done.**
- **Migration bug corrected and regression-tested:** ESP32 Preferences::putString returns 0 for successfully stored empty strings; Phase 2's original AID=2 migration saved a blank MAC and could falsely halt startup. Fixed by readback validation and added a mock-Preferences integration test for initial migration, AID stability and reboot.
- **Merged to main:** [Phase 4 PR #2](https://github.com/ggrant111/wled-applehome/pull/2). Phase 1 on/off/reboot, Phase 2 multi-device, Phase 3 preset behavior and Phase 4 portal/OTA acceptance tests remain pending.

## 2026-11-07 — User hardware verification: web portal and Apple Home enrollment

- **User-confirmed:** v0.4 web portal is working on their ESP32, WLED controllers were discovered from the portal and new lights were successfully added to Apple Home.
- This is real hardware validation of the portal discovery/enrollment path, not proof that all eight lights, independent power/brightness, WLED preset switches, remote OTA, reboot recovery, credential controls or 24-hour uptime have passed.
- Phase 4B priority order: preset switch end-to-end test; safe OTA and pairing/NVS retention; nonblocking WLED traffic and offline recovery; flash-size optimization; extended stability tests.
- Existing v0.4 build uses about 96.2% OTA app slot; substantial code/UI expansion on this 4 MB board should be avoided until footprint is reduced or hardware capacity increased.

## 2026-11-07 — v0.4.1 preset-add CSRF 403 correction

- User reported HTTP 403 `{"ok":false,"error":"Missing or invalid CSRF token"}` when adding a WLED preset through the working portal.
- **Probable cause from code:** Per-boot random token is replaced when the bridge restarts after newly added lights. The browser tab keeps the prior token until reloaded; other read-only GETs remain functional. Confirm on hardware after update.
- Immediate workaround: hard-refresh the portal tab; this does not delete any paired HomeKit devices or WLED configuration.
- Code fix: return token from Digest-authenticated same-origin status GET, refresh JavaScript token on status updates, and on CSRF 403 fetch a new token + retry the mutation only once if the token changed. Send same token in request header and form field; server validates either with exact-match policy.
- Added tests for C++ CSRF validation and for the **actual** portal JavaScript request code simulating reboot, retry bounds, invalid token and real application errors.
- **CI VERIFIED:** [run 42442460633](https://github.com/ggrant111/wled-applehome/actions/runs/42442460633): all host tests passed (including actual-browser-helper reboot simulation), firmware builds passed on D1 Mini and CYD, and 70KB minimum OTA space guard passed. Both full binaries are 1,956,928 bytes, with 74,688 bytes free in each OTA app slot. **Hardware test pending.**
- **Merged to main:** [PR #3](https://github.com/ggrant111/wled-applehome/pull/3). No user HomeKit pairing/WLED NVS data changed.

## 2026-11-07 — v0.5 background HTTP worker candidate

- Implemented a single FreeRTOS network worker on ESP32 core 0. Routine WLED GETs and HomeKit/web/preset POSTs run off the main HomeSpan/Arduino loop, avoiding up-to-650ms synchronous WLED delays there.
- Fixed-length 24-command queue sends immediately from HAP/web callbacks, protects WLED identity with per-slot host-version checks, discards stale GETs when a newer command arrives, and reconciles state using a 450ms post-write read. Worker polls every ~8 seconds and backs off ~30 seconds on failures.
- Per-light health metrics now include pending commands, HTTP failures and dropped commands. A queued write is **not** a confirmed WLED execution; expect eventual state reconciliation and inspect diagnostics for errors.
- Added build-time gzip compression of the HTML portal: `web/portal.html` is the editable source, `tools/prepare_portal.py` generates `include/PortalGzip.generated.h`. Remove HTML String allocation at page request time and preserve reboot-safe CSRF recovery using authenticated status header.
- No HomeKit/NVS registry/administrator secret/partition changes.
- **CI PASSED:** [run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) — all host tests (including browser security and worker freshness), both D1 Mini and CYD firmware builds, and binary headroom gate passed. HTML compresses **13,425 → 5,001 bytes**. D1 Mini firmware.bin **1,957,408 bytes (74,208 free)**; CYD **1,957,424 (74,192 free)**. **Hardware remains unverified;** consult [Phase 5 test plan](PHASE5_TEST_PLAN.md).
- **Merged into main:** [PR #4](https://github.com/ggrant111/wled-applehome/pull/4). All previous NVS/HomeKit identities and admin configuration should persist, but no physical v0.5 upgrade has yet been tested.

## 2026-11-07 — v0.6 Apple Home Hue/Saturation code candidate

- Implemented HomeSpan Hue and Saturation on existing WLED bridged LightBulb services while keeping original AIDs and On/Brightness characteristic order.
- HomeKit hue/saturation converted to WLED RGB via portable pure C++ code, with explicit RGBW white=0 and targeted WLED main segment Solid effect only when user changes color; includes an explicit main segment ID to protect other selected segments. No segment arrangement, other segments, NVS, or preset switch definitions changed.
- WLED main-segment primary RGB read back during routine background polls and converted to HomeKit color characteristics; no attempt to show rapidly changing animation colors.
- Reused v0.5 nonblocking queue, combined HomeKit color/power/brightness updates and existing stale-response safeguards.
- Added native conversion/round-trip tests. **CI passed:** [run 42445302149](https://github.com/ggrant111/wled-applehome/actions/runs/42445302149) — HSV↔RGB tests, existing host/browser/security checks, D1 Mini/CYD firmware and OTA image-size check. D1 Mini image 1,545,312 / 2,031,616 bytes (486,304 free); CYD image 1,545,296 bytes (486,320 free). **On-device Apple Home color control, pairing persistence and OTA still require testing.** [Phase 6 test plan](PHASE6_TEST_PLAN.md).
- **Merged to main:** [PR #5](https://github.com/ggrant111/wled-applehome/pull/5). Keep the prior v0.5 binary available for rollback until the -Oz-optimized image is tested on hardware.

## 2026-11-07 — v0.6 real-device color and Siri confirmation

- **User confirmed:** Apple Home's new color controls successfully change WLED colors, and Siri voice color changes also work with installed v0.6 firmware.
- Only the color control and Siri path are verified by this report; do **not** infer successful WLED-to-Home color synchronization, preset effects, eight-device stress, OTA update, multi-segment isolation or long-run stability.
- Discussed native effects: HomeKit's LightBulb service does not expose an effects/palettes picker. Recommend user-created Apple Home scenes that activate selected existing preset switches, giving named scenes and Siri invocation while retaining standard light color controls. No firmware change required for user-created scenes.

## 2026-11-07 — v0.7 experimental TV-input effects selector candidate

- User approved an experiment to expose several WLED saved presets as one HomeKit input selector rather than individual momentary switches.
- Implemented an **opt-in** prototype that publishes one separate bridged Television service with linked InputSources (up to 5), persistent AID and preset IDs/names. This leaves normal HomeKit light, Siri colors, and existing preset switch AIDs untouched.
- One experimental controller, bound to one WLED light, is added from the authenticated web portal after choosing the light and saved preset. Subsequent additions append inputs until reaching five; user can remove the entire controller. Reboot required for the HomeKit accessory topology to refresh.
- Selection queues a saved WLED preset using v0.5 worker. TV Off sends WLED Off; TV On re-applies the last selected preset. WLED-originated saved preset selections are reconciled back to TV input when observed.
- HomeSpan's TV guide warns that Television behind a bridge may show a **generic icon**, and iOS has no supported Lighting effects list. Actual Apple Home UI on user's iPhone remains to be validated.
- **CI passed:** [run 42448093812](https://github.com/ggrant111/wled-applehome/actions/runs/42448093812) — new NVS/AID persistence test, existing host regressions, D1 Mini/CYD firmware builds, and OTA margin. Both `firmware.bin` files: 1,557,792/2,031,616 bytes (**473,824 bytes free**). **No physical Apple Home TV input UI or preset actions verified.** See [Phase 7 test plan](PHASE7_TEST_PLAN.md).
- **Merged into main:** [PR #6](https://github.com/ggrant111/wled-applehome/pull/6). Feature is still disabled until user adds a saved preset to an Effects Controller in the portal.

## 2026-11-07 — v0.7 hardware result and grouped-buttons alternative

- The user's iPhone screenshots confirm the experimental v0.7 TV-style accessory registered, and all four saved WLED names (Mammoth, Wednesday, Dodger Celebrate, Rainbow by Shelf) appear under Settings → Inputs. There is **no selectable input control** in the accessory detail view—only power. This fails the user-facing feature requirement.
- Added an opt-in **Convert to grouped buttons** alternative: one new HomeKit accessory with 1–5 named momentary Switch services, populated from the same previously selected preset names/IDs. Keep the old TV until the user explicitly converts. New AID avoids caching a different service under the TV AID. Existing RGB light, Siri color, and preset-switch AIDs unchanged.
- **CI verified:** [run 42449994004](https://github.com/ggrant111/wled-applehome/actions/runs/42449994004) — host test suite including TV-to-grouped NVS migration, D1 Mini/CYD builds, OTA size guard, each image 1,560,144 bytes/2,031,616 (471,472 free). **Physical iPhone grouped controls and effect actuation pending.** [Phase 7B test plan](PHASE7B_TEST_PLAN.md).
- **Merged into main:** [PR #7](https://github.com/ggrant111/wled-applehome/pull/7). This update adds no extra HomeKit accessory until the user explicitly selects conversion in portal, then reboots.

## 2026-11-07 — v0.8 grouped effect power and WLED segment diagnosis

- User confirms v0.7.1 **grouped preset switches work in Apple Home**, but applying a saved preset does not restore some WLED segment settings. The user also requests preset power-button behavior.
- Code investigation: all preset switch paths call `WledClient::applyPreset`, which queues the normal WLED `{"ps":id}` through the background worker. No bridge API is intentionally omitting `seg` during preset recall. WLED itself may lack saved `start`/`stop` segment bounds, depending on its **Save segment bounds** preset option. This must be inspected, not assumed.
- Added read-only per-preset inspection in the authenticated portal with streaming ArduinoJson filter, no WLED preset mutations. Reports total saved segment entries, boundaries present, and whether the preset appears to be a macro.
- Updated only grouped named effect switches to keep a selected preset On while the WLED `ps` and power match; selecting another preset turns the previous indicator Off after background reconciliation; toggling the current preset Off queues WLED Off. Standalone momentary switches stay unchanged. No AID/NVS/partition changes.
- **CI passed:** [run 42452061034](https://github.com/ggrant111/wled-applehome/actions/runs/42452061034) — new power/bounds host tests, existing regressions, D1 Mini/CYD firmware and OTA margin gate. Both images 1,564,800 bytes/2,031,616 slot, with 466,816 bytes free. **Physical segment restoration and selected On/Off HomeKit behavior still pending.** [Phase 8 checklist](PHASE8_TEST_PLAN.md).
- **Merged to main:** [PR #8](https://github.com/ggrant111/wled-applehome/pull/8). Existing light and grouped-effect accessory AIDs/characteristics preserved.

## 2026-11-07 — v0.8 physical verification and v0.9 proposed scope

- **User confirms** installed v0.8's grouped preset active On/Off indicators and direct-WLED-to-Apple-Home preset reconciliation work as previously described. This is direct hardware confirmation of that behavior; do not infer exhaustive tests of every WLED preset/segment, all devices, OTA pairing persistence, or 24h uptime.
- Earlier the user identified and corrected a problematic saved segment preset directly inside WLED. Exact segment restoration regression results were not supplied; continue to track as partially tested.
- Proposed Phase 9 / v0.9: expand the currently single grouped Effects Controller to separate, explicitly enrolled controllers for up to eight WLED lights, each tracking its own WLED `ps`/power and managing up to five named saved presets. Priority: migration without changes to live HomeKit accessory IDs or pairing; per-device admin portal; true isolation; host tests and real multi-device physical test.
- **No v0.9 firmware changes yet.** See [roadmap](ROADMAP.md).

## 2026-11-07 — v0.9 per-WLED grouped effects candidate

- Began v0.9 on explicit user request. Existing v0.8 On/Off stateful presets and WLED-to-Home preset sync were physically confirmed in preceding messages.
- Changed the single-controller registry into eight opt-in groups, indexed by the stable WLED light AID. **Legacy v0.7/v0.8 `wled-fx` storage is used in place, not migrated/reallocated**: the user's existing grouped Effects accessory keeps its same HomeKit identity, preset IDs, names and service order after OTA.
- New groups use a separate NVS namespace `wled-fx9` with a spare slot for crash-resilient structural edits. All new groups use named grouped Switch controls, not Television Inputs. WLED state polling and queued `{"ps":id}` commands remain isolated per WLED device.
- Portal supports adding presets to the selected WLED, renaming a control without changing identity, safely removing/reordering individual preset controls by explicitly rebuilding **only that group** under a new AID, removing a complete group, and loading independent saved preset catalogs.
- Tests added for eight-group capacity, NVS AID preservation across reboot, physical group isolation mapping, rename, remove/reorder AID change, interrupted rebuild recovery and legacy TV conversion. New endpoints keep Digest+CSRF protection and user-entered labels use HTML escaping.
- **CI passed:** [run 42454610880](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880): eight-controller NVS and legacy-AID tests, independent groups, structural edit recovery, browser/CSRF tests, D1 Mini and CYD OTA builds and 70KB minimum flash headroom. D1 Mini image 1,574,848 bytes, 456,768 bytes free; CYD image 1,574,832 bytes, 456,784 bytes free.
- **Merged to main:** [PR #9](https://github.com/ggrant111/wled-applehome/pull/9) at commit a299c2c. **Physical validation with two or more independent WLED controllers and effects groups remains pending.** [Phase 9 test plan](PHASE9_TEST_PLAN.md).

## 2026-11-08 — v0.9.1 WLED playlist support implementation

- User confirmed **second WLED effects group and its presets** are live and working in Apple Home (v0.9 multi-group physical validation for onboarding and triggering). Full eight-device stress/OTA pairing still pending.
- User requests adding saved WLED playlists as Effects selections. Verified WLED official JSON API: playlist resources are saved by numeric IDs next to ordinary presets, and `{"ps":playlistId}` activates them. During playback, WLED `pl` is the playlist's saved ID while `ps` changes to child presets; using `ps` alone would incorrectly deactivate the playlist button.
- Code candidate: filtered catalog now returns both names and a playlist-vs-preset type; per-device dashboard labels both types. Grouped inputs persist a playlist-kind flag in existing (legacy/new) NVS namespaces, default false for original presets; selecting a preexisting playlist input updates its kind without changing any AID.
- WLED state poll and HomeKit switch synchronizer use playlist ID as authoritative while playlist is active. Ordinary preset buttons ignore playlist child steps. Clicking any playlist sends the same standard saved resource activation command; WLED itself plays/cycles the playlist.
- Added native tests for state semantics, migrations/reclassification and type preserved across group reorder; browser catalog/type contract. **CI passed:** [run 42505219721](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721) — native playlist and preset-state tests, NVS persistence/reclassification/reorder and browser label validation, D1 Mini/CYD firmware + 70KB minimum OTA margin. D1 Mini image 1,576,656 bytes with 454,960 free; CYD 1,576,640 with 454,976 free.
- **Merged to main:** [PR #10](https://github.com/ggrant111/wled-applehome/pull/10) at 71b0ebb. **Real playlist playback and iPhone controls not yet hardware verified.** See [Phase 9 playlist test checklist](PHASE9_PLAYLIST_TEST_PLAN.md).

## 2026-11-08 — v0.9.2 local portal UX overhaul

- User requested a cleaner, more intuitive UX/UI for the WLED Apple Home web management portal.
- Reorganized the former two-column page into five distinct workflows with a responsive nav, overview and real-time WLED state, clear per-device presets/playlists management, standalone momentary switches, and bridge settings diagnostics.
- The Effects view now shows the active preset/playlist for the selected WLED light, descriptive resource labels, direct Run controls, a real rename modal, and explicit warnings about changing a group's HomeKit ID on remove/reorder. The controls remain bound to existing HomeKit accessories and NVS identities.
- Added a narrow, authenticated and CSRF-protected WLED Effects Run API that validates group/resource membership before queueing the existing saved preset/playlist. No changes to WLED presets or HomeSpan pairing.
- Added client-side VM tests for view switching, status rendering, playlists, safe HTML labels, POST authentication semantics and pending restart; existing browser security tests remain. The page is still a static, gzip-compressed single HTML asset.
- **CI/build/device validation pending.** [v0.9.2 checklist](PHASE9_PORTAL_UX_TEST_PLAN.md).

## Next actions

1. **Done:** v0.4 host tests, migration regression, D1 Mini/CYD builds and OTA size guard passed.
2. **Done (user verified):** v0.4 web portal works and discovers/enrolls WLED controllers in Apple Home.
3. **Next user test:** Verify v0.4.1 preset-add success and trigger before upgrading to v0.5; then run the Phase 5 background worker hardware test plan without clearing NVS.
4. Test independent on/off/brightness on multiple devices and restart/power-cycle retention of all existing pairing/IDs.
5. Validate OTA safely, then prioritize nonblocking WLED networking, monitoring, flash-budget optimization and 24h soak.
