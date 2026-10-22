# ME3 Squad Co-op

Read README.md and docs/VALIDATION.md before changing claims. This is a two-player LE3 research prototype with more guests planned.

- Work on focused branches from dev/campaign-coop; do not modify main directly after the initial snapshot.
- Preserve original games and saves. Launch only a verified independent copy with private profiles, original-file read locks and bounded process cleanup.
- Never commit local/, game assets, saves, raw runtime logs, private paths or credentials.
- Keep host story authority and native package validation intact.
- Rebuild native binaries after native source changes. Report only tests actually run.
- Synthetic handoff tests are not real conversations; ammo consumption alone is not reliable combat.
- Do not publish issues or PRs or contact others unless the owner requested it.
