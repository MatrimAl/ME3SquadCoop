// Candidate action5 driver. Package mounting/shared class assignment are not
// integrated; usage/timer cleanup retains unresolved state without guessing.
class SFXSquadCoop_ReviveAction extends SFXCustomAction_SingleAnim;

var SFXSquadCoop_ReviveContext Context;
var bool bBaseStarted;
var bool bAdmissionClosed;
var bool bFinishing;
var bool bStopping;
var bool bTargetCleanupPending;
var bool bRecoveryObserved;
var int AttemptSerial;
var float RecoveryStartedAt;
var float RecoveryDeadline;
var float RecoveryWait;
var bool bPresentationDurationStarted;
var float PresentationStartedAt;
var float PresentationDeadline;

function bool OwnsSlot()
{
    return m_oPawn != None && !m_oPawn.bDeleteMe
        && m_oPawn.WorldInfo == class'WorldInfo'.static.GetWorldInfo()
        && m_oPawn.CurrentCustomAction == 5 && m_oPawn.CustomActions.Length > 5
        && m_oPawn.CustomActions[5] == Self;
}

function bool PresentationContext(BioPawn SyncPawn)
{
    if (m_oPawn == None || m_oPawn.bDeleteMe || SyncPawn == None || SyncPawn.bDeleteMe
        || SFXPawn_PlayerParty(m_oPawn) == None || SFXPawn_PlayerParty(SyncPawn) == None
        || m_oPawn == SyncPawn || m_oPawn.WorldInfo != SyncPawn.WorldInfo
        || m_oPawn.WorldInfo != class'WorldInfo'.static.GetWorldInfo() || m_oAI != None)
        return false;
    return m_oPC == None || (m_oPC == m_oPawn.Controller && m_oPC.Pawn == m_oPawn);
}

function bool InternalCanDoCustomAction(BioPawn SyncPawn, bool bForced)
{
    if (bAdmissionClosed || bBaseStarted || bStopping || bFinishing || bTargetCleanupPending
        || !PresentationContext(SyncPawn))
        return false;
    if (int(m_oPawn.Role) != 3)
        return Super.InternalCanDoCustomAction(SyncPawn, bForced);
    if (Context == None)
        Context = new(Self) class'SFXSquadCoop_ReviveContext';
    return Context != None && Context.Capture(Self, SyncPawn)
        && Super.InternalCanDoCustomAction(SyncPawn, bForced);
}

function StartCustomAction()
{
    local float AnimLength;
    local float AnimPosition;
    local int ThisAttempt;
    local bool bAnimationPlaying;

    if (bBaseStarted || bStopping || bFinishing || bTargetCleanupPending
        || !OwnsSlot() || !PresentationContext(BioPawn(m_oPawn.SyncPawn)))
        return;
    if (!(RecoveryWait > 0.0) || RecoveryWait - RecoveryWait != 0.0)
    {
        InterruptThisCustomAction();
        return;
    }
    if (int(m_oPawn.Role) == 3)
    {
        if (Context == None)
            Context = new(Self) class'SFXSquadCoop_ReviveContext';
        if (Context == None || !Context.Capture(Self, BioPawn(m_oPawn.SyncPawn))
            || !Context.AcquireReservation() || !Context.AcquireUsageDisable()
            || !Context.AcquireDeathTimerPause())
        {
            ReleaseContext(true);
            InterruptThisCustomAction();
            return;
        }
    }
    bFinishing = false;
    bRecoveryObserved = false;
    bPresentationDurationStarted = false;
    RecoveryStartedAt = 0.0;
    RecoveryDeadline = 0.0;
    AttemptSerial++;
    ThisAttempt = AttemptSerial;
    bBaseStarted = true;
    Super.StartCustomAction();
    if (ThisAttempt != AttemptSerial || !bBaseStarted || bStopping || bFinishing || !OwnsSlot())
        return;
    // SingleAnim can fail to play while leaving the action started. Do not
    // silently replace the required animation with a health-only interaction.
    if (BS_Anim.AnimName.Length == 1)
        bAnimationPlaying = m_oPawn.GetAnimLengthAndPos(BS_Anim.AnimName[0], AnimLength, AnimPosition);
    if (ThisAttempt != AttemptSerial || !bBaseStarted || bStopping || bFinishing || !OwnsSlot())
        return;
    if (!bAnimationPlaying || !(AnimLength > 0.0) || AnimLength - AnimLength != 0.0)
    {
        FinishAttempt(true);
        return;
    }
    if (int(m_oPawn.Role) == 3 && (Context == None || !Context.BeginDuration()))
        FinishAttempt(true);
    if (int(m_oPawn.Role) != 3 && !BeginPresentationDuration())
        FinishPresentation();
}

// The reference revive has a timer on presentation peers as well. Local
// expiry ends only this presentation; it never dispatches Revive or success.
function bool BeginPresentationDuration()
{
    local float Duration;

    if (!bBaseStarted || !OwnsSlot() || int(m_oPawn.Role) == 3
        || !PresentationContext(BioPawn(m_oPawn.SyncPawn)))
        return false;
    Duration = SFXPawn_PlayerParty(m_oPawn).fTimeToRevive;
    PresentationStartedAt = m_oPawn.WorldInfo.TimeSeconds;
    PresentationDeadline = PresentationStartedAt + Duration;
    if (!(Duration > 0.0) || Duration - Duration != 0.0
        || PresentationStartedAt - PresentationStartedAt != 0.0
        || !(PresentationDeadline > PresentationStartedAt)
        || PresentationDeadline - PresentationDeadline != 0.0)
        return false;
    bPresentationDurationStarted = true;
    return true;
}

