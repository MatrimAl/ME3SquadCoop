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

The latest package is 146712 bytes with SHA-256
`78f28acddff76c2568da10d0346b77bba3231cea41d20e0dab2d75cae5ab8aee`.
Its seven classes were freshly compiled after adding the service/setup
`Suspend` and `CanActivate` methods; the previous source receipt was not reused.
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

## Private addon with the working activation bundle

`tools/prepare_revive_addon.ps1` combines the current verified four-file starter
activation receipt with this private owner-asset receipt in a fresh repository
`local/revive-addon/` directory. It reuses the seven-class PCC when every recorded source
and tool hash still matches; it does not compile again or install anything.

```powershell
& tools/prepare_revive_addon.ps1 -ActivationReceipt $workingStarterReceipt `
  -CompileReceipt $privateOwnerAssetReceipt -StarterConfigTemplate $pinnedStarterTemplate `
  -LegendaryExplorerPath $existingToolRoot -OutputDirectory $freshPrivateOutput
```

The tool first runs the existing activation verifier read-only, preserves its
Mount/TLK bytes and all eight authored config assets, and adds only the startup
package entries for `ME3SquadCoopActions`; the existing seekfree paths remain.
It copies the PCC unchanged and rebuilds the new directory's TOC using invariant
filename hashing, then checks the exact five-file payload, sizes and buckets.
The older standalone script-DLC candidate is not a substitute for this merged
starter bundle: its minimal config and zero-TLK Mount omit the working metadata.

The result remains private with `ContainsOwnerGameAssets=true` and
`DistributionAllowed=false`; public archive/installer allowlists are unchanged.
Startup loading of the script classes/animation, peer package agreement,
skeleton compatibility and actual revival still require owner-side evidence.

The current private payload is
`local/revive-addon/owner-r2/DLC_MOD_ME3SquadCoop`, with its receipt at
`local/revive-addon/owner-r2/preparation.json`. Its lineage is the fresh
`local/script-build/revive-assets-r2/compile-result.json` and
`local/revive-assets/owner-r4/asset-result.json` receipts. All recorded input
hashes still match, the 146712-byte PCC was copied unchanged, Mount/TLK bytes
were preserved and the exact five-file TOC/hash buckets were verified. The
merged config is 2126 bytes and root TOC is 316 bytes; no installation or game
execution occurred. The older owner-r1 receipt retains its historical preparer
hash from before a console-summary cast fix; that note does not apply to r2.
