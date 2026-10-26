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
var bool bDeathTimerCaptured;
var bool bDeathTimerPresent;
var bool bDeathTimerPausedByUs;
var TimerData DeathTimerBaseline;
var bool bUsageCaptured;
var bool bUsagePresent;
var bool bUsageChanged;
var bool bUsageRestoreStarted;
var SFXSimpleUseModule UsageModule;
var SFXSelectionLensFlareComponent UsageLens;
var delegate<SFXSimpleUseModule.OnUsed> UsageDelegateBaseline;
var float UsageRangeBaseline;
var byte UsageTipBaseline;
var bool bUsageTargetableBaseline;
var bool bUsageCombatBaseline;
var bool bUsageOldTargetableBaseline;
var bool bUsageOldCombatBaseline;
var bool bUsageDeactivatedBaseline;
var bool bUsageLensHiddenBaseline;

function bool Reset()
{
    // Never discard a still-owned target just to accept a new action.
    if (bReservationOwned || bDeathTimerPausedByUs || bUsageChanged)
        return false;
    bCaptured = false;
    bDispatched = false;
    bDurationStarted = false;
    bCancelled = false;
    StartedAt = 0.0;
    CompleteAt = 0.0;
    bReservationAttempted = false;
    bDeathTimerCaptured = false;
    bDeathTimerPresent = false;
    bUsageCaptured = false;
    bUsagePresent = false;
    bUsageRestoreStarted = false;
    UsageModule = None;
    UsageLens = None;
    UsageDelegateBaseline = None;
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
        && (!bReservationAttempted || OwnsReservation())
        && (!bDeathTimerCaptured || DeathTimerMatches(bDeathTimerPausedByUs || DeathTimerBaseline.bPaused))
        && (!bUsageCaptured || UsageStateMatches());
}

function bool SameUsageModule()
{
    return bUsageCaptured && OwnsCurrentAction()
        && SFXSimpleUseModule(Target.GetModule(class'SFXSimpleUseModule')) == UsageModule;
}

function bool UsageMetadataMatches()
{
    return SameUsageModule() && bUsagePresent && UsageModule != None
        && UsageModule.OnUsed == UsageDelegateBaseline
        && UsageModule.fUseRange == UsageRangeBaseline
        && UsageModule.m_TargetTipText == UsageTipBaseline
        && UsageModule.m_bCombatTargetable == bUsageCombatBaseline
        && UsageModule.OldTargeTable == bUsageOldTargetableBaseline
        && UsageModule.OldCombatTargetable == bUsageOldCombatBaseline
        && UsageModule.bDeactivated == bUsageDeactivatedBaseline
        && UsageModule.LensFlareComp == UsageLens;
}

function bool UsageBaselineRestored()
{
    return UsageMetadataMatches() && UsageModule.m_bTargetable == bUsageTargetableBaseline
        && (UsageLens == None || UsageLens.HiddenGame == bUsageLensHiddenBaseline);
}

function bool UsageStateMatches()
{
    if (!SameUsageModule())
        return false;
    if (!bUsagePresent)
        return UsageModule == None;
    if (!bUsageChanged)
        return UsageBaselineRestored();
    return !bUsageRestoreStarted && UsageMetadataMatches() && !UsageModule.m_bTargetable
        && (UsageLens == None || UsageLens.HiddenGame);
}

function bool AcquireUsageDisable()
{
    if (bUsageCaptured || bUsageChanged || !CanContinue() || !OwnsReservation())
        return false;
    UsageModule = SFXSimpleUseModule(Target.GetModule(class'SFXSimpleUseModule'));
    bUsageCaptured = true;
    bUsagePresent = UsageModule != None;
    if (!bUsagePresent)
        return SameUsageModule();
    UsageDelegateBaseline = UsageModule.OnUsed;
    UsageRangeBaseline = UsageModule.fUseRange;
    UsageTipBaseline = UsageModule.m_TargetTipText;
    bUsageTargetableBaseline = UsageModule.m_bTargetable;
    bUsageCombatBaseline = UsageModule.m_bCombatTargetable;
    bUsageOldTargetableBaseline = UsageModule.OldTargeTable;
    bUsageOldCombatBaseline = UsageModule.OldCombatTargetable;
    bUsageDeactivatedBaseline = UsageModule.bDeactivated;
    UsageLens = UsageModule.LensFlareComp;
    if (UsageLens != None)
        bUsageLensHiddenBaseline = UsageLens.HiddenGame;
    if (UsageRangeBaseline - UsageRangeBaseline != 0.0 || !UsageMetadataMatches())
        return false;
    if (!bUsageTargetableBaseline)
        return UsageStateMatches();
    bUsageChanged = true;
    // Preserve the callback, tip, range and selection save variable. This
    // native path also hides its lens flare; that state is owned separately.
    UsageModule.SetTargetable(false, false);
    return OwnsReservation() && UsageStateMatches();
}

