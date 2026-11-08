# Phase 7 — Experimental Apple Home TV-style Effects Controller

**CI [42448093812](https://github.com/ggrant111/wled-applehome/actions/runs/42448093812) passed all tests and both board builds. Do not treat a successful firmware build as proof that Apple Home shows linked TV InputSource entries behind a bridge.** HomeSpan's [Television Services](https://github.com/HomeSpan/HomeSpan/blob/master/docs/TVServices.md) document warns that a bridged TV can appear as a generic icon. This is an explicit opt-in experiment.

## Update and opt-in
- [ ] Save the last known-good firmware image and note your current Apple Home light AIDs / existing preset switches.
- [ ] OTA-install the correct ESP32 D1 Mini or CYD binary **without erasing NVS or modifying partitions**.
- [ ] Open the web portal. Check **v0.7.0-dev** and verify your original full-color WLED lights still respond to Apple Home and Siri.
- [ ] Load saved presets for **one** WLED light under Preset Switches. Choose a preset and press **Add selected preset to Effects Controller**.
- [ ] Add 2–4 more saved WLED presets to the SAME controller (maximum 5). Do not create existing preset switches unless desired.
- [ ] Restart via the portal so HomeSpan publishes the additional TV-style accessory and linked inputs.

## Apple Home
- [ ] Locate the **Effects** accessory; it may have a TV or generic icon, not a light icon.
- [ ] Long-press/open its controls. Verify a named **Input** selector appears. Record iOS version and screenshot.
- [ ] Confirm 3–5 configured names display and remain in the expected order.
- [ ] Select Rainbow / Fire / etc., and verify the real WLED applies the correct **saved preset**.
- [ ] Switch HomeKit TV-style accessory OFF: WLED should turn off. Switch ON: the last selected preset should resume.
- [ ] Confirm normal RGB light color wheel, brightness, Siri and existing preset switches remain functional and consistent.
- [ ] Change to one selected saved preset through WLED directly, then confirm Home's input display updates after state reconciliation.
- [ ] Unplug WLED temporarily: the HomeKit bridge must not block, and the worker should log a failed queued preset request.
- [ ] Restart or power cycle the ESP32 and confirm all existing light/preset AIDs and this controller AID persist.

## Removal
- [ ] Open portal → Effects Controller → **Remove controller**; restart bridge. The accessory disappears, but WLED devices and any original preset switches remain.
- [ ] Verify no new controller appears on a fresh v0.7 install until explicitly configured.
- [ ] Avoid re-pairing or erasing NVS unless an explicit recovery process requires it.

## Known limitations
- ONE experimental effects selector per bridge, FIVE saved preset names, bound to ONE WLED light.
- This may appear as a generic TV/AV tile, not inside the WLED light tile.
- Input/preset list changes require restarting the ESP32; existing Apple Home cached accessory topology may not update immediately.
- TV ON reapplies selected preset, TV OFF powers the light off.
- The current TV selector may show the last selected input when WLED runs an unlisted effect; it does not represent live animation frames.
- End-to-end UI behavior depends on the installed Apple Home version; it is *not* a supported native lighting effects characteristic.
