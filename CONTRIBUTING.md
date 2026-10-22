# Contributing to ME3 Squad Co-op

Welcome! Code, careful testing, research, documentation and translations are useful contributions. Please use English in public reports. This is a research prototype: read the README and docs/VALIDATION.md before choosing a task.

## Start here

1. Search existing issues and open PRs. For substantial work, open a research/task issue first and describe your plan. Say you are working on it in that issue to avoid duplication; this is coordination, not exclusive ownership.
2. Good areas to help: equipment readiness, stop-firing reliability, real dialogue lifecycle, mission transitions, isolated testing, packaging, and later more guests. Small documentation improvements need no prior assignment.
3. Fork the repository. Start a focused branch from `dev/campaign-coop` in your fork. Do not commit directly to `main`.
4. Keep the change small enough to review. Add meaningful regression tests for behavior changes and update evidence limits when results change.
5. Open a draft pull request targeting **dev/campaign-coop**, fill the PR template and link the issue. Select Ready for review when finished.

Example (replace YOUR-USERNAME):

```powershell
git clone https://github.com/YOUR-USERNAME/ME3SquadCoop.git
cd ME3SquadCoop
git remote add upstream https://github.com/MatrimAl/ME3SquadCoop.git
git fetch upstream
git switch -c fix/short-description upstream/dev/campaign-coop
# Edit, test, then commit your files.
git add path/to/changed-file
git commit -m "Describe the concrete change"
git push -u origin fix/short-description
```

On GitHub choose Compare & pull request, set base repository to MatrimAl/ME3SquadCoop and base branch to dev/campaign-coop. Forks let you propose changes without repository write access.

## Build and test

Use Python 3.10+ and Zig 0.15.2; see README.md for pinned build identity and commands. No game is needed for these checks:

```powershell
python build.py C:\Tools\zig-0.15.2\zig.exe
python -m unittest discover -s tests -p "test_*.py" -v
python tests/native_probe_harness.py
```

The native harness needs Windows and a completed build. The archive integration test also needs 7-Zip. Report skipped tests and why. GitHub CI runs Python fixtures and a Windows native build; it does not run the game or prove campaign compatibility. First-time external workflows may await maintainer approval.

## In-game evidence

Use only a verified independent game copy with isolated profiles and original-file read locks, as documented in docs/DEVELOPMENT.md. Never test on your only save or modify the original installation. Match the exact executable build before using native offsets.

For every behavioral claim, record the build, scenario, input, expected behavior, observed behavior, duration, cleanup and integrity result. Distinguish real game sessions, synthetic fixtures, derived observations and hypotheses. A synthetic scene gate is not a real dialogue test; ammo consumption alone is not complete combat QA. More than one guest is planned, not verified.

Share a short sanitized summary. Keep full logs, dumps, saves, private paths, VPN addresses and credentials out of issues and PRs. Do not upload game files or third-party mod assets. Do not disable native package validation.

## Review and merge

PRs need passing required checks, resolved conversations and a maintainer review. CODEOWNERS requests @MatrimAl for review. An outside contributor's PR needs one approval; new commits dismiss old approvals. Only maintainers merge. Fix review comments on the same branch; do not close and recreate the PR.

The maintainer promotes reviewed development snapshots to main. The sole owner cannot approve their own PR: owner-authored changes may use the administrator exception after review and passing checks. This exception is not an invitation to skip review for external contributions. There is no automatic merge, approval bot or automatic deployment.

## AI-assisted contributions

AI assistance is welcome and used extensively in this project. State what assistance was used in the PR, read and understand the resulting change, verify sources and licenses, and run the relevant checks. Do not submit fabricated test results or large unexplained generated patches. The contributor remains responsible for the submission.

## License and conduct

Project-owned code is MIT licensed. By submitting a contribution you agree that your contribution is available under the same MIT license and that you have the right to submit it. Preserve any third-party license notices; game assets are not covered by this license.

Be respectful, describe technical disagreements concretely and avoid personal attacks. Public issues are for project work. For a sensitive security report use SECURITY.md rather than posting exploit details publicly.
