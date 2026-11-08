# v0.2 setup and hardware test guide

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
