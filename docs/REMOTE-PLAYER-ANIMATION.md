# Remote Shepard presentation

The latest protected ordinary pair used source `9a512fa` and the unchanged
native fingerprint from `4afddca`. All 37 paired samples observed the remote
Shepard mesh. Host and replica used `BIOG_Combat_Player_O` from
`BIOG_AnimTree_Humanoid`, separate instantiated trees and 15 animation sets.
Neither animation pause nor skeleton-update suppression was set. Eight
additional read-only samples retained the same tree identities and advancing
root NodeTickTag values: host 21140 to 21778, Ashley 5222 to 5684 and replica
5009 to 5471; root weight was 1.0. An absent or non-ticking tree is therefore
not supported by this observation. Tick progression alone does not prove
correct poses, rendering or combat.

The owner's latest image shows Shepard in a weapon-holding pose with no visible
weapon or mask. Host and replica appearance integers differ at the final field,
`OverrideHelmetID` (pawn+0x1514): host 2, replica -1; ordinary `HelmetID`
(0x150c) is 0 on both. This override can represent forced environmental helmet
state; it is not simply the selected cosmetic preference. Earlier progress
notes already recorded unequal overrides before the first appearance repair,
so a newly introduced helmet regression has not been established.

The runner and four integrated observers exited 0. Original profiles stayed
unchanged, 8,210 original files remained locked, the sandbox TOC was restored
byte-identically, and cleanup recorded no native exceptions, forced process
cleanup or remaining games. These are isolation results, not a gameplay pass.
The earlier `4afddca` run's reported T-pose remains an owner observation; it
must not override the newer measured tree state.

## Selected exact-build script evidence

LE3 Windows x64 2.0.0.48602, executable SHA-256
`b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001`.
SFXGame.pcc SHA-256
`1d8f7acb6f28e78030982c9f2d935cddec16877272aba314ce1c21dff5ed2d18`.
Private source exports and digest receipts remain in the retained RE workspace.

- `SFXPawn_Player.UpdateMeshComponent`, export 39907: sets a new skeletal
  mesh through the native component API. Its preserve-animation boolean is
  initially false and can become true only for an existing compatible mesh
  during personalization. This is not proof of a failed native initialization.
- `BioPawn.CacheCrucialAnimNodes`, export 11435: reads existing nodes only when
  both Mesh and Mesh.Animations are present. It cannot create a missing tree.
- `SFXPawn_Player.PostBeginPlay` (40099), `SFXPawn_PlayerParty.PostBeginPlay`
  (39505), and `SFXPawn.PostBeginPlay` (39240) lead to parent initialization;
  their selected bodies do not explicitly construct a new animation tree.
- `BioPawn.PostBeginPlay`, export 11434: also performs inventory, squad,
  selectable-actor and character-class setup. Replaying the lifecycle is not
  a narrowly scoped animation repair.
- `SFXPawn_Player.LoadCharacterClassData`, export 39824: appends power and
  squad-screen arrays and consumes class auto-level data before calling the
  BioPawn implementation (11443). It should not be replayed for T-pose repair.

## Remaining work

The paired presentation observer records template/tree identities and paths,
animation sets, bPauseAnims and bNoSkeletonUpdate for both local pawns and the
channel-bound remote Shepard. SDK Engine_classes.hpp identifies template/tree at
0x268/0x270, AnimSets at 0x320, bNoSkeletonUpdate at 0x408 mask 1 and bPauseAnims
at 0x43c mask 0x10000000. Pointer presence alone is not playback evidence.

The next repair concerns effective helmet-state construction and remote weapon
attachment, using the existing ordinary lifecycle. Do not reset the measured
tree, replay pawn/class startup, hardcode the observed override value or copy
process-local pointers. The current appearance adapter covers default male
mesh admission only; full host appearance and customization remain unfinished.

## Scoped presentation candidate

The first protected run of `518b21a` completed with 43 paired samples. After
two startup samples, the remaining 41 matched all four forced slots
(`[2,0,0,1]`), derived override ID 2 and the same attached, non-hidden headgear
asset. Native logs recorded the correlated host reply, scoped getter
continuation, stock slot application and loaded helmet-only build. The 41
post-completion replica samples retained one instantiated tree with root tick
147 to 5659. The first two samples preceded tree creation, so independent
snapshots do not bracket the first helmet callback with a pre-existing tree.
Window video contains overlapping characters and does not cleanly attribute
the complete mask to Shepard. This proves bounded state/asset/attachment
agreement, not full rendered presentation or other appearance variants.

The same run identified the weapon's missing stage: the host-selected Predator
actor and mesh reached the guest on channel 4, but its Owner and Shepard's
selected Weapon were null. The inventory chain contained shield/Avenger while
ReplicatedWeaponIndex was 3, so stock index resolution could not select that
existing Predator. Replaying `ReplicatedWeaponUpdated` alone would wait on
missing chain data. The attachment-only candidate correctly did not run.

The scalar reply now includes the host's existing selected-weapon channel.
The guest captures that actor/channel lifetime and requires a second correlated
reply before admitting it. For an unselected remote role-1 pawn and role-1
inventory manager, the narrow continuation invokes base
`Inventory.ClientGivenTo` for a missing Owner and stock
`SFXInventoryManager.SetWeaponImmediately` for that existing weapon. The pawn
must have no controller or pending weapon. This avoids pickup events, new
inventory, guessed replicated indices and autonomous/authority equip RPCs.
The stock loadout function cannot write local engine loadout without a player
controller; the remote pawn has none. This continuation needs live validation.

The first run's runner and four observers exited 0, with original profiles
unchanged, 8,210 locks, byte-identical TOC restoration, no native exceptions,
forced cleanup, cleanup errors or remaining games.

The ordinary runtime now requests the host story pawn's four transient forced
helmet-controller slots through the existing native owner-PC RPCs. Requests
and replies carry a monotonically increasing request ID and the current pawn
actor-channel index. Incoming messages require the native receive frame,
reciprocal controller/connection/channel ownership and current world identity;
guest replies additionally require the exact outstanding request, captured
local/remote actor lifetimes and a three-second freshness bound. Ordinary chat
continues through its original dispatcher. The host supplies only validated
0..3 slot values for the admitted default male appearance; no override ID,
asset pointer, role, ammunition or story state is transferred.

The guest applies changed slots through stock `ForceSquadHelmet`, retaining
the exact loader group and appearance IDs for completion. Its continuation
builds only the helmet component from the loaded group. Missing headgear
attachment/head/hair visibility uses the matching native component APIs.
Body mesh and animation-tree identities are checked around completion; no
tree-construction API or pawn/class startup replay is added.

The existing thread-local callspace bridge also admits the exact
`RequiresFullHelmet` getter during that validated stock request. Its original
body computes the result from the pawn/world; the bridge changes only the
original skip result for that actor/function, with a bounded call count and
a null callspace frame. Unrelated calls, other threads and original
non-skip results retain their original behavior. No role or function flag is
changed.

A separate catch-up calls stock simulated `SFXWeapon.AttachWeaponTo` for an
existing selected weapon with loaded assets and a null skeletal parent after
our completed appearance request. Pawn, inventory and weapon must retain their
reciprocal chain and actor channels on the same connection. Foreign nonnull
attachments are refused. This candidate does not create a missing weapon or
inventory, choose a weapon, change ammunition or establish rendered visibility.

These changes require a protected live observation before claiming that either
reported symptom is fixed. The expanded read-only observer distinguishes
missing weapon actors, missing selection/ownership, missing assets and missing
attachments, and records forced slots, headgear and animation root ticks.
