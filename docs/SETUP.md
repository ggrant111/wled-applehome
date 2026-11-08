# WLED Apple Home Bridge — setup and hardware validation

## Phase 4A — local web management & HomeSpan OTA

**v0.4.0-dev is under CI/hardware validation.** This build incorporates previous phases but does not eliminate the need to test v0.2 multi-WLED discovery and v0.3 preset switches on physical hardware.

### Safe installation

1. Download the **correct board binary** from the GitHub Actions artifacts after the D1 Mini/CYD builds pass.
2. Flash onto the **same ESP32** paired with Apple Home, using a normal PlatformIO upload. **Never erase NVS or change the partition table while upgrading**.
3. Open HomeSpan serial monitor at 115200 baud. Verify `@l` still shows original WLED as HomeKit AID=2.
4. **Opt into LAN management + OTA** via `@k <your-unique-admin-password>` (14–64 ASCII characters without spaces; enter a unique value and do **not** share it in GitHub or chat). Reboot with `R`.
5. On the ESP32 serial output, read the management portal URL: `http://<ESP32-local-IP>:8080/`. Sign in as `admin` with the password you set.
6. Use the Dashboard for WLED discovery, status, control, selected preset switches and restarting after accessory additions/removals.

**The web portal does not start until an admin password has been configured.** `@k` reports provisioning status without displaying the password. To disable web access and OTA, use `@k -` then `R` (HomeSpan pairing and WLED devices are not erased).

### OTA updates (no web upload form in this version)

HomeSpan supplies its built-in password-protected **ArduinoOTA** uploader. Use your administrator password rather than HomeSpan's public default; for example, select the ESP32's **network port** in Arduino IDE and upload the next compatible firmware sketch. PlatformIO ESP OTA clients are also possible when configured locally with `espota` (keep passwords out of version-controlled files).

- Both OTA app slots are `0x1F0000` bytes. Firmware CI rejects images with less than 70KB spare.
- HomeSpan validates that an OTA-uploaded sketch is still a HomeSpan sketch with OTA enabled.
- `SpanRollback.h` defers marking a newly booted OTA image as good until it has run ~60 seconds with Wi-Fi. An early crash/panic is intended to let ESP32 bootloader return to the last valid image; this **must be tested** before unattended use.
- Firmware uploaded via **USB** is not protected from all kinds of bricking by this rollback mechanism.
- Do not trigger an OTA update mid-celebration or while controlling lights.

### Security and limitations

- **Never port-forward** port 8080 or the ESP32 OTA endpoint to the internet.
- Dashboard is accessible on trusted local Wi-Fi only, over HTTP with Digest authentication and CSRF protection; **there is no HTTPS**.
- The portal password is stored in the device's local NVS for use by the web server and HomeSpan OTA. Plain NVS is not encrypted against physical flash extraction.
- Discovery, WLED catalog fetches and control POSTs are on-demand; a slow WLED or busy LAN may temporarily pause HomeKit servicing. Test with eight controllers.
- A portal restart changes accessible HomeKit accessories; pairing identity should remain, but must be verified on hardware before marking production ready.
- Presets cannot be created or edited here: the portal exposes existing WLED presets as HomeKit trigger switches.

See [Phase 4 physical acceptance plan](PHASE4_TEST_PLAN.md).


## Phase 3 — v0.3 firmware (CI + physical testing required)

Phase 3 adds **on-demand** saved-preset discovery and **opt-in** Apple Home preset switches. You do not need to upload it until you return home. Phase 2 hardware testing is still pending.

**Important:** Keep the ESP32's NVS and partition layout intact. Flash onto the same paired device as before, without an erase command.

After updating, open the 115200 baud HomeSpan serial CLI:

| Command | Meaning |
| --- | --- |
| `@l` | List WLED lights and their stable light AIDs |
| `@c2` | Show saved presets for WLED light AID 2; names are read from WLED's `/presets.json` |
| `@p2:5` | Expose WLED light AID 2, preset ID 5 as a **new** Apple Home switch (use an actual saved ID) |
| `@s` | List exposed preset switches and their own HomeKit AIDs |
| `@q9` | Remove preset switch AID 9 (replace 9 with the value in `@s`) |
| `R` | Reboot so HomeSpan publishes changed bridged accessories |

Adding a preset to Home changes **only the bridge's NVS**. Triggering the switch sends `{"ps":5}` to the target WLED device. The switch should reset Off after ~250ms so it can be used repeatedly. WLED may change brightness/power when a preset runs, so the light's On/Brightness characteristics are reconciled after a ~450ms settle delay.

