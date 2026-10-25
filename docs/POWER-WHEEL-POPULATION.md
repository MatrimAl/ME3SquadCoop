# Native power-wheel population

The retained REA/Ghidra result for the supported executable completed
successfully. Target RVA `0xc539a0` accepts a wheel handler, pawn and group
index. This is static pseudocode evidence, not proof of a safe callable ABI
or a working guest wheel. The original extracted output stays private.

The result confirms these implementation prerequisites:

- Groups0/1/2 select existing integer-index arrays at handler offsets
  `0x3c4/0x3d4/0x3e4` and associated UI strings at
  `0x57c/0x58c/0x59c`. Other group values are unsupported.
- Powers come from the supplied pawn's manager at `pawn+0xc18`, through
  helper RVA `0xbce450`, followed by sorting. The method fills existing
  group icon capacity; it does not construct or resize that capacity.
- The method dispatches a handler event before filling icons. Its event
  identity and side effects still need resolution.
- A nonnull pawn path dereferences the handler's controller at `+0x6f8`,
  then controller input at `+0x6dc` and another controller object at
  `+0xc50`, whose field `+0x70` is passed to icon availability logic.
  These dependencies are not locally null-checked in the observed path.
- Each populated icon receives native SetPower, availability/state updates,
  styling and a handler virtual callback at `+0x520`. Unused group icons
  are cleared and refreshed. Blocker text/visibility also changes.

Consequently, calling the population function with Ashley alone is
insufficient. Integration needs validated controller dependencies, existing
group capacity, icon ownership, callback identities and retirement behavior.
Painting only the assigned icons would leave native state, blocker and
callback behavior unresolved. Existing caster selection protection still
requires the entire selected group to belong to the guest's current catalog.

The successful CLI operation reported823870ms (about13m44s), including import
and analysis. Its profile explicitly reports `ephemeral-source-immutable`.
Retained output can be reused, but this result does not establish a persistent
Ghidra project or guarantee that another CLI invocation avoids reimport.
Before a larger unattended batch, establish project reuse and collect related
function evidence together rather than repeatedly importing the executable.

The private dossier now checks the result's success flag, target digest,
provider, operation, exact procedure and nonempty pseudocode before labeling
it verified target pseudocode. It also records the evidence/output identities,
duration and import mode. These checks establish artifact identity, not
semantic completeness or gameplay correctness. No game or tests ran.