function FinishPresentation()
{
    if (!bBaseStarted || bFinishing || bStopping || !OwnsSlot() || int(m_oPawn.Role) == 3)
        return;
    bFinishing = true;
    // EndCustomAction is a local DoCustomAction(0) script path. Unlike
    // InterruptThisCustomAction it sends no server-interrupt request.
    EndThisCustomAction();
}

function ReleaseContext(bool bCancel)
{
    if (Context == None)
    {
        bTargetCleanupPending = false;
        return;
    }
    if (bCancel)
        Context.Cancel();
    // Keep a failed release visible and refuse a fresh capture. It is not
    // permission to clear a target after losing the owning action identity.
    bTargetCleanupPending = !Context.ReleaseTargetState();
}

function FinishAttempt(bool bInterrupted)
{
    if (bFinishing || bStopping || !bBaseStarted || !OwnsSlot())
        return;
    bFinishing = true;
    ReleaseContext(bInterrupted);
    if (bInterrupted)
    {
        InterruptThisCustomAction();
    }
    else
    {
        EndThisCustomAction();
    }
}

function TickCustomAction(float DeltaTime)
{
    local int ThisAttempt;
    local float Now;
    local bool bDidDispatch;

    ThisAttempt = AttemptSerial;
    Super.TickCustomAction(DeltaTime);
    if (ThisAttempt != AttemptSerial || !bBaseStarted || bFinishing || bStopping || !OwnsSlot())
        return;
    if (int(m_oPawn.Role) != 3)
    {
        Now = m_oPawn.WorldInfo.TimeSeconds;
        if (!bPresentationDurationStarted || !PresentationContext(BioPawn(m_oPawn.SyncPawn))
            || Now - Now != 0.0 || Now < PresentationStartedAt || Now >= PresentationDeadline)
            FinishPresentation();
        return;
    }
    if (Context == None)
    {
        FinishAttempt(true);
        return;
    }
    if (Context.bDispatched)
    {
        Now = m_oPawn.WorldInfo.TimeSeconds;
        if (Now - Now != 0.0 || Now < RecoveryStartedAt)
        {
            FinishAttempt(true);
            return;
        }
        bRecoveryObserved = Context.RecoveryObserved();
        if (bRecoveryObserved)
        {
            FinishAttempt(false);
        }
        else if (!Context.OrdinaryWorld() || Now >= RecoveryDeadline)
        {
            FinishAttempt(true);
        }
        return;
    }
    CheckMoving();
    if (ThisAttempt != AttemptSerial || !bBaseStarted || bStopping || !OwnsSlot() || bFinishing)
        return;
    if (Context.DurationComplete())
    {
        // Publish the observation interval before native Revive can call back
        // into this action. The interval is not evidence of recovery.
        RecoveryStartedAt = m_oPawn.WorldInfo.TimeSeconds;
        RecoveryDeadline = RecoveryStartedAt + RecoveryWait;
        if (RecoveryStartedAt - RecoveryStartedAt != 0.0
            || !(RecoveryWait > 0.0) || RecoveryWait - RecoveryWait != 0.0
            || !(RecoveryDeadline > RecoveryStartedAt) || RecoveryDeadline - RecoveryDeadline != 0.0)
        {
            FinishAttempt(true);
            return;
        }
        bDidDispatch = Context.DispatchRevive();
        // Native resurrection may stop this action or start a later attempt.
        // Never let the returning old stack end that later attempt.
        if (ThisAttempt != AttemptSerial || !bBaseStarted || bStopping || bFinishing || !OwnsSlot())
            return;
        if (!bDidDispatch)
            FinishAttempt(true);
    }
    else if (Context.bCancelled)
        FinishAttempt(true);
}

function StopCustomAction()
{
    // The selected pawn script invokes this before its own surrounding native
    // transition is fully resolved. Never clear a foreign reservation here.
    if (bStopping)
        return;
    bStopping = true;
    bPresentationDurationStarted = false;
    ReleaseContext(true);
    if (bBaseStarted)
    {
        bBaseStarted = false;
        Super.StopCustomAction();
    }
    bFinishing = false;
    bStopping = false;
}

function Replicate()
{
    Super.Replicate();
    if (m_oPawn != None)
        m_oPawn.ReplicatedCustomActionInfo.Target = m_oPawn.SyncPawn;
}

function ClientDoCustomAction(bool bForced)
{
    if (m_oPawn == None)
        return;
    m_oPawn.SyncPawn = BioPawn(m_oPawn.ReplicatedCustomActionInfo.Target);
    Super.ClientDoCustomAction(bForced);
}

// Authority owns actual revival; each nonauthority peer has a local
// presentation deadline. Animation notification alone declares neither.
function BodyStanceAnimEndNotification(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime)
{
}

defaultproperties
{
    bAdmissionClosed=true
    BS_Anim={AnimName=('CB_MP_Revive')}
    fAnimDuration=0.0
    fAnimBlendInTime=0.4
    fAnimBlendOutTime=0.4
    bReplicateCustomAction=true
    bClientPredictCustomAction=false
    bForceLocalSimulation=false
    bDisableMovement=false
    ERootMotionMode=ERootMotionMode.RMM_Ignore
    ERootMotionRotationMode=ERootMotionRotationMode.RMRM_Ignore
    RecoveryWait=2.0
}
