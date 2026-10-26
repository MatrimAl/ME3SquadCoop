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

Detailed evidence and limitations live in [ammunition replication](docs/AMMO-REPLICATION.md),
[runtime integration](docs/RUNTIME-INTEGRATION.md) and [validation](docs/VALIDATION.md).
