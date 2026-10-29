# Dialogue advice preparation

The guest must see the host's actual dialogue choices and may suggest a reply.
The host retains the final story choice. This feature is not connected to the
game UI yet.

The protocol represents the six actual visible rows, their native slot mapping,
display modes and localized UTF-16 text. Bounded fragments reconstruct one
complete catalog before publication. A digest and scope/catalog generations
bind advice to that exact entry and row. Disabled and navigation rows cannot
be submitted as reply advice. Parsing alone never establishes peer authority.

The native runtime harness covers fragment order, commit-before-fragments,
maximum text/wire sizes, split surrogate pairs, conflicting fragments, invalid
slots and stale/disabled advice. These are protocol fixtures, not dialogue or
network gameplay validation.

The pending runtime integration must capture the real host wheel after native
filtering, preserve its localized strings and modes, authenticate the current
owner-PC connection even during companion unpossession, and display the native
guest panel. Guest selection callbacks must produce advice without executing
QueueReply, SkipNode or interruption methods. Native panel acquisition,
visibility/input lifecycle, host advice presentation and real callback reachability
remain incomplete. This work does not replicate cinematic character animation.