function bool ReleaseUsageDisable()
{
    if (!bUsageChanged)
        return true;
    if (!SameUsageModule() || UsageModule == None
        || int(Reviver.Role) != 3 || int(Target.Role) != 3)
        return false;
    // Downed.EndState owns the recovered pawn's disabled usage. Preserve that
    // state without restoring a downed interaction or its old delegate.
    if (RecoveryObserved() && !UsageModule.m_bTargetable)
    {
        bUsageChanged = false;
        return true;
    }
    if (!UsageMetadataMatches())
        return false;
    if (UsageBaselineRestored())
    {
        bUsageChanged = false;
        return true;
    }
    if (!Target.bIsDowned || Target.bIsDead || !Target.IsInState('Downed'))
        return false;
    if (!bUsageRestoreStarted)
    {
        if (UsageModule.m_bTargetable || (UsageLens != None && !UsageLens.HiddenGame))
            return false;
        bUsageRestoreStarted = true;
        UsageModule.SetTargetable(bUsageTargetableBaseline, false);
    }
    if (!UsageMetadataMatches() || UsageModule.m_bTargetable != bUsageTargetableBaseline)
        return false;
    if (UsageLens != None && UsageLens.HiddenGame != bUsageLensHiddenBaseline)
    {
        // SetTargetable sets HiddenGame to !targetable, which need not equal
        // the original hidden state. Do not overwrite an unexpected value.
        if (UsageLens.HiddenGame != !bUsageTargetableBaseline)
            return false;
        UsageLens.SetHidden(bUsageLensHiddenBaseline);
    }
    if (!UsageBaselineRestored())
        return false;
    bUsageChanged = false;
    return true;
}

// Native timer binding is the actor plus function name, not its array index.
// An identical remove/recreate between observations has no exposed generation.
function int FindDeathTimer()
{
    local int Index;
    local int Found;

    Found = -1;
    if (Target == None)
        return -2;
    for (Index = 0; Index < Target.Timers.Length; Index++)
    {
        if (Target.Timers[Index].FuncName == 'PermaDeath' && Target.Timers[Index].TimerObj == Target)
        {
            if (Found != -1)
                return -2;
            Found = Index;
        }
    }
    return Found;
}

function bool DeathTimerMatches(bool bExpectedPaused)
{
    local int Index;

    if (!bDeathTimerCaptured || !OwnsCurrentAction())
        return false;
    Index = FindDeathTimer();
    if (!bDeathTimerPresent)
        return Index == -1;
    if (Index < 0)
        return false;
    return Target.Timers[Index].bPaused == bExpectedPaused
        && Target.Timers[Index].bLoop == DeathTimerBaseline.bLoop
        && Target.Timers[Index].Rate == DeathTimerBaseline.Rate
        && Target.Timers[Index].Count == DeathTimerBaseline.Count
        && Target.Timers[Index].TimerTimeDilation == DeathTimerBaseline.TimerTimeDilation;
}

function bool AcquireDeathTimerPause()
{
    local int Index;

    if (bDeathTimerCaptured || bDeathTimerPausedByUs || !CanContinue() || !OwnsReservation())
        return false;
    Index = FindDeathTimer();
    if (Index == -2)
        return false;
    bDeathTimerCaptured = true;
    bDeathTimerPresent = Index >= 0;
    if (!bDeathTimerPresent)
        return true;
    DeathTimerBaseline = Target.Timers[Index];
    if (!(DeathTimerBaseline.Rate > 0.0) || DeathTimerBaseline.Rate - DeathTimerBaseline.Rate != 0.0
        || !(DeathTimerBaseline.Count >= 0.0) || DeathTimerBaseline.Count - DeathTimerBaseline.Count != 0.0
        || DeathTimerBaseline.TimerTimeDilation - DeathTimerBaseline.TimerTimeDilation != 0.0)
        return false;
    // Preserve an existing pause; this action must never resume someone else's.
    if (DeathTimerBaseline.bPaused)
        return DeathTimerMatches(true);
    // Record before native entry. A refused readback remains pending cleanup.
    bDeathTimerPausedByUs = true;
    Target.PauseTimer(true, 'PermaDeath', Target);
    return OwnsReservation() && DeathTimerMatches(true);
}

function bool ReleaseDeathTimerPause()
{
    local int Index;

    if (!bDeathTimerPausedByUs)
        return true;
    if (!OwnsCurrentAction() || int(Reviver.Role) != 3 || int(Target.Role) != 3)
        return false;
    Index = FindDeathTimer();
    if (Index == -1)
    {
        // Native recovery may remove the timer. Never recreate it.
        bDeathTimerPausedByUs = false;
        return true;
    }
    if (DeathTimerMatches(false))
    {
        bDeathTimerPausedByUs = false;
        return true;
    }
    // Do not resume a replaced timer or a death callback on a recovered pawn.
    if (!DeathTimerMatches(true) || !Target.bIsDowned || Target.bIsDead
        || !Target.IsInState('Downed'))
        return false;
    Target.PauseTimer(false, 'PermaDeath', Target);
    if (!DeathTimerMatches(false))
        return false;
    bDeathTimerPausedByUs = false;
    return true;
}

function bool ReleaseTargetState()
{
    if (!ReleaseDeathTimerPause())
        return false;
    if (!ReleaseUsageDisable())
        return false;
    return ReleaseReservation();
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
    if (bDeathTimerPausedByUs || bUsageChanged)
        return false;
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

    if (bDurationStarted || bCancelled || bDispatched || !bDeathTimerCaptured || !bUsageCaptured || !CanContinue()
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
    ReleaseTargetState();
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