**Capacity:** 8 WLED lights + up to 12 **selected** HomeKit preset switches across all lights. WLED may store up to 250 presets per controller; only the ones you enroll appear in Home.

This does not introduce WLED preset editing, custom scene creation, advanced per-segment controls, WebSockets, OTA web uploads, or a web dashboard.

See [Phase 3 test plan](PHASE3_TEST_PLAN.md). **Do not reset or re-pair the bridge just to make a switch show up.** Gather serial/Home logs first; HomeKit sometimes caches accessory lists.

## Phase 2 — v0.2 discovery and multi-device setup

**v0.2 is a development build:** The first release paired successfully with Apple Home and controlled real WLED brightness (user confirmed 2026-11-07). The discovery/multi-WLED changes need real hardware testing. Preserve your Apple Home bridge pairing.

## Flash the paired ESP32 without erasing NVS

Use the **same physical ESP32** already paired to Apple Home. Its HomeSpan pairing is stored on-board. Do not run a full erase or change partition layouts.

```bash
git clone https://github.com/ggrant111/wled-applehome.git
cd wled-applehome
pio run -e d1_mini_esp32 -t upload
pio device monitor -b 115200
```

If you are actually using the CYD, replace `d1_mini_esp32` with `cyd_2432s028r`. CYD display/touch remain disabled.

On the first v0.2 boot, the previous single-light IP stored in `wled-home` is copied into the new `wled-reg` registry as **AID=2**. This should preserve the original light in Apple Home. Your existing Wi-Fi and HomeKit credentials should also survive a normal upload.

**IMPORTANT:** Keep a note of the current WLED IP and an exact screenshot of the original Apple Home light before installing. If pairing is lost, collect logs before using HomeSpan's reset CLI commands.

## Serial commands (115200 baud)

| Command | Purpose |
| --- | --- |
| `@l` | Show enrolled lights, IP addresses, MAC identities, and accessory IDs |
| `@d` | Discover WLED on the LAN via mDNS and verify `/json/info` |
| `@a1` | Enroll the first scanned device (use `@a2` for the second, etc.) |
| `@m192.168.1.50` | Manually verify and enroll a controller by IP |
| `@r3` | Remove accessory AID=3 from the persistent registry |
| `@w192.168.1.50` | Keep the original AID=2 light and set its WLED IP (legacy compatible) |
| `R` | HomeSpan reboot, needed after adding/removing accessories |
| `?` | HomeSpan help (Wi-Fi setup `W`, pairing code command `S`) |

**Replace example IPs** with your WLED controllers' actual local IPv4 addresses. WLED controllers must provide a stable hardware MAC in `/json/info` (or mDNS TXT) to be enrolled. WLED normally advertises `_wled._tcp`, but some networks block mDNS—use `@m<IP>` if `@d` is empty.

Do **not** run `H`, `F`, `E`, or erase flash casually; those reset pairing/credentials and undermine the preservation test.

## Example: bring a second WLED into Apple Home

1. Flash v0.2 and type `@l`; verify your original light remains **AID=2** and operates in Apple Home.
2. Type `@d`; check the serial list of verified WLED names, IPs, and MACs.
3. Type `@a2` (replace 2 with the scan list index for the *other* controller). Already-known devices report their existing AID instead of being duplicated.
4. Type `@l`; confirm a **new AID** was allocated.
5. Type `R` to reboot. Reopen Apple Home; the new accessory should appear under your bridge. You should not have to pair the bridge again.
6. Test both lights independently: on/off, brightness, and external-state changes.
7. Repeat enrollment up to **eight** active WLED devices.

If the Home app doesn't immediately refresh its accessory list, close/reopen the app. Don't factory reset the bridge just to refresh Home's cache.

## Known boundaries and safety

- v0.2 has no web setup UI; commands are intentionally serial-only.
- Discovery scans only default port 80 and filters devices without a validated MAC.
- AID values are never reused; removing then adding a different WLED should assign a new AID.
- State polling is staggered, but **HTTP and scan operations remain blocking**. Performance with eight lights needs testing and later nonblocking refactor.
- WLED's presets, segments, colors and effects are untouched by the bridge unless the user changes power or brightness.
- WLED devices on another VLAN may require multicast/mDNS routing, or a manual IP if reachable.
- Presets and touchscreen management are **not yet included**.

See [Phase 2 testing plan](PHASE2_TEST_PLAN.md), [roadmap](ROADMAP.md), and [progress log](PROGRESS.md).
