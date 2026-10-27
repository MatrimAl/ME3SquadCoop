// Coordinates authored leases only. A runtime must own this object's lifetime,
// session admission, per-peer package/assets and pre-handoff retirement.
class SFXSquadCoop_ReviveSetup extends Object;

var SFXSquadCoop_ReviveBinding Binding;
var SFXSquadCoop_ReviveAnimation Animation;
var bool bPreparing;
var bool bRetiring;
var bool bReady;
var bool bBusy;
var name LastFailure;

function bool HasPendingWork()
{
    return bPreparing || bBusy || bRetiring || bReady
        || (Binding != None && (Binding.bBound || Binding.bInstancing))
        || (Animation != None && (Animation.bPending || Animation.bMutating));
}

function bool Prepared()
{
    return bReady && !bBusy && !bPreparing && !bRetiring && Binding != None
        && Binding.ValidBinding() && Binding.OrdinaryWorld()
        && (Animation == None || (Animation.IsRegistered()
            && Animation.Pawn == Binding.ControlledPawn && Animation.Mesh == Binding.CapturedMesh
            && Animation.ExpectedAnimation == Binding.CapturedAnimation));
}

function bool Ready()
{
    return Prepared() && Binding.AdmissionOpen();
}

function bool Activate()
{
    return Prepared() && Binding.OpenAdmission() && Ready();
}

// Opening an already open admission is unnecessary while its action is busy.
// A closed binding uses exactly the same idle/pending guards as OpenAdmission.
function bool CanActivate()
{
    return Prepared() && (Binding.AdmissionOpen()
        || (Binding.IdlePawn() && !Binding.ActionHasPendingWork()));
}

// Peer readiness can expire without changing ownership. Close only the new
// admission gate; keep this binding and its animation available for rearming.
function bool Suspend()
{
    if (bBusy || bPreparing || bRetiring || !bReady || Binding == None)
        return false;
    return Binding.CloseAdmission() && !Binding.AdmissionOpen();
}

// bBusy remains held by the caller across native registration/instancing.
// Release binding first: the action must no longer depend on a temporary set.
function bool RetireOwnedParts()
{
    if (Binding != None && !Binding.Release())
        return false;
    // The action is detached before native animation removal can become
    // pending. Retain only that animation lease across a permitted handoff.
    Binding = None;
    if (Animation != None && !Animation.Release())
        return false;
    Animation = None;
    bRetiring = false;
    return true;
}

function bool Retire()
{
    local bool bDone;

    if (bBusy || bPreparing)
        return false;
    bReady = false;
    bRetiring = true;
    bBusy = true;
    bDone = RetireOwnedParts();
    bBusy = false;
    return bDone;
}

// Handoff readiness is distinct from complete retirement. The runtime must
// continue polling Retire on this same lifetime and forbid a replacement
// setup until native removal completes.
function bool HandoffReady()
{
    return bRetiring && !bBusy && !bPreparing && !bReady && Binding == None
        && (Animation == None || Animation.HandoffReady());
}

function bool FailPreparation(name Reason)
{
    LastFailure = Reason;
    bReady = false;
    bPreparing = false;
    bRetiring = true;
    RetireOwnedParts();
    bBusy = false;
    return false;
}

// Return value is local preparation, not network agreement or revival proof.
// False may leave HasPendingWork true; retain and poll Retire, do not discard.
function bool Prepare(SFXPawn_PlayerParty Candidate, SFXPlayerController Admission,
    AnimSet OptionalLoadedSet, bool bSimulatedPresentation, optional bool bDeferAdmission)
{
    if (HasPendingWork())
        return false;
    bBusy = true;
    bPreparing = true;
    LastFailure = 'None';
    Binding = new(Self) class'SFXSquadCoop_ReviveBinding';
    if (Binding == None || Candidate == None || Candidate.bDeleteMe || Candidate.Mesh == None)
        return FailPreparation('MissingContext');
    if ((bSimulatedPresentation && (int(Candidate.Role) != 1 || Candidate.Controller != None))
        || (!bSimulatedPresentation && (int(Candidate.Role) != 2 && int(Candidate.Role) != 3
            || SFXPlayerController(Candidate.Controller) == None)))
        return FailPreparation('InvalidPresentationRole');
    if (Candidate.Mesh.FindAnimSequence('CB_MP_Revive') == None)
    {
        if (OptionalLoadedSet == None)
            return FailPreparation('MissingLoadedAnimation');
        Animation = new(Self) class'SFXSquadCoop_ReviveAnimation';
        if (Animation == None || !Animation.Acquire(Candidate, Admission, OptionalLoadedSet))
            return FailPreparation('AnimationRegistrationRefused');
    }
    // This coordinator uses the authored lifecycle only. The existing class
    // alternative remains a separate investigation, with different cleanup.
    if (!Binding.Bind(Candidate, Admission, class'SFXSquadCoop_ReviveAction', bSimulatedPresentation))
        return FailPreparation('ActionBindingRefused');
    if (!Binding.ValidBinding() || !Binding.OrdinaryWorld()
        || (Animation != None && (!Animation.IsRegistered()
            || Animation.ExpectedAnimation != Binding.CapturedAnimation)))
        return FailPreparation('PreparedContextChanged');
    if (!bDeferAdmission && !Binding.OpenAdmission())
        return FailPreparation('AdmissionOpenRefused');
    bPreparing = false;
    bReady = true;
    bBusy = false;
    return true;
}
