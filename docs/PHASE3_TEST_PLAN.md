# Phase 3 — Presets and WLED/Apple Home synchronization acceptance

**Status:** Code candidate only; **hardware checks unverified**. This test plan builds on the unfinished Phase 2 checklist. First verify original AID=2 and at least one additional WLED light without new pairing.

## Preserved HomeKit / NVS identities
- [ ] Flash without NVS erase; previously paired bridge remains paired.
- [ ] Original WLED light retains AID=2 and works as before.
- [ ] Existing Phase 2 light entries remain intact; new preset switches get non-recycled IDs.
- [ ] Reboots preserve lights and preset switches across multiple cycles.

## WLED saved preset catalog
- [ ] `@c2` lists saved preset ID and name from first controller.
- [ ] Catalog query does not turn on/off LEDs or modify colors/effects/presets.
- [ ] `@c<otherAid>` lists presets on a different WLED device.
- [ ] Offline WLED / invalid AID fails safely without NVS changes.
- [ ] Test an unusually large `presets.json` for memory use, scan duration, and HomeKit responsiveness.

## Opt-in preset HomeKit switches
- [ ] `@p2:<knownID>` adds exactly one selected preset with a stable AID.
- [ ] `@s` shows switch AID, parent light AID, preset ID and label.
- [ ] Repeating `@p2:<sameID>` returns the same switch AID and doesn't duplicate.
- [ ] `R` refreshes Apple Home without re-pairing original bridge/light.
- [ ] Triggering from Apple Home activates the correct preset on the **correct** WLED.
- [ ] Trigger switch returns to Off within ~250ms and can be activated repeatedly.
- [ ] Triggering a preset that turns WLED Off or changes brightness updates the light's On/Brightness values after ~450ms.
- [ ] Deleting via `@q<switchAid>` + `R` removes the switch without affecting other lights or WLED presets.
- [ ] Adding a light after adding a preset switch does not duplicate any AID.
- [ ] Test all 12 available switch slots; 13th should be rejected without changing settings.

## Two-way state, timing and resilience
- [ ] Changing power/brightness from Apple Home sends WLED command successfully.
- [ ] External changes in WLED or GlowDeck reflect in Apple Home within normal 8-second polling.
- [ ] Local HomeKit/preset POST isn't immediately overwritten by stale polling.
- [ ] WLED HTTP failure causes a rejected HomeKit preset write rather than a false success.
- [ ] While a WLED device is offline, other lights remain responsive.
- [ ] Compare measured flash/heap usage with v0.2 (85.3% of 1,937.5KiB slot).

## Explicitly not complete
- [ ] Fully nonblocking WLED networking, WebSocket streaming, recovery under extensive network faults.
- [ ] Phase 2 eight-light hardware and HomeKit database-refresh validation.
- [ ] Phase 4 web setup, OTA firmware update path and release acceptance.
