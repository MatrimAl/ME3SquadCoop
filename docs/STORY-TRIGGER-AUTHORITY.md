# Story triggers and shared exits

The ordinary runtime adds two default-on rules for the current supported LE3
executable. Set `ME3_SQUAD_COOP_STORY_GATES=0` to disable this adapter.

* A classified conversation or camera-directed Matinee entry belongs to Shepard.
  Guest activation is refused before the native event changes its counters.
  For brush volumes, a proposed guest movement into the volume is rejected
  through the original movement function with zero translation. A short local
  Canvas message asks the player to wait for Shepard. Rotation, movement flags,
  native hit output and the original return value remain native.
  That movement fence opens once the same Shepard has actually entered or
  crossed the authored brush. The runtime retains a bounded passage record
  across rediscovery and scene handoffs, tied to event/volume/component/world,
  Shepard and driver lifetimes. Guest event activation remains host-only.
  Passage uses the native brush query and actual observed Shepard positions;
  stale samples or long teleports cannot create a swept passage. On the guest
  the existing verified remote Shepard channel supplies that observation.
* Mars shuttle closure waits until every current human pawn has cleared the
  cabin. Either player can leave first. A blocked close request is retained and
  submitted once, with Shepard as instigator, after all players are outside.
  A fresh eligible native touch consumes the pending request too.

The Mars exit trigger consists of outside walkway regions. Testing whether
every player occupies that trigger would be incorrect. The adapter instead
queries the authored cabin brush with each human pawn's current collision
extent. It enumerates the host and native driver connections, rather than
counting squad AI as players. Missing pawn/shape observations postpone closure;
they do not mean an empty cabin. A disconnected participant disappears only
when the native connection list actually removes it.

The exact Mars rule uses the authored `BioD_ProMar_110Shuttle` exit event and
cabin identities. It does not redefine every animated door as a group exit.
Generic story classification follows a bounded, read-only authored graph to
`SFXSeqAct_StartConversation` or a Matinee containing an `InterpGroupDirector`.
Ordinary prop/door Matinees and cinematic-mode restoration alone are not story
endpoints. Disabled links and completed/disabled event entries do not form
movement barriers. An already-inside guest can leave a volume.

This is **not universal campaign trigger coverage**. Unknown layouts, graph
bounds, indirect/nested event links and non-volume triggers can remain outside
physical entry protection. They must be recorded during gameplay and supported
from their actual authored behavior. The adapter does not emulate the entire
Kismet virtual machine.

A late-joining guest that never observed Shepard inside or crossing a still-
armed volume cannot infer prior passage from an arbitrary later position.
This local passage cache does not fabricate a historical route. Such a late-
join case needs an authoritative progression snapshot if reproduced.

## Native scope

The adapter pins the native `SequenceEvent::CheckActivate` entry at RVA
`0x724670`, `World::MoveActor` at `0x584770` and the brush query at `0x6bbb40`.
Installation requires the supported executable and matching entry bytes. Native
calls use the measured x64 argument order, including stack arguments. The
ordinary feature only intercepts the active runtime's game thread and current
network world. Other calls retain their original arguments and are forwarded
once.

Event, volume, collision component, world, driver and pending instigator use
captured object lifetimes. The pending event is dropped after invalidation,
disable or consumption. Activation indices are copied, and replay forwards a
stack copy rather than retaining native or mutable global array storage.

No actor coordinates, trigger counters, plot variables, collision shapes,
animation trees, package assets or TOC entries are written by this feature.
`runtime_story_gate` JSON rows record installation, discovered story/group
entries and cabin availability, refusals and the deferred native result. The
existing passive recorder collects these native rows.

## Validation and owner checks

Native fixtures cover participant-mask decisions in both exit orders, unknown
occupancy, generation invalidation, swept broad-phase geometry, warning expiry,
disabled/spent events, original call arguments/return/LastError, native hit
output and private replay-index storage. These fixtures do not prove live
trigger discovery, brush overlap, warning rendering or door animation agreement.

At the next protected manual session:

1. Leave the shuttle as the guest while Shepard remains inside. Closure must
   wait. Leave as Shepard last; closure should run once. Repeat in reverse order.
2. While a close is pending, re-enter the cabin; the door must continue waiting.
3. Approach a conversation/camera-scene trigger as the guest first. Check the
   warning and entry boundary, then let Shepard trigger it. Check ordinary doors
   and backtracking after the scene too.
   At the early shuttle walkway, let Shepard pass first and then follow with
   the guest: the cleared movement fence must stay open for that live volume.
4. Check guest control after the scene and the previously crashing handoff.
   The receive-scope correction remains an unverified crash candidate; this
   feature does not establish that the crash or Ashley's falling is resolved.

Full campaign co-op, exact native guest HUD parity, sustained combat and
cross-PC play remain incomplete.
