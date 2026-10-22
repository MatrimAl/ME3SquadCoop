# Development and safety

## Workspace

Use a full independent game copy, never hardlinks to the original. Keep private manifests, profiles, logs and test runtime under the ignored sibling `coop-anything/local/me3-le/` directory. `boot_probe.py` and `pair_probe.py` currently assume this layout. Their preconditions intentionally prevent a fresh clone from launching against an arbitrary installation.

The source-copy helper is available as `python src/safe_workspace.py --help`. Prepare and verify the copy and original-file inventory before using launch scripts. Original installation and profile files must be held under read-sharing locks; the test game's Documents route and write guards must point to its private profile. Do not bypass missing manifests or weaken the guards to get a test running.

Disable conflicting LEMP components only in the independent copy, preserving the original installation. Run the game and tools in the same normal user elevation context. Low-integrity game startup failed in earlier experiments and is not the supported boot method.

## Bounded experiments

After building and verifying the isolation prerequisites, the current paired research command is:

```powershell
python pair_probe.py --prewarm-mars --refresh-host-map --reload-host-mars --pair-control --session-test
```

Read the script's options and safeguards before running it. This is an automated bounded experiment, not a player-facing launcher. Log files and manifest data stay private. Verify process cleanup, original-profile integrity and probe disablement after each run.

## Contributions

Start from `dev/campaign-coop`, use a focused branch, and describe exactly which tests ran. Native modifications require rebuilding before launch. Keep host story references intact, restore AI on release/disconnect, and preserve package validation. More guests need explicit per-player identity and ownership tests; increasing a capacity constant is insufficient.

Do not add game assets, decompiled game dumps, executables, saves, private manifests, raw logs or credentials. The public repository contains authored mod source and synthetic tests only. Existing local helper scripts that patched hardcoded workspace paths are deliberately omitted.

The first public snapshot is a copy of the research source. The private original workspace is retained for provenance and existing test evidence. Do not silently switch a live test to the public copy without rebuilding and verifying its private workspace setup.
