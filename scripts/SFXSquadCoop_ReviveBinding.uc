// Session admission and per-peer class/animation availability are supplied by
// the owning runtime. This object is not an RPC, loader or session authority.
class SFXSquadCoop_ReviveBinding extends Object;

var SFXPawn_PlayerParty ControlledPawn;
var SFXPlayerController AdmissionController;
var SFXPlayerController PawnController;
var WorldInfo SharedWorld;
var Object ActorOuter;
var SkeletalMeshComponent CapturedMesh;
var AnimSequence CapturedAnimation;
var class<BioCustomAction> SelectedClass;
var class<BioCustomAction> OriginalClass;
var BioCustomAction CreatedAction;
var int ClassCount;
var int ActionCount;
var int CapturedRole;
var bool bPresentationOnly;
var bool bBound;
var bool bInstancing;

function bool Reset()
{
    if (bBound || bInstancing)
        return false;
    ControlledPawn = None;
    AdmissionController = None;
    PawnController = None;
    SharedWorld = None;
    ActorOuter = None;
    CapturedMesh = None;
    CapturedAnimation = None;
    SelectedClass = None;
    OriginalClass = None;
    CreatedAction = None;
    ClassCount = 0;
    ActionCount = 0;
    CapturedRole = 0;
    bPresentationOnly = false;
    return true;
}

function bool SupportedClass(class<BioCustomAction> Candidate)
{
    return Candidate != None && (Candidate == class'SFXSquadCoop_ReviveAction'
        || PathName(Candidate) == "SFXGameContent.SFXCustomAction_Revive");
}

function bool SameActors()
{
    if (ControlledPawn == None || ControlledPawn.bDeleteMe || AdmissionController == None
        || AdmissionController.bDeleteMe || SharedWorld == None || SharedWorld.bDeleteMe
        || ControlledPawn.WorldInfo != SharedWorld || AdmissionController.WorldInfo != SharedWorld
        || class'WorldInfo'.static.GetWorldInfo() != SharedWorld
        || AdmissionController.Pawn == None || AdmissionController.Pawn.bDeleteMe
        || AdmissionController.Pawn.Controller != AdmissionController
        || AdmissionController.Pawn.WorldInfo != SharedWorld
        || !AdmissionController.IsLocalPlayerController()
        || ControlledPawn.Outer != ActorOuter || int(ControlledPawn.Role) != CapturedRole)
        return false;
    if (bPresentationOnly)
        return CapturedRole == 1 && ControlledPawn.Controller == None && PawnController == None;
    return (CapturedRole == 2 || CapturedRole == 3) && PawnController != None
        && !PawnController.bDeleteMe && PawnController.WorldInfo == SharedWorld
        && ControlledPawn.Controller == PawnController && PawnController.Pawn == ControlledPawn;
}

function bool OrdinaryWorld()
{
    local BioWorldInfo Info;

    if (!SameActors())
        return false;
    Info = BioWorldInfo(SharedWorld);
    if (Info == None || SharedWorld.bPlayersOnly || SharedWorld.bPlayersOnlyPending
        || SharedWorld.Pauser != None || AdmissionController.bCinematicMode
        || AdmissionController.GameModeManager2 == None
        || AdmissionController.GameModeManager2.CurrentMode != 0)
        return false;
    return Info.m_pConversationManager == None
        || (Info.m_pConversationManager.m_aActiveConversations.Length == 0
            && !Info.m_pConversationManager.m_bActivatingConversation);
}

function bool SameTables()
{
    return SameActors() && ControlledPawn.CustomActionClasses.Length == ClassCount
        && ControlledPawn.CustomActions.Length == ActionCount && ClassCount > 5 && ActionCount > 5;
}

function bool IdlePawn()
{
    return SameActors() && ControlledPawn.CurrentCustomAction == 0
        && ControlledPawn.CurrentPowerCustomAction == 0;
}

function bool InstanceContextMatches()
{
    return CreatedAction != None && CreatedAction.Class == SelectedClass
        && CreatedAction.Outer == ActorOuter && CreatedAction.m_oPawn == ControlledPawn
        && CreatedAction.m_oPC == PawnController && CreatedAction.m_oAI == None;
}

// A true result proves neither peer admission nor actual animation playback.
function bool ValidBinding()
{
    return bBound && !bInstancing && SameTables()
        && ControlledPawn.CustomActionClasses[5] == SelectedClass
        && ControlledPawn.CustomActions[5] == CreatedAction && InstanceContextMatches()
        && ControlledPawn.Mesh == CapturedMesh && CapturedMesh != None
        && CapturedMesh.FindAnimSequence('CB_MP_Revive') == CapturedAnimation;
}

