// Already-loaded asset lease only. The runtime must supply session admission
// and retire the action binding before this lease; this object loads nothing.
class SFXSquadCoop_ReviveAnimation extends Object;

var SFXPawn_PlayerParty Pawn;
var SFXPlayerController LocalController;
var Controller CapturedController;
var WorldInfo SharedWorld;
var SkeletalMeshComponent Mesh;
var AnimSet Set;
var AnimSequence ExpectedAnimation;
var bool bPending;
var bool bUnregisterRequested;
var bool bMutating;
var int CapturedRole;

// Read-only removal observation may outlive the captured controller/role.
// Mutating registration/unregistration still requires SameContext below.
function bool SameAssetLifetime()
{
    return Pawn != None && !Pawn.bDeleteMe && SharedWorld != None && !SharedWorld.bDeleteMe
        && Pawn.WorldInfo == SharedWorld && class'WorldInfo'.static.GetWorldInfo() == SharedWorld
        && Mesh != None && Pawn.Mesh == Mesh && Set != None;
}

function bool SameContext()
{
    return SameAssetLifetime() && LocalController != None && !LocalController.bDeleteMe
        && LocalController.WorldInfo == SharedWorld && LocalController.IsLocalPlayerController()
        && LocalController.Pawn != None && LocalController.Pawn.Controller == LocalController
        && LocalController.Pawn.WorldInfo == SharedWorld && !LocalController.bCinematicMode
        && LocalController.GameModeManager2 != None && LocalController.GameModeManager2.CurrentMode == 0
        && SharedWorld.Pauser == None && !SharedWorld.bPlayersOnly && !SharedWorld.bPlayersOnlyPending
        && Pawn.Controller == CapturedController && int(Pawn.Role) == CapturedRole
        && ((CapturedRole == 1 && CapturedController == None)
            || (CapturedRole == 2 || CapturedRole == 3) && SFXPlayerController(CapturedController) != None
                && CapturedController.Pawn == Pawn);
}

function int RecordIndex()
{
    local int Index;
    local int Found;

    Found = -1;
    for (Index = 0; Index < Pawn.TemporaryAnims.Length; Index++)
    {
        if (Pawn.TemporaryAnims[Index].TempAnimSet == Set)
        {
            if (Found != -1)
                return -2;
            Found = Index;
        }
    }
    return Found;
}

function int MeshOccurrences()
{
    local int Index;
    local int Count;

    for (Index = 0; Index < Mesh.AnimSets.Length; Index++)
    {
        if (Mesh.AnimSets[Index] == Set)
            Count++;
    }
    return Count;
}

function bool IdleContext()
{
    local BioWorldInfo Info;

    if (!SameContext() || Pawn.CurrentCustomAction != 0 || Pawn.CurrentPowerCustomAction != 0)
        return false;
    Info = BioWorldInfo(SharedWorld);
    return Info != None && (Info.m_pConversationManager == None
        || (Info.m_pConversationManager.m_aActiveConversations.Length == 0
            && !Info.m_pConversationManager.m_bActivatingConversation));
}

function bool IsRegistered()
{
    local int Index;

    if (!bPending || bMutating || bUnregisterRequested || !SameContext())
        return false;
    Index = RecordIndex();
    return Index >= 0 && Pawn.TemporaryAnims[Index].RefCount > 0 && MeshOccurrences() == 1
        && ExpectedAnimation != None && Mesh.FindAnimSequence('CB_MP_Revive') == ExpectedAnimation;
}

function bool Acquire(SFXPawn_PlayerParty Candidate, SFXPlayerController Admission, AnimSet LoadedSet)
{
    local int Index;
    local int SequenceIndex;
    local int MatchingSequences;
    local AnimSequence Sequence;

    if (bPending || bMutating || Candidate == None || LoadedSet == None)
        return false;
    Pawn = Candidate;
    LocalController = Admission;
    CapturedController = Candidate.Controller;
    SharedWorld = Candidate.WorldInfo;
    CapturedRole = int(Candidate.Role);
    Mesh = Candidate.Mesh;
    Set = LoadedSet;
    ExpectedAnimation = None;
    if (!IdleContext() || Pawn.bIsDowned || Pawn.bIsDead || Pawn.IsDead()
        || Pawn.IsInState('InRagdoll') || Pawn.IsInState('RagdollRecovery')
        || Mesh.FindAnimSequence('CB_MP_Revive') != None || RecordIndex() != -1 || MeshOccurrences() != 0)
        return false;
    for (SequenceIndex = 0; SequenceIndex < Set.Sequences.Length; SequenceIndex++)
    {
        Sequence = Set.Sequences[SequenceIndex];
        if (Sequence != None && Sequence.SequenceName == 'CB_MP_Revive')
        {
            if (!(Sequence.SequenceLength > 0.0) || Sequence.SequenceLength - Sequence.SequenceLength != 0.0)
                return false;
            MatchingSequences++;
            ExpectedAnimation = Sequence;
        }
    }
    if (MatchingSequences != 1 || !IdleContext() || RecordIndex() != -1 || MeshOccurrences() != 0)
        return false;
    bPending = true;
    bUnregisterRequested = false;
    bMutating = true;
    Pawn.RegisterTemporaryAnim(Set);
    bMutating = false;
    if (!SameContext())
        return false;
    Index = RecordIndex();
    return Index >= 0 && Pawn.TemporaryAnims[Index].RefCount == 1
        && MeshOccurrences() == 1 && Mesh.FindAnimSequence('CB_MP_Revive') == ExpectedAnimation;
}

// This does not claim disposal. It permits a control handoff only after the
// binding is gone and the native unregister's zero-reference result is read
// back. Shared references or changed mesh/world keep it false.
function bool HandoffReady()
{
    local int Index;

    if (bMutating)
        return false;
    if (!bPending)
        return true;
    if (!bUnregisterRequested || !SameAssetLifetime())
        return false;
    Index = RecordIndex();
    return (Index == -1 && MeshOccurrences() == 0)
        || (Index >= 0 && Pawn.TemporaryAnims[Index].RefCount == 0 && MeshOccurrences() == 1);
}

function bool Release()
{
    local int Index;

    if (bMutating)
        return false;
    if (!bPending)
        return true;
    if (Pawn == None || Pawn.bDeleteMe || SharedWorld == None || SharedWorld.bDeleteMe)
    {
        bPending = false;
        return true;
    }
    // Once requested, only observe exact asset removal. Do not require an
    // old owner or ordinary gameplay to remain alive while native ticking
    // finishes; never issue another unregister from this branch.
    if (bUnregisterRequested)
    {
        if (!SameAssetLifetime())
            return false;
        if (RecordIndex() == -1 && MeshOccurrences() == 0)
        {
            bPending = false;
            return true;
        }
        return false;
    }
    if (!IdleContext())
        return false;
    Index = RecordIndex();
    if (Index == -1 && MeshOccurrences() == 0)
    {
        bPending = false;
        return true;
    }
    if (Index < 0 || Pawn.TemporaryAnims[Index].RefCount != 1
        || MeshOccurrences() != 1)
        return false;
    // Native cleanup owns delayed removal after the count reaches zero. Do
    // not force-remove or decrement again on a later retirement poll.
    bUnregisterRequested = true;
    bMutating = true;
    Pawn.UnregisterTemporaryAnim(Set, 0.0, false);
    bMutating = false;
    return false;
}
