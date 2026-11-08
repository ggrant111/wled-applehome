# v0.9.1 — Saved WLED playlists in Apple Home (hardware acceptance)

**CI verified:** [run 42505219721](https://github.com/ggrant111/wled-applehome/actions/runs/42505219721) passed all host tests, D1 Mini/CYD builds and OTA-size gates. **Hardware still pending.**

WLED's JSON API documentation confirms saved playlists reside in numbered entries of `/presets.json`, started via `{"ps":playlistId}`. The `pl` field reports the currently loaded playlist, while `ps` can represent the currently playing step.

## Update without losing HomeKit state
- [ ] Keep v0.9 working firmware available and note IDs/names of original and second grouped effects accessories.
- [ ] OTA install matching v0.9.1 D1 Mini or CYD image; do not erase NVS, change partitions or re-pair Apple Home.
- [ ] Open authenticated bridge dashboard on port 8080; select WLED light; load **presets & playlists**. Verify entries display distinct "Preset" and "Playlist" labels.
- [ ] Choose a saved WLED playlist with at least two component presets and add to the existing Effects group. Maximum five total entries. Reboot to publish the newly appended switch.

## Verify playlist behavior
- [ ] Activate playlist in Apple Home. WLED runs its preset steps at the configured durations/transitions; Home reports playlist control On through at least THREE different steps (WLED `ps` changes).
- [ ] A child preset which is ALSO exposed as a normal Effects control must remain Off during playlist playback; only the Playlist control should report On.
- [ ] Tap another preset in Home. WLED stops previous playlist; the playlist button turns Off and normal preset button turns On within the next successful WLED poll.
- [ ] Start a playlist directly on WLED. Apple Home reflects correct running playlist after state polling (healthy devices around 8s).
- [ ] Turn running playlist Off from Apple Home. Associated WLED light powers Off, and all Effects controls go Off. Other WLED devices/groups remain unaffected.
- [ ] Test a finite/repeating playlist through its final step: when WLED `pl` becomes -1, playlist control turns Off; if the playlist ends on a saved preset, its normal preset control may show On.
- [ ] Check a playlist that had been enrolled as a generic preset in v0.9: add the SAME ID again and verify it becomes typed "Playlist" without a new Effects AID; restart and verify on/off synchronization.
- [ ] Restart bridge; all earlier light, original grouped, and second grouped AIDs/names remain the same. No WLED playlist data changed by bridge.

## Not included
- The bridge does not create a new playlist sequence, alter steps/durations/repeats, or expose a custom Apple Home playlist editor. Configure the playlist inside WLED.
- On very short steps below the bridge's polling interval, the dashboard may not observe each child step, but `pl` should remain stable so the playlist's Home switch stays On.
- Physical hardware playlist behavior and long-running eight-device stress are open gates until actually observed.
