// Local world actor retains the setup as a reflected reference. Runtime
// admission, peer agreement, asset loading and polling are still external.
class SFXSquadCoop_ReviveService extends Actor;

var SFXSquadCoop_ReviveSetup Setup;
var SFXPawn_PlayerParty SessionPawn;
var SFXPlayerController LocalAdmission;
var int ScopeToken;
var bool bInitialized;
var bool bBusy;
var bool bRetiring;
var bool bRetired;

function bool Initialize(SFXPawn_PlayerParty Candidate, SFXPlayerController Admission,
    AnimSet LoadedSet, int Token, bool bSimulatedPresentation)
{
    local bool bPrepared;

    if (bInitialized || bBusy || bDeleteMe || Token <= 0 || Candidate == None || Admission == None
        || Candidate.bDeleteMe || Admission.bDeleteMe || Candidate.WorldInfo != WorldInfo
        || Admission.WorldInfo != WorldInfo || WorldInfo != class'WorldInfo'.static.GetWorldInfo()
        || Owner != Admission || !Admission.IsLocalPlayerController())
        return false;
    bBusy = true;
    bInitialized = true;
    ScopeToken = Token;
    SessionPawn = Candidate;
    LocalAdmission = Admission;
    Setup = new(Self) class'SFXSquadCoop_ReviveSetup';
    if (Setup != None)
        bPrepared = Setup.Prepare(Candidate, Admission, LoadedSet, bSimulatedPresentation, true);
    bRetiring = !bPrepared;
    bBusy = false;
    return bPrepared;
}

function bool SameScope(int Token)
{
    return Token > 0 && Token == ScopeToken && bInitialized && !bDeleteMe
        && SessionPawn != None && !SessionPawn.bDeleteMe && SessionPawn.WorldInfo == WorldInfo
        && LocalAdmission != None && !LocalAdmission.bDeleteMe && Owner == LocalAdmission
        && LocalAdmission.WorldInfo == WorldInfo && LocalAdmission.IsLocalPlayerController()
        && WorldInfo != None && !WorldInfo.bDeleteMe
        && WorldInfo == class'WorldInfo'.static.GetWorldInfo();
}

// Spawn callbacks can invalidate Initialize's preflight before it captures
// the token. Preserve a destroyable lifetime for that unused actor without
// touching a pawn, creating Setup or pretending preparation succeeded.
function bool RetireUninitialized(int Token)
{
    if (bBusy || bInitialized || Setup != None || bDeleteMe || Token <= 0)
        return false;
    ScopeToken = Token;
    bInitialized = true;
    bRetiring = true;
    bRetired = true;
    return true;
}

// Runtime may call only after its real peer-readiness agreement. A token
// selects a local lifetime; it is not authentication or that agreement.
function bool Activate(int Token)
{
    local bool bActivated;

    if (bBusy || bRetiring || bRetired || !SameScope(Token) || Setup == None)
        return false;
    bBusy = true;
    bActivated = Setup.Activate();
    bBusy = false;
    return bActivated;
}

function bool CanActivate(int Token)
{
    return !bBusy && !bRetiring && !bRetired && SameScope(Token)
        && Setup != None && Setup.CanActivate();
}

function bool Suspend(int Token)
{
    local bool bClosed;

    if (bBusy || bRetiring || bRetired || !SameScope(Token) || Setup == None)
        return false;
    bBusy = true;
    bClosed = Setup.Suspend();
    bBusy = false;
    return bClosed;
}

// 0=unavailable, 1=prepared/closed, 2=active, 3=retiring/pending, 4=retired.
// Readiness is sampled, not authorization for a later native operation.
function int Status(int Token)
{
    if (bBusy || !bInitialized || Token <= 0 || Token != ScopeToken || bDeleteMe)
        return 0;
    if (bRetired)
        return 4;
    if (bRetiring)
        return 3;
    if (!SameScope(Token) || Setup == None || !Setup.Prepared())
        return 0;
    if (Setup.Ready())
        return 2;
    return 1;
}

function bool Retire(int Token)
{
    local bool bDone;

    if (bBusy || !bInitialized || Token <= 0 || Token != ScopeToken || bDeleteMe)
        return false;
    if (bRetired)
        return true;
    bBusy = true;
    bRetiring = true;
    bDone = Setup == None || Setup.Retire();
    if (bDone)
    {
        Setup = None;
        bRetired = true;
    }
    bBusy = false;
    return bDone;
}

function bool DestroyRetired(int Token)
{
    if (bBusy || Token <= 0 || Token != ScopeToken || !bInitialized || !bRetired
        || Setup != None || bDeleteMe || WorldInfo != class'WorldInfo'.static.GetWorldInfo())
        return false;
    return Destroy();
}

// Release the action and request nonforced native animation removal while
// control is still captured. True may leave Status==3: retain and poll this
// actor after handoff; DestroyRetired remains unavailable until Status==4.
function bool RetireForHandoff(int Token)
{
    if (Retire(Token))
        return true;
    return !bBusy && bInitialized && bRetiring && !bRetired && !bDeleteMe
        && Token > 0 && Token == ScopeToken && Setup != None && Setup.HandoffReady();
}

defaultproperties
{
    bHidden=true
    bStatic=false
    bNoDelete=false
    bTickIsDisabled=true
    bCollideActors=false
    bBlockActors=false
    bReplicateMovement=false
    bAlwaysRelevant=false
    RemoteRole=ENetRole.ROLE_None
    LifeSpan=0.0
}
