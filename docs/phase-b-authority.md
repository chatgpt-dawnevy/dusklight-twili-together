# Phase B — authoritative enemy damage design

**Status: design only; no relay or game-client implementation is claimed here.**

## Why this is a design gate, not an owner-only relay check

The current v5 protocol deliberately allows an `ENEMY_DAMAGE` sender to be any teammate in the same stage and layer. A hit is derived by that sender's game and contains an enemy spawn key plus `dmg`, `pct`, `hpAfter`, and optional Chu Worm `hits`. Receivers reduce their own copies' health; native death is a separate `ENEMY_DEFEATED` event. The relay validates/deduplicates a sender's optional `sourceSequence` and routes the message to same-stage/layer teammates. It does not establish that a hit occurred or reconcile two copies' health.

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
- A handoff invalidates delayed messages from the previous authority, loads the last acknowledged checkpoint, and resumes without healing, double damage, or accepting a stale defeat.
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

This note does not change server code, C++ gameplay code, protocol version 5, or tests. It does not claim that damage is currently authoritative, that a hit is cryptographically or physically verified by the relay, or that the proposed schema/state machine has been implemented or tested.