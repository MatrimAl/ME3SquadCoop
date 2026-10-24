# Validation status

The latest protected pair **failed gameplay continuity** despite completing
three ownership epochs and native disconnect/AI return. After the synthetic
scene release, the guest Ashley jumped roughly54,107 units toward the world
origin. After reacquisition the host Ashley also jumped roughly54,025 units,
lost its world base and fell. Ordering is observed; the underlying engine or
packet cause remains unproven. No collision repair ran.

An earlier mode-0 landing exchange returned control without the focus-owned
pause. This does not validate full mode-7 dialogue or safe position continuity
across every scene. See [background focus](BACKGROUND-FOCUS.md) and the latest
[progress evidence](PROGRESS.md).

Ordinary key delivery, reliable walking and combat remain unverified. The
read-only input capture is asynchronous, not a true post-Tick observation.
The attempted normal W key occurred after the position failure and does not
explain its onset.

Original protection covered8,210 files, profiles were unchanged, native
exceptions were zero and no game remained. The memory helper exited1 without
a final summary, so this is not an unqualified passing observer audit.

Status reviewed: 2026-10-24; the latest gameplay evidence below is from 2026-10-23.
Private raw logs and profiles are not published.

A subsequent protected pair also failed continuity after synthetic release.
The guest changed from walking with zero velocity to physics mode10, lost its
Ashley channel association, and developed horizontal velocity above100,000
units/s while the host remained walking. The guest actor flags changed from
0x32b22910 to0x36326910, including the reference SDK's tear-off bit0x04000000.
This is an observed transition, not proof of its initiating cause.
The exact executable's ReceivedBunch tear-off branch checks actor+0x25c bit26,
sets local role3 / remote role0 and clears channel Actor+a0 after its event.
The previous receive observer required Actor+a0 after forwarding and could
therefore miss that very branch. The opt-in observer now retains the validated
pre-call actor identity when the slot becomes null, recording velocity,
physics and actor flags as well as position. Replacement identities remain
rejected. Authored-memory wrapper checks cover this diagnostic behavior; no
new runtime result or gameplay fix is claimed from those checks.


Guest powers remain unverified. Keyboard and gamepad power inputs use different
entry points in retained bindings; see [power input research](POWER-INPUT-RESEARCH.md).

Latest ordinary-entry host check: the separate runtime and guards-only probe
loaded together in the protected copy. The ordinary entry opened a real native
listener in the current Mars world without mission reload. Its startup status
was 8; an independent read-only current-world observation measured net mode 2,
and Windows reported the game process owning UDP port 17779 on the requested
local Radmin interface. No guest was launched. This proves the bounded ordinary
host startup/local bind, not two-PC Radmin, continuous control or campaign play.
All 8,210 original game/profile locks remained active, profiles were unchanged,
the temporary authored ASI/Mount were removed, the probe ended disabled and no
game process remained. A later PlayersOnly observation prevents a continuous
control claim. Integrated offline checks ran 257 Python tests (2 skipped),
ordinary native and collision lifecycle fixtures, and three compiled startup
contract negative controls. Positive live collision repair remains unverified.

2026-10-23 completed route update: ordinary input cleared the shuttle exit and
reached the first road node. A real mode-0 landing conversation interrupted
control for about 13.39 seconds; the route's two-second context allowance expired.
Release/reacquisition and disconnect ownership checks passed, but the guest's
collision remained disabled after control return and Ashley later fell on both
processes. Shared travel and firing/stop failed their separate checks. No full
mode-7 conversation or hostile damage was verified. Original locks covered
8,210 files, profiles were unchanged, recorded native exceptions were zero,
no forced orphan cleanup occurred, the probe was disabled and no game remained.
This completed experiment does not establish usable campaign control.

Latest observer-off repeat: the unchanged native build failed authoritative firing, settling at host/guest consumed counts 0/3 with reserves 210/210. The host executed the mode-2 start-fire script, but remained `Active` with pending mode 2 while `bPlayingPortArmsAnim` stayed set after `bInPortArms` cleared. The exact state getter confirms this defers firing. The second epoch had no eligible guest equipment chain for another pulse. Synthetic lifecycle and original-file protections passed, with unchanged profiles, zero native exceptions, no forced orphan cleanup and no remaining game processes. This rules out a repeatability claim for the successful run below; mesh/node ticking and natural combat still require verification.

Latest 2026-10-23 observation: with snapshot correction disabled, a protected pair recorded six native `FireAmmunition` enter/leave pairs on the host and six on the guest. Each original call advanced consumed count by one. Two 250 ms input pulses settled at consumed counts 3/3 and 6/6, reserves 210/210, with stable post-release counts. Incoming host start-fire calls requested mode 2, advanced the script cursor and entered `WeaponFiring`. Synthetic lifecycle checks and original-file/profile protections passed; no native exceptions, forced orphan cleanup or remaining game processes were recorded. This proves native firing and consumption in that bounded run. It does not establish repeatability or enemy damage. The preceding failed firing observation below remains relevant; no intentional firing behavior fix separates the two runs.

