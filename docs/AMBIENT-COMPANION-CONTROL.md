# Preserve guest control during ambient companion lines

The ordinary runtime distinguishes proven ambient conversation lines from a
full conversation or cinematic handoff. It retains an existing human companion
lease only when **every** active conversation controller is marked ambient and
has neither an attached camera nor a depth-of-field track. The list, flags,
manager/resource back-pointers, reflected layout and current world/controller/
pawn identities are checked again before accepting that exception.

The previous environment gate released control whenever the active-conversation
count was nonzero. The owner reported that, after the Mars ladder, Shepard
arriving at the corpse interaction made the controlled Ashley teleport to a
small background animation while gameplay continued. The retained authored
Gatehouse interaction uses `SFXSeqAct_HenchmanUseInteractionPoint`. Its native
scripts require an `SFXAI_Henchman` controller in both admission and start.
Releasing the human pawn to AI made that path eligible. Keeping the existing
human controller lets the stock interaction refuse it without moving actors,
restarting animations or changing Kismet assets.

This is retention only. It does not acquire a pawn from an AI that is already
performing an interaction. Full, mixed, preactivating or unrecognized
conversations still block control, as do native cinematic flags, nonordinary
modes, pause and PlayersOnly. The protected probe keeps its previous policy
because the shared core callback defaults to null. No conversation flag, game
mode, camera permission or actor position is written.

The preceding owner run contains two mode-0 conversation handoffs and a later
mode-8 scene handoff. That supports the distinction but does not prove the
runtime ambient metadata or the new retention behavior. Native fixtures cover
all-ambient versus mixed/camera/DOF/unknown/preactivation lists and preservation
of the core cinematic/pause gates. Live corpse-interaction behavior remains an
owner check.

At the next protected test, keep the guest away from the corpse after the
ladder and let Shepard arrive first. Ashley must remain under guest control
and keep her position. Check normal walking, firing and powers during the
ambient line, then start a real camera conversation and confirm normal
release/reacquisition. `runtime_ambient_conversation` rows identify retained
control. Shepard-death continuation and the reported AI T-pose are deferred
at the owner's request.
