# Phase B — authoritative enemy damage design

**Status: design only; no relay or game-client implementation is claimed here.**

## Why this is a design gate, not an owner-only relay check

The current v5 protocol deliberately allows an `ENEMY_DAMAGE` sender to be any teammate in the same stage and layer. A hit is derived by that sender's game and contains an enemy spawn key plus `dmg`, `pct`, `hpAfter`, and optional Chu Worm `hits`. Receivers reduce their own copies' health; native death is a separate `ENEMY_DEFEATED` event. The relay validates/deduplicates a sender's optional `sourceSequence` and routes the message to same-stage/layer teammates. When present, the source sequence is preserved in the relayed packet (the relay tracks its watermark per connection); the protocol version is also part of recipient matching. The relay does not establish that a hit occurred or reconcile two copies' health.

By contrast, the current `ENEMY_COMBAT` sender election selects the lowest eligible client ID in the same team/game, stage, room, and layer. That election is for transient combat snapshots, not damage. `ENEMY_COMBAT` sequence numbers and its per-sender input sequence are explicitly distinct from damage validation. The existing Bokoblin health checkpoint is a one-way, non-increasing replica update; it does not resolve competing hits or provide a complete authority checkpoint.

Therefore, simply rejecting `ENEMY_DAMAGE` from a non-owner would reject valid attacker traffic under the published protocol and would not make owner-supplied damage trustworthy. No code change is safe until sender/authority/replica roles and the game-side validation and commit path are defined together. In particular, the relay cannot infer a legitimate game hit from the current damage fields.

## Proposed wire contract (new, negotiated protocol revision)

Keep the v5 `ENEMY_DAMAGE` behavior unchanged for v5 peers. Introduce an explicitly negotiated versioned transaction path for updated peers; do not silently reinterpret v5 packets. Names below describe required information, not implemented packet names.

Every transaction must identify:

- **Combat scope:** team ID and game identity/key, stage, room, and layer. Relay derives sender identity from the authenticated connection; clients cannot assert it.
- **Zone generation:** relay-issued monotonically changing generation for the scoped combat instance. Bind it to the team/game and stage/room/layer, and invalidate it on scope exit or authority re-election.
- **Authority lease:** relay-issued generation/token, current authority client/connection, and lease validity. Scope it to the zone generation. A client-provided owner ID alone is not proof of authority.
- **Enemy incarnation:** the existing enemy spawn key (including the placement/duplicate data used by `ENEMY_DEFEATED`) plus a spawn generation assigned/confirmed for this appearance. A respawn must not inherit pending damage or a death from an earlier incarnation.
- **Proposal identity and ordering:** sender-connection identity (derived by relay), a strictly increasing sender sequence, and a unique proposal/hit identity. Include the expected zone, lease, and spawn generations. Replays are idempotent; gaps are allowed; stale or malformed values do not advance a watermark.
- **Hit evidence:** the existing bounded damage facts needed by the client-side game adapter, with a precisely versioned schema. Treat the attacker's report as a proposal, not as an authoritative health result. The game adapter must validate/apply the native reaction and report its outcome to the elected authority.
- **Commit:** authority-issued transaction ID/sequence, incarnation, accepted hit(s), canonical resulting health (and any counted-hit state), and a checkpoint revision. The authority only commits outcomes validated by its game adapter. Death remains a distinct, incarnation-bound native defeat confirmation; it cannot be inferred from a stale health proposal.

The relay's role is authentication, scope/lease enforcement, ordering, deduplication, and delivery. It cannot certify game physics or native combat. The authority client must not accept a peer's asserted `hpAfter` as truth; updated clients need a client implementation that can validate/apply the hit and produce a committed result. Do not broadcast provisional damage as committed state.

## State machine

