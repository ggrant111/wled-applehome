# WLED Apple Home Bridge — Roadmap

> **Source of truth:** Update this file whenever a milestone changes, and record decisions in `docs/DECISIONS.md`. Do not mark hardware or HomeKit pairing tests complete from a successful compile alone.
>
> **Updated:** 2026-11-07 · **Active milestone:** v0.9.3 screenshot-driven dashboard usability polish (CI passed/merged; user desktop/mobile visual check pending) · **Primary target:** ESP32 D1 Mini · **Secondary target:** ESP32-2432S028R CYD (headless mode first).

## Goal

An always-on, **local-only** ESP32 bridge that makes existing WLED devices appear as independent lights in Apple Home, without changing their WLED firmware. Power, brightness, and selected presets must be controllable from Apple Home/Siri while preserving WLED device settings and supporting other WLED clients.

## Definition of MVP (v1.0)

- [ ] Bridge pairs with Apple Home using a device-specific setup code.
- [ ] At least **8 WLED devices** can be configured and exposed as **separate, stable bridged light accessories**.
- [ ] Per-device on/off and 1–100% brightness work from Apple Home.
- [ ] Selected WLED presets are exposed through **optional trigger switches** or scene helpers (not every preset).
- [ ] State changes originating in WLED/other clients are reflected back in Home.
- [ ] Network dropouts, device reboot, and ESP32 restart recover automatically.
- [ ] WLED IP/hostname changes can be corrected without losing HomeKit pairings.
- [ ] Local configuration UI, meaningful logs, and repeatable setup instructions.
- [ ] OTA update path that preserves pairing, stable accessory IDs, and device config.
- [ ] Successful PlatformIO firmware build and on-device smoke tests for supported hardware.

## Phases

### Phase 1 — v0.1: HomeKit + one WLED proof of concept — **CODE COMPLETE / HARDWARE PENDING**

- [x] P1.1 PlatformIO firmware for ESP32 D1 Mini on Arduino-ESP32 3.3+ with HomeSpan 2.1.8.
- [x] P1.2 CYD ESP32-2432S028R board build, initially without display/touch drivers.
- [x] P1.3 One HomeKit *bridged* LightBulb, containing On and Brightness characteristics.
- [x] P1.4 WLED JSON API GET state and POST on/off/brightness, with status checks.
- [x] P1.5 Configure a WLED IPv4 address over serial CLI and persist it in ESP32 NVS.
- [x] P1.6 Poll WLED for external changes; notify HomeKit only on changes.
- [x] P1.7 GitHub Actions build both firmware configurations and run host-only checks.
- [ ] P1.8 **Physical verification:** iPhone pairs with bridge; on/off and brightness control a real WLED device.
- [ ] P1.9 **Physical verification:** power-cycle the ESP32 and verify the pairing remains.

**Acceptance gate:** P1.1–P1.9 passed, with actual device evidence. Compiling is not sufficient.

### Phase 2 — v0.2: Discovery and stable multi-device bridge — **PORTAL DISCOVERY / ADD-TO-HOME HARDWARE-VERIFIED; FULL TESTS PENDING**

