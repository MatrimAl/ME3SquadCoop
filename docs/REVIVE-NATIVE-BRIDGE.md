# Native revive preparation candidate

`src/runtime_revive_prepare.h` connects the ordinary guest Tick to the authored
factory/service. It is disabled by default. The explicit research opt-in is
`ME3_SQUAD_COOP_REVIVE_PREPARE_QA=1`, read once per process. This is preparation
work, not a supported player setting or a completed revive feature. The
protected probe does not include this path.

The candidate requires an enabled, transport-scoped ordinary guest session,
registered reciprocal local party-pawn ownership and the current native world.
It resolves unique full identities for the already loaded factory class,
factory default object and service class under `ME3SquadCoopActions`. It does
not load packages or animations. The initial call supplies LoadedSet=None and
presentation=false: preparation can succeed only if the pawn mesh already
resolves the required animation. Shared asset availability remains unfinished.

## Reflected call boundary

For CreateFor, Status, Retire, RetireForHandoff and DestroyRetired, the bridge
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

One factory attempt belongs to captured controller/pawn/world stamps and the
guest context registration generation. A positive local token is never reused
within the process. The returned service is retained even if its initialization
failed; status0 does not trigger repeated spawning. A nonzero unvalidated
return blocks replacement rather than authorizing cleanup against an unknown
object. A None return is recorded without retrying the same captured scope.

The script factory now marks an unused actor retired with the matching token
if spawn callbacks invalidated Initialize before it captured any setup. That
actor can follow normal DestroyRetired handling without touching pawn state.
Partial initialization with real Setup still uses normal ordered retirement.

The bridge never calls Activate. Status1 means prepared with admission closed,
not peer agreement. Any other status or a scene/disable/ownership/context loss
latches retirement. RetireForHandoff is polled before guest adapters may release
their context. Its Boolean result is not treated as disposal: destruction is
requested only after a fresh Status4. A missing read or changed current world
retains the old handle without calling old-world objects; positive registry
disposal permits dropping the native handle. A new preparation still requires
a new admitted scope.

## Unfinished integration

This preparation-only path does not hold back a narrative handoff and does not
claim its initial cleanup request precedes every native scene transition.
It retains unresolved cleanup instead of opening admission or overwriting a
slot. Runtime host services, simulated-peer counterparts, shared loading,
authenticated peer readiness, production pre-handoff ordering and actual
mutual revival remain unfinished. No game, package installation, runtime ABI
observation, factory dispatch, fixture or behavioral test was performed for
this revision. All three native targets built; seven authored classes compiled
without errors/warnings with unchanged tracked inputs. A fresh test stage is
required because both native code and authored package changed.
