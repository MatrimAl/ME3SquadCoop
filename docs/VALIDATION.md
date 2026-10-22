# Validation status

Status snapshot: 2026-10-22. Private raw logs and profiles are not published.

Latest iteration: see [PROGRESS.md](PROGRESS.md). Two public-source paired runs did not acquire the squadmate; two later runs exercised synthetic lifecycle handoffs. One produced only a client-predicted shot; the latest observed host and guest firing/stop behavior but differing ammunition counts. Startup results are not yet repeatable. The world-floor guard build was rerun, but Ashley was already nearby and no catch-up call occurred; that specific fix remains unverified in-game. The possession/lifecycle results below are bounded observations, not a campaign guarantee.

Observed: native two-process connection on one PC, short guest squadmate movement, guest shoulder-camera correction, actual intro Matinee camera motion and reciprocal-registration cleanup, local player-context registration/cleanup, synthetic scene release/reacquire, and ordinary disconnect restoring AI. Earlier weapon input caused authoritative ammo consumption.

Not established: real full conversation control return, reliable equipment/firing/stop-firing, full combat, sustained mission travel, save/load, cross-PC Radmin, more than one guest, or end-user Mod Manager installation/disable/rollback.

One earlier run kept consuming ammunition after release; another equipment audit generated no eligible firing pulses due to inventory/action readiness. The latest run observed two host/guest firing pulses with consumption stopping after release, but consumed-round counts disagreed (4/6 and 8/9). These mixed results do not establish reliable combat or synchronized equipment.

Connection reports distinguish a sustained interval observed earlier from client mode still active at the end. Intentional disconnect can leave `sustained_connection_observed` true and `connection_states_verified` false. Historical observation alone does not prove clean disconnection; the separate lifecycle evaluator must establish that.

Real dialogue requires actual engine conversation state while a guest lease exists, followed by measured detachment and successful control/camera return. A synthetic gate and moving intro cameras do not satisfy this requirement.

Original-file protection in the audited environment covered 8,194 game files plus 16 profile files (8,210 total). Recent completed audit runs recorded zero native exceptions and unchanged original profiles. These observations apply only to those runs, not all possible future sessions.

Synthetic evaluators and their fixture tests are shipped so verification logic is reviewable. Private game-run evidence is required separately; a passed unit test or build does not mean the campaign works.

## Public source snapshot checks

The prepared public source copy built all three native outputs with Zig 0.15.2. Python unittest discovery ran 152 tests: OK, 2 skipped (platform/privilege-dependent fixtures). The separate Windows native harness passed memory/opt-in guards and temporary low-integrity write-denial fixtures. No game was launched for these publication checks.
