# Progress

## 2026-10-22: world-floor catch-up guard and firing audit

- A later bounded pair exercised two ownership epochs, synthetic scene release/reacquisition and native disconnect/AI restoration. This does not establish real dialogue or repeatable campaign progression.
- Its 250 ms guest input consumed one predicted round locally, while the authoritative host count stayed zero. Firing/stop-firing verification now requires observed consumption on both sides; stable zero counts cannot pass. Client-only prediction remains an unresolved combat failure.
- A read-only live capture observed a player floor normal of `[0, 0, 1]` with a null supporting actor (`Base`). Catch-up no longer requires a supporting actor: it validates finite health, velocity and floor data, while preserving walking, world, squad and ownership checks and the game's normal navigation/catch-up routine. No position is directly written.
- The guard change has native fixture coverage for ordinary world-floor readiness and invalid, steep, falling, non-finite and dead-player cases. An in-game rerun of this specific guard change is still required; it is not a confirmed fix for every stranded-companion case.
- The bounded pair recorded zero native exceptions, unchanged original profiles, no forced orphan cleanup and a disabled probe after shutdown. Its generic persistent-connection summary was false after the intentional disconnect despite the separate lifecycle evidence passing; this reporting discrepancy needs investigation.

Next: repeat the catch-up scenario, investigate the absent authoritative shot and inventory restoration after reacquisition, then exercise actual dialogue, cross-PC Radmin and installation/rollback gates. No gameplay release is newly claimed.

## 2026-10-22: public-source runtime checks and acquisition diagnosis

The public checkout now uses the renamed Windows native harness correctly; the old launcher path prevented protected boots from this checkout.

### Changes

- Replaced the ineffective ProcessEvent query of intrinsic GetStateName with an exact-build, read-only state-frame reader. Native code bytes, live state objects and stable frame/node identities are checked; unsupported or unreadable states remain unknown. No state is changed by this reader.
- Added bounded acquisition-wait reasons and firing-method resolution diagnostics without relaxing possession, story, distance, equipment or bytecode guards.
- Included firing/stop-firing and real-dialogue evidence in paired-run reports. Dialogue verification now also requires observed ownership before the scene; a release/reacquire label alone cannot pass it.

### Actual runtime results

- A protected 35-second boot from the public source checkout passed. The original profile remained unchanged and the probe was disabled after the run.
- Two subsequent isolated paired runs connected through native networking, but neither acquired the guest squadmate. They did not exercise firing or real dialogue and are not successful session lifecycle tests.
- In the diagnostic run, acquisition reported `companion-out-of-range` 35 times. The host and squadmate became separated during the Mars intro transition. The native state reader observed `PlayerWalking` and AI `Idle`, rather than the previous misleading `None` result.
- The diagnostic run recorded zero native exceptions, unchanged original profiles, no forced orphan cleanup and a disabled probe after shutdown.

Earlier bounded possession/lifecycle observations remain documented in VALIDATION.md. The latest failure shows those results are not yet repeatable for this startup route. No campaign, combat, cross-PC Radmin or release-package milestone is newly claimed.

### Next investigation

Inspect the normal squad-follow/catch-up path and its actual ground/readiness conditions at the intro transition, then repeat ownership and equipment tests. Do not remove the distance guard or directly relocate actors to turn this failure into a pass. Reliable acquisition is required before the weapon and real-dialogue gates can be exercised.
