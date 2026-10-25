# Existing guest wheel discovery

The optional native icon lease can now discover the guest's existing PC power
wheel before the first power hotkey. Discovery reads the native GUI registry;
it neither loads a movie nor invokes a guessed native getter ABI. The existing
native-hotkey observation path remains available.

Static evidence establishes this chain in the supported image:

- Static registration stores, including their staged SSE values, associate
  GetInstance/CastGetMovie/GetMovie with native thunks at RVAs
  `0xb36af0/0xb35a20/0xb35220`. Taking the closest preceding LEA would
  incorrectly identify the first two thunks.
- CastGetMovie and GetMovie reach native getter `0xbc1a40`, which calls
  singleton leaf `0xc1f860` and movie lookup `0xc1efa0`.
- The singleton leaf ends at its return at `0xc1f867`; a larger fallback
  disassembly window includes unrelated functions. Its RIP-relative load
  reads global RVA `0x18b6be0`.
- Movie lookup reads arrays at manager offsets `0x4c` and `0xbc`, and
  reads each record's movie pointer at `0x5c`. The first-list match return
  at `0xc1f0e1` returns that movie. These observations span separate unwind
  fragments; the first fragment alone is not the whole lookup.

The adapter pins the singleton load and both list-access instruction sequences.
It reads at most128 records per list and retries no faster than250ms while
acquisition is pending. It does not scan GObjects. Each candidate must have
the exact native PC-wheel vtable, the current guest controller and world,
reflected owner fields and all eight correctly owned quickslot widgets.
Multiple distinct matching wheels refuse discovery. Both registry headers,
record arrays, movie links, singleton pointer and registered ownership epoch
are rechecked before publishing an observation to mod state.

Registry observations expire before the next acquisition attempt and must be
revalidated on that Tick. A native hotkey observation retains its earlier
bounded freshness window. Existing icon lease capture, setter ABI checks,
caster/drop protections, drag rules and restoration policy remain responsible
for any later UI modifications.

Discovery admits only an already registered guest in ordinary mode0. It does
not acquire context inside an already open menu. Existing player-context
retention now has a separate verified mode3 predicate, independent of icon
lease readiness or the optional icon flag. An early Shift opening therefore
does not alone require releasing a healthy existing registration. It still
cannot acquire a new icon lease inside the menu. Missing/unready/oversized
lists or widgets refuse; no menu is fabricated.
This change also does not populate Ashley's full wheel groups. See
[population prerequisites](POWER-WHEEL-POPULATION.md).

Three native targets compiled and build fingerprints matched. No game or
tests ran. Initial discovery, native rendering, menu transitions, casting and
restoration still require gameplay validation.
