# Guest quickslots through native refresh

The optional native-icons candidate now coordinates the stock PC-wheel refresh
with the mod's name-based guest slots. It retains the existing native setup,
permission/state and UpdateDisplay paths rather than repeatedly restoring
painted pointers after stock setup has overwritten them. Actual rendering and
gameplay remain unverified.

Two native boundaries are installed once on the game thread after icon/context
readiness: a MinHook detour for GetPawnFromName at RVA `0xc4c8e0`, and checked
vtable CAS at PC-wheel virtual `+0x510`, original `0xc4c960`. Their original
entries and three relevant call instructions are byte-pinned. The direct
GetPawnFromName call inside SetupQuickSlotPowers returns at `0xc544f5`; the
per-icon setup caller returns at `0xc5455b`. FName is passed as its complete
eight-byte value at these native call sites; no script frame is parsed.

For that exact pair on an admitted guest wheel, pawn resolution supplies the
owned Ashley. The per-icon adapter identifies a unique one of eight native
widgets, validates its reflected layout and picks the currently assigned full
power name from the mod map. It calls the original native per-icon helper with
that pawn/name. The helper still resolves the power in the pawn's manager and
uses native SetPower; stock setup retains its blocked/cooldown/order-state and
display handling. Invalid/unassigned slots are cleared through the verified
native UI setter and return false so native setup cannot make an empty slot
active. Existing fully cleared slots require no setter call.

The PC Tick also resolves a pawn at `0xc5836e` (return `0xc58373`) solely to
decide whether displayed names should be rebuilt from saved definitions. For
the owned valid guest context, that one comparison is suppressed. Periodic
native setup, squad-change setup and the remaining UpdateDisplay loop continue.
This avoids treating the unchanged saved Shepard definitions as the authority
for the mod's guest slot map. No saved definition, input profile, actor tag,
gameplay permission, cooldown or tactical-order state is rewritten.

Before a native UI call, an active paint lease must still hold its expected
power/pawn/full-name value, including slots the mod has not changed yet. The
captured baseline includes both FName words. Acquisition refuses a populated
baseline whose icon name disagrees with its power's current PowerName, and
empty baselines require the native None name. After the call, the adapter
rechecks registration, catalog, map,
widget and exact resulting power/pawn/name before acknowledging the refresh
to that lease. Foreign replacements are not adopted. During an actual or
pending drag, the per-icon adapter preserves its transitional pointer values;
the pawn resolver remains nonnull so stock setup cannot clear them first.
This adapter does not enqueue or execute a gameplay power request.

Retirement likewise requires the exact last installed power/pawn/full-name
triple before invoking a restore setter. A native name-only replacement is
preserved even if its object addresses still match. A retained original power
whose PowerName has changed is no longer a valid restoration baseline; that
slot clears through native SetPower(None) only if the current installed triple
still belongs to the lease. No name field is written directly. Tick painting,
native setup and the non-cleared drag/drop destination use the same expected
value predicate. The independently verified native drop wrapper's destination
clear remains an explicit accounted transition.

Unrelated callers, other wheels, other threads, disabled runtime and inactive
owned context retain original forwarding. A partially published hook also
forwards. The enabled detour and trampoline remain resident until process exit;
scene cleanup does not remove them. The active check verifies both the virtual
slot and captured post-install entry bytes. A replaced hook retires optional
painting. The separate full-wheel context lease remains independent.

MinHook v1.3.4 is vendored from its pinned official commit with its license;
only the ordinary ASI links it. The build fingerprints its sources and copies
its binary redistribution notice beside the ASIs. No hook was activated in a
running game during development. Three native targets compiled and current
fingerprints matched; no game or tests ran. Hook installation/forwarding,
stable rendering, drag/drop, cooldown visuals, native permissions, retirement
and casting still need gameplay validation. The full campaign is incomplete.
