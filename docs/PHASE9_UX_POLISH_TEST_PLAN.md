# v0.9.3 — Screenshot-driven dashboard refinement

## Visual and navigation
- [ ] OTA-install board-appropriate v0.9.3, preserving ESP32 NVS and HomeKit pairing.
- [ ] On desktop monitor, confirm the Overview dashboard takes advantage of more of the available width and text is more legible; no horizontal overflow.
- [ ] Open Effects & Playlists: first configured Effects group is shown initially rather than a light with zero favorite controls.
- [ ] Desktop has **only one** device selection surface: named WLED group tabs. Switch between five lights and ensure the correct group/0-favorites empty state appears.
- [ ] On iPhone, tabs are replaced by a single touch-friendly device dropdown. Confirm no duplicated selection controls, clipped buttons, or too-small tap targets.
- [ ] For an existing Effects group, the preset/playlist list is the prominent content and the Add editor is initially collapsed.
- [ ] For an unconfigured light, the Add editor opens; select Load from WLED, then choose a preset or playlist. Empty catalog displays a meaningful message rather than a misleading unloaded state.
- [ ] Switching between different WLED devices invalidates the prior catalog selection and disables Add until the new device's catalog is loaded.
- [ ] After selecting an explicit light, periodic live status refreshes do NOT change selected light or interrupt adding a favorite.
- [ ] Verify Run button, Rename, Reorder/Remove warnings, HomeKit restart banner, preset switches, device discovery and bridge settings still work.
- [ ] Verify existing lights and grouped Effects accessories in Apple Home, including their current AIDs, remain unchanged.

## Automated checks
- [ ] Dashboard JS VM smoke & CSRF regressions and gzip compression.
- [ ] D1 Mini and CYD PlatformIO builds + OTA-size guard.
- [ ] Actual user desktop/mobile test distinct from successful CI.
