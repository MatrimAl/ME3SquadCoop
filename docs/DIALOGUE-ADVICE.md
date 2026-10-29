# Guest dialogue advice

The guest must see the host's actual dialogue choices and may suggest a reply.
The host retains the final story choice. The candidate mirrors the existing
native conversation wheel; actual rendering and input still need gameplay
validation.

The protocol represents the six actual visible rows, their native slot mapping,
display modes and localized UTF-16 text. Bounded fragments reconstruct one
complete catalog before publication. A digest and scope/catalog generations
bind advice to that exact entry and row. Disabled and navigation rows cannot
be submitted as reply advice. Parsing alone never establishes peer authority.

The native runtime harness covers fragment order, commit-before-fragments,
maximum text/wire sizes, split surrogate pairs, conflicting fragments, invalid
slots and stale/disabled advice. These are protocol fixtures, not dialogue or
network gameplay validation.

At the stock AS_SetConversationOptions boundary, the host capture uses the real
six localized strings, modes and already computed native slot mapping. Native
conversation identity, resource, entry and content changes create a new catalog.
The existing owner-PC RPCs authenticate the native actor channel, connection,
world and controller even while companion possession is released. The guest
publishes only complete catalogs during fresh host scene viewing. Reliable
close messages and scene/scope expiry retire the panel.

Only the guest's existing registered PC conversation panel is used. Native
visibility, enabled state, input, focus and key handling are borrowed, with
conditional restoration of still-owned state. A real local conversation or
native options replacement takes precedence. No startup, game-mode activation,
local conversation creation or input injection is used. The native display-only
pointer update runs after ordinary movie update when that update has no local
full conversation to drive it. The first update of a new catalog uses stock's
force-refresh argument; subsequent updates consume normal mouse axes.

Real mouse-left/A GUI events may confirm the existing widget. The resulting
guest selection, skip and interruption callbacks return through validated stock
return sites before their story code. They cannot call QueueReply or advance a
plot choice. Navigation rows follow the host's current layout and are not reply
suggestions. Each catalog accepts one enabled reply suggestion. Unreliable
guest advice is retried at bounded intervals and acknowledged by the host.
Late acknowledgements still qualify after the last retry. The host marks the
suggested native option with `[Ashley]`; original host selection remains in
control.

Supported native/script bodies, parameter layouts and the owned native update
hook are checked. Ordinary script forwarding remains exactly once with native
LastError preserved; selected guest callbacks change only their script cursor
to the validated early return. An owned selection whose return cursor cannot
be changed is refused without entering its story graph. Native fixtures cover catalog closure and newer
generations, fragment cursor advancement, Boolean/void return selection and
ordinary forwarding and refusal when a selected return cannot be installed.
These fixtures do not establish real native UI behavior.

The earlier protected startup captured an existing guest panel and its method
ABI. This candidate's native preparation, rendered wheel, mouse confirmation,
host marker and scene-exit restoration still need verification. Missing native
panels defer presentation; no movie is created. This work does not replicate
cinematic character animation, subtitle or audio tracks.