1. **Unleased:** No usable authority/checkpoint exists for this zone generation. Do not accept authoritative commits. Preserve legacy v5 routing only for v5 peers; do not mix legacy damage mutations into the new canonical stream.
2. **Electing:** Relay elects from currently eligible, synchronized clients using an explicitly specified deterministic rule (initially compatible with the existing lowest-ID election only if its membership, game key, and tie/leave behavior are fully specified). Issue a new lease generation. A client joining/leaving or changing room/layer/team-game scope invalidates or re-evaluates the lease before accepting more commits.
3. **Recovering:** New authority loads the latest committed checkpoint for this zone and each live enemy incarnation. It publishes a checkpoint acknowledgement for the new lease. If no trustworthy checkpoint exists, do not merge independently mutated health by taking a minimum or maximum; mark that enemy unresolved and use a documented safe resynchronization policy (for example, defer to the local native encounter/reload boundary), not a fabricated value.
4. **Active:** Any valid teammate attacker may submit a hit proposal; attacker need not be the authority. Relay checks sender scope, negotiated protocol, current zone/lease, incarnation, schema, and sender sequence, then forwards the proposal to the current authority. Authority validates the proposal through the native game adapter, applies at most once, and returns a commit or explicit rejection. Relay validates that commit came from the leased authority and broadcasts it to eligible replicas. Replicas apply commits idempotently and never treat a proposal as a commit.
5. **Handoff:** On authority disconnect or scope change, stop accepting commits under the old lease. Retain only acknowledged committed state; reject late old-lease proposals/commits. Elect a successor with a new lease, recover from the latest checkpoint, then resume. Pending but uncommitted proposals may be retried with the same identity and resolved once, or explicitly rejected; they must never be applied twice.
6. **Ended/respawned:** A committed native defeat closes the incarnation. Reject later hits for it. A new appearance receives a new spawn generation even when its legacy spawn key is identical.

Generation comparisons are equality checks within their scope, not client wall-clock time. A reconnect is a new connection identity and cannot reuse another connection's sender watermark. Persist/recover only the state needed by this protocol; specify process-restart behavior before claiming durability.

## Acceptance tests required before enabling the new path

### Relay and protocol tests

- Valid non-owner attacker proposal is delivered to the current authority; it is not rejected merely because the attacker is not snapshot authority. Other teams, games, stages, rooms, and layers receive nothing.
- Wrong/missing zone generation, stale/future lease, non-authority commit, old spawn generation, closed incarnation, malformed bounds, and packets from unsynchronized/ineligible clients are rejected without advancing valid sequence state.
- Duplicate, older, retransmitted, and reordered proposal/commit identities are idempotent. Distinct senders may use equal local sequence values; sequences are connection-scoped. Gaps are accepted if the protocol allows them.
- Concurrent hits from two valid attackers converge to one ordered set of commits and the same resulting health on every replica; retries of either hit do not apply damage twice.
- A handoff invalidates delayed messages from the previous lease, loads the last acknowledged checkpoint, and resumes without healing, double damage, or accepting a stale defeat.
- Changing stage, room, layer, team-game identity, disconnecting/reconnecting, and a room with no eligible authority each exercise lease revocation/recovery. No packet leaks across scopes.
- A respawn with the same legacy placement key has a distinct incarnation: old damage, checkpoint, and defeat cannot affect it.
- v5 compatibility is explicit: v5-only sessions retain their documented current behavior; mixed v5/new sessions either use a documented safe fallback that cannot double-apply state or disable authoritative mode with a clear negotiation result. No silent partial upgrade.

### Game-adapter and two-client tests

- Real native hits by either client (including a non-owner attacker) cause one commit, the expected native reaction, and matching health on both clients.
- A forged or impossible client-reported hit/`hpAfter` is refused by the validating adapter; a valid hit whose reaction is delayed by an event is committed once after it is safe to apply.
- Two near-simultaneous hits, hit during authority handoff, authority loss after apply but before acknowledgement, and retransmission after reconnect converge exactly once.
- A hit immediately before defeat, hit after committed defeat, native one-hit enemies, Chu Worm counted hits, health multipliers, and supported boss phase/native damage adapters preserve their existing native behavior and never produce two deaths/rewards.
- Authority migration with a nonlethal enemy resumes from a checkpoint, while two clients that independently fought during a partition follow the documented conflict policy rather than silently choosing whichever health is lower.
- Run existing relay suite plus the new tests, and two-client native scenarios on supported platforms. Record platform, protocol versions, fixtures, logs, and untested limits; relay unit tests alone do not establish gameplay correctness.

## Implementation sequence and non-claims

1. Freeze the versioned message schema, compatibility negotiation, scope-generation rules, spawn-incarnation source, checkpoint persistence/restart policy, and native adapter responsibilities.
2. Add pure schema/state-transition tests and relay tests before enabling acceptance of commits.
3. Implement relay lease, proposal, commit, ordering and handoff enforcement without changing v5 routing.
4. Implement the game-side validator/commit adapter and update clients atomically with negotiation/fallback.
5. Run relay and two-client tests above; only then enable the new mode by default.

## Exact currently established v5 `ENEMY_DAMAGE` contract

This section records existing behavior, not the proposed transaction contract above. Source of truth checked: `docs/protocol.md`, `src/enemy/EnemyDamage.hpp`, `src/enemy/EnemyDamagePacket.cpp`, `src/enemy/EnemySync.hpp`, `src/enemy/EnemySync.cpp`, and `server.js` on the feature branch.

