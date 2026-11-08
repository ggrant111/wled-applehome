# Phase 7B — v0.7.1 grouped effects controls

**Motivation:** User's iPhone Home app shows all v0.7 TV-style preset input names under Settings → Inputs, but the accessory panel offers only a power button; no actionable input selector. A successful build is not proof the new grouped controls will display either.

## OTA + conversion
- [ ] Save the last working firmware and verify original WLED Apple Home full-color light + Siri first.
- [ ] OTA install the correct v0.7.1 build on the same paired ESP32 **without erasing NVS or changing partitions**.
- [ ] Portal on port 8080 → **Effects Controller** shows TV-style mode and all original saved preset names.
- [ ] Tap **Convert to grouped buttons** and confirm. Portal reports new AID, mode Grouped, restart needed. Preset names and light AID unchanged.
- [ ] Restart bridge. The original TV-style accessory should disappear; new effects accessory should appear (Apple Home may cache former tile until reconnect).

## iPhone usability
- [ ] Open new Effects accessory. Look for **multiple named independently tappable switches** for Mammoth, Wednesday, Dodger Celebrate, Rainbow by Shelf.
- [ ] Tap two or more named switch controls; confirm actual WLED runs the associated saved presets, not just a switch tile state.
- [ ] Each action switch resets to Off after ~250ms so it can retrigger.
- [ ] The grouped buttons do NOT have a master TV power control; control the WLED power via the existing LightBulb.
- [ ] Check original WLED light's On/Brightness/Hue/Saturation and Siri still work.
- [ ] Confirm separately exposed preset switches and all other enrolled WLED lights still work.
- [ ] Restart/power cycle bridge: original light and preset-switch AIDs are unchanged and new effects AID persists.
- [ ] Remove grouped controller in portal and restart: light remains, TV and grouped effect accessories are absent.

## Limitations and recovery
- One grouped accessory per bridge (experimental), one WLED light, up to five saved presets.
- Services may be grouped or represented differently depending on Apple Home/iOS UI. Do not mark complete until actual UI passes.
- If it fails or interferes with original lights, **remove only Effects Controller** in portal and reboot. Do not reset/re-pair the bridge.
