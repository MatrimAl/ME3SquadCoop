# Remote Shepard T-pose investigation

The owner observed a T-pose on Shepard from guest Ashley in the connected
ordinary pair from `4afddca`. The paired channel observer established an actor
and skeletal mesh, not animation. Do not classify that run as healthy remote
presentation or a gameplay pass.

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

## Remaining distinction

A missing animation tree, missing animation sets, paused animation, and a
non-updating existing tree are separate possibilities. The existing paired
presentation observer now records template/tree identities and paths, animation
sets, bPauseAnims and bNoSkeletonUpdate for both local pawns and the channel-bound
remote Shepard. These fields are read-only; no live sample has yet been taken
with this extension. SDK Engine_classes.hpp identifies template/tree at
0x268/0x270, AnimSets at 0x320, bNoSkeletonUpdate at 0x408 mask 1 and bPauseAnims
at 0x43c mask 0x10000000. Pointer presence alone is not playback evidence.

The next repair must follow the missing state actually observed and use the
component's normal initialization API, without copying host process pointers
or rerunning inventory/character startup. No game or native runtime mutation
was performed during this investigation. Seven new selected script bodies were
successfully decompiled and the observer's Python syntax was checked.