- **Connection protocol:** handshake `protocolVersion: 5`; WebSocket subprotocol is `twili-together.5`. The relay requires at least its configured protocol version and routes team presence only to peers with the same `protocolVersion`. `ENEMY_DAMAGE` itself has `v: 1`; this is not a protocol-version negotiation for an authoritative transaction.
- **Sender packet:** `type: "ENEMY_DAMAGE"`, `v: 1`, `quiet: true`, `stageName` (string), `layerNo` (integer), `hits` (array, 1–16), `sourceSequence` (positive safe integer when the updated sender includes it), and `sourceEnemyZoneEpoch` (the sender's currently advertised nonzero enemy-zone epoch on updated senders). Common synchronized packet metadata is stamped by `sync::stampPacket(packet, false)`; this does not add an authority lease or commit identity.
- **Each `hits[]` element:** spawn key `{roomNo, procName, params, setId, home, dup}` and `dmg` (0–30000), `pct` (100–500), `hpAfter` (-1–30000), optional `hits` (0–4; omitted when zero). The client parser requires a valid spawn key and those ranges, and rejects `dmg == 0 && hits == 0`. The documented key has `dup` 0 or 1–4 for Enemy Count extras; `home` is a three-coordinate placement. `EnemySync::SpawnKey` types are `int8_t roomNo`, `int16_t procName`, `uint32_t params`, `uint16_t setId`, three `float home` coordinates, and `uint8_t dup`. The protocol documentation defines a spawner child's synthetic `setId`/`home`; those conventions are still not a spawn-incarnation generation.
- **Relay checks:** stale/mismatched `sourceEnemyZoneEpoch` is rejected before damage handling when supplied; unmarked legacy sends remain accepted. If `sourceSequence` is absent, `acceptSourceSequence` allows it only while that connection's damage watermark is zero. If present, it must be a safe integer greater than the connection watermark; for a numbered packet, relay also requires `v === 1`, a 1–16 element `hits` array, online/save-loaded sender, and `stageName`/`layerNo` equal to the sender's current state. It updates the watermark and relays the packet; it does not validate each hit's spawn key or damage numbers. Common delivery is to online, save-loaded teammates in the same team, stage and layer, with sync enabled and the same protocol version; sender is excluded. Relay replaces any sender-supplied recipient epoch and stamps `targetEnemyZoneEpoch` per recipient when known. Damage is not cached. `sourceSequence` is not authority ordering: it is sender-connection deduplication, and the current relay preserves it in the relayed JSON.
- **Client effect:** sender detects local health/count changes and batches them; recipients match the local spawn key and lower their own health using the damage/percentage and (when lower) the sender's `hpAfter`, respecting local floors. Chu Worm counted hits are decremented without allowing this path to deliver the final counted hit. This is replica mutation based on peer-reported state, not an authority-approved result. `ENEMY_DEFEATED` remains separate (packet `v: 2`) and carries the spawn key plus native defeat effects; no common spawn generation distinguishes a reloaded identical key.

## Required migration/version boundary

Do not retrofit authority semantics onto `v: 1` packets or silently treat `sourceSequence`, `sourceEnemyZoneEpoch`, `targetEnemyZoneEpoch`, `ENEMY_COMBAT.sequence`, or the snapshot election as a lease/commit. The client handshake and relay currently use protocol v5, and the damage message's own `v: 1` is already assigned. Before implementation, allocate a new connection protocol version and distinct versioned proposal/commit message schemas; document whether the damage message sub-version is bumped or new message types are used. Define capability negotiation in the handshake/response (including explicit authoritative-mode availability), server routing/validation for that capability, and an explicit mixed-version policy. Do not assume the current server's minimum-version handshake alone provides capability negotiation: it accepts newer numbers, but only same-version peers receive team packets.

Migration order must be: (1) freeze exact wire schema, scope and spawn generations, checkpoint/restart durability, and game-adapter validation; (2) add relay-only schema/state tests; (3) implement relay proposal/commit/lease validation alongside untouched v5 routing; (4) implement the game client adapter and atomic negotiation/fallback; (5) run relay suite and supported two-client native scenarios. Keep v5 sessions on their published legacy behavior, and disable authoritative mode for incompatible/mixed sessions unless a tested fallback prevents legacy and committed state from double-applying. No new path is enabled until those tests pass.

This note does not change relay or C++ gameplay code, does not claim Phase B is complete, and does not claim that current damage is authoritative or verified by the relay.