2026-10-23 update: a protected pair with the snapshot correction disabled observed two start-fire and two stop-fire native RPC entries on the authoritative host for validated guest weapon channel 21. Original forwarding returned with valid scope in all four cases, while the weapon remained `Active` and host consumed count stayed 0; guest counts reached 3 and 6. Commands therefore arrived at the host, but dispatch entry is not proof of complete script execution, actual native firing or enemy damage. Internal VM calls may bypass the observed entries. All three native outputs built; the harness, three compiled forwarding/channel negative controls and 197 Python tests (2 skipped) passed. Synthetic lifecycle, original-file/profile protection and clean bounded shutdown passed. Full combat remains unverified.

Latest iteration: see [PROGRESS.md](PROGRESS.md). Protected paired runs have both failed before acquisition and exercised synthetic lifecycle handoffs. Weapon results varied between client-only prediction, differing counts and matching consumed/reserve counts. Native field observations distinguish encoding/decoding from ordinary samples. A subsequent default-off adapter completed two owner-controller RPC exchanges and corrected guest consumed count 1 to authoritative 0, with reserve count 210 unchanged. The host consumed no ammunition during either pulse; this verifies counter correction in that run while authoritative guest firing failed. It does not establish reliable combat, repeated reconciliation under combat load or cross-PC behavior. Startup and weapon results are not yet repeatable. The world-floor guard build was rerun, but Ashley was already nearby and no catch-up call occurred; that specific fix remains unverified in-game. The possession/lifecycle results below are bounded observations, not a campaign guarantee.

Observed: native two-process connection on one PC, short guest squadmate movement, guest shoulder-camera correction, actual intro Matinee camera motion and reciprocal-registration cleanup, local player-context registration/cleanup, synthetic scene release/reacquire, and ordinary disconnect restoring AI. Earlier weapon input caused authoritative ammo consumption.

Not established: real full conversation control return, reliable equipment/firing/stop-firing, full combat, sustained mission travel, save/load, cross-PC Radmin, more than one guest, or end-user Mod Manager installation/disable/rollback.

One earlier run kept consuming ammunition after release; another equipment audit generated no eligible firing pulses due to inventory/action readiness. Later counts varied between disagreement and agreement. Profiling identified costly function discovery and cold channel lookup. A validated function cache and current-world channel enumeration reduced those measured costs. The preceding channel-lookup pair matched pawn/weapon channels and passed two ownership epochs, synthetic release/reacquisition and native disconnect restoration. Recorded input intervals were 250 and 312 ms; both sides consumed ammunition and stopped. Counts matched after the first pulse (3/3), then differed after the second (7/8), with reserves 210/210. Channel-resolution maxima were about 24 ms on host and 27 ms on guest, compared with 318/668 ms in the prior paired run. These are CPU-path wall-clock observations with different call counts and scene timing, not a controlled FPS benchmark. The remaining peaks and ammunition disagreement are not solved combat. See [lookup measurements](LOOKUP-PERFORMANCE.md) for scope limits. The evaluator rejects equal consumed counts with differing reserves; the offline condition recognizer and sampled trace described in [AMMO-REPLICATION.md](AMMO-REPLICATION.md) do not prove live packet delivery.

Connection reports distinguish a sustained interval observed earlier from client mode still active at the end. Intentional disconnect can leave `sustained_connection_observed` true and `connection_states_verified` false. Historical observation alone does not prove clean disconnection; the separate lifecycle evaluator must establish that.

The preceding native-field run recorded six host encodes and six guest decodes over five selected-weapon receives, without exhausting the observation budget. Six decoded values matched final storage; one receive changed the guest consumed count from 4 to 3. The first pulse ended at host/guest 2/3 after another local increment followed the last observed decode; the second ended at 5/5. Both 250 ms pulses stopped, reserves were 210/210, and synthetic lifecycle checks passed. This demonstrates a live correction path in that run, not reliable end-of-fire reconciliation or one-to-one packet matching. Full combat remains unverified.

The new reconciliation run completed two four-stage request/response/application exchanges, with guest-local roundtrips of 281 and 375 ms. Two ownership epochs, synthetic scene release/reacquisition and native disconnect/AI restoration passed. Original-file protection covered 8,210 game/profile files; original profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred and the probe was disabled. No game processes remained. All three native outputs built, the Windows harness passed, and three compiled identity/idle-policy mutants were rejected. Python discovery passed 197 tests, 2 skipped. These checks apply to the research adapter and its bounded run; the zero authoritative consumption expressly fails the separate firing requirement.

Real dialogue requires actual engine conversation state while a guest lease exists, followed by measured detachment and successful control/camera return. A synthetic gate and moving intro cameras do not satisfy this requirement.

Original-file protection in the audited environment covered 8,194 game files plus 16 profile files (8,210 total). Recent completed audit runs recorded zero native exceptions and unchanged original profiles. These observations apply only to those runs, not all possible future sessions.

Synthetic evaluators and their fixture tests are shipped so verification logic is reviewable. Private game-run evidence is required separately; a passed unit test or build does not mean the campaign works.

## Public source snapshot checks

The prepared public source copy built all three native outputs with Zig 0.15.2. Python unittest discovery ran 152 tests: OK, 2 skipped (platform/privilege-dependent fixtures). The separate Windows native harness passed memory/opt-in guards and temporary low-integrity write-denial fixtures. No game was launched for these publication checks.
