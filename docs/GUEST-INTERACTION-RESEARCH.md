# Normal guest interaction and revival route

The intended guest uses ordinary interaction input, target selection and the
game's authoritative use/revive path. The panel is not a combat interaction
interface. The following findings come from selected owned LE3 script exports;
they do not establish actual guest input delivery or successful revival.

## Selected script coverage

Fourteen functions decoded with matching serialized/expanded lengths and55
aligned explicit branch/iterator destinations. Delegate call, assignment and
property tokens were handled using the already retained Legendary Explorer
format before claiming complete decoding. Package and compression-library
hashes were unchanged before/after extraction. Private records retain exact
exports, bytecode trees and coverage. No game or test ran.

| Function | Relevant behavior |
| --- | --- |
| SFXGameModeDefault.Used | Resolves Outer.Pawn as BioPawn, requires the controller's current selection and refuses animated transitions/current custom actions before TryUse |
| BioPlayerController.CanUse | Applies interaction distance, action bDisableUse, target/module/event eligibility and module/event use ranges |
| BioPlayerController.TryUse | Calls CanUse; authority triggers the target event and module path, while the nonauthority branch calls ServerUseSelection |
| BioPlayerController.ServerUseSelection | Invokes TryUse again for the supplied actor; retained function flags are0x2200c2, native index0 |
| BioPlayerController.ActivateUseModule | Resolves the target's SFXSimpleUseModule, requires targetable state, then invokes its OnUsed delegate |
| SFXSimpleUseModule.IsDefaultActionPossible | Returns the inverse of bDeactivated |
| SFXPawn_PlayerParty.EnableUsage | When enabled, makes the module targetable, binds OnUsed to Used, assigns fReviveRange and revive tip13; disabling clears that delegate/targetability |
| SFXPawn_PlayerParty.Used | Requires a party-pawn user, downed/nondead target, authority and a user whose IsDead is false; calls the user's StartRevive |
| SFXPawn_PlayerParty.StartRevive | Cancels reload handling and starts custom action5 on authority with the target pawn |
| BioPlayerController.Revive | Delegates to party resurrection with separate campaign/multiplayer health and vocalization policy |
| SFXPawn_PlayerParty.UpdateReviveHud | Requires a local player controller and an existing countdown movie before updating its animations |
| SFXSimpleUseModule.HandlePostBeginPlay | Delegates selection initialization and adjusts one combat-targetable selection-range case |
| SFXPawn_Henchman.PostBeginPlay | Configures radar/damage-effect context then delegates party initialization |
| BioPlayerController.AutoUseFromStorm | Has separate ladder/jump interactions and a normal authority/client use split |

These selected script routes cast the using character to BioPawn or
SFXPawn_PlayerParty rather than requiring SFXPawn_Player. This differs from
the separately researched gamepad power cast restriction. It supplies no
reason to replace Ashley's class, write health, set the multiplayer GRI flag
or bypass CanUse/authority with a mod-specific revive request.

## Remaining boundaries

Normal physical input must first reach GameModeDefault.Used and the existing
player-selection object must contain a valid target. The selected scripts do
not establish native selection registration/scanning, network delivery or
script/native custom-action5 behavior on the possessed guest. Its cached
controller context, progress/abort handling and actual authoritative completion
remain unresolved. HUD method availability alone does not prove that the
required countdown movie exists in a campaign guest session.

No forced use/delegate binding, additional revive RPC, automatic resurrection,
profile edit, GRI multiplayer toggle or native order was added. Existing
death, scene, ownership and host story policies remain authoritative. Ordinary
interaction and usable control after revival still need gameplay evidence.

## Native selection context follow-up

Exact-image UTF-16 native registration identifies FindCurrentSelectionTarget
at string0x137e9c0/pair0x179e7c0/exec0xb75b80 and UpdateSelection at
string0x137e960/pair0x179e7b0/exec0xb75bc0. The selected thunk instructions
tail-jump to0xa9bf90 and0xaa55f0 respectively. Both exact native entries
decompiled from the retained project in about10 seconds, with terminal exit0
and nonempty bodies. Manifest/source/output identities remain private.

The selected update body uses the selection object's Outer+0x40 as its
controller and requires its current Pawn+0x3bc. It consults the native
input-side gate via controller.PlayerInput+0x6dc, checks the possessed BioPawn
state through the already identified BioPawn cast, retains the old selected
target and calls FindCurrentSelectionTarget. Later paths update the selection
state, permission event and highlight/flare presentation. This is not a
pointer-only initializer suitable for blind manual invocation.

FindCurrentSelectionTarget clears its own target+0x70 and candidate count,
reads the owning controller's camera and rotation, and enumerates a native
world selection registry. Its candidate path excludes the controller's current
Pawn; relevant distance calculations use that current pawn's location. Native
module/class/visibility/scoring processing remains in the inspected body and
its unresolved helpers. The SDK selection layout has candidate/current/last
target and presentation/configuration fields, rather than a separate cached
Shepard pawn or AI-controller field like the custom-action context.

These observations narrow prerequisites to a correctly associated existing
selection object, usable controller input/camera and actual native update/
candidate registration. They do not show a guest receiving those prerequisites
at runtime, nor establish all nested helper semantics or class restrictions.
No target override, new selection object, forced UpdateSelection call or
cleared native gate was added. Source game identity stayed unchanged; no game
or test ran. Normal target acquisition and action5 completion remain unverified.
