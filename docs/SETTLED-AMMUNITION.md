# Settled guest ammunition

The protected real-fire route previously recorded a guest request at consumed
count 5, an authoritative host response at 4, and a guest correction to 4 with
reserve 210 unchanged. The ordinary runtime now has a separate adapter for this
settled disagreement. That observation does not explain or fix a visible
9-to10-to8 rebound during rapid fire. Native firing, hit messages, prediction
and native ammunition receive processing remain unchanged.

The guest requests a snapshot only after local ammunition activity and 250 ms
of unchanged idle counts. The host independently waits 250 ms for its actual
idle counts. Requests contain an ID and weapon channel, never client counts.
Both use the existing native owner-controller RPCs through the current
ReceivedBunch connection. `ME3AM1` is separate from the protected experiment's
legacy prefix and from scene, dialogue, revival and appearance messages.

The existing selected-weapon scope validates reciprocal controller, pawn,
inventory and weapon ownership, world, native connection/channel lifetimes and
the current possession epoch or registration token. Scope changes invalidate
both peers' pending state. The process sequence never resets and stops before
wrapping. Ordinary operation can continue beyond the protected experiment's
64-request collection limit; that experiment retains its original limit.

Native FireAmmunition cancels outstanding guest replies before its unchanged
original call. This operational cancellation works after the diagnostic record
budget is exhausted and when diagnostic fire capture is disabled. The supported
native slot/method must still be installed and validated; missing coverage
refuses the adapter. A cheap per-Tick activity sample also cancels a reply on
fire input, custom actions, powers, weapon busy flags, pending fire or weapon
selection, changed counts and failed reads. Full scope and scheduler work runs
at most 20 Hz. Paused scenes, menus, control loss and disconnect discard the scope.

Only the guest's normal Tick may apply a reply. It rechecks the same pending
request, local generation, weapon channel, five-second local freshness and
unchanged counts. The weapon must be in native Active state, with no fire,
reload, swap, action, power or pending inventory request. Exact reflected native
ammo fields are checked. The two counter cells use compare-before-write and
conditional rollback; host counters are never written. An already matching
reply needs no counter change. No HUD clamping or per-shot acknowledgement is
inferred. A new activity cancels an old reply even if counts later return to the
same value.

The feature defaults on in the ordinary candidate and can be disabled with
`ME3_SQUAD_COOP_SETTLED_AMMO=0`. Native refusal paths preserve ordinary behavior.
Private records retain request, receipt, host reply and application stages;
their event collection budget does not limit cancellation or operational use.

Local fixtures cover strict namespace separation, request/retry/stale-scope
state, count ABA cancellation, sequence exhaustion, compare-before-write and
the real native fire wrapper after its 64-record limit, including exactly-once
stock forwarding and LastError preservation. The ordinary adapter has not been
tested by a player. Required check: fire a short burst, release, wait a second,
then reload, swap and fire again. Confirm host/guest clip and reserve agreement,
that a new action never receives an old correction, and that the separate
rapid-fire rebound is still reported if present.