- [x] **Code delivered:** On-demand mDNS scan of `_wled._tcp` and HTTP `/json/info` verification. **Web portal discovery user-verified (2026-11-07); serial CLI scan remains unverified.**
- [x] **Code delivered:** CLI manual add/remove and de-duplicate by normalized hardware MAC; rename remains planned.
- [ ] Device renaming (separate from WLED's own name) and polished web configuration.
- [x] **Code delivered:** NVS-backed monotonic HomeKit AIDs; original light retains AID=2, without reallocating removed AIDs.
- [x] **Code delivered:** Per-light WLED polling stagger, timeout cap, and 30s offline backoff.
- [ ] Nonblocking WLED networking, exposed device-health UI, and advanced reconnection behavior.
- [x] **Code delivered:** Eight WLED registry slots and bridged accessory creation.
- [ ] Hardware test with eight WLED devices and evaluate memory/latency.
- [ ] Hardware verify both add **and removal**, reboot, and HomeKit pairing continuity. **User confirmed adding new WLED devices via portal into Apple Home (2026-11-07); removal and identity persistence not yet reported.**
- [ ] Test reboot, IP changes, reordering, and missing devices.

### Phase 3 — v0.3: Presets and state sync — **CI PASSED / HARDWARE TESTS PENDING**

- [x] **Code implemented:** On-demand read of `/presets.json` by light AID; streaming filtered JSON retains only names (avoids allocating full WLED preset configurations). Hardware discovery test pending.
- [x] **Code implemented:** Explicitly enroll up to 12 preset triggers (`@p<lightAID>:<presetId>`) as HomeKit momentary Switch accessories with stable, never-reused AIDs. Reboot to advertise. Hardware behavior pending.
- [x] **Code implemented:** After successful HTTP POST, refresh WLED power/brightness after a 450ms settle delay; no state overwrite during that delay, and normal periodic external-change polling retained. Hardware timing/recovery pending.
- [ ] Hardware verify observation of WLED/GlowDeck/external state and selected preset changes on multiple lights; polling-based sync code is in place.
- [ ] WebSocket-assisted live updates / fully nonblocking transport (deferred; 8s periodic polling retained as fallback).

### Phase 4 — v0.4/1.0: Web setup, OTA, reliability and release — **WEB DISCOVERY / HOME ENROLLMENT VERIFIED; REMAINING GATES OPEN**

- [x] **Code delivered:** Password-gated local web manager on port 8080 (device list, status, 8-light discovery/add/remove, preset selection, controls, diagnostics and reboot). **Portal loads and discovers/enrolls WLED devices into Apple Home on user hardware (2026-11-07); the rest of its functions remain unverified.**
- [ ] Test real browser Digest login, CSRF protections, reconnection and Apple Home coexistence; optimize any WebServer-induced HomeKit latency.
- [x] **Code delivered:** HomeSpan built-in password-protected ArduinoOTA enabled only after serial-set administrator password (no default OTA password); OTA partition validation delayed 60s with `SpanRollback.h`. **OTA hardware recovery/pairing test pending.**
- [ ] Validate OTA with firmware image and ensure pairing, Wi-Fi and WLED registry survive; test rollback after invalid/new image.
- [x] **Code delivered:** Existing per-device 30s offline backoff; portal begins only once Wi-Fi becomes connected; read-only cached health endpoint, no additional WLED polling on dashboard refresh.
- [ ] Nonblocking WLED polling, improved network recovery and 24h soak / reboot/power loss tests.
- [x] **Code delivered:** Serial-only admin-secret provisioning, Digest login, CSRF tokens for POST mutations; portal and OTA both disabled until provisioned. No default credentials or embedded secrets.
- [ ] Hardware security review, no open router forwarding, authentication rate-limit/timeouts, threat-model/credentials lifecycle review. Note local HTTP is not TLS.
- [x] **Code delivered:** Release documentation, web/OTA setup guide, GitHub CI firmware artifacts and binary OTA-partition headroom check.
- [ ] Configuration export/import with secret redaction, release changelog and versioned release tagging.
- [ ] Final **8-device and 24-hour run verification**; phase must remain open until real hardware evidence.

### Phase 4B — next release priorities (acceptance gated)

1. **Preset controls / end-to-end state:** User selects a real saved WLED preset in the portal; it appears in Apple Home after bridge restart, activates the correct WLED preset, resets for another trigger, and power/brightness stay synchronized. Confirm independent power/brightness changes for at least two enrolled lights.
2. **Safe OTA:** Upload a compatible OTA image using HomeSpan's authenticated OTA service; demonstrate that HomeKit pairing, device identities (including AID=2), WLED IPs, preset switches and admin settings survive. Confirm the delayed sketch-valid marker and test rollback only on a safe spare/test device.
3. **Networking and stability:** Replace/limit synchronous WLED HTTP calls so an offline light cannot impair all Apple Home lights; test missing Wi-Fi, disconnected WLED, state-update conflicts, discovery latency and 24-hour continuous uptime.
4. **Capacity management:** Optimize firmware footprint and quantify RAM/flash at 1, 3, and 8 WLED devices; preserve a substantial safety margin for OTA. On classic ESP32 4MB the current 76KB spare is a blocker for major added features.
5. **Production polish:** Security review, diagnostic logs, backup/export with sensitive data redacted, versioned release, and documented recovery instructions.

**User-verified hardware evidence:** Web manager operational; discovered WLED controllers using portal and successfully added lights to Apple Home. Do not infer tests of every light's independent power/brightness, pairing across reboot, preset activation, removal, or OTA.

### Phase 5 / v0.5 — Background WLED networking & firmware footprint — **CI PASSED / HARDWARE VERIFICATION PENDING**

- [x] **Code:** Moved routine WLED polling and HomeKit/portal light + preset JSON POSTs into one bounded FreeRTOS task on ESP32 core 0. The HomeSpan main loop now queues commands without synchronously waiting on WLED HTTP.
- [x] **Code:** Fixed-length command queue (24 commands), enqueue rejection on saturation, per-light queue depth, HTTP failures, drop counts, online state and last-success timestamps surfaced in cached portal status.
- [x] **Code:** Queue commands ahead of polling; stagger periodic GETs by device, use 8-second healthy poll, 30-second offline backoff, and 450ms post-write reconciliation.
- [x] **Code:** In-flight GETs are invalidated when a newer command or host/IP update arrives; only the main loop updates HomeKit characteristics from worker snapshots.
- [x] **Code:** Portal page now precompressed to gzip at build time, served directly from flash with no 13KB HTML String allocation; authenticated status endpoint supplies the live CSRF token.
- [x] **Code:** Added native freshness/scheduler checks, compressed asset tests and existing browser CSRF regression to CI.
- [x] **CI PASSED:** [run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) — host C++/browser/asset tests, both firmware builds, 70KB OTA headroom guard.
- [ ] **Hardware:** Validate multiple connected WLED lights, offline device isolation, commands queued/failed, preset switch timing, HomeKit state reconciliation, Wi-Fi disconnect/reconnect, 24-hour soak and stability.
- [ ] On-demand discovery, WLED preset catalog and manual IP verification still use bounded **blocking** calls in the portal/serial action path; make these async later if real latency tests justify.
- [ ] Continue measuring heap and browser performance, and evaluate queuing/coalescing latest brightness changes if heavy automation fills the queue.

**Behavior change:** A successful HomeKit/portal write means **accepted by the bridge command queue**, *not* confirmed executed by WLED. Failed WLED POSTs appear as per-light errors/health diagnostics and are reconciled on the next successful poll. Queue saturation rejects a new command rather than blocking HomeKit.

**Preservation:** No NVS schema, HomeKit AID, password, preset registry, partition table or pairing change in v0.5. Upgrade without erase.

### Phase 6 / v0.6 — Apple Home full-color light controls — **CI PASSED / HARDWARE PENDING**

- [x] **Code:** Append Hue (0–360 degrees) and Saturation (0–100%) HomeSpan characteristics to existing light services after On and Brightness, preserving original service/characteristic ordering and accessory AIDs.
- [x] **Code:** Convert HomeKit HSV (value remains under the independent Brightness characteristic) to 8-bit WLED RGB primary color on the **main WLED segment** using a targeted JSON `seg` object with an explicit main-segment ID, leaving other segment layout untouched. Explicit color selection sets the main segment to Solid effect (`fx:0`) to ensure the color is visible; restores effects through normal preset switches.
- [x] **Code:** Combine HomeKit power/brightness/color edits in one queued worker command. Default WLED white channel to 0 on RGB color choice, without changing other segment colors.
- [x] **Code:** Parse current main-segment RGB primary from WLED's `/json/state`, publish Hue/Saturation to HomeKit after a fresh background poll. Never claim a color if WLED doesn't return an RGB primary.
- [x] **Code:** Add portable HSV↔RGB tests for the six color-wheel anchors, grayscale, rounding and representative round-trips.
- [x] **CI PASSED:** [run 42445302149](https://github.com/ggrant111/wled-applehome/actions/runs/42445302149) — all host tests, D1 Mini/CYD firmware builds and OTA 70KB headroom check.
- [x] **User verified (2026-11-07):** New Apple Home color controls change WLED colors and work through Siri.
- [ ] **Hardware still pending:** Specific red/green/blue/white and RGBW edge cases; reverse color sync after direct WLED edits/presets; multi-segment isolation; full pairing/preset AID persistence and uptime.
- [ ] **Future:** Color temperature (actual CCT/white-channel handling), group/segment selection and WLED effects, palettes, speed and intensity in a separate advanced web interface; not represented as native HomeKit effects.

**Limitations:** Hue/Saturation represent the **main segment's configured primary RGB color**, not the instantaneous output color of each LED when an animation or palette runs. Changing color deliberately selects Solid mode for the main segment; use an existing WLED preset switch to restore your animation. Existing HomeKit pairing and registry are not erased or renumbered.

### Apple Home effect selection — supported UI and follow-on plan

Apple Home's standard LightBulb characteristic list includes On, Brightness, Hue, Saturation and ColorTemperature, **not WLED Effect, Palette, Speed or Intensity**. Custom characteristics cannot add a native effect picker to Apple's built-in light tile.

- **Recommended UX:** Keep HomeKit preset trigger switches internally for WLED JSON preset actions, but let users organize a small number into named **Apple Home scenes** (e.g., Fireplace, Rainbow, Movie Night). Scenes expose a natural Home app tile and Siri invocation without hijacking the color wheel.
- **Future software exploration (not implemented):** Optional scene-helper setup documentation and clearer preset switch names; possible Apple Shortcuts integration. Apple Home scenes must be configured by the user in Apple's Home app; the ESP32/HomeSpan bridge cannot independently create scenes in the user's Apple Home database.
- **Avoid:** Mapping special colors or brightness values to effects. That undermines basic light control and breaks Siri/color semantics. Alternative HomeKit service types won't place WLED effects inside the native LightBulb controls.

### Phase 7 / v0.7 — Experimental HomeKit Effects Controller using Television inputs

- [x] **Candidate code:** Create exactly ONE **opt-in** TV-style HomeKit accessory linked to ONE existing WLED light, initially with up to FIVE explicitly selected **saved** WLED presets.
- [x] **Candidate code:** A list of `Service::InputSource` items with `ConfiguredName`, `Identifier` (1–5), and `IsConfigured(1)`, linked to `Service::Television` with `ActiveIdentifier`, `Active`, and deterministic `DisplayOrder`.
- [x] **Candidate code:** Selecting an input queues the chosen WLED saved preset using the existing nonblocking WLED worker; TV accessory Off turns off the light, On reapplies last selected preset. WLED preset changes can update selected input after state polling.
- [x] **Candidate code:** New `wled-fx` NVS namespace, allocated AID from existing non-recycled allocator, independent from other WLED lights and preset switches; controller absent unless user opts in. Portal can add selected presets / remove controller; changes require bridge reboot.
- [x] **Candidate code:** Native NVS migration/identity/slot-limit tests and portal web security checks; roadmap and test plan.
- [x] **CI passed:** [run 42448093812](https://github.com/ggrant111/wled-applehome/actions/runs/42448093812) — new NVS/AID host tests, existing regression suite, both ESP32 targets, and OTA size guards.
- [ ] **Hardware:** Verify Apple Home displays selector for a **bridged** Television service (HomeSpan documents generic icon in bridged cases). Verify input names, five selections, actual WLED preset actions, HomeKit light state sync, restart/pairing persistence, removing selector without losing lights.
- [ ] **Future:** If Apple Home displays input selection reliably, expand from one prototype to optional controller per light, better refresh handling, and scene integration.

**Important:** Apple Home **does not have a WLED effects API**. This repurposes TV input selection and may appear under Television/AV rather than Lighting, with TV power semantics. No guarantee that the iOS Home app accepts a TV service behind the bridge. The existing light tile is NOT modified; user must enroll an effects controller in the web portal after updating.

### Phase 7B / v0.7.1 — Grouped named effect buttons, fallback from bridged TV

- [x] **User hardware feedback:** v0.7 Effects Controller appears in Apple Home with four correct WLED saved preset names in **Settings → Inputs**, but its main control panel shows ONLY power; no usable input selector. This fails the native effects input UI acceptance gate.
- [x] **Research:** HomeSpan Television docs describe input picker on an ordinary TV; HomeSpan's developer has shown that bridged TV icons become generic. Their [grouped Switch service example](https://github.com/HomeSpan/HomeSpan/discussions/485) demonstrates several named, tappable Switch controls within one HomeKit accessory.
- [x] **Code candidate:** Add a manual, authenticated/CSRF-protected portal action **Convert to grouped buttons**. The existing TV accessory stays unchanged unless the administrator chooses conversion.
- [x] **Code candidate:** Preserve up to five configured WLED saved preset names/IDs and the original WLED light, while allocating a **fresh HomeKit accessory AID** for a multi-`Switch` accessory. Each grouped switch queues a WLED preset and resets after 250ms.
- [x] **CI passed:** [run 42449994004](https://github.com/ggrant111/wled-applehome/actions/runs/42449994004) — NVS/AID conversion integration, existing host/browser tests, D1 Mini/CYD firmware, 70KB OTA space guard. Both images 1,560,144 bytes with 471,472 bytes free.
- [ ] **Hardware:** Verify one grouped Home tile exposes separately named controls, preset activation, light/Siri continuity, restart/pairing, and no change to existing independent switches.
- [ ] **Fallback:** Native Apple Home scenes and existing separate preset switches, if multi-switch presentation disappoints.

### Phase 8 / v0.8 — Segment restoration diagnosis and preset power selection

- [x] **User verified:** v0.7.1 grouped named effect switches in Apple Home work; hardware success supersedes previous hardware pending status, with unresolved segment-layout restoration behavior.
- [x] **Research:** WLED's JSON `{"ps":id}` is the normal full saved-preset activation request (the bridge already sends it). Segment bounds are only restored if WLED saved `start` and `stop` geometry. WLED's own guide instructs enabling **Use current state** and **Save segment bounds** when saving a preset.
- [x] **Code candidate:** Add a **read-only** authenticated API and portal inspection for the chosen WLED preset, stream-filtering only the selected preset's `seg[].start`/`stop`, and reporting segment counts with saved bounds. Diagnose missing preset data without rewriting WLED `presets.json`.
- [x] **Code candidate:** Grouped effect controls persist On for the active saved WLED preset; activating a different effect synchronizes other On indicators to Off after the background worker's authoritative WLED GET; turning the active effect Off powers WLED Off. Keep standalone switch actions momentary. Preserve all existing paired AIDs and HomeKit service types.
- [x] **CI passed:** [run 42452061034](https://github.com/ggrant111/wled-applehome/actions/runs/42452061034) — host behavior, browser/CSRF, both ESP32 images and minimum OTA headroom. Both images 1,564,800 bytes; 466,816 bytes free.
- [x] **User-verified 2026-11-07:** v0.8 grouped effect power/selection and WLED-to-Home active-preset synchronization behaved as described on the user's Apple Home setup.
- [ ] **Other hardware checks pending:** Specific saved segment-bounds inspection results and all preset segment layout variations; independent stress test across multiple lights, OTA continuity and Siri regression, long uptime.
- [ ] **Unresolved:** If preset includes bounds and direct WLED recall fully restores segments but Apple Home doesn't, capture WLED `/json/state` before/after plus logs. Do not force-write segment geometry without clear evidence; WLED's native preset engine owns it.

**Home UI limitation:** Existing grouped accessories are standard HomeKit Switch services; making On/Off persistent does **not** force an Apple-designed TV-style large round power picker for each service. A new LightBulb or Outlet service subtype would change grouping and has not been tested.

### Phase 9 / v0.9 — Multi-WLED grouped effects (MERGED / CI PASSED / HARDWARE PENDING)

**Goal:** Extend the physically working v0.8 grouped-effects design from one selected WLED device to separate opt-in grouped-effects accessories for up to all eight registered WLED devices.

1. **Registry design:** Evolve one global `wled-fx` controller into a collection keyed by *existing light AID* (not discovery list position). Migrate the existing live grouped controller without changing its AID, saved preset names, mapping, or position. Separate new effect accessories get monotonically allocated AIDs; never recycle former TV AIDs.
2. **HomeKit:** Keep grouped `Service::Switch` controls and v0.8 On/Off behavior. Each group observes its corresponding WLED light's authoritative power and preset ID independently; changing one light must not affect the other groups.
3. **Per-light management:** Portal shows grouped presets beneath each WLED device. Allow adding selected saved presets, removing selected entries, changing display names, and reordering display when safe. Preserve HomeKit characteristic/service identifiers for existing entries; **do not silently repack/reassign switch IIDs on a reboot**. If HomeKit cannot safely reorder the actual services, offer portal-only display order or require explicit recreation with warning.
4. **Safety and tests:** Plan registry migration tests from v0.7 TV and v0.7.1/v0.8 grouped states, duplicate AID/preset rejection, removal/restore, reboot persistence, NVS power-failure conditions, mixed offline/online controllers, queue saturation, memory budget, and both ESP32 OTA image builds.
5. **Acceptance:** Physical verification with at least two separate WLED devices, each with independent preset groups, switching from Home and WLED directly. Confirm no lost pairing, original color and Siri functionality, and no accidental cross-device triggering.

**Scope exclusions:** Do not automatically add effects groups for every discovered device, rewrite WLED saved presets or segments, swap a working light accessory's service type, or erase NVS. Retain five selected presets per light initially; reassess limit based on actual accessory behavior/ESP32 memory.

**Implementation:** 
- [x] Per-device registry supports eight active effects groups. Existing v0.7/v0.8 groups remain in their original `wled-fx` namespace and retain their exact original AID, slot order, preset names and IDs. New groups are opt-in and stored separately in `wled-fx9`, with up to 9 persistent modern slots for eight active groups and one spare transactional rebuild slot.
- [x] New effects groups begin as named, stateful grouped Switch services; each group attaches to its *own* WLED light and reflects its own current `ps`/power via existing poll logic. Existing legacy Television conversion still supported.
- [x] Dashboard shows each light's independent effects group and offers per-light saved preset loading, add, rename, remove, move up/down, and whole-group removal. New controller creation and appending a preset preserve all other AIDs. Rename preserves service identity; restarts are required for Home app to refresh names.
- [x] Remove or reorder an individual effect **changes that group's HomeKit service topology** and therefore requires an explicit warning and new non-reusable AID for that **one group** (not the WLED light). Existing Home scenes or automations targeting that group might need to be updated. A new group is persisted BEFORE retiring the previous AID. A reboot after an interrupted swap favors the larger/newer AID and retires the superseded copy.
- [x] Light removal now removes only effects groups bound to that light; unrelated groups are unchanged.
- [x] Host integration test covers v0.8 legacy migration, eight groups, independent state mapping, name persistence, per-group rebuild, NVS crash recovery, legacy TV conversion and duplicate rejection; JS endpoint contract enforces CSRF and escaped labels.
- [x] **CI passed:** [run 42454610880](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880) — eight-device registry and v0.8 NVS migration, browser/CSRF, all existing host tests, both ESP32 firmware builds and >=70KB OTA flash budget. D1 Mini 1,574,848 bytes / 2,031,616 (**456,768 free**); CYD 1,574,832 bytes (**456,784 free**).
- [ ] Hardware gate: physically test **two WLED devices**, each with independent grouped effects; verify active state from WLED, editing, reboot/pairing persistence, offline WLED isolation and no impact on Siri/color controls. Repeat across more devices as available.

**Version handling:** v0.9 still uses the exact existing LightBulb and PresetSwitch HAP services with unchanged AIDs; only specifically opted-in new Effects groups get new accessories. Removing/reordering buttons is the only operation that intentionally rebuilds that group's AID after confirmation. No WLED presets, segments, colors or WLED firmware are modified by management operations.

**Status:** [PR #9](https://github.com/ggrant111/wled-applehome/pull/9) merged to main and all CI gates passed. Multi-device physical HomeKit tests remain outstanding.

### Phase 9B / v0.9.1 — Saved playlists as HomeKit effect selections

- [x] **API verified:** WLED stores saved playlist objects alongside presets in `/presets.json`, and triggers either using `{"ps":ID}`. Running playlist status uses `pl` (playlist resource ID); `ps` changes as child presets advance. WLED playback timing/durations/transitions remain handled by WLED.
- [x] **Code candidate:** Filter-fetch catalog names and playlist IDs, expose `kind: preset|playlist` through the existing authenticated portal. Mixed preset/playlist selector on each WLED device; no duplicate controller or custom HomeKit service.
- [x] **Code candidate:** Persist `isPlaylist` as per-input metadata in legacy and new NVS namespaces, defaulting to normal preset for existing keys. Updating the same existing resource from Preset to Playlist is a metadata-only change and keeps its HomeKit AID/IIDs.
- [x] **Code candidate:** Parse `pl` in WLED polling. HomeKit playlist Switch remains On while WLED reports playlist active even as child `ps` changes; ordinary preset switches stay Off during playlist playback. Playlist finish, stop, power-off, and direct WLED playlist changes synchronize to Apple Home using existing worker/poll loop.
- [x] **Code candidate:** Native tests for playlist multi-step state and NVS persistence/migration, multi-device group reorder preserving playlist type, and browser labels/type contract. Existing standalone saved preset switches remain momentary and may trigger playlists via the same `ps` API.
- [x] **CI passed:** [run 42505219721](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721) — playlist-state/NVS migration and JS contract host tests, existing regressions, both ESP32 builds and 70KB minimum OTA headroom. D1 Mini image **1,576,656 bytes**, **454,960 free**; CYD **1,576,640 bytes**, **454,976 free**.
- [ ] **Hardware:** Save a WLED playlist on device, add it alongside a normal preset, verify continuous On during multiple playlist steps and Off when finished, select playlist from WLED directly and watch Home sync; test two independent WLED groups and preserve existing lights/pairing.

**Merge:** [PR #10](https://github.com/ggrant111/wled-applehome/pull/10) merged into `main`. **Hardware acceptance pending**: actual WLED playlist progression and selected HomeKit On indicators must be verified.

**Non-goals:** The bridge does not build/edit playlist sequences, duration, transition, shuffle, or repeats; those stay in WLED. No HomeKit effect picker beyond the already physically working named grouped Switch controls.

### Phase 9C / v0.9.2 — Redesigned local dashboard

- [x] **Code candidate:** Replace scrolling two-column admin page with responsive five-section workspace (Overview, WLED Lights, Effects & Playlists, Preset Switches, Bridge Settings). No new dependencies and no remote assets.
- [x] **Code candidate:** Group per-light Effects management around one WLED selector, active playlist/preset badges, named favorites, clear Add/Run/Rename/Move/Remove actions; highlight state using existing `ps`/`pl` status. Separate momentary switches to avoid confusing them with persistent favorites.
- [x] **Code candidate:** Dedicated light health, power/brightness, discovery and enrollment; device-count and memory summaries; firmware, RSSI, uptime, heap, OTA instructions and clear restart affordance.
- [x] **Code candidate:** New Digest + CSRF protected `POST /api/effects/run` validates an existing Effects-group AID and enrolled resource before queuing a normal WLED `{"ps":id}` request. Does not create an Apple Home accessory or modify WLED presets/playlists/segments.
- [x] **Code candidate:** Accessible status feedback and native rename dialog; responsive phone navigation and focus states; prevent resetting a focused brightness input during 10-second status refresh; selected WLED device persists across refresh; refreshed CSRF token retry remains unchanged.
- [x] **Code candidate:** Add VM-backed browser regression tests for five-page navigation, device/playlist rendering and HTML escaping, command POST/CSRF, pending-restart banner and quick navigation. Static endpoint/authorization test updated for Run.
- [x] **CI passed:** [run 42506904065](https://github.com/ggrant111/wled-applehome/actions/runs/42506904065) — all JS/host tests including navigation and CSRF, gzip portal, D1 Mini/CYD firmware builds and OTA size guards. Both firmware images 1,584,544 bytes; 447,072 bytes free per OTA slot.
- [ ] **Hardware/UI:** Check dashboard on iPhone and desktop, grouped playlist labels, device light selection, inline commands, rename and safe edit warnings, startup reconnect and two WLED devices. Existing HomeKit pairing and AIDs must remain untouched.

**Risk controls:** No new NVS schema, WLED controller modification, HomeKit accessory topology or OTA service change. All POSTs still require Digest + per-boot CSRF. UI-only change is except the explicitly scoped Run endpoint. Do not claim hardware validation from browser unit tests.

### Phase 9D / v0.9.3 — Screenshot-driven web UI polish

- [x] **Actual desktop observation:** User confirmed v0.9.2 Overview and Effects screens render on bridge. Overview shows five enrolled devices, five online, two groups, one standalone preset switch, and ~132 KB free ESP32 heap. Effects screenshot shows two groups (4/5 each), while a selected unconfigured light occupies the main panel. Desktop content is visually narrow with wide unused margins and redundant device selector plus tabs.
- [x] **Code candidate:** Widen desktop workspace to 1460 px, slightly enlarge type, preserve narrow/mobile responsive layout.
- [x] **Code candidate:** Use single rich per-device group tabs on desktop; use one device dropdown on mobile. Default to first **existing** Effects group only on initial page load, then preserve explicit choices through periodic status updates.
- [x] **Code candidate:** Keep current named effects front and center; move adding a saved preset or playlist behind an expandable section, automatically opened for a light without a group. Maintain visible AID-change warnings for destructive edits.
- [x] **Code candidate:** Disable Add until WLED catalog for the selected light is explicitly loaded; changing light invalidates stale catalog. Distinguish untouched catalog from WLED with no saved resources.
- [x] **Code candidate:** Extend existing JS VM browser tests to verify default configured group, tab/label escaping, catalog lifecycle, cross-device safety and unchanged CSRF/Run behavior.
- [x] **CI passed:** [run 42508487092](https://github.com/ggrant111/wled-applehome/actions/runs/42508487092) — all host/VM browser regression and CSRF/gzip tests, D1 Mini and CYD firmware builds and OTA size guard. Both images 1,585,216 bytes with 446,400 bytes free in each 2,031,616-byte OTA slot.
- [ ] **User UI:** Confirm wider desktop canvas, first configured group shown by default, correct mobile dropdown and expandable Add flow on actual bridge; existing Apple Home lights and pairing unaffected.

**No underlying HomeKit/NVS/REST route changes**; v0.9.3 is a dashboard-only refinement with an app/firmware version bump.

## After MVP

| Version | Focus | Status |
| --- | --- | --- |
| v1.1 | Polished web dashboard, log viewer, per-device diagnostics | Planned |
| v1.2 | Multi-WLED scenes, virtual groups, master brightness/on-off | Planned |
| v1.3 | CYD touchscreen dashboard with light tiles and controls | Planned |
| v1.4 | Advanced segment mapping, palettes, effects, speed/intensity, and color temperature | Planned (basic Hue/Saturation in v0.6) |
| v1.5 | WebSocket-led event sync and reliability/performance improvements | Planned |
| v1.6 | Health monitoring, configuration backup and safe rollback | Planned |
| v2.0 | Research Matter bridge for Apple Home/Google Home/Alexa | Research |

### Phase 3 feature boundaries

- The bridge does **not** create or edit any WLED presets; `@c<AID>` reads names and `@p<AID>:<ID>` exposes a selected saved preset as a HomeKit switch. Only `{"ps":<ID>}` is sent when invoked.
- Up to **12 selected preset switches total**, independent of the 8 WLED-light limit; no preset tiles are added automatically.
- Existing light AIDs, HomeKit pairing, and WLED NVS registry are preserved; **new switches get fresh AIDs from the same allocator used for lights**.
- Instant WebSocket state sync is **still open** (a local portal now exists in Phase 4A). Large WLED catalog reads are on-demand and may temporarily block HomeKit while the request finishes.
- Portal-based WLED discovery and Apple Home enrollment are user-verified; other v0.2 and v0.3 tests remain open. Do not mark those phases released solely from CI.

## v0.4.1 — Portal 403 CSRF reboot recovery fix

- [x] Root cause identified: the portal generates a new random CSRF token on each ESP32 boot, while an already-open browser tab keeps the old one after users restart to publish newly added Apple Home accessories.
- [x] Dashboard now refreshes a boot token from authenticated same-origin `/api/status` responses and retries **one** rejected POST after a 403 only when a different valid token is returned.
- [x] Device accepts the exact current token through either the collected `X-Bridge-Token` request header (reliable modern browsers) or legacy form field (backward compatibility); no authentication or CSRF bypass.
- [x] C++ CSRF policy and real browser JS regression tests passed, including token refresh after restart, one safe retry, invalid tokens and unrelated errors. [CI run 42442460633](https://github.com/ggrant111/wled-applehome/actions/runs/42442460633).
- [ ] **Pending hardware:** Update via normal upload/OTA without erasing NVS; reboot with browser tab open; add preset; validate Apple Home preset switch appears and triggers WLED.

**Immediate user workaround on v0.4.0:** Reload the browser dashboard after an ESP32 reboot, then add the preset again. Do not reset Apple Home pairings.

## Phase 4 safety & deployment notes

- This is **v0.4.0-dev with Phase 4B next**, not a finished v1.0 release. Preserve outstanding Phase 1–4 hardware gates.
- **The portal is OFF by default**. On USB serial console, enter `@k <strong-unique-password>` (14–64 printable non-space ASCII characters) and reboot with `R`; then visit `http://<bridge-ip>:8080/`, sign in with username `admin`.
- The administrator password is stored in local ESP32 NVS for web Digest login and reused to activate HomeSpan's password-protected ArduinoOTA. It is **not hardcoded in firmware or written to git**, but plaintext NVS is not encryption-at-rest—physical flash access remains a security boundary.
- Disable portal + OTA via `@k -` followed by `R`; HomeKit/WLED configuration must remain unchanged.
- **No web firmware upload form in Phase 4A.** Use HomeSpan's OTA workflow with Arduino IDE / compatible uploader. No remote flashing at all until unique secret is configured. `SpanRollback.h` marks pending OTA firmware valid after ~60 seconds of running with Wi-Fi.
- Portal uses HTTP Digest authentication and CSRF tokens; **it does not use HTTPS**. Trusted LAN only; never expose port 8080 or OTA publicly.
- Network operations are still partially blocking. Portal is polled after HomeSpan's loop and starts only with Wi-Fi. Multi-device latency and 8-light capacity remain to test.
- Firmware CI now rejects builds if OTA app partition headroom falls below 70,000 bytes.

## Non-goals / guardrails

1. Do **not** flash or change firmware on any WLED controller.
2. Do **not** change WLED presets, segments, or running effects unless commanded.
3. Do **not** put SSIDs, passwords, or HomeKit codes in git.
4. Do **not** assign HomeKit accessory IDs from discovery order. Once assigned, preserve IDs.
5. Home app does **not** provide a native WLED preset dropdown; use optional trigger accessories.
6. **CYD v0.1 is headless**, even though it builds for the touchscreen hardware.
7. The D1 Mini must be **ESP32**, not ESP8266.
8. Keep HomeKit/UDP/mDNS traffic on the same permitted LAN; no cloud required.

## Phase 2 deployment notes

1. Use the **existing paired ESP32**. Flash without erasing flash/NVS (do not run `pio run -t erase`).
2. v0.2 migrates the existing `wled-home` IP to AID=2 before creating any new accessories.
3. On the serial CLI: `@l` list lights, `@d` discover, `@a1` enroll discovery result 1, `@m192.168.1.50` manually enroll an IP, and `@r3` remove AID=3.
4. New additions/removals take effect in Apple Home on the next `R` reboot. Discovery alone never enrolls devices.
5. Restore stable IP identity via matching WLED MAC. If mDNS fails, use manual verified IP enrollment.
6. v0.2 intentionally remains serial-managed; web UI is still a Phase 4 goal.

## Build/test tracking

| Check | Status | Evidence |
| --- | --- | --- |
| Host-side logic checks | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| D1 Mini PlatformIO build | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| CYD PlatformIO build | Passed | [CI run 42427318407](https://github.com/ggrant111/wled-applehome/actions/runs/42427318407) |
| User hardware Apple Home pairing and brightness | **Verified by user (2026-11-07)** | First firmware installed, paired and changed brightness of real WLED |
| D1 Mini physical on/off and reboot persistence | Not yet reported | Requires physical test |
| CYD physical pairing / WLED actuation | Not tested | User hardware |
| v0.2 D1 Mini firmware, CYD firmware, host tests | **Passed** | [CI run 42430738405](https://github.com/ggrant111/wled-applehome/actions/runs/42430738405) |
| v0.2/v0.4 portal discovery and Apple Home enrollment | **User-verified (2026-11-07)** | Portal discovers devices and adds lights in Apple Home; other Phase 2 checks pending |
| v0.2 full 8-light / remove-reboot / identity recovery | Pending hardware | [Phase 2 test plan](PHASE2_TEST_PLAN.md) |

| v0.3 D1 Mini firmware, CYD firmware and host tests | **Passed** | [Phase 3 CI run 42438580722](https://github.com/ggrant111/wled-applehome/actions/runs/42438580722) |
| v0.3 preset control / switch-reset / state reconciliation | Pending hardware | [Phase 3 test plan](PHASE3_TEST_PLAN.md) |

| v0.4 D1 Mini, CYD firmware builds + host/NVS migration tests | **Passed** | [CI run 42439628403](https://github.com/ggrant111/wled-applehome/actions/runs/42439628403) |
| v0.4 portal load / discovery / add to Home | **User-verified (2026-11-07)** | Web portal works; WLED devices discovered and added to Apple Home |
| v0.4 web authentication hardening / OTA recovery | **Not yet verified** | [Phase 4 test plan](PHASE4_TEST_PLAN.md) |

| v0.4.1 CSRF fix, browser regression, D1 Mini/CYD firmware & OTA size | **Passed** | [CI run 42442460633](https://github.com/ggrant111/wled-applehome/actions/runs/42442460633) |
| v0.4.1 live preset-add / stale-tab recovery | **Hardware pending** | User to verify without NVS erase |

| v0.5 worker / gzip portal / D1 Mini + CYD builds | **Passed** | [CI run 42443428109](https://github.com/ggrant111/wled-applehome/actions/runs/42443428109) |
| v0.5 background worker behavior with offline controllers | **Pending hardware** | [v0.5 test plan](PHASE5_TEST_PLAN.md) |

| v0.6 HomeKit Hue/Saturation, D1 Mini + CYD builds, -Oz sizing | **Passed** | [CI run 42445302149](https://github.com/ggrant111/wled-applehome/actions/runs/42445302149) |
| v0.6 real Apple Home color wheel, multi-segment targeting | **Hardware pending** | [Phase 6 test plan](PHASE6_TEST_PLAN.md) |

| v0.7 optional TV Effects Controller, both boards, host tests | **Passed** | [CI run 42448093812](https://github.com/ggrant111/wled-applehome/actions/runs/42448093812) |
| v0.7 Apple Home selector UI + WLED preset selection | **Hardware pending** | [Phase 7 test plan](PHASE7_TEST_PLAN.md) |

| v0.7.1 opt-in grouped effect buttons, AID migration and both ESP32 builds | **Passed** | [CI run 42449994004](https://github.com/ggrant111/wled-applehome/actions/runs/42449994004) |
| v0.7.1 Apple Home multi-switch effect UI and WLED triggering | **Hardware pending** | [Phase 7B](PHASE7B_TEST_PLAN.md) |

| v0.8 preset power/bounds diagnostics, host tests and both ESP32 builds | **Passed** | [CI run 42452061034](https://github.com/ggrant111/wled-applehome/actions/runs/42452061034) |
| v0.8 actual WLED segment restoration and HomeKit power status | **Hardware pending** | [Phase 8 checklist](PHASE8_TEST_PLAN.md) |

| v0.9 eight effects groups, migration, browser/CSRF, both ESP32 builds | **Passed** | [CI run 42454610880](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880) |
| v0.9 two physical WLED groups and HomeKit state/identity isolation | **Hardware pending** | [Phase 9 checklist](PHASE9_TEST_PLAN.md) |

| v0.9.1 saved playlist parsing / NVS / power policy and both ESP32 builds | **Passed** | [CI run 42505219721](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721) |
| v0.9.1 iPhone playlist playback and direct WLED change reconciliation | **Hardware pending** | [Playlist test plan](PHASE9_PLAYLIST_TEST_PLAN.md) |

| v0.9.2 local portal UX, browser/CSRF/gzip tests, D1 Mini + CYD builds | **Passed** | [CI run 42506904065](https://github.com/ggrant111/wled-applehome/actions/runs/42506904065) |
| v0.9.2 desktop/iPhone UI and direct WLED effect Run | **Hardware pending** | [UX test plan](PHASE9_PORTAL_UX_TEST_PLAN.md) |

| v0.9.3 screenshot-driven UX refinements, host/browser regressions and both ESP32 builds | **Passed** | [CI run 42508487092](https://github.com/ggrant111/wled-applehome/actions/runs/42508487092) |
| v0.9.3 live desktop/mobile UX flow and HomeKit continuity | **Hardware pending** | [v0.9.3 test plan](PHASE9_UX_POLISH_TEST_PLAN.md) |

## Engineering risk / budget

- **4MB OTA flash budget is tight.** v0.2 compiled binary uses ~1,733,463 bytes (85.3%) of each 2,031,616-byte app slot; approximately 298KB remains. Major additions (especially touchscreen/web assets) will need explicit size-budget tests, optimization, larger flash hardware, or an altered OTA strategy.
- v0.3 linked flash use was **1,758,847 bytes (86.6%)** of 2,031,616 per OTA application slot.
- **v0.9.3 full firmware.bin with GCC -Oz:** D1 Mini and CYD each **1,585,216 bytes**, leaving **446,400 bytes free** in each 2,031,616-byte OTA slot; [CI 42508487092](https://github.com/ggrant111/wled-applehome/actions/runs/42508487092) passed. Physical browser layout still needs user verification.
- **v0.9.2 full firmware.bin with GCC -Oz:** D1 Mini and CYD each **1,584,544 bytes**, with **447,072 bytes free** in 2,031,616-byte OTA slots; [CI run 42506904065](https://github.com/ggrant111/wled-applehome/actions/runs/42506904065) passed build/size checks. Physical dashboard layout and WLED Run request still require user testing.
- **v0.9.1 full firmware.bin with GCC -Oz:** D1 Mini **1,576,656 bytes** (**454,960 bytes free**); CYD **1,576,640 bytes** (**454,976 bytes free**) of 2,031,616. [CI 42505219721](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721) passed all build gates. Physical playlist behavior pending.
- **v0.9 full firmware.bin with GCC -Oz:** D1 Mini **1,574,848 bytes** (456,768 free); CYD **1,574,832 bytes** (456,784 free) in each 2,031,616-byte OTA slot. [CI run 42454610880](https://github.com/ggrant111/wled-applehome/actions/runs/42454610880) passed all gates; on-device multi-WLED performance and HomeKit display still unverified.
- **v0.8 full firmware.bin with GCC -Oz:** D1 Mini and CYD **1,564,800 bytes** (**466,816 bytes free** of 2,031,616); [CI run 42452061034](https://github.com/ggrant111/wled-applehome/actions/runs/42452061034) passed minimum OTA headroom. Real segment recall depends on WLED preset content and still requires user hardware validation.
- **v0.7.1 full firmware.bin with GCC -Oz:** both D1 Mini and CYD **1,560,144 bytes** (**471,472 bytes free** of 2,031,616); [CI run 42449994004](https://github.com/ggrant111/wled-applehome/actions/runs/42449994004) passed flash budget. Grouped HomeKit switch UI still requires physical verification.
- **v0.7 full firmware.bin with GCC -Oz:** D1 Mini and CYD each **1,557,792 bytes** (**473,824 free** in the 2,031,616-byte OTA slot); [CI 42448093812](https://github.com/ggrant111/wled-applehome/actions/runs/42448093812) passed 70KB headroom. Home app TV input rendering remains untested.
- **v0.6 full firmware.bin with GCC -Oz:** D1 Mini **1,545,312 bytes** (**486,304 free**, 76.1% OTA slot); CYD **1,545,296 bytes** (**486,320 free**). [CI 42445302149](https://github.com/ggrant111/wled-applehome/actions/runs/42445302149) passed both build and size gates. The -Oz option substantially changes compiler optimization: validate real device stability and OTA before release.
- **v0.5 full firmware.bin:** D1 Mini **1,957,408** / 2,031,616 bytes (**74,208 free**); CYD **1,957,424** bytes (**74,192 free**) — **96.3%** of OTA slot. Portal raw HTML 13,425 bytes compresses to 5,001 bytes at build time. CI 42443428109 passed all gates.
- **v0.4.1 full firmware.bin:** 1,956,928 / 2,031,616 bytes (96.3% OTA slot); **74,688 bytes remaining** on both D1 Mini and CYD.
- **v0.4 linked flash:** D1 Mini **1,876,139 bytes (92.3%)**, CYD 1,876,155 (92.3%). **Full firmware.bin** (includes headers/padding): D1 Mini **1,955,232** bytes, CYD **1,955,248**; **96.2% OTA slot** and only **76,384 / 76,368 bytes spare** respectively. Do not plan significant more UI code on 4MB boards without moving to larger-flash hardware, compression, or optimizing flash use. CI requires at least 70KB spare.
- **Important migration fix:** Empty WLED MAC and IP strings are valid on the v0.1 original light, but ESP32 Preferences::putString returns 0 for a successful empty write. DeviceRegistry now verifies NVS string writes by readback; a native mock-Preferences regression test checks AID=2 migration and stable IDs across reboots.
- Current network operations use a bounded blocking HTTP client; eight offline lights and mDNS scans may temporarily delay HomeKit servicing. Treat nonblocking IO as a priority before closing Phase 2.

## Release decision process

Each phase changes status from **PLANNED → IN PROGRESS → CODE COMPLETE → HARDWARE VERIFIED** only after the relevant evidence exists. Add a dated note in `docs/PROGRESS.md` for every milestone, plus CI run URLs, hardware observations, and known blockers.

## References

- [HomeSpan](https://github.com/HomeSpan/HomeSpan) — HomeKit HAP bridge; requires Arduino-ESP32 3.3.0+ and at least ~1.9 MiB app partition.
- [WLED JSON API](https://kno.wled.ge/interfaces/json-api/) — state/power/brightness/presets.
- [PlatformIO Espressif32 8.0.0](https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0) — Arduino-ESP32 3.3.12.
