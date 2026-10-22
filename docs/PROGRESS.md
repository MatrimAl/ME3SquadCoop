# Progress

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
