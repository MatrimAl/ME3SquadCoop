# Revive animation and class availability

Offline package reads located the authored action's CB_MP_Revive animation in
two selected installed LEMP player packages. A new
[private owner-derived package](REVIVE-ASSET-PACKAGE.md) now contains that one
clip and its dependency closure, anchored to the authored factory default.
Actual loading/assignment remains unobserved. No generated assets were
installed or added to the public repository. The sections below retain the
earlier investigation and its revision-specific evidence.

## Concrete asset chain

| Carrier | Observed serialized chain |
| --- | --- |
| HumanFemale_Soldier_MP.pcc | HumanFemale_Soldier.BioPawnSkeletalMeshComponent.AnimSets includes HumanFemale_Soldier.MY_DYN_HMM_DG_Deaths; that set's Sequences includes the revive export |
| HumanMale_Soldier_MP.pcc | HumanMale_Soldier.BioPawnSkeletalMeshComponent.AnimSets includes HumanMale_Soldier.MY_DYN_HMM_DG_Deaths; that set's Sequences includes the revive export |

Both selected carriers contain an AnimSequence export named
BIOG_HMM_DG_A.HMM_DG_Deaths_CB_MP_Revive with SequenceName=CB_MP_Revive,
SequenceLength=5.3 and NumFrames=159. The female carrier also uses the HMM
death set; these names are observations, not a new skeleton compatibility
claim. Matching paths/properties in two carriers do not establish identical
animation bytes or working playback on Ashley.

The sequence length is not a replacement for the native party pawn's
fTimeToRevive. Animation completion, native Revive dispatch and observed
recovery remain separate events. The candidate retains the configured native
revive duration and refuses an unsuccessful animation query at action start.

## Basegame action alternative

BioP_Char.pcc contains a local Class export SFXGameContent.SFXCustomAction_Revive
extending SFXGame.SFXCustomAction_SingleAnim, plus its serialized default object.
The selected LEMP startup package also contains that class path/default object.
Both defaults select CB_MP_Revive, blend in/out0.4, bDisableMovement=false and
bReplicateCustomAction=true. They also contain additional override, timeline,
root-motion and charge-holding settings that the authored candidate does not
fully reproduce.

This narrows a useful integration alternative: evaluate a scoped binding to
the already supplied basegame class before assuming a new class package is
required for every mission. A serialized class in one package does not prove
that it is loaded, uniquely resolved or assigned to a live pawn. It does not
supply the animation to Ashley. Actual slot5 availability, cached controller
context, shared package loading, admission and cleanup remain unresolved.

The authored candidate now explicitly sets bReplicateCustomAction=true and
bDisableMovement=false, with0.4 blend settings. This expresses replication
intent and permits movement input to reach the existing cancellation path;
it does not prove network delivery or movement interruption. Its root motion
remains Ignore, whereas the inspected reference selects Translate. No native
timeline or override asset/list was copied into the authored package.

## Temporary animation registration candidate

`scripts/SFXSquadCoop_ReviveAnimation.uc` supplies an uncalled lease for an
already loaded AnimSet. Runtime admission and asset loading remain external.
The [setup coordinator](REVIVE-SETUP.md) now uses this optional lease before
binding action5 and releases it only after the action binding retires.
It captures current pawn/controller/world/role/mesh, requires idle ordinary
control, and refuses existing revive lookup, matching temporary records or a
mesh occurrence of the supplied set. Existing availability should use the
binding directly. The supplied set must have exactly one CB_MP_Revive sequence
with positive finite length; readback must resolve that exact sequence.

Acquire records pending state before RegisterTemporaryAnim. It requires one
temporary reference and one mesh occurrence afterwards. A false result can
retain pending state and must not be treated as no mutation. Skeleton
compatibility and actual playback remain unverified.

Release must follow action-binding retirement in the same idle ordinary
context. It requests UnregisterTemporaryAnim once, with zero delay and no
force-remove, only when the record has one reference and one mesh occurrence.
It remains pending until later polling sees both removed. Shared/changed
counts remain pending rather than guessing ownership. Deleted pawn/world
state drops bookkeeping without a table write. No direct animation-array or
refcount writes occur. Same-value external replacement is not detectable.

Eight selected script exports matched serialized/VM lengths with16 aligned
explicit branch targets. RegisterTemporaryAnim increments an existing
temporary record, leaves an already installed mesh set unowned, or adds a
count1 record and calls AddAnimSet. UnregisterTemporaryAnim decrements its
matching record and sets TimeLeft at zero; the candidate never requests its
force-remove fallback. Native deferred removal remains unresolved.

AddAnimSet, RmvAnimSet and RegisterCustomAnimset are native declarations:
their short script payloads are parameter stubs, not native body evidence.
The nonempty ReplaceAnimSetWithDynamic override merely sets a power's
CastAnimSet. LoadAnimSet uses shared PowerManager.LastPowerAnimSet state and
RegisterCustomAnimset, which the revive component does not borrow.

