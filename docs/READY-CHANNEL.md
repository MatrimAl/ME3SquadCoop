# Optional host readiness synchronization candidate

This ordinary-entry candidate adds an advisory UDP readiness exchange for
automatic guest recovery. It does not implement native travel, authenticate
players, replace native package negotiation or prove a playable campaign.
No game, socket, packet or reconnect test has been executed for it.

Both host and guest must explicitly opt in with planner `--ready-channel`,
which emits `ME3_SQUAD_COOP_READY_CHANNEL=1`. Host `--auto-relisten` and guest
`--auto-rejoin` remain separate options. All are off by default. The channel
uses the configured local IPv4; the host binds UDP game-port+1 and the guest
uses an ephemeral port on its configured interface. The adjacent host port must
be available/reachable. Port65535 cannot support this option. No firewall,
interface, DNS, game installation or OS configuration is changed.

The host replies ready only while its previously verified runtime listener
belongs to the current native root, its captured driver remains valid, the
current endpoint/client admission scope passes and Shepard is in ordinary
unpaused control without an outstanding transition/fault. Other valid queries
receive not-ready. Existing native connections are unaffected.

The guest sends queries only after an observed owned Ashley client origin is
positively disposed and a different standalone root has no driver/pending
travel, or while its resulting automatic intent is still pinned to that root.
Queries stop on cancellation/disable and their local context changes invalidate
cached responses. Fresh readiness is required before queuing automatic recovery
and before the eventual ConsoleCommand. Waiting for a reply does not consume
the one-shot recovery intent. Manual Join/Retry remains independent; without
this flag the previous automatic policy remains available.

The standalone channel owns one nonblocking socket and its WSA reference,
receives at most four datagrams per poll and sends guest queries at most once
per500ms. Its fixed24-byte packet contains magic `M3RD`, version1, message kind,
a ready bit, reserved zero and a random16-byte request nonce. Replies must echo
the current nonce, come from the configured host IPv4/adjacent port and arrive
within1000ms of the request. No game object pointer, map, save, actor state or
engine command crosses this channel. An unanswered nonce is retained for that
entire response window, so a valid reply after500ms is not discarded by early
query rotation. Only expiry or local context invalidation permits replacement.
A nonce is freshness correlation, not
player authentication; native connection validation remains authoritative.

Fatal local initialization/socket/random failures leave automatic recovery waiting rather
than bypassing readiness. A local diagnostic reports unavailability without
logging endpoints or packets. Disabling closes the channel; a sticky channel
failure is not cleared by toggling enable. Manual recovery remains available.
Normal nonblocking backpressure, unreachable peers and UDP ICMP reset reports
remain waiting conditions. Oversized datagrams are dropped; they do not close
the channel. Send cadence is retained across local context invalidations.
Only the ordinary entry links/uses the channel; the protected probe does not
open a readiness socket.
The local panel reports the sampled readiness/recovery states, including a
channel failure which preserves manual Join/Retry. Host-ready publication also
requires the current root's actual driver to equal the captured verified driver.

Deferred execution checks must cover wire parsing/size, malformed or oversized
datagrams, unexpected senders/nonces, expiry/context replacement, socket cleanup
and bind conflicts, host scene/transition states, host/guest startup timing,
cancel/disable, packet loss and actual cross-PC native reconnect. Successful
compilation alone cannot establish any of those behaviors.
