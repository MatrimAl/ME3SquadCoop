# Private owner-derived revive animation package

`tools/prepare_revive_assets.ps1` adds the selected owner's revive animation
closure to a private copy of the compiled seven-class script package. The
generated PCC contains owner game assets and is not redistributable. Neither
those bytes nor private source paths belong in Git or a public release.

The current supported source is the reviewed installed LEMP female Soldier
carrier. The tool identifies its exact dynamic set and CB_MP_Revive sequence,
validates sequence metadata and embedded compressed data, and narrows the set's
Sequences array and native sequence-name map to that one entry in memory.
SourcePackage is never saved. LEC CloneAllDependencies ports the narrowed set
into a fresh private copy of ME3SquadCoopActions.pcc.

The observed closure adds ten exports: a dynamic set, one animation, bone-track
data, two package containers, four audio notify objects and one Foley event.
No pawn, skeletal mesh, world or game-class export was ported. The animation
container is renamed ReviveAnimationData and the set becomes
`ME3SquadCoopActions.ReviveAnimSet`. Foley bank/stream/event imports remain
external dependencies of the owner's installation; audio resolution/playback
has not been verified.

## Reflected dependency anchor

The factory declares an AnimSet ReviveAnimation property. The derivation tool
injects exactly that reference into its otherwise sparse default object after
porting. This supplies a normal reflected package dependency and GC-visible
reference; it is not a claim of observed runtime loading/rooting. The factory
also uses its default if a caller supplies None. All original exports except
this explicit factory-default change retain identical data, original name and
import indices remain intact, and saved readback checks the anchor.

The native preparation candidate now resolves the exact already-loaded set
and supplies it to CreateFor. It waits when that asset is absent, rather than
creating a doomed setup or loading a whole MP player. Missing animation does
not prevent an existing service from polling retirement. Actual mesh
registration still validates one sequence and exact native lookup readback;
admission remains closed pending peer agreement.

## Offline preparation

```powershell
& tools/prepare_revive_assets.ps1 -CompileReceipt $privateCompileReceipt `
  -SourcePackage $ownerCarrier -GamePath $ownerGameRoot `
  -LegendaryExplorerPath $existingToolRoot -OutputDirectory $freshPrivateOutput
```

The derived receipt has Status=compiled-with-owner-assets-not-installed,
ContainsOwnerGameAssets=true and DistributionAllowed=false. It retains the
original compilation receipt/digest and source/tool input hashes. Successful
readback checks script executable data, original names/imports, the factory
anchor, one sequence-name map entry, bone and translation-track names,
sequence metadata and byte-identical compressed animation data. Relinker
reports must be empty and unexpected dependency classes are rejected.

The latest package is 144894 bytes with SHA-256
`48f2c24ba5b027b09f39ba50dc869a11b86922c7febf80b9885e5496c8ac9dc2`.
Its animation remains 5.3 seconds/159 frames; native fTimeToRevive continues to
govern revival duration. These are serialized properties, not measured frames.
All tracked inputs stayed unchanged. Existing DLC/feature staging accepts this
distinct private receipt and propagates its asset/distribution flags. Release
gates and installer allowlists were not expanded.

Engine startup acceptance, actual loaded object paths, skeleton compatibility,
mesh playback, Foley dependencies, peer readiness and mutual revival remain
unverified. No installation, game, fixture or behavioral tests ran for this
revision. The earlier unanchored private outputs are retained as research,
not the current staging candidate.