The four authored classes at this revision compiled together without errors/warnings and with
unchanged tracked inputs. The saved private package digest is
`0d5cecf7464dbd267087b04477e69e9f351124d3f7794bc8434550d3e5019af9`.
No runtime invokes this lease. Asset loading, native mesh refresh, reference
concurrency, deferred removal and peer playback remain unfinished. No game,
fixture, install, mount or tests ran.

## Native animation mutation and deferred cleanup follow-up

Exact-image registration names resolve AddAnimSet through string0x1383970,
pair0x179fb70 and thunk0xb3bbc0 to body0xa87140; RmvAnimSet through
string0x1383938, pair0x179fb60 and thunk0xb3bc40 to0xa93070;
RegisterCustomAnimset through string0x1383060, pair0x179f980 and thunk0xb3cd30
to0xa92020. The bridge calls were checked in decoded unwind-bounded assembly.
A retained-project export completed with three requested/nonempty bodies.

AddAnimSet reads the pawn Mesh at+0x3f4 and its AnimSets array at+0x320. An
absent set is appended. The existing-set branch invokes a removal helper
before appending it again; it is not a harmless reference-count increment.
The mesh's+0x43c bit0x800, declared bAnimSetUpdated, is set. RmvAnimSet removes
a matching mesh entry and sets that update bit. Both contain additional
helper-returned-object handling that is not a complete mesh-cache/refresh
contract. RegisterCustomAnimset writes a separate pawn+0x584 map and appends
to the mesh; it does not operate on TemporaryAnims refcounts.

The explicit direct RmvAnimSet call at0xa94a8d was verified as an instruction
inside fragment0xa94a06. Its Windows unwind CHAININFO records lead through
0xa9492d and0xa94925 to parent entry0xa948e0. That evidence supplied the exact
parent export instead of guessing a nearby function. One requested/nonempty
parent decompilation completed, giving four successful entries in this review.

The parent's TemporaryAnims loop at pawn+0x76c/count+0x774 retains an entry
while its RefCount at+8 is nonzero. At zero it subtracts the update delta from
TimeLeft at+0xc; after expiry it removes the temporary record and calls
RmvAnimSet for the saved set. The selected processing block is skipped when
actor+0x25c mask0x41 is set, WorldInfo is absent, or WorldInfo+0x7c0 mask0x400
(PlayersOnly) is set. Other calls and the full tick scheduling/mesh refresh
lifecycle are not established by this cleanup observation.

This supports using normal temporary registration and waiting for actual
record/mesh removal. A pause can defer cleanup; wall-clock expiry must not be
treated as disposal permission. The current lease does not force native removal
or write the update bit, mesh list or temporary count. It still needs live
removal/frame/playback evidence and runtime pre-handoff coordination.

The authored lease now separates SameAssetLifetime from its stricter control
context. Once the one nonforced unregister request has been issued, Release
only observes exact pawn/world/mesh/set identity and record/mesh removal; it
does not require the old controller, role or unpaused state to remain intact.
Before that request, ordinary idle control is still required. HandoffReady
requires native zero-reference readback (or actual removal), not elapsed time,
and is not full disposal. Changed mesh/world and duplicate/shared records do
not authorize a write or replacement setup. This revision compiled together
with all seven classes without errors/warnings; live handoff remains untested.

The first export attempt failed before analysis because Ghidra could not save
its JDK location; it produced zero decompilations and remains separate. The
permission-corrected runs completed with exact source identity, terminal exit0
and nonempty per-function outputs. Private review records preserve hashes,
address chains, bounded facts and unknown nested helpers. No game or tests ran;
this step changed research evidence, not runtime behavior or the authored PCC.

## Search and evidence boundary

The selected scans read AnimSequence object names and SequenceName properties
in ten distinct carrier packages, totaling1450 sequence exports. The initial
four carriers (LEMP startup, SFXGame, BioH_Ashley_00, BioP_Char) contained no
revive-named sequence export among569 inspected sequences. The base
BioP_Global, BioP_Global_NC, Startup and GesturesConfig expansion supplied no
additional revive match; the two selected player carriers supplied the two
matches above. Repeated package reads are not counted as new unique coverage.
This is not an exhaustive game/DLC search or proof that no imported or runtime
animation path can provide the name elsewhere.

Selected serialized default/mesh/dynamic-set links and class presence were
retained privately with unchanged package hashes. No game, tests, package
mounting, runtime asset lookup or animation playback ran. The authored action
and context compiled with zero errors/warnings and unchanged tracked inputs;
the private package digest for the explicit-default revision is
`d5098e166210004a03187a17b04df28829d86bfe70f953f34d2925a8644a2e7d`.

The remaining work is a shared, scoped way to make the required animation
available on each actual reviver mesh and to bind an available action class
on both peers, preserving current animation/slot ownership and native story
admission. Loading whole MP player archetypes, replacing Ashley, forcing the
multiplayer GRI or treating a matching asset name as playback are not
implemented by this investigation. See [action limits](REVIVE-ACTION.md).
