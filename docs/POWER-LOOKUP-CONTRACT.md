# Native power-name identity

The guest map stores complete SFXPowerCustomActionBase.PowerName values. The
supported native manager GetPower implementation confirms that lookup uses
both32-bit parts of that name, not the action Class's name or a catalog index.

The identity chain was read from the owned supported executable:

- SFXPowerManager.GetPower registration thunk: RVA `0xb50c40`, dispatching
  manager virtual `+0x2b0` with the complete name.
- Exact UTF-16 SFXPowerManager class name: `0x14d81e2`, referenced by class
  factory `0xc0cbb0`. Its constructor callback is `0xc0f1e0`, installing
  vtable `0x14d7ee8`.
- That vtable's `+0x2b0` entry is `0xc0c4d0`. The instruction-grounded leaf
  ends at return `0xc0c517`; the larger512-byte fallback window contains
  adjacent unrelated code. Ghidra reported no function at this exact entry,
  so no pseudocode success is claimed for this leaf.

The leaf scans manager Powers data/count at `+0x60/+0x68`, skips null entries
and returns the first object whose name words at `+0x46c/+0x470` match. It
does not filter rank, enabled or HUD flags before that comparison. Therefore
the first match in the full native array can differ from the unique match
in the mod's filtered eligible catalog.

A shared read-only predicate now checks the current manager dispatch target,
native entry bytes, reflected array/name layout and exact array header. It
finds the native first match, requires the expected current power object and
rechecks the examined prefix pointers/names, headers and owner links before
returning success. It does not alter array order or names. Another GetPower
implementation refuses this modeled route rather than assuming the same
semantics for an override.

The explicit power queue checks this before native order dispatch. Full-wheel
caster validation checks populated group entries, and the quickslot adapter
checks it before invoking native per-icon setup. Existing native permissions,
rank/cooldown checks, current action/controller identities and post-call UI
readback remain in force. A conflicting earlier entry is refused; the mod
does not silently select another power or rewrite native membership.

Three native targets compiled and fingerprints matched. No game or tests ran.
This establishes the implemented identity guard and its static basis, not a
successful cast, runtime name collision or tested override compatibility.
