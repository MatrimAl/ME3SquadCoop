# Host player-order intent for guest powers

The retained owner run delivered two authenticated guest Inferno Grenade
requests through the stock ServerStartPowerCustomAction script. The host's
Ashley stayed at action/power 0/0 across both RPC boundaries. Nearby samples
showed player-ordered intent set on the guest power and clear on its host
counterpart. Those samples do not establish the synchronous pawn-start result.

The stock ChoosePowerTarget script sets m_bPlayerOrderedPowerUse before local
target selection. That field is transient and is not replicated. The stock
server-start script forwards the received target and location directly into
Pawn.StartPowerCustomAction, without ChoosePowerTarget. GrenadeBase.CanUsePower
rejects a henchman when this player-order intent is absent.

The host ingress now carries that intent into an already established, same
remote-player power context. Admission requires the actual received actor
channel, current host possession scope, reciprocal PC/pawn and power links,
the indexed native power instance, unique catalog membership, and the exact
reflected transient BoolProperty and bit mask. It cannot acquire a new control
lease. A zero-to-one transition owns only that bit; preexisting native intent
is preserved. Normal idle context retirement conditionally clears the owned
bit while preserving unrelated flags and foreign or disposed contexts.

The original server script still executes exactly once with unchanged
arguments. There is no new RPC, cast retry, target selection, forced action,
rank/cooldown change, permission-cache invalidation or input synthesis. The
intent bridge runs independently of the diagnostic quota. Existing held
contexts can carry requests during native reload/evade; native action and
cooldown code still decides whether the power starts.

The observer additionally records Pawn.StartPowerCustomAction's actual return,
resolved power class, henchman/grenade ancestry, order flag, stock permission
cache timestamps and cooldown fields. This distinguishes stock rejection from
a later presentation problem; it never invokes a permission query or a power.

Offline checks cover the current intent/retirement helpers (87 checks) and
the production ingress/observer forwarding (17 cases). Their native scope,
UObject discovery and permission execution are explicit fixture boundaries.
Real host acceptance, charge consumption, cooldown, damage and visible effects
remain owner gameplay checks. Successful transport or startup is not a cast.
