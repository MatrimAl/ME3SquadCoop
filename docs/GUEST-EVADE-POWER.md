# Preserve native power input during a player evade

The guest command catalog previously discarded numbered power input whenever
the pawn had a current action. Reload was admitted separately, but the four
owned PlayerEvade actions still invalidated that input before the game could
apply its normal blocking-action and queue rules.

The existing typed-action input route now also accepts the exact four player
evade classes in slots 54–57, with matching native class/instance tables,
reciprocal pawn ownership and the action's current PC cache. Its AI cache must
be empty. One movement-ignore count is allowed only when this verified evade
itself disables movement; other input-ignore states remain rejected. No
movement-ignore field is cleared.

The input reaches the existing `GameModeDefault.UsePower` once. The retained
stock script decides whether the current blocking action permits queuing,
then uses its ordinary tactical order/application path. The adapter does not
end an evade, start a power directly, add a timer, replay input or bypass
cooldowns. Idle powers and the existing reload route retain their behavior.

The ordinary runtime fixture uses the production dispatcher and checks all
four class/slot pairs, foreign classes, controller/AI mismatches and allowed
versus excessive input-ignore state. These checks do not prove a real queued
cast or its timing. At the next test of this candidate, press one available
power key during each evade direction: the evade should finish normally and
the power should follow the stock queue/refusal decision once. Compare the
host effect and camera as well. This change is separate from rendered
cinematic parity.
