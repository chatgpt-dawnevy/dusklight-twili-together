# Phase 0 — Cooperative synchronization audit

Audit baseline: 2026-11-16, Twili-Together 0.5.0, protocol 5.

## Repositories and reproducibility

- Upstream `Ori-Jakob/dusklight-twili-together` main and the fork branch merge base were both
  `0c61df46588f950fc57bed9de9a14f054cfdd38e` at audit time.
- GitHub confirmed the public fork `chatgpt-dawnevy/dusklight-twili-together` and draft PR #1,
  `feature/complete-coop` into `main`. Its local baseline is `8ea3ea3`.
  The older local `enemy-sync` branch ends at `3f90c65`, an ancestor of that baseline.
- CMake pins `Ori-Jakob/dusklight` to `a6dcc2e726d9c94b033cd08634dfaebf62ba4468`;
  the local SDK checkout matches. The public Dusklight upstream is `TwilitRealm/dusklight`.
  README lists Dusklight 2.0.3 as the minimum and documents fork-only overlay support.
  Older runtime services can lack request_quit and host config lookup. The randomizer
  detection fallback uses its active save filename in that case. A test binary reporting
  0.0.0.0 is evidence for that runtime only. It does not establish 2.0.3 compatibility.
- Configure with CMake >= 3.25, a C++20 toolchain and the pinned SDK, then run
  `cmake --build <build-dir>`. Use `-DDUSKLIGHT_DIR=<pinned-sdk-checkout>` to reuse the SDK.
  CI produces separate x86-64 mod bundles for Windows and Linux. Baseline comparison:
  the two builds at `47f800c` passed GitHub Actions and were downloaded. This confirms
  compilation and packaging. Runtime validation is tracked separately.
- No LICENSE or COPYING file was found in the tracked Twili-Together tree. Dusklight's
  SDK checkout contains CC0 1.0 text and separate third-party notices. Record mod
  redistribution authorization and dependency notices before publishing a release.
  Keep the ISO, saves, extracted game assets, and runtime binaries outside the mod
  repository and release. Use each player's own copy of the game.

## Code and coverage

| Area | Current implementation | Evidence and remaining work |
| --- | --- | --- |
| Identity and spawn | EnemySync/EnemyCount match room, actor profile, parameters, set ID, home coordinates and duplicate index. Stage/layer scope comes from the packet. Spawner children have a separate convention. | No independent zone-instance ID, spawn generation, or general authoritatively managed spawn/despawn. Similar dynamic actors and respawns can be ambiguous. |
| Health and death | EnemyDamage derives deltas recent to local hits and clamps remote damage to nonlethal health. EnemySync detects explicit defeat, native endings or paired disappearance/deletion. | No authoritative damage transaction, replay deduplication or health recovery. Simultaneous hits and death races still need verification. |
| Movement and combat | EnemyCombat uses sampled Bokoblin movement, attacks and animation, plus Fyrus knockdown and Deku Toad bomb stun. | The lowest eligible client ID in a room/team is the snapshot source. There is no explicit authority generation, transfer checkpoint, or recovery snapshot. This is separate from both room ownership (settings) and team leadership (game identity). |
| One-hit enemies | Rat, Leever, Bombskit and Shadow Keese handling exists. | Rat and Bombskit have a two-client scripted defeat scenario together with a Shadow Insect. Leever and Shadow Keese still need direct evidence, including native strikes and respawning variants. |
| Twilight | EnemySync applies supported insect deaths and their switches. LightDrops and world sync handle tears and progress. | River insect variant 7 is excluded. Reconnection, delayed packets and sequence completion need tests. |
| Boss progress | BossFight and BossHold keep remote story writes from interrupting local fights. Selected bosses call native ending routines. | Scripted endings are not full shared encounters. Reward uniqueness and mixed-platform progression remain open. |
| Multiplayer options | RoomState and relay carry PvP friendly fire, 100–500% health, 100–300% compatible enemy count and optional shared consumable stock. | Relay tests cover routing/settings. Two Linux instances verified shared and individual rupee effects. All resources, concurrency across rooms and real network behavior need coverage. |
| Randomizer | GameIdentity/TeamGame scope sync by game, compatibility and seed evidence. | Portal, keys, Poe souls, bottle and trap scenarios passed on Linux. A complete seed on a real mixed-platform session is unverified. |

### Boss evidence already available

| Encounter / primary actor | Existing shared behavior | Two-client evidence and limits |
| --- | --- | --- |
| Diababa / B_BQ | Native ending | Scripted end scenario exists. Full fight, head/projectile control and rewards across reconnect need verification. |
| Fyrus / E_FM | Native ending; knockdown counter | Verified Linux native vulnerable phase. Chains, heat attacks and full combat remain open. |
| Deku Toad / E_DT | Native ending, nonlethal health, bomb stun | Verified Linux scripted bomb reaction. Intro, spawned tadpoles and full encounter remain open. |
| Twilit Bloat / E_YMB | Native ending | Full phases, spawning and tears sequence unverified. |
| Argorok / B_DR | Nonlethal damage | Native kill starts a separate B_DRE child and timed exit. Flight, phase transitions and child lifecycle are unverified. |
| Death Sword / E_VT | Native ending | Linux fixture bypassed opening. Both clients reached native clear in both directions. No full fight validation. |
| Dangoro / E_GOB | Native ending | Linux fixture placed both copies in combat. Both finished the native scene and opened the exit in both directions. |
| Blizzeta / B_YO | Experimental ending reverted by 8ea3ea3 | Both clients entered the ending but timed out before the heart and warp were confirmed. |
| Other encounters | See arena/profile lists in BossFight.cpp | Need an actor, phase, attack, damage, death and reward adapter inventory per encounter. |

## Test evidence and gaps

`tools/server` has relay unit/integration tests (126 passed on the recorded branch);
`tools/autotest` verifies actual instances, actor state, progression and scenarios rather
than just messages. The Linux runner requires private Xvfb. It uses isolated save directories
and fails before launching the game if no virtual display is available. The workspace contains
historical test output under `tests-coop`, including both Death Sword results in
`boss-results/20261115-231109`, Dangoro in `boss-results/20261115-235113` and the
failed Blizzeta scenario in `boss-results/20261115-235025`.
Windows/Windows, Windows/Linux, WAN, and full vanilla and randomizer campaigns remain open.

Next: introduce a relay-issued authority generation scoped to the team/game and stage/room/layer.
Keep existing gameplay snapshots until generation validation and transfer tests pass. Add
stable spawn incarnations before applying snapshots or deduplicating damage across respawns.
