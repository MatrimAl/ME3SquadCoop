# Guest incoming weapon-hit direction

The owned guest now supplements the native damage-direction queue from a real
received weapon-impact notification. Shepard's `PlayHit` queues a hostile
attacking pawn on the local player controller. Ashley inherits the generic
route; the retained weapon-impact notification supplies its animated reaction
without this player-only queue. The player controller's `NotifyTakeHit` override
is an empty non-native function, so that override does not provide the missing
queue.

The bridge accepts only `BioPawn.ReplicatedEvent(ReplicatedWeaponImpactInfo)`
inside the current owned Ashley actor-channel receive. It validates the exact
supported image, function ABI and reflected impact layout. It baselines the
current counter on admission, ignores equal counters, and resets on ownership,
world or channel changes. A counter change is a received-state marker; neither
the server impact time nor an exact number of hits is inferred.

The weapon and its actual instigating pawn are captured before forwarding the
notification. The original executor runs once with unchanged arguments and
preserved incoming/resulting LastError. The bridge then defers up to eight
distinct attackers to the game-thread tick. Entries expire after 500ms and
revalidate the same ownership, channel, weapon and attacker before and after the
stock `Pawn.IsHostile` query. Only a hostile source reaches the stock local
`BioPlayerController.QueueDamageIndicator` call. Native overrides with another
hostility implementation are left alone. Repeated pending impacts from one
attacker coalesce; separate attackers can retain separate entries.

No `PlayHit`, damage, reaction, shield effect, screen shake or framebuffer
effect is replayed. Health/shield deltas do not invent an attacker or direction.
The native queue and HUD remain responsible for presentation. A queued event
does not prove a rendered indicator. Native queue deduplication outside this
bridge has not been established. The bridge can be disabled with
`ME3_SQUAD_COOP_GUEST_HIT_FEEDBACK=0` independently of the reaction leases.

Native fixtures cover initial state, duplicates, counter wrapping, ownership/
channel changes, bounded multi-attacker pending entries, expiry including clock
wrap, and unchanged executor forwarding/LastError. They do not emulate the game
HUD or establish runtime network reachability.

Later owner checks:

- Take ordinary enemy gunfire from the left, right and behind. Compare the
  guest's direction indication with Shepard's native indication, including
  shielded hits. Check for doubled flashes or missing directions.
- Take fire while reloading, using a power and in cover. The normal hit,
  animation, shield feedback and camera must continue independently.
- Have two enemies fire from different sides; then enter a real scene or
  relinquish the guest pawn. Old indicators must not appear for a new owner.
- Correlate the bounded `guest_hit_feedback` native events with the read-only
  impact-counter and native damage-causer queue samples. These samples alone
  cannot certify what was rendered.

No game has run this addition. Full player/MP feedback parity remains unproven.
