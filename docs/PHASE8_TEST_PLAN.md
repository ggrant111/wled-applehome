# Phase 8 — Saved WLED segments and grouped power semantics

**CI:** [run 42452061034](https://github.com/ggrant111/wled-applehome/actions/runs/42452061034) passed all host tests plus D1 Mini and CYD firmware and OTA budget. Both images 1,564,800 bytes; 466,816 bytes free. This does not certify on-device WLED or Apple Home behavior.

**Reported by user:** v0.7.1 grouped effects work on iPhone. Problem: some WLED preset segment settings do not restore. User would prefer preset power selection rather than momentary switches.

## Diagnose segments first
- [ ] Install v0.8 firmware via OTA WITHOUT erasing NVS, modifying partitions or changing HomeKit pairing.
- [ ] Portal `http://<bridge-ip>:8080` → Preset switches → select WLED light, Load presets, select problem preset, **Inspect saved segments**.
- [ ] Record `savedSegments`, `segmentsWithBounds` and warning. If there are segment entries but fewer saved boundaries, go to WLED, configure desired segments, edit/re-save the preset using **Use current state / Overwrite with state** and **Save segment bounds**. This action is deliberately performed in WLED, not by this bridge.
- [ ] Confirm segment start/stop, segment count, effects and colors restore when activating that saved preset **directly inside WLED**.
- [ ] Repeat from Apple Home. The bridge still sends the identical WLED `{"ps":id}` command; compare direct vs Home.
- [ ] If direct WLED recall restores boundaries but the bridge doesn't, capture WLED `/json/state` before and 1–2 seconds after both actions plus bridge serial logs. Do not guess the segment ID layout.

## Preset power
- [ ] Select Mammoth: Mammoth control remains **On**, other grouped effects Off after a successful WLED poll.
- [ ] Select Wednesday: Mammoth changes to Off, Wednesday stays On, without switching off the light between presets.
- [ ] Turn Wednesday Off: entire WLED light becomes Off; all grouped effects show Off.
- [ ] Turn Wednesday On again: WLED runs Wednesday; no unwanted Solid effect overwrite.
- [ ] Change preset using WLED UI: Home grouped selection updates after background poll (~8s); change brightness/color in Apple Home and confirm On/Off synchronization.
- [ ] Verify standalone preset switches are STILL momentary and the underlying RGB LightBulb and Siri still work.
- [ ] Stop if WLED effect changes are overwritten or other segments are touched unexpectedly.

## Known limits
- HomeKit grouped services remain `Switch` services. This changes behavior/state, not the iOS Home app's widget style.
- An existing preset may intentionally omit segment bounds; this is not a firmware bug and WLED must be told to save geometry to restore it.
- A WLED macro preset may change segments through a custom API; the read-only inspector labels it rather than inferring missing bounds as corruption.
- Do not erase paired bridge NVS or recreate Apple Home accessories for this upgrade.
