# Cinematic Player identity

The exact native `Player` selector returns `WorldInfo.Squad.PlayerPawn`.
Registered guest gameplay sets that squad field to Ashley. This can select the
guest for a Shepard cinematic track even though the camera follows the host.
Temporary scene release may restore a correct native result, so the adapter
always calls the original resolver first.

The existing authenticated owner-controller appearance exchange already proves
which native actor channel is the host's actual Shepard. The scene adapter
captures that same actor, world, owner-controller connection and channel into
an independent identity lease before ordinary possession is released. The
lease has no short message-age timeout during a scene; every use revalidates
the object and channel lifetimes. This identity request remains available when
optional appearance rendering is disabled and for host appearance variants
outside the old default-male cosmetic guard. Cosmetic application retains its
own separate checks. No second role protocol or guessed actor index is added.

Only a real native `Player` FName with number 0 and FindByTag mode 0 is changed.
Track resolution requires the exact selected Liara action, actual current
Matinee actor channel from the host connection, and matching action/group/track
instance membership. The complete parallel track-instance arrays exist before
native initialization calls the resolver. The original result is returned for
other selectors, numbered names, unknown contexts and UseGroupActor mode 2.
The native content validator runs normally after a substituted actor is returned.

Groups have a separate native resolver. The adapter handles only its natural
InitGroupInst call, whose unchanged caller stores the returned actor before
constructing tracks and calling their normal initialization. Every later group
getter preserves its native cache behavior. No GroupActor or track actor field
is written by the adapter, and initialization is never repeated. Group and
track `Owner` fallback is narrower still: only a null original result in the
authored main later-Liara conversation can use the unique existing Liara actor
proved by its exact tag; see [Owner binding](LIARA-OWNER-BINDING.md).

Capability version 2 is advertised only after both native resolver hooks and
the authenticated host identity are ready, in addition to the resident story
guards. The host rejects version 1, avoiding promotion before the new guest
binding requirements are met. Native start admission also preserves stock
support for existing group instances and pending-deletion Matinee replacement.

Local fixtures cover exact selector/mode/name refusal, original argument and
return forwarding, LastError preservation, capability version rejection, and
native admission policies. These checks do not prove rendered body or face
animation. Linked-variable/UseGroupActor selection, other conversation roles,
effects, voices, subtitles, timing and postscene weapon state remain separate
paired gameplay checks. The adapter neither creates a local conversation nor
changes animation trees, transforms, clocks, character roles or story state.
