// Shared context for the campaign revive action. Not an RPC or an action-slot
// installer. The owning action must manage animation, reservation and timers.
class SFXSquadCoop_ReviveContext extends Object;

var BioCustomAction Action;
var SFXPawn_PlayerParty Reviver;
var SFXPawn_PlayerParty Target;
var SFXPlayerController PlayerController;
var Controller TargetController;
var WorldInfo SharedWorld;
var bool bCaptured;
var bool bDispatched;
var bool bDurationStarted;
var bool bCancelled;
var float StartedAt;
var float CompleteAt;
var bool bReservationAttempted;
var bool bReservationOwned;

function bool Reset()
{
    // Never discard a still-owned target just to accept a new action.
    if (bReservationOwned)
        return false;
    bCaptured = false;
    bDispatched = false;
    bDurationStarted = false;
    bCancelled = false;
    StartedAt = 0.0;
    CompleteAt = 0.0;
    bReservationAttempted = false;
    Action = None;
    Reviver = None;
    Target = None;
    PlayerController = None;
    TargetController = None;
    SharedWorld = None;
    return true;
}

function bool SameActors()
{
    return bCaptured && Action != None && Reviver != None && Target != None
        && PlayerController != None && TargetController != None && SharedWorld != None
        && !Reviver.bDeleteMe && !Target.bDeleteMe && !PlayerController.bDeleteMe
        && !TargetController.bDeleteMe && !SharedWorld.bDeleteMe
        && Reviver.WorldInfo == SharedWorld && Target.WorldInfo == SharedWorld
        && PlayerController.WorldInfo == SharedWorld && TargetController.WorldInfo == SharedWorld
        && PlayerController.Pawn == Reviver && Reviver.Controller == PlayerController
        && Target.Controller == TargetController && TargetController.Pawn == Target
        && Action.m_oPawn == Reviver && Action.m_oPC == PlayerController && Action.m_oAI == None
        && class'WorldInfo'.static.GetWorldInfo() == SharedWorld;
}

function bool OrdinaryWorld()
{
    local BioWorldInfo Info;
    local BioConversationManager Conversation;

    if (!SameActors())
        return false;
    Info = BioWorldInfo(SharedWorld);
    if (Info == None)
        return false;
    Conversation = Info.m_pConversationManager;
    if (Conversation != None && (Conversation.m_aActiveConversations.Length != 0
        || Conversation.m_bActivatingConversation))
        return false;
    return SameActors() && !SharedWorld.bPlayersOnly && !SharedWorld.bPlayersOnlyPending
        && SharedWorld.Pauser == None && !PlayerController.bCinematicMode
        && PlayerController.GameModeManager2 != None
        && PlayerController.GameModeManager2.CurrentMode == 0;
}

function bool EligibleTarget()
{
    return OrdinaryWorld() && !Reviver.bIsDowned && !Reviver.bIsDead && !Reviver.IsDead()
        && !Reviver.IsInState('InRagdoll') && !Reviver.IsInState('RagdollRecovery')
        && Target.bIsDowned && !Target.bIsDead && Target.IsInState('Downed')
        && Target.fReviveRange > 0.0
        && VSize(Target.Location - Reviver.Location) <= Target.fReviveRange;
}

function bool Capture(BioCustomAction Candidate, BioPawn SyncPawn)
{
    if (!Reset())
        return false;
    if (Candidate == None || Candidate.m_oPawn == None)
        return false;
    Action = Candidate;
    Reviver = SFXPawn_PlayerParty(Candidate.m_oPawn);
    Target = SFXPawn_PlayerParty(SyncPawn);
    if (Reviver == None || Target == None || Reviver == Target)
    {
        Reset();
        return false;
    }
    PlayerController = SFXPlayerController(Reviver.Controller);
    TargetController = Target.Controller;
    SharedWorld = Reviver.WorldInfo;
    bCaptured = true;
    if (!EligibleTarget() || Target.bBeingRevived)
    {
        Reset();
        return false;
    }
    return true;
}

function bool OwnsCurrentAction()
{
    return SameActors() && Reviver.CurrentCustomAction == 5
        && Reviver.CustomActions.Length > 5 && Reviver.CustomActions[5] == Action
        && Reviver.SyncPawn == Target;
}

function bool CanContinue()
{
    return !bCancelled && OwnsCurrentAction() && EligibleTarget() && !Reviver.IsFiring()
        && (!bReservationAttempted || OwnsReservation());
}

function bool OwnsReservation()
{
    return bReservationOwned && bReservationAttempted && OwnsCurrentAction()
        && int(Reviver.Role) == 3 && int(Target.Role) == 3 && Target.bBeingRevived;
}

function bool AcquireReservation()
{
    if (bReservationAttempted || bReservationOwned || !CanContinue()
        || int(Reviver.Role) != 3 || int(Target.Role) != 3 || Target.bBeingRevived)
        return false;
    // No native call occurs between the final false check and this write.
    bReservationAttempted = true;
    bReservationOwned = true;
    Target.bBeingRevived = true;
    return OwnsReservation();
}

function bool ReleaseReservation()
{
    if (!bReservationOwned)
        return true;
    if (!OwnsCurrentAction() || int(Reviver.Role) != 3 || int(Target.Role) != 3)
        return false;
    if (Target.bBeingRevived)
        Target.bBeingRevived = false;
    bReservationOwned = false;
    return true;
}

// Progress follows the native world clock, without adding callbacks to the
// pawn's timer list. An interrupted attempt cannot silently start again.
function bool BeginDuration()
{
    local float Duration;
    local float Now;

    if (bDurationStarted || bCancelled || bDispatched || !CanContinue()
        || !OwnsReservation())
        return false;
    Duration = Reviver.fTimeToRevive;
    Now = SharedWorld.TimeSeconds;
    // x-x is zero only for finite values; NaN/infinity are refused.
    if (!(Duration > 0.0) || Duration - Duration != 0.0 || Now - Now != 0.0)
        return false;
    StartedAt = Now;
    CompleteAt = Now + Duration;
    if (!(CompleteAt > StartedAt) || CompleteAt - CompleteAt != 0.0)
        return false;
    bDurationStarted = true;
    return true;
}

function Cancel()
{
    bCancelled = true;
    ReleaseReservation();
}

function bool DurationComplete()
{
    local float Now;

    if (!bDurationStarted || bCancelled || bDispatched)
        return false;
    if (!CanContinue() || !OwnsReservation())
    {
        Cancel();
        return false;
    }
    Now = SharedWorld.TimeSeconds;
    if (Now - Now != 0.0 || Now < StartedAt)
    {
        Cancel();
        return false;
    }
    return Now >= CompleteAt;
}

// A true return means one native dispatch, never verified resurrection. The
// action must observe target recovery independently and still run its cleanup.
function bool DispatchRevive()
{
    if (!DurationComplete() || !CanContinue() || !OwnsReservation()
        || int(Reviver.Role) != 3 || int(Target.Role) != 3)
        return false;
    bDispatched = true;
    PlayerController.Revive(Target);
    return true;
}

function bool RecoveryObserved()
{
    return bDispatched && OrdinaryWorld()
        && int(Reviver.Role) == 3 && int(Target.Role) == 3
        && !Reviver.bIsDowned && !Reviver.bIsDead && !Reviver.IsDead()
        && !Target.bIsDowned && !Target.bIsDead && !Target.IsDead()
        && !Target.IsInState('Downed');
}
