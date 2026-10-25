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
