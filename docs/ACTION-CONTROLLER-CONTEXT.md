# Companion action controller context

Existing AI-created custom actions may retain their creation-time controller
context after possession. This is a static integration question, not a diagnosed
runtime fault. The retained VerifyCAHasBeenInstanced script assigns m_oPawn and
either m_oAI or m_oPC when creating an instance; populated instances skip that
creation block. Native/inherited refresh behavior remains unresolved.

A bounded owned-package follow-up decoded seven functions completely:

| Function | Serialized / VM bytes |
| --- | --- |
| BioCustomAction.ClientDoCustomAction | 135 / 215 |
| BioCustomAction.ServerStartCustomAction | 90 / 138 |
| SFXCustomAction_DrawWeapon.StartCustomAction | 92 / 144 |
| SFXCustomAction_DrawWeapon.StopCustomAction | 125 / 197 |
| SFXCustomAction_HolsterWeapon.StartCustomAction | 117 / 177 |
| SFXCustomAction_HolsterWeapon.StopCustomAction | 125 / 197 |
| SFXPawn_Player.PossessedBy | 93 / 137 |

Draw/holster Start and Stop delegate to BioCustomAction's shared lifecycle and
operate on m_oPawn's body stance. Their Stop methods resolve that pawn's current
Weapon and notify EquipFinished/UnEquipFinished; no controller-cache assignment
appears in these four overrides. The shared lifecycle's previously inspected
cached-controller use still matters. ClientDoCustomAction forwards the pawn's
replicated action/target/power information into StartCustomAction. The inspected
SFXPawn_Player.PossessedBy override updates personalization game effects after
BioPawn possession; it contains no explicit custom-action cache refresh.
These observations do not rule out native refresh methods elsewhere.

The presentation adapter now retains its shoulder-camera adjustment during a
draw/holster action only when that actual action references the controlled pawn,
the current player controller and no AI controller. It reads m_oAI at+0xa8 at
eligibility and final validation, alongside the existing pawn/controller and
camera checks. An unreadable or non-null AI link refuses retention and falls
back to the existing camera cleanup policy. It does not stop the action, rebind
it, clear references or alter camera behavior outside this opt-in adapter.

This aligns the camera's action-context predicate with the existing guest
power-order requirement. It does not repair a stale action context or establish
that such a context occurs during real guest play. Native initialization or
refresh needs further evidence before writing these caches.

All seven serialized/expanded lengths matched and explicit branch targets
aligned with decoded expressions. Package and compression-library hashes were
unchanged before/after the offline read. Extracted bodies remain private.
The narrowed camera change was compiled only; no game, fixture or test ran.

## Base possession follow-up

The owned Engine package was then inspected separately. Eight additional
functions decoded completely with matching serialized/VM lengths:

| Function | Serialized / VM bytes |
| --- | --- |
| Controller.Possess | 241 / 337 |
| Controller.UnPossess | 43 / 59 |
| PlayerController.Possess | 437 / 641 |
| PlayerController.UnPossess | 135 / 179 |
| Pawn.PossessedBy | 265 / 381 |
| Pawn.UnPossessed | 48 / 68 |
| Pawn.UpdateControllerOnPossess | 32 / 48 |
| Weapon.CacheAIController | 50 / 78 |

Pawn.PossessedBy assigns Controller and replication-related fields, invokes
UpdateControllerOnPossess and reaches a weapon CacheAIController call. The
player-controller possession path also invokes Restart and touches sequence
events; replaying the whole possession sequence is not a narrow action-cache
repair. The existing session already enters normal PlayerController.Possess.

The decoded UpdateControllerOnPossess body only conditionally sets the current
controller's rotation from the pawn's rotation according to vehicle-transition
state. It does not refresh BioCustomAction.m_oPC/m_oAI. No same-named Function
export override was found in the retained SFXGame symbol inventory. This bounds
the inspected script path, not native behavior or other packages.

Weapon.CacheAIController is a different cache: it assigns Weapon.AIController
from AIController(Instigator.Controller), or clears it if Instigator is absent.
It does not enumerate or modify the pawn's custom-action instances. It must not
be substituted for refreshing their controller context or called repeatedly
to conceal an unmeasured ownership problem.

A targeted Function-name inventory of BioPawn, SFXPawn, SFXPawn_PlayerParty,
SFXPawn_Henchman and BioCustomAction exposed native reset leads such as
SoftResetMovementAndAnimationState and HardResetActionAndAnimationState, but
their names and bridge declarations do not establish cache-refresh semantics.
No reset, additional possession or cache write was added from these leads.