function bool Bind(SFXPawn_PlayerParty Candidate, SFXPlayerController LocalAdmission,
    class<BioCustomAction> ActionClass, bool bSimulatedPresentation)
{
    local bool bNativeAccepted;

    if (!Reset() || Candidate == None || !SupportedClass(ActionClass))
        return false;
    ControlledPawn = Candidate;
    AdmissionController = LocalAdmission;
    SharedWorld = Candidate.WorldInfo;
    ActorOuter = Candidate.Outer;
    CapturedRole = int(Candidate.Role);
    bPresentationOnly = bSimulatedPresentation;
    if (!bPresentationOnly)
        PawnController = SFXPlayerController(Candidate.Controller);
    ClassCount = Candidate.CustomActionClasses.Length;
    ActionCount = Candidate.CustomActions.Length;
    SelectedClass = ActionClass;
    if (!OrdinaryWorld() || !IdlePawn() || !SameTables() || Candidate.bIsDowned || Candidate.bIsDead
        || Candidate.IsDead() || Candidate.IsInState('InRagdoll') || Candidate.IsInState('RagdollRecovery')
        || Candidate.CustomActions[5] != None
        || (Candidate.CustomActionClasses[5] != None && Candidate.CustomActionClasses[5] != SelectedClass))
    {
        Reset();
        return false;
    }
    CapturedMesh = Candidate.Mesh;
    if (CapturedMesh == None)
    {
        Reset();
        return false;
    }
    CapturedAnimation = CapturedMesh.FindAnimSequence('CB_MP_Revive');
    if (CapturedAnimation == None || CapturedAnimation.SequenceName != 'CB_MP_Revive'
        || !(CapturedAnimation.SequenceLength > 0.0)
        || CapturedAnimation.SequenceLength - CapturedAnimation.SequenceLength != 0.0
        || !OrdinaryWorld() || !IdlePawn() || !SameTables() || Candidate.Mesh != CapturedMesh
        || Candidate.CustomActions[5] != None
        || (Candidate.CustomActionClasses[5] != None && Candidate.CustomActionClasses[5] != SelectedClass))
    {
        Reset();
        return false;
    }
    OriginalClass = Candidate.CustomActionClasses[5];
    bBound = true;
    bInstancing = true;
    Candidate.CustomActionClasses[5] = SelectedClass;
    bNativeAccepted = Candidate.VerifyCAHasBeenInstanced(5, 0);
    bInstancing = false;
    if (SameTables() && Candidate.CustomActionClasses[5] == SelectedClass)
        CreatedAction = Candidate.CustomActions[5];
    if (!bNativeAccepted || !ValidBinding() || !IdlePawn() || CreatedAction.bStartedCustomAction)
    {
        Release();
        return false;
    }
    return true;
}

function bool ActionHasPendingWork()
{
    local int Index;
    local SFXSquadCoop_ReviveAction Authored;

    if (CreatedAction == None)
        return false;
    if (CreatedAction.bStartedCustomAction)
        return true;
    for (Index = 0; Index < ControlledPawn.Timers.Length; Index++)
    {
        if (ControlledPawn.Timers[Index].TimerObj == CreatedAction)
            return true;
    }
    Authored = SFXSquadCoop_ReviveAction(CreatedAction);
    return Authored != None && (Authored.bBaseStarted || Authored.bStopping || Authored.bFinishing
        || Authored.bTargetCleanupPending || (Authored.Context != None
            && (Authored.Context.bReservationOwned || Authored.Context.bDeathTimerPausedByUs
                || Authored.Context.bUsageChanged)));
}

function bool Release()
{
    if (bInstancing)
        return false;
    if (!bBound)
        return true;
    if (ControlledPawn == None || ControlledPawn.bDeleteMe || SharedWorld == None || SharedWorld.bDeleteMe)
    {
        // Leave a destroyed actor's tables to native destruction; no write.
        bBound = false;
        return Reset();
    }
    if (!SameTables() || !IdlePawn() || ControlledPawn.CustomActionClasses[5] != SelectedClass
        || ControlledPawn.CustomActions[5] != CreatedAction
        || (CreatedAction != None && (!InstanceContextMatches() || ActionHasPendingWork())))
        return false;
    // No forced Stop, timer clear, object destruction or array resizing. Only
    // this unused created slot and its captured class value are restored.
    ControlledPawn.CustomActions[5] = None;
    ControlledPawn.CustomActionClasses[5] = OriginalClass;
    bBound = false;
    return Reset();
}
