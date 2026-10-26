# Private UnrealScript package compilation

`tools/compile_script_package.ps1` prepares compilation of owner-authored `.uc`
classes with an existing Legendary Explorer installation. It accepts an explicit
LE3 game root for dependency reading, a tool root, ordered source paths, a
package name and a fresh absolute output directory below this repository's
`local/`. Sources should be ordered before classes that depend on them.

The compiler is not invoked by the native build or a game launcher. This tool
does not install, mount, register or activate a package. Engine loading and
package/replication compatibility remain separate requirements.

```powershell
& tools/compile_script_package.ps1 -GamePath $privateGameRoot `
  -LegendaryExplorerPath $existingToolRoot -Sources $authoredClassPaths `
  -PackageName ME3SquadCoopActions -OutputDirectory $freshPrivateOutput
```

Use PowerShell7. The supplied tool installation must expose the inspected
LegendaryExplorerCore FileLib, UnrealScriptCompiler and PackageSaver APIs.
No tool or runtime is downloaded. The assembly resolver reads dependencies
from the explicitly supplied tool directory without changing PATH.

Output paths outside repository `local/`, existing output directories and
reparse-point ancestors are refused. A source set with compiler errors is
not passed to the final save. Only the fresh output package is supplied to
PackageSaver; no installed DLC or basegame package is a save target.
Treat every failed output directory as unusable, even if a library created an
initial package file while preparing it. Use a new directory after a failure.

The private JSON receipt separates compiler errors/warnings, package digest,
input-integrity failure and `compiled-not-installed`. It hashes the supplied
class sources, core tool assembly, Oodle library and Core/Engine/SFXGame
packages before and after work, and refuses final save when those inputs have
already changed. This is bounded input coverage, not an audit of every
transitively resolved package or bundled tool assembly. The receipt can contain
private paths and must stay outside Git.

## Current evidence

The required constructor/method/error-report signatures were read from the
already supplied local Legendary Explorer6.5 assembly. PowerShell source syntax
was parsed successfully. The tool has now initialized dependencies, compiled
the authored `SFXSquadCoop_ReviveContext` and `SFXSquadCoop_ReviveAction` classes
together without compiler errors/warnings and saved one private LE3 package.
Compilation supplies the source filename's
class name explicitly through `intendedClassName`; a new export cannot use an
empty intended name. The receipt confirmed unchanged tracked inputs.

No package reload, fixture, game, install or mount was executed. This establishes
two-class compilation/save, not universal tool behavior, engine acceptance or
a working full revive feature. See the
[revive context component](REVIVE-CONTEXT.md) and
[authored action candidate](REVIVE-ACTION.md).

The authored action candidate supplies context, duration polling, cancellation,
a scoped native timer pause, usage/lens-state accounting and a replicated-target
path. Animation availability, actual timer/selection/recovery cleanup, client
presentation and end-to-end native action behavior remain
unfinished as identified in [interaction research](GUEST-INTERACTION-RESEARCH.md).
Compilation alone will not establish those behaviors. A shared package can
avoid per-mission edits only once class availability and scoped assignment on
both peers are implemented and verified.
