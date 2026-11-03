# Co-op movie worker recovery

The protected `aa7b1d9` run ended with a guest access violation in the retained
`bink2w64_original.DLL`, at RVA `0xc8fe`. The dump identifies a null internal
worker-table entry0 in the `BinkDoFrameAsyncMulti` route. The selected game
caller uses four zero worker IDs and ignores the native return value. Returning
failure alone would not provide a demonstrated decode fallback. The crash site
does not establish that other runtime changes were causally unrelated.

The later paired owner test reproduced the same access violation on the host,
at the same native DLL offset with a null worker-table entry and four zero
worker IDs. Its retained exception context returns directly to the selected
`BinkDoFrameAsyncMulti` loop. That loop had passed its eight-word empty-job
check before reaching the fault; the movie page itself was not retained in
this smaller dump. The host later remained responsive while its world clock
stopped. This connects the host failure to the same missing-worker route;
it does not establish that every streaming wait has that cause.

The candidate uses the original `BinkDoFrame` only for that exact co-op call
when the worker is missing and all eight native pending-job words are zero.
Retained native code shows that this synchronous route performs preparation,
decode and completion; the normal `BinkDoFrameAsyncWait` accepts an empty job
list. Real native return values, including zero, and LastError are preserved.
Other calls retain their original asynchronous dispatch. Preparation now admits
both configured host and guest roles under the existing ready configuration,
enabled control and installed bridge gates. The worker, job, caller, image and
hook-ownership checks are unchanged.

Admission requires the supported game caller, owned import slot, exact already
loaded proxy/native DLL paths, hashes, export forwarding and code pins. The
callback rechecks the actual IDs, worker and job words. Module references stay
resident with the hook. Game-thread preparation publishes immutable targets and
an atomic enable snapshot before the import slot; the movie worker does not
inspect UObjects, log files, discover configuration or create threads. It only
updates atomic diagnostic counters, which are logged on the game thread.

No movie fields, worker IDs, pending jobs, packets, profiles or assets are
written. The adapter does not skip cinematics or manufacture completion.
`ME3_SQUAD_COOP_BINK_MOVIE_SYNC_GUARD=0` disables this narrow fallback.
Unknown DLL versions, other callers and nonempty job lists retain stock behavior.

All four targets built and ordinary/protected native fixtures passed.
Independent review matched all selected pins and both DLL hashes. Fixtures
cover original/synchronous exactly-once forwarding, unchanged arguments, real
zero/nonzero return and LastError, plus ID/count/address, worker and pending-job
refusals. These are not rendered playback or successful recovery evidence. The
next owner test must verify the movie, return to control and both peers'
campaign progression; the crash is not yet claimed fixed.

The host-role regression fixture executes the current preparation prefix
through its atomic enable snapshot. Its 75 cases cover host/guest admission,
unconfigured or unknown roles, every non-ready configuration status, opt-out,
disabled control or bridge, replaced hook and non-game-thread refusal. The
fixture rejects the previous guest-only gate as a deliberate mutant. It does
not exercise DLL discovery, import installation or rendered movie playback.
