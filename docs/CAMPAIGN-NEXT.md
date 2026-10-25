# Campaign co-op: current evidence and next integration

This remains a research prototype. The target is a playable shared LE3 campaign:
Shepard retains host story authority and the guest controls the existing Ashley.
Arena setup automation is a development aid, not the player startup experience.

## Retained unattended startup evidence

Offline review of the latest retained trial confirms the five normal setup
callbacks: party selection, queued party confirmation, weapon confirmation,
character commit and character close. Conversation/cinematic skip callbacks
were dispatched through normal game APIs. The owner also reported successful
automation. Dispatch alone does not establish that every scene was skipped.
These automatic skip policies belong only to the protected arena experiment;
ordinary campaign dialogue must remain under the host's control.

The first sampled mode0 occurred at about87 seconds, but the complete session
scene gate did not clear until about295 seconds. A native package-map refresh
was verified at294.766 seconds. Host readiness had already timed out at220
seconds, so no guest was started. Mode0 is insufficient evidence of ordinary
control: pause, cinematic and conversation gates remain authoritative.
An active radio conversation was observed during mode0; its causal role in the
entire delay is not established by the retained diagnostics.

The current480-second host and240-second guest budgets cannot accommodate
that serial startup:480 -294.766 -10 leaves175.234 seconds for the guest.
Increasing only the readiness deadline cannot fix the complete orchestration.

Original profiles were unchanged and the copied arena package was restored.
No game remained. One exception was observed in another image, with neither
the probe nor game image identified as its faulting module; the source module
and cause remain unknown. The memory helper exited1. This is not an all-pass
runtime audit. No new game run was performed for this offline review.

## Next implementation boundary

Prepare both independently owned processes concurrently. Each must complete
its own SPCer setup and exact Ashley/James roster load before joining. Keep
original locks across both jobs, role-specific profiles, exact executable and
process-creation verification, and bounded cleanup after partial failures.
Do not join merely because a mode0 sample or listener exists. Require the full
host control gate, successful native map refresh, and completed guest prewarm.
Reject an insufficient remaining lifetime explicitly before issuing travel.
Record each phase and the individual reason a full scene gate is blocked.

The existing sequential launcher must not be presented as supporting this flow
until its ownership registry and cleanup paths actually admit two launches
before readiness. The native package/GUID checks must remain unchanged.
The last exception needs attribution before a positive stability verdict.
Live validation is deferred at the owner's request.

## Requirements beyond the arena

| Requirement | Current evidence and remaining work |
| --- | --- |
| Ordinary player entry | Separate host/listen and guest/join paths exist; protected actual-entry guest control and matching adapters remain unverified. |
| Existing Ashley ownership | Short protected possession exists; sustained native identity, movement, collision and equipment continuity must hold through campaign transitions. |
| Shared combat | Earlier native guest hits reduced a real Trooper's health; complete encounter, kill attribution, powers, reload and reliable stop remain unverified. |
| Story and scenes | Host story authority is retained; genuine mode7 dialogue with guest detach, cameras and control restoration is still required. |
| Mission travel | Door/shuttle appearance and streaming transitions have unresolved evidence; validate both peers through a real mission boundary. |
| Guest team registration | Protected stock ChangeTeam candidate exists; do not promote it to ordinary entry without real control, cleanup and combat verification. |
| Saves and recovery | Original saves remain protected; ordinary campaign persistence, reconnect and recovery remain unverified. Arena saves are not a prerequisite. |
| Delivery | Cross-PC direct IP/Radmin and reversible player packaging come after gameplay continuity. |

An arena result cannot substitute for these campaign requirements. Promote
only demonstrated shared gameplay behavior into the ordinary player entry;
keep setup bots, forced scene skips and private test assets out of that path.

## Prepared parallel prewarm (not executed)

The protected probe now has a default-off `ME3_SQUAD_COOP_ARENA_PARALLEL_QA`
option. It requires the existing arena and automatic setup options plus a
32-character lowercase hexadecimal run identifier. With it enabled, a guest
that completes its own setup waits for a coordinator-owned Windows event.
The guest can only open that event for synchronization and polls without
blocking Tick. It cannot create or signal the event. Missing/unreleased events
leave it waiting; its native scene and roster checks still run before travel.

The private coordinator source now prepares two registered native launches
before waiting for readiness. The proposed host/guest limits are480/450
seconds, with120 seconds of common observation and10 seconds of cleanup
reserved before release. This changes orchestration budgets, not gameplay
success criteria or the ordinary player runtime. It binds readiness streams
to the retained child identities and requires recent full readiness, the
Ashley/James roster, and native host map refresh. Exceptions before release
refuse the join. A new random event identifier is created for each explicit run.

All three native outputs were rebuilt. The generated parallel launcher source
was assembled and syntax-compiled without executing its launch path. No game,
native fixture or integration test was run for this change. Deferred checks
include missing/wrong/stale event identity; withheld host readiness; child exit
between readiness and release; insufficient common lifetime; partial launch
cleanup; and actual native join after full same-map prewarm. Successful compile
and source inspection do not verify any of those behaviors.
