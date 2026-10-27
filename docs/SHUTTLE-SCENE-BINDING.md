# Guest shuttle presentation

The owner supplied paired host/guest images on 2026-10-27. The host shuttle
has a complete hull while the guest shows exposed interior and differently
posed/missing body and door pieces. The owner also reports missing guest
shuttle animation. This is separate from the corrected remote Shepard helmet
and existing-weapon initialization. The images do not establish damaged assets
or which mesh physically blocks a player.

Retained same-action evidence already identifies asymmetric scene bindings.
For the opening `SeqAct_Interp_2`, host Shuttle group 3 binds
`SFXStuntActor_0` and ShuttleDoor group 7 binds `SFXStuntActor_4`. The matched
guest groups have null GroupActor despite a later live door object-variable
value. Camera groups bind on both peers. These are matched action/director
observations, not unrelated objects selected by name alone.

The camera registration adapter operates only on already-bound camera groups;
broadening its class filter cannot fill these null stunt-actor bindings.
The exact native group initializer assigns GroupActor only while its track
instance array is empty. Calling it again on an initialized null group would
skip that assignment and repeat downstream callbacks. Replaying all InitInterp,
copying host pointers/transforms or directly forcing GroupActor is not a
supported repair.

The next narrow boundary is native actor admission before the original first
group initialization. Existing selected native exports and the early paired
shuttle observer are retained privately. The observer records group initializer
vtable targets, variable links, track state, mesh/pose identities and collision
separately. The missing binding can explain the visual difference, but that
causal link and a safe repair still require evidence. No shuttle fix is claimed.

## Client-level initialization candidate

The early paired capture from `ef11491` found no living shuttle/body/door
counterparts after guest join. Before join, the guest's local map had live
stunts and normal groups. After world replacement, its object-variable targets
were deleted/pending-delete and absent from the current actor array. Normal
TermGroupInst/InitGroupInst cannot repair an absent actor. The selected stunt
script startup does not destroy them, and Actor.PreBeginPlay's relevance
destruction excludes clients.

Selected exact-build native analysis identifies the preceding removal.
Level actor initialization at RVA 0x592e40 tests native world authority; a
server connection means client. Before script initialization, nonstatic actors
without bNoDelete are passed to World.DestroyActor at the single callsite
returning to 0x592ef3. The authored shuttle stunts take this branch. Normal
execution continues with the actor's own initializer only if it is not deleted.
Three selected decompilations reused the retained database without changing it.

The ordinary guest candidate intercepts only this exact prune call for an
existing live SFXStuntActor in the current client world's loaded map level and
actor array. Current world, connection, archetype/class and object identities
are rechecked. Already-deleted, pending-delete, script-initialized actors and
all other DestroyActor callers/arguments/actor classes forward unchanged.
The skipped request returns false, a result the selected caller does not use;
the normal level loop continues initialization. No actor is created or revived,
and no actor/scene/cache/role/transform field is authored. This requires live
validation of retained actors, actual group binding, pose and teardown before
the reported shuttle defect can be called fixed.

The protected run from `3ec584a` exercised this boundary. Native logs preserved
exactly the authored shuttle body and two doors at the selected callsite.
Independent post-join actor-array observations retained all three alive,
with script initialization complete, valid body meshes and instantiated trees.
The sampled body's local-bone pose hash matched the host. One door's local
pose hash still differed; the opening action did not run during the connected
interval, so full door animation/late-join pose agreement is not established.
The window frame was inspected but the camera remained inside the shuttle;
it does not independently prove the complete outside hull appearance.

This run also retained normal remote weapon/helmet initialization. Ashley
stayed at the landing and no scene-return repair was exercised. Thus the
latest collision predicate/role correction remains a compiled, fixture-checked
candidate, not a preventive gameplay result. Runner and six observers exited
0; the original profile and TOC were preserved, 8,210 locks held through cleanup,
and there were no native exceptions, forced cleanup, cleanup errors or games
left running. Next validation must exercise the normal opening/closing scene
and Ashley's control return; repeating a stationary startup cannot prove it.
