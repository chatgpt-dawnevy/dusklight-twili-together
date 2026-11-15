# Cooperative synchronization test report

## 2026-11-16 — Combat snapshot sequencing

Twili-Together 0.5.0, commit `d885f4b3d3755e1df2314d946c39ac41c8e99688`.
Built locally for Linux x86-64 using the Dusklight SDK pinned at
`a6dcc2e726d9c94b033cd08634dfaebf62ba4468`. The game runtime identifies
itself as `0.0.0.0`. Relay on localhost and two separate Linux save profiles
on a private Xvfb display.

| Scenario | Result | Scope |
| --- | --- | --- |
| Relay combat routing | Passed | Wrong room, layer, team and stage excluded. Non-authority traffic rejected. Relay replaces client sequence and keeps it monotonic through a handoff. |
| Bokoblin combat | A and B passed | Receives position, facing, animation, combat phase and active collision state. Scripted Linux fixture, without packet reordering, a full fight, or WAN. |
| Windows/Windows and Windows/Linux | Not run | Build status is separate from gameplay validation. |
| Long campaigns and authority reconnect | Not run | Existing targeted fixtures cannot establish whole-run coverage. |

Local reproduction evidence: `tests-coop/authority-results/20261116-000631/enemy-bokoblin-combat/`
contains both scripts, results, stderr and the relay log. The runtime lacks
`HostService.request_quit`; the runner ends each process after reading its
result. Command: `node tools/autotest/run.js --exe <dusklight> enemy-bokoblin-combat`.

Earlier evidence and the Blizzeta failure are indexed in [audit.md](audit.md).

## 2026-11-16 — Authority handoff after disconnect

Same Linux runtime and pinned SDK as above. The `enemy-combat-authority-reconnect`
scenario passed on both clients with isolated saves and a localhost relay.
A disconnected, B inherited room ownership without disabling combat sync,
and A rejoined with a new client ID. A then observed B's Bokoblin swing,
including position, facing, animation and collision activation. The test injects
the attack into B's local actor. It does not verify recovery of a shared health state.

Local evidence: `tests-coop/authority-results/20261116-001548/enemy-combat-authority-reconnect/`.
The relay logged one missed heartbeat before the scripted disconnect and successfully
reconnected A. WAN and Windows gameplay remain untested.
