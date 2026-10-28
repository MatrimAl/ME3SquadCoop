# Mars entrance airlock primary player

The owner reported Shepard being pushed back after the first two enemy phases
in the protected `e915a55` test. The retained region corresponds to the Mars
entrance airlock in `BioD_ProMar_125Elevator`. Its authored transition moves a
TargetPoint to the value of `Base_Entrance_Airlock.SeqVar_Player_2`, then starts
the cinematic toggle and the Interp action labelled "seamless transition".

The stock `SeqVar_Player.GetObjectValue` rebuilds its player list and returns
the pawn of `Players[0]` when `bAllPlayers` is enabled. That flag is the native
default; this authored variable also enables `bReturnPawns`. Native controller
enumeration does not express "campaign Shepard". A second human controller
can therefore make list order a poor source of protagonist identity. The old
recording did not capture this variable's lists, so this ordering issue has not
been proven to be the cause of the reported pushback.

The adapter guards only this exact loaded airlock variable on the host. It
forwards the original native list builder once and then checks fresh, paired
two-entry Players/PlayersPawns lists against the actual local Shepard and the
single native connected peer. A temporarily unpossessed peer may have a null
pawn; an unrelated controller, extra player, changed world or inconsistent
pair is refused. The variable must retain its authored all-player/pawn flags
and zero PlayerIdx, with validated reflected array element types and sizes.

If Shepard is already first, the lists are untouched. If the verified guest is
first and Shepard second, only the four pointers in these two private scratch
arrays are swapped. Failed partial updates restore only still-owned cells and
preserve a foreign replacement. No graph link, plot value, PlayerIdx, selection
flag, actor transform or teleport action is changed. Native timing and the
authored transition continue normally.

The native builder hook covers both script and direct native callers. Exact
wrapper/body signatures and native/function metadata are checked. The owned
patch, unchanged code tail and complete trampoline/relay slot are revalidated;
foreign changes disable adaptation. Other variable names/classes forward
directly without metadata discovery. The guard can be disabled independently
with `ME3_SQUAD_COOP_AIRLOCK_PLAYER=0`.

Native fixtures cover the two valid orders, unrelated/mismatched pairs, a null
peer pawn, partial-update rollback, unchanged forwarding/LastError and owned
versus foreign code/trampoline guards. They do not prove a real airlock entry,
native list order, actor binding, scene completion or campaign progress.

Later owner check: after the first two enemy phases, approach the entrance as
Shepard with Ashley first behind and then nearby. Compare both views for any
pushback, anchor shift or failure to enter. The native events distinguish
`native-primary-player-retained` from `primary-player-order-corrected`; neither
alone proves successful progression. The V19 recorder also retains separate
Shepard position, velocity and active-action samples, including periods without
reciprocal guest possession. A sampled position jump alone does not establish
its cause. No game has run this new guard, and the reported rollback remains
unverified as fixed.
