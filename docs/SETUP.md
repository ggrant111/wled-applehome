# Phase 1 setup: one WLED device in Apple Home

**Current state:** firmware candidate; hardware pairing/operation not yet verified.

## Prerequisites

- One **ESP32** D1 Mini (NOT ESP8266) or ESP32-2432S028R CYD (screen disabled in v0.1).
- USB data cable, 2.4GHz Wi-Fi, iPhone with Apple Home.
- An already working WLED controller on the same reachable LAN. Know its IPv4 address.
- PlatformIO Core 6+ / IDE extension. Python 3.12 recommended.
- For reliable/supported HomeKit, a HomePod or Apple TV as home hub.
- Local multicast/mDNS allowed (isolation/VLAN/firewall rules can prevent discovery).

## 1. Flash

```bash
git clone https://github.com/ggrant111/wled-applehome.git
cd wled-applehome
pio run -e d1_mini_esp32
pio run -e d1_mini_esp32 -t upload
pio device monitor -b 115200
```

For the CYD use `-e cyd_2432s028r` instead; in v0.1 the screen stays blank because this is **headless** firmware.

Do not flash ESP8266 hardware. Confirm your board variant and its flash capacity before proceeding. The partition table is intended for **4MB flash**, with two ~1.94MiB application slots; if your variant differs, stop and adapt the table.

## 2. Provision Wi-Fi and a unique HomeKit code

In the PlatformIO Serial Monitor at **115200 baud**:

1. Type `?` for HomeSpan CLI help.
2. Type `W` to configure Wi-Fi credentials using the CLI. Follow its prompts.
3. Type `S` to set your own **unique eight-digit HomeKit pairing code**. Keep it private and record it somewhere safe. HomeSpan's documented default code is public and must not be kept for permanent use.
4. Restart if prompted. Confirm the bridge has Wi-Fi. Do not place passwords or HomeKit codes in `platformio.ini`, source files, GitHub issues, or CI secrets.

If serial provisioning is inconvenient, HomeSpan also offers a setup access point via its `A` CLI command; follow the HomeSpan official guide and disable the setup AP when finished.

## 3. Configure the WLED IPv4 target

Type a HomeSpan user command with the address of **your own** controller:

```text
@w192.168.1.50
```

Then `@w` to verify what was stored. This command is backed by ESP32 NVS and survives reboots. Only IPv4 targets are accepted in Phase 1. Do not include `http://`, port, or path. An unconfigured target safely rejects HomeKit writes.

Check that `http://<your-wled-ip>/json/state` responds in a browser or curl and that the WLED device itself works.

## 4. Pair Apple Home

1. On iPhone: Home → Add Accessory → More Options.
2. Select **WLED Apple Home Bridge** when discovered by local mDNS.
3. Enter your unique code. The WLED Light should appear as a bridged light.
4. An 'Uncertified Accessory' warning is expected with DIY HomeSpan firmware; review before accepting.

If the bridge doesn't appear: confirm same reachable LAN, Wi-Fi connection, and multicast/mDNS. If you have paired previously and changed the HomeKit database, cached accessories may require additional investigation — don't immediately factory-reset NVS.

## 5. Phase 1 physical smoke test (record in docs/PROGRESS.md)

- [ ] Apple Home identifies bridge and WLED Light.
- [ ] On works.
- [ ] Off works.
- [ ] Brightness ~25% and ~80% work.
- [ ] Changing WLED from WLED's UI updates Apple Home within ~5 seconds.
- [ ] Reboot ESP32; paired accessory reappears without a new pairing session.
- [ ] Reboot WLED; bridge resumes within polling intervals.
- [ ] No unexpected changes to WLED presets or LED segments.

## Limitations

- One WLED light, configured manually by IPv4. **No discovery or presets yet**.
- Polling every 5 seconds, with a bounded blocking network request.
- No GUI, remote access, dedicated OTA upload action, or screen/touch mode yet.
- Updating/reflashing with an incompatible partition/NVS layout can erase pairing.
- CI compilation is not a substitute for iPhone and WLED tests.

## Troubleshooting

- `[Config] Invalid IPv4 address`: use `@w192.168.x.x`, no URL or spaces inside the address.
- `[WLED] GET/POST failed`: verify IP, WLED API availability, Wi-Fi routing/firewall.
- HomeKit reports 'No Response': check HomeSpan CLI pairing/Wi-Fi status, WLED network, and whether the bridge was rebooted.
- A successful build doesn't guarantee a specific clone's flash pinout/flash capacity. Confirm your actual D1 Mini/CYD hardware.
