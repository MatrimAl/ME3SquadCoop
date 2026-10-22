# Validation status

Status snapshot: 2026-10-22. Private raw logs and profiles are not published.

Observed: native two-process connection on one PC, short guest squadmate movement, guest shoulder-camera correction, actual intro Matinee camera motion and reciprocal-registration cleanup, local player-context registration/cleanup, synthetic scene release/reacquire, and ordinary disconnect restoring AI. Earlier weapon input caused authoritative ammo consumption.

Not established: real full conversation control return, reliable equipment/firing/stop-firing, full combat, sustained mission travel, save/load, cross-PC Radmin, more than one guest, or end-user Mod Manager installation/disable/rollback.

One earlier run kept consuming ammunition after release; another had differing client/server counts. The latest extended equipment audit generated no eligible firing pulses due to inventory/action readiness. These are unresolved observations, not completed combat QA.

Real dialogue requires actual engine conversation state while a guest lease exists, followed by measured detachment and successful control/camera return. A synthetic gate and moving intro cameras do not satisfy this requirement.

Original-file protection in the audited environment covered 8,194 game files plus 16 profile files (8,210 total). Recent completed audit runs recorded zero native exceptions and unchanged original profiles. These observations apply only to those runs, not all possible future sessions.

Synthetic evaluators and their fixture tests are shipped so verification logic is reviewable. Private game-run evidence is required separately; a passed unit test or build does not mean the campaign works.

## Public source snapshot checks

The prepared public source copy built all three native outputs with Zig 0.15.2. Python unittest discovery ran 152 tests: OK, 2 skipped (platform/privilege-dependent fixtures). The separate Windows native harness passed memory/opt-in guards and temporary low-integrity write-denial fixtures. No game was launched for these publication checks.
