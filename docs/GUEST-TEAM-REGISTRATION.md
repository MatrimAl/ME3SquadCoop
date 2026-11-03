# Guest combat team registration

The ordinary host now registers an unassigned guest PlayerReplicationInfo in
Shepard's existing team through the stock `SFXGame.ChangeTeam` function. It
does not create teams, change damage values or override hostility decisions.
`ME3_SQUAD_COOP_GUEST_TEAM=0` disables this adapter.

The retained native `PlayerController.GetTeamNum` implementation returns 255
when `PlayerReplicationInfo.Team` is null. The measured `BioPawn.IsHostile`
implementation rejects that unassigned team in either direction, and the
damage path can reject an attacker with a PRI that is not hostile. This
initialization was previously present only in the protected combat experiment;
the ordinary player runtime omitted it.

The host requires the current possession lease, reciprocal remote controller
and pawn, actual native connection, clear scene state, live PRI, validated
existing team table and the same host team/index. It checks the stock function's
20-byte parameter layout and ProcessEvent identity. A non-null existing guest
team is preserved. Dispatch is bounded to one attempt for the captured missing
PRI/team state, and the result and assigned team are checked afterward.

The team registration belongs to the connected player, so normal scene releases
and reacquisitions retain it. Disable, disconnect or a changed world/Game/PRI
retires the registration. Cleanup uses stock `ChangeTeam(...,255,false)` only
for the adapter's own still-valid assignment in the captured native team table.
Native replacements are preserved. Invalid/disposed objects are never repaired
by raw pointer writes.

Native fixtures exercise registration and cleanup policy, exact stock call
arguments, return handling and rejected indices. The owner's preceding test
reported that guest damage failed in both directions. A partial read-only
snapshot retained valid Ashley actor channels and damage/projectile target
flags; it did not establish the current PRI/team state before the games closed.
The retained native contract supports this fix, but it does not prove that it
resolves every combat problem.

At the next protected owner test, confirm a `runtime_guest_team` registration
row on the host, then verify the native guest PRI/team on both peers. Shoot an
enemy as the guest and confirm authoritative health/shield loss; let an enemy
hit the guest and confirm both peers' Ashley damage. Repeat after an ambient
line, a full scene and reconnect. Ammo consumption alone is not damage proof.
