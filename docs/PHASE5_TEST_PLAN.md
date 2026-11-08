# v0.5 background WLED network worker — physical acceptance checklist

> Source code may compile without hardware validation. Do not mark a feature verified from CI alone. Preserve the existing paired ESP32 NVS/HomeKit identity while testing.

## Before update
- [ ] Record existing firmware version, HomeKit bridge pairing, device AIDs/IPs, preset switch count, and current portal password.
- [ ] Make sure current v0.4.1 portal works. Test one saved preset switch if possible.
- [ ] Flash new binary to the same hardware **without erasing NVS**; never change partition layout.

## Baseline behavior
- [ ] Reboot and verify every WLED light and preset HomeKit accessory remains present (especially AID=2).
- [ ] Portal login still works; version reads `0.5.0-dev`, browser page loads gzip asset, token refresh still permits preset enrollment.
- [ ] HomeKit on/off and brightness control at least two WLED devices independently.
- [ ] Preset switch activates correct saved WLED preset; resets in Apple Home for repeated use.
- [ ] WLED-originated changes reflect back to HomeKit within the normal ~8-second window.

## Background queue and reconnection
- [ ] Serial shows `[Network] Background WLED HTTP task started`.
- [ ] During HomeKit control, serial logs `[HomeKit] ... queued` followed later by `[Network] POST ... HTTP OK`.
- [ ] Portal light status displays pending commands and total HTTP failures without blocking.
- [ ] Power off/unplug one WLED controller; turn on/adjust other lights and verify they respond normally.
- [ ] Attempt control of offline light; initial HomeKit write may appear accepted by queue, but actual execution fails. Observe `HTTP FAILED`, device marked offline, and a failure counter increment.
- [ ] Restore offline WLED power; within next retry/poll window it returns online, and external state reconciles.
- [ ] Change WLED state directly in WLED app while another controller is offline; remaining lights update correctly.
- [ ] Stress quick brightness changes and several simultaneous preset switches; no HomeSpan reboot/hang or queue deadlock.
- [ ] If queue fills under stress, new commands must reject quickly and increment a dropped-commands counter, not hang.
- [ ] Change the original WLED IP (if possible) and verify old-host GET cannot overwrite newly read state.
- [ ] With several controllers offline, portal pages and Apple Home still remain responsive except during user-triggered scan/catalog fetch (known blocking paths).

## OTA/heap/long-run
- [ ] Confirm firmware binary still satisfies CI 70KB OTA headroom gate, and examine free/min heap values.
- [ ] Validate Wi-Fi disconnect/reconnect, ESP32 power cycle, 1/3/8 devices, and 24h soak.
- [ ] Verify authenticated OTA keeps pairing, credentials, registered devices, and preset configuration.
- [ ] Record any WLED device-specific slow response or browser errors in `docs/PROGRESS.md`.

## Known limitations
- One queued POST does not guarantee WLED executed it; HomeKit sees queue acceptance, with failure via dashboard/serial instead of a deferred HomeKit error.
- User-triggered discovery, manual WLED probe and saved-preset catalog use bounded synchronous HTTP, so those rare UI actions can pause main HomeSpan processing briefly.
- Task-based queue + worker is implemented; per-device independent parallel HTTP and WebSocket event push are not part of this build.
