# Native revive admission candidate

`src/runtime_revive_prepare.h` connects ordinary Tick to the authored
factory/service. The guest has local Ashley and a validated role1 remote
Shepard presentation slot; the
[host preparation path](REVIVE-HOST-PREPARATION.md) has separate story-pawn and
guest-companion slots. The ordinary runtime now enables this path when its
native owner-PC bridge is installed; `ME3_SQUAD_COOP_REVIVE=0` disables it.
The private owner-derived addon must be loaded on both peers. The old
`ME3_SQUAD_COOP_REVIVE_PREPARE_QA=1` still permits closed preparation alone.
This candidate has not demonstrated mutual revival in-game. The protected
probe does not include this path.

The candidate requires an enabled, transport-scoped ordinary session and the
current native world. Guest admission uses registered reciprocal local Ashley
ownership; host admission uses the captured story/possession lease and the
exact guest connection. Every scope also captures its native actor-channel,
connection and driver identity/index.
It resolves unique full identities for the already loaded factory class,
factory default object and service class under `ME3SquadCoopActions`. It does
not load packages or animations. It now also resolves the exact already-loaded
`ME3SquadCoopActions.ReviveAnimSet` from the
[private owner-derived package](REVIVE-ASSET-PACKAGE.md), checks its native
Engine.BioDynamicAnimSet class, and supplies it with presentation=true only for
the guest's remote Shepard. Missing
animation delays preparation without blocking retirement of an existing
service. The private addon declares startup/seekfree loading; actual runtime
loading and shared availability remain owner checks.

## Reflected call boundary

For CreateFor, Status, CanActivate, Activate, Suspend, Retire, RetireForHandoff
and DestroyRetired, the bridge
walks actual loaded UFunction/UProperty metadata using the supported Core SDK
layout. It checks exact function owner, function/property class identities,
static versus instance policy, nonnative/nonnetwork flags, parameter count,
names, scalar dimensions, property sizes and object parameter class paths.
It reads parameter offsets, return offset and single-bit bool masks at runtime,
rejecting duplicate/overlapping/out-of-bounds fields. Buffers are bounded and
zero-initialized. No C struct padding or disk-export offset is assumed to be
the runtime ABI. This is implemented validation, not evidence that an actual
loaded function passed it.

Calls are confined to the recorded engine game thread and current world with
live full object stamps and an in-image ProcessEvent entry. Reentrancy is
blocked while dispatching. After any call, ordinary guest adapters refresh
the whole native transport/world gate and local context. Loaded identities
are cached and revalidated; missing/changed contracts retry one shared object
snapshot scan no more than once per five seconds. Service polling is bounded
to 250ms cadence.

## One captured preparation lifetime

One factory attempt per slot belongs to captured controller/pawn/world/channel
stamps and the guest registration generation or host session epoch. A positive local token is never reused
within the process. The returned service is retained even if its initialization
failed; status0 does not trigger repeated spawning. A nonzero unvalidated
return blocks replacement rather than authorizing cleanup against an unknown
object. A None return is recorded without retrying the same captured scope.

The script factory now marks an unused actor retired with the matching token
if spawn callbacks invalidated Initialize before it captured any setup. That
actor can follow normal DestroyRetired handling without touching pawn state.
Partial initialization with real Setup still uses normal ordered retirement.

Status1 means prepared with admission closed; status2 means active. Both are
retained during normal actions and downed-target handling. Initial factory
creation waits for idle actions and a living pawn, so ordinary busy input does
not waste the one attempt. Other statuses or scene/disable/ownership/context loss
latch retirement. RetireForHandoff is polled before guest adapters may release
their context. Its Boolean result is not treated as disposal: destruction is
requested only after a fresh Status4. A missing read or changed current world
retains the old handle without calling old-world objects; positive registry
disposal permits dropping the native handle. A new preparation still requires
a new admitted scope.

## Native peer agreement

`runtime_revive_peer.h` shares the existing ServerSay/ClientAdminMessage owner-PC
RPC wrappers with remote presentation. Exact native receive frame, actor channel,
connection, driver, world and registration stamps authorize the source; parsing
a message does not authorize anything. Receive callbacks only queue bounded
scalars. All authored calls happen on normal Tick at a bounded cadence.

ME3RV1 carries a guest nonce, host nonce, canonical Shepard/Ashley channel indices
and the loaded package GUID. No pointers, health, use target or gameplay command
is serialized. A query/offer/arm/commit/ack exchange binds the same two local
service tokens and full lifetime stamps on each peer. Both slots must pass the
authored idle/pending preflight before opening admission. Wrong phases, nonces,
channels, GUIDs or stale scopes cannot activate. Partial native activation failure
uses ordinary authored retirement, without forced Stop or slot overwrites.

Active peers exchange a heartbeat once per second. Three seconds of silence, or
an unconfirmed commit lasting more than three seconds, suspends admission while
retaining the same prepared actions/animation. Retransmissions cannot extend
that commit deadline. A fresh agreement can rearm these existing services.
Actual ownership/world changes retire them instead. An old agreement cannot
close a later preparation. Narrative release is never blocked by cleanup.

The normal Used/TryUse/StartRevive path remains the gameplay entry; this bridge
does not force resurrection or change health. Native fixtures cover scalar
parsing, pair identity/freshness, preflight policy and phase/deadline refusal.
Seven authored classes compile without errors/warnings. Runtime ABI acceptance,
normal interaction delivery, actual animation and mutual revival remain untested
in-game. No game was launched or package installed for this integration.
