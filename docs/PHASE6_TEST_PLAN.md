# v0.6 Apple Home Hue/Saturation — hardware acceptance

Code and CI alone do not prove that Apple Home refreshes optional characteristics or that RGB works with the user's WLED LED hardware.

## Before update
- [ ] Confirm WLED color strip is RGB or RGBW; record your current preset switch names, AIDs and working firmware version.
- [ ] Update the same ESP32 using the matching D1 Mini or CYD image, **without erasing flash/NVS**, changing partitions or resetting HomeKit.
- [ ] Confirm the existing Apple Home bridged lights remain available, plus previously exposed preset switches.

## HomeKit color wheel
- [ ] After ESP32 restarts and Home reconnects, each WLED light shows the Apple Home color control (Hue/Saturation) in addition to On/Brightness. If not, reconnect Home app before attempting re-pairing.
- [ ] Choose red, green, blue, yellow and white from Apple Home. Confirm matching colors on WLED's main segment.
- [ ] Adjust brightness without altering color and switch power off/on while retaining selected color.
- [ ] Check RGBW strips: white LEDs do not unexpectedly overpower a chosen saturated color.
- [ ] With several WLED controllers, changing one color never alters another controller.

## WLED state and advanced behavior
- [ ] Manually change primary color using WLED web UI, then confirm Apple Home updates within approximately 8 seconds while controller online.
- [ ] Trigger a selected WLED preset in Apple Home; confirm its effect/colors are restored and Home's color indicator tracks main-segment primary color.
- [ ] On a WLED device with multiple segments, change color from Apple Home. Confirm only WLED **main segment** gets `fx:0` and new primary RGB; other segments retain existing effects/positions.
- [ ] Change WLED's main segment to a different segment (including when other segments are selected) and repeat. Only the explicit current main-segment ID should change. A main-segment switch just before a HomeKit command may require the next poll.
- [ ] Disconnect one WLED and verify another controller still handles HomeKit color changes.
- [ ] Test rapid hue wheel and brightness interactions, observing queue failures/status under load.

## Caveats
- Standard Apple Home lights do not offer WLED palettes/effects/speed/intensity, and color temperature is not yet implemented.
- The main segment changes to Solid on explicit Apple Home color edits, because an ongoing effect might ignore the chosen color.
- HomeKit sees the configured primary RGB color, not the live colors of animated LEDs.
- OTA rollback, Wi-Fi reconnection, 8-controller stress test and 24-hour soak remain unverified until tested on hardware.
