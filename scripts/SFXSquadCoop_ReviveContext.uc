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

function Reset()
{
    bCaptured = false;
    bDispatched = false;
    Action = None;
    Reviver = None;
    Target = None;
    PlayerController = None;
    TargetController = None;
    SharedWorld = None;
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
        && Action.m_oPawn == Reviver && Action.m_oPC == PlayerController && Action.m_oAI == None;
}

function bool OrdinaryWorld()
{
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
    Reset();
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
    return OwnsCurrentAction() && EligibleTarget() && !Reviver.IsFiring();
}

// A true return means one native dispatch, never verified resurrection. The
// action must observe target recovery independently and still run its cleanup.
function bool DispatchRevive()
{
    if (bDispatched || !CanContinue() || !Target.bBeingRevived
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
