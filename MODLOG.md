# Development journal

The native UE3 route uses the existing experimental ASI runtime and exact-build
guards. Original games, profiles and saves remain outside the editable lab.
Current work is offline; no game launch or installation is part of this step.

The current ammunition work separates scope identity, scheduling and incoming
native RPC provenance from protected trace tooling. The shared native scope now
captures transport object lifetimes. Next: a forwarding-only receive context in
the ordinary runtime, followed by authenticated reconciliation integration.
The scope predicate and scheduler alone do not establish playable co-op.

The forwarding-only ordinary receive context is now present under the existing
explicit ammunition opt-in. Equipment research then identified script slot223
as the ClientGivenTo activation target. The shared host equipment bridge now
checks that indexed target against its validated method identity and records
the selected Engine/empty-SFX path. Guest method execution/state override still
requires a dedicated observation; no forced activation was added. See the
indexed activation section of the inventory research notes for evidence limits.

The first protected activation-observer pair completed from a9ff7be. Guest
observer installed but captured no paired selected override entry. A bounded
21.047-second shared walking interval passed; epoch2 control lasted133.469s.
Nine reconciliation exchanges included one6-to5 correction;14 selected host
Trooper health decreases were observed. This does not close ordinary entry,
route-wide agreement, disconnect restoration or complete campaign playability.
Private corrected evidence revision2 retains all source hashes and explicitly
supersedes an initial summary whose equipment-row filter was incomplete.

The early opcode observer run from afe068c captured two ClientGivenTo-parent
named calls within the weapon's incoming native receive context, both Active
before/after. Session lifecycle including disconnect/AI restoration passed,
with188.688s of epoch2 control; walking failed the downward-velocity bound.
Do not force superclass activation based on the former no-entry observation.
Normal package acceptance and movement repeatability are higher-value next
work. Guest recorder shutdown exposed a closed-stdin race; terminal-state-aware
stop handling now preserves live failures without discarding exit metadata.

Detailed evidence and limitations live in [ammunition replication](docs/AMMO-REPLICATION.md),
[runtime integration](docs/RUNTIME-INTEGRATION.md) and [validation](docs/VALIDATION.md).