All22 explicit branches in the eight decoded functions aligned with expression
boundaries. Engine package and compression-library hashes were unchanged.
Private extraction/decoding records retain the exact identities and outputs.
No game or behavioral test ran; native Restart, reset bodies and actual
action-instance transitions remain outside this evidence.

## Native reset candidates are not narrow controller refreshes

Exact-image native registration resolves SoftResetMovementAndAnimationState
through its UTF-16 name at0x13840d0, name/function table pair0x179fd00 and
exec thunk0xb39fb0. The thunk jumps directly to0xa941e0. This leaf body ends
at the return at0xa9421c: it zeros several pawn movement/root-motion fields
and sets mesh root-motion mode bytes. There is no action-instance traversal
in that leaf. Its mesh pointer is dereferenced without a local null guard.
The generic private reader's fallback512-byte window includes adjacent code
after this leaf; that adjacent code is not evidence about this method.

HardResetActionAndAnimationState is registered by name0x1384070 and pair
0x179fcf0, with exec thunk0xb3a040. That thunk dispatches receiver virtual
slot+0xad0. BioPawn class construction at0xa8e960 supplies trampoline
0xa8fb20, whose non-null branch reaches constructor0xa863e0. Its final primary
vtable write at0xa8641d selects0x133c3f0; slot+0xad0 contains0xa8f020.
This proves the native base target, not every subclass's actual runtime slot.

The bounded0xa8f020..0xa8f1a6 body resets multiple pawn/animation fields,
clears a root-motion flag and writes zero to the current player's adjacent
input-ignore bytes at controller+0x781/+0x782 after a native player-controller
cast. Nested calls remain incompletely characterized. This is a broad reset
with scene/input side effects, not an established custom-action cache refresh.
It cannot be used merely to make guest input pass while preserving native
campaign cinematic restrictions.

Neither reset is invoked by the mod from these findings. No native cache
refresh, action instance recreation or arbitrary field-write repair is claimed.
The investigation narrowed which APIs are unsuitable; actual inherited native
action initialization and live cache behavior still need to be established.
Only the owned supported executable was read, without launching it.

## Native action-array candidate review

A static instruction scan selected14 unwind fragments with typed pointer or
array-address reads at the BioPawn action-array offsets+0x6ac/+0x6cc. Each
candidate instruction was checked against decoding from its unwind-fragment
start. Offset matches alone do not establish the receiver's class. No selected
fragment also contained a direct write at+0xa0/+0xa8; this is a bounded
co-occurrence observation, not proof that native refresh does not exist.

The retained Ghidra project processed all14 entries in about11 seconds, with
terminal exit0 and12 nonempty decompilations. Exact entries0x7e5c17 and
0xa94aae were not recognized as functions. Neither was replaced by a nearby
function or counted as a successful decompilation.

| Candidate RVA | Reviewed body evidence |
| --- | --- |
| 0xa864b0 | BioPawn-vtable cleanup path releases many arrays, including both action arrays |
| 0xa8cc50,0xa8ce10 | Read a current action's scalar fields for a returned movement-related value |
| 0xa8ff50,0xa905f0 | Read action flags or class ancestry and return a boolean |
| 0xa908c0,0xa960b0 | Read action flags for movement-related scalar processing |
| 0xa94db0 | Reads action flags and updates pawn/controller presentation fields |
| 0xa97000 | Dispatches current action virtual+0x2c8 and a separate pawn delegate |
| 0x49b4c0,0x7e82f0,0xc482d0 | Matching offsets without established BioPawn action-array receiver identity |

None of these reviewed bodies establishes a narrow controller-cache refresh.
In particular, cleanup/freeing arrays must not be used to force lazy action
recreation. The action virtual dispatch and other indirect/native callees
remain unresolved by this batch. Alternate addressing, pointer aliases,
script property writes and other packages are outside the scanner's coverage.

The private evidence index now retains the completed export, input identities,
per-entry statuses, reviewed classifications and output hashes. Existing
script initialization shows where caches are initially assigned; it still
does not establish that Ashley retains stale caches during actual possession.
No runtime cache write, reset, recreation or additional possession was added.
No game or behavioral test ran. Live cache observation remains necessary
before treating this research hypothesis as an observed gameplay fault.

## Ordinary power integration candidate

The ordinary runtime now conditionally initializes existing idle power-controller
caches on both the guest and the authoritative host Ashley. This closes a static
integration gap: the prior adapter only ran on the guest. It is not new evidence
that a stale host cache was observed in-game. The host path is restricted to the
exact captured guest lease, an empty PC cache, and either no AI or the detached
captured original AI. It preserves existing nonempty PC and native replacements.
Pre-handoff cleanup is conditional and cannot block narrative/AI release.
See [the implemented scope and owner checks](GUEST-PLAYER-HUD.md).
