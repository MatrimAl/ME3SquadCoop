// Separate factory avoids the supplied compiler's new-class self-type lookup
// limitation. It performs no package loading or peer/session authorization.
class SFXSquadCoop_ReviveFactory extends Object;

// Keep the returned service even after failed preparation: it can retain
// pending cleanup. Runtime must inspect Status and retire the same token.
static function Actor CreateFor(SFXPawn_PlayerParty Candidate,
    SFXPlayerController Admission, AnimSet LoadedSet, int Token, bool bSimulatedPresentation)
{
    local SFXSquadCoop_ReviveService Service;

    if (Token <= 0 || Candidate == None || Candidate.bDeleteMe || Admission == None
        || Admission.bDeleteMe || !Admission.IsLocalPlayerController()
        || Candidate.WorldInfo != Admission.WorldInfo
        || Candidate.WorldInfo != class'WorldInfo'.static.GetWorldInfo())
        return None;
    Service = Candidate.Spawn(class'SFXSquadCoop_ReviveService', Admission);
    if (Service != None)
        Service.Initialize(Candidate, Admission, LoadedSet, Token, bSimulatedPresentation);
    return Service;
}
