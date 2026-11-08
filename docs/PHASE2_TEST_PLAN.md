# Phase 2 — Physical acceptance test plan

> Validate using the existing paired ESP32. Record dates, screenshots and serial logs in `docs/PROGRESS.md`. Compilation alone does **not** verify HomeKit behavior.

## Test A: migration (critical regression)
- [ ] Note existing Apple Home accessory and WLED IP, flash firmware **without NVS erase**.
- [ ] CLI `@l` lists original device as AID=2 with the old IP.
- [ ] The original WLED light continues to respond in Apple Home **without repairing**.
- [ ] Apple Home on/off, 25% brightness, and 80% brightness work.
- [ ] Changes made in WLED UI update Apple Home.
- [ ] Reboot with `R`; original light stays paired.

## Test B: discovery
- [ ] `@d` enumerates WLED devices and displays sensible names/IP/MACs.
- [ ] All results pass `/json/info` verification.
- [ ] Already-enrolled device can be selected again without creating a duplicate AID.
- [ ] Try `@m<IP>` for an undiscovered but reachable WLED.
- [ ] Invalid IP and a non-WLED IP do not modify the registry.

## Test C: new light and re-pairing protection
- [ ] `@a<number>` adds a different controller with an AID greater than 2.
- [ ] Reboot `R`; Apple Home adds another independent light **without losing pairing**.
- [ ] Both lights' power and brightness work independently.
- [ ] Power off/change brightness from WLED and verify Home state updates.

## Test D: identity and lifecycle
- [ ] Adding the same MAC with a new IP changes its existing endpoint without creating another AID.
- [ ] Remove a light using `@r<AID>`, reboot, and confirm it disappears from Apple Home.
- [ ] Add a different controller afterward; it gets a **new** AID (old AID not recycled).
- [ ] Repeated boots do not reorder or duplicate devices.
- [ ] Power loss/restore keeps Wi-Fi, pairing and registry.

## Test E: capacity and resilience
- [ ] Enroll 3 or more WLED devices and test independent commands under load.
- [ ] Scale to 8 active lights; measure Home responsiveness, heap and request failures.
- [ ] Simulate an offline WLED; other lights stay responsive and the offline light recovers.
- [ ] Confirm discovery does not turn on/off or change any WLED states.
- [ ] Note mDNS query latency and potential HomeKit delays before closing Phase 2.

## Known limits

Scan/HTTP operations are **currently blocking**. Deliberately staggered polls and 650ms timeouts reduce the impact but do not eliminate it. Web UI, renaming and full nonblocking networking remain open roadmap work.
