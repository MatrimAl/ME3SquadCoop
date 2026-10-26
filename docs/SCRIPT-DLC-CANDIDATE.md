# Offline authored-script DLC candidate

`tools/prepare_script_dlc.ps1` prepares a private candidate for the compiled
revive script package. It is separate from the asset-free activation container
in [DLC packaging](DLC-PACKAGING.md). The existing installer/release allowlist
has not been expanded for its new components; it is not a validated install.

The tool accepts a successful private compile receipt, an existing Legendary
Explorer directory and a fresh output below repository local/. It checks
current compile inputs, diagnostics and package digest before copying, then
rechecks tracked inputs/tool identity after generation. Reparse ancestors and
existing output directories are refused. Failed preparation directories stay
unusable even if some files were written.

```powershell
& tools/prepare_script_dlc.ps1 -CompileReceipt $privateCompileReceipt `
  -LegendaryExplorerPath $existingToolRoot -OutputDirectory $freshPrivateOutput
```

## Authored payload

| Path under DLC_MOD_ME3SquadCoop | Content |
| --- | --- |
| CookedPCConsole/ME3SquadCoopActions.pcc | Byte-identical copy of the successful five-class compilation |
| CookedPCConsole/Default_DLC_MOD_ME3SquadCoop.bin | Newly authored minimal BioEngine config compiled by existing LEC |
| CookedPCConsole/Mount.dlc | Reviewed108-byte LE3 mount layout, research priority5100 |
| PCConsoleTOC.bin | LEC CreateDLCTOCForDirectory output, read back through TOCBinFile |

The config adds this DLC's seekfreepcpaths and engine.startuppackages
dlcstartuppackage (type3) / dlcstartuppackagename (type0), using the compiled
package's unchanged filename stem. The installed LEMP config was read offline
for property names/types, and base Coalesced for the asset header. Input hashes
remained unchanged. No LEMP configuration payload/assets were copied.

Only those three properties are authored. Their presence does not prove
sufficient startup merging/loading. No whole game config, TLKs, copied GUIDs
or fabricated startup PCC is added. Package bytes are not renamed to a
Startup_* carrier. The retained official starter-kit source shows config
generation and an optional startup add-in; that add-in implementation was not
obtained in this step. Full generator equivalence and startup naming/
localization conventions are not established.

## Readback and limits

Preparation decompiles the newly compiled config and checks both startup
values/types. Generated TOC readback requires one correctly sized entry for
each PCC/Mount/config. The private receipt fingerprints all four files and
records installed/engine-accepted/runtime-invoked=false. The authored PCC copy
matches the successful compile receipt's digest.

This establishes generated file/config/TOC consistency. It does not show the
engine opens Mount, merges config, loads the PCC, resolves imports, retains
classes across travel or instantiates setup. No game install/deploy, package
loading in the game, fixture or behavioral tests ran. Existing tool assemblies
ran locally only to compile/read authored config and generate/read TOC. Native
ASI sources did not change.

Shared startup acceptance, per-peer dependencies, runtime setup ownership and
pre-handoff retirement remain unfinished. Revive animation assets are not
included. The prior corrupt-DLC report remains unexplained; this candidate
does not prove a TOC/config fix for it or alter the validated release gates.
