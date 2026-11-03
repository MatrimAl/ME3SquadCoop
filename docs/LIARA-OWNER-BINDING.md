# Selected Liara conversation owner

The later introductory Liara conversation authors its Owner variable as
FindByTag `hench_liara`. This fact applies to the
`promar_intro_liara_m_d` line family in the selected 205B package, not to every
conversation using an Owner selector. The separate `promar_level02_b_d` family
and the earlier 205A intro remain excluded.

When the natural native resolver has already returned null for a mode0 Owner
query, the authenticated scene adapter can request the existing local Liara
actor. This helper separately restricts the exact action path and scans the
current persistent and loaded level actor lists. It requires exactly one live
SFXPawn_Liara in the current WorldInfo with Tag or UniqueTag `hench_liara` and
FName number zero. A duplicate tag, unknown actor memory, unreadable list,
replaced identity or exceeded bound returns no actor. It never chooses the
nearest actor and never creates a conversation, actor or animation state.

Collection is bounded to 64 loaded levels plus the persistent level, a total
of 100,000 actor entries and a three-millisecond performance-counter budget.
It tries at most once per 250 ms. Learning the immutable native tag name may
resume between bounded attempts, but actor selection always requires a fresh
complete pass after that name is known. A short-lived cache rechecks current
world and list headers, exact actor identity, original list membership and
the actor's world and tag fields before reuse. It expires after 250 ms. Optional
read-only Tick prewarming can learn the name and fill this cache before the
first natural Owner query; it grants no action admission or later replay.

The caller remains responsible for actual native Matinee/action channel and
owner-connection provenance, the original null result, the exact Owner query
and the unchanged native validation after the returned actor. The helper only
reads existing game objects. Budget refusal can leave a natural query
unresolved; it does not authorize a delayed initialization or scene replay.
Correct body/face animation and voice playback still require scene testing.
