# Power input research

Guest power use is **unverified**. No native binding/profile rewrite, forced
unlock, cooldown change or replacement transport has been implemented.
An explicit ordinary-runtime request API is now implemented as a compiled
candidate below; no player binding or live cast has been verified.

## Evidence from the supported package

A private offline read of the owned SFXGame package extracted11 selected
function exports. Their serialized script and expanded-memory lengths matched,
and decoded branch targets landed on decoded instruction boundaries. Input
package and compression-library hashes were unchanged. This is static evidence;
it does not establish that a power was accepted in a running campaign.

- Retained guest bindings map keyboard One through Eight to PC_HotKey1 through
  PC_HotKey8, then `UseAbility 0` through `UseAbility 7`.
- The actual `SFXGameModeDefault.UseAbility` script checks the combat getter,
  resolves the power-wheel GUI for its own controller and calls `doHotKey`.
  The PC-specific `doHotKey` implementation is native; its script is a bridge.
- `ApplyTacticalOrders` uses the controlled `BioPawn` and calls the local
  `BioPlayerInput.ActivatePower` for a pending power order. The latter rejects
  missing manager/power, a disabled power and active cooldown, then requires
  the selected power in that pawn's power-action list before starting it.
- `BioPlayerController.StartPowerCustomAction` uses its own controlled
  `BioPawn`, applies prediction/action checks and, for the autonomous client
  role, sends the existing server RPC after a successful can-start result.
- `ServerStartPowerCustomAction` resolves the server controller's own pawn as
  `BioPawn` and calls its normal power-action method. It does not accept a
  replacement caster pawn argument.
- The captured gamepad aliases instead use `TryUsePower`. That script casts
  the controlled pawn to `SFXPawn_Player` before its FindPower/UsePower path.
  It cannot substitute for testing the keyboard route on a squad member.

## Exact native bridge findings

Read-only analysis of the supported executable resolved the two native bridges.
This combines registration data, startup pointer initialization, unwind bounds,
class vtables and disassembly; no native power function was invoked.

- `SFXSFHandler_PCPowerWheel.doHotKey`: registration thunk RVA0xb69570,
  implementation RVA0xc5ccc0. It requires the handler's controller, valid
  quickslot definitions and initialized cached power icons. Possessing Ashley
  alone does not establish those prerequisites.
- The inherited `SFXSFHandler_PowerWheel.UsePower` script forwards to
  `m_pPlayerController.GetGameModeDefault().UsePower`. Exact owned bytecode
  establishes this receiver; matching event names alone would not.
- `BioPlayerController.SquadOrderUsePower`: thunk RVA0xb64a20,
  implementation RVA0xaa38b0. The initial supplied-pawn cast accepts `BioPawn`.
  When that pawn equals the controller's controlled pawn, it stages the normal
  pending power order; the other-pawn branch issues a henchman order.

The static route is therefore `UseAbility -> doHotKey -> inherited UsePower ->
GameModeDefault.UsePower -> SquadOrderUsePower -> ApplyTacticalOrders`.
These are function-entry RVAs for the supported image only, not portable hook
signatures or permission to call a function through an assumed ABI. Native
unwind fragments must not be mistaken for complete function boundaries.

## Next verification

An offline comparison with the already-present LEMP reconstruction found a
concrete reuse candidate: `SFXPlayerControllerMP.CastPower` checks combat input
and the possessed pawn's power-list index, then calls the existing
`SquadOrderUsePower` with that power's name and the possessed pawn. Its wrapper
does not require an MP player-pawn cast. The selected script decoded completely
(156 serialized bytes, 244 expanded VM bytes). This corroborates reuse of the
existing squad-power route for Ashley; it does not establish native acceptance,
cooldown, targeting or damage in a live guest session. LEMP is a research source
for this comparison, not a runtime requirement.

Measure the actual guest controller's quickslot definitions and the wheel's
cached icons after acquisition. Then test an available Ashley power through
its mapped keyboard input, observing delivered input, action acceptance,
cooldown and an authoritative effect on the intended target. Preserve native
restrictions. Static acceptance of the pawn base class does not prove a cast
or a target effect occurred in a running game.

The external [LExSDKv2 LE3 reference](https://github.com/llamathings/LExSDKv2/tree/000d7c3799f8f9de5db276b49008b539f5f97830/Src/LESDK/LE3)
provided initial declaration leads. The primary
[LegendaryExplorer reader](https://github.com/ME3Tweaks/LegendaryExplorer/tree/b5e746233491ec5a7cbfc077685996af37a3d125/LegendaryExplorer/LegendaryExplorerCore)
provided package/bytecode format references. Extracted game data and research
readers stay private; no game assets or decompiled bodies are distributed here.

## Action-instance ownership follow-up

Additional owned-script analysis found that the power-action eligibility path
uses the controlled BioPawn's own action classes and instances. When a missing
action is instantiated, the script records its pawn and either AI or player
controller context. Existing populated instances skip that creation block.
Action start/stop subsequently use cached controller context for input-ignore
and reticle handling. The power-specific check also rejects ragdoll/recovery
and weapon equip/put-down states. These native gameplay restrictions remain
part of the intended power path.

This raises a concrete ownership question for previously AI-controlled squad
members: do existing action instances still reference their former controller
after possession? That has not been measured. Inherited/native refresh paths
have not been ruled out, so no rebind, array recreation or forced cast is
implemented from this finding. Six selected functions decoded completely with
matching serialized/VM lengths and valid branch targets; this remains offline
evidence and does not establish successful guest powers.

## Tactical-order call contract

A new read-only owned-package extraction verified the exact
BioPlayerController.SquadOrderUsePower declaration: native index0,
function flags0x24401 and nine linked parameter fields including its boolean
return. Input package and compression-library hashes were unchanged. The
declaration uses a NameProperty power name and the supplied pawn, followed by
optional queue, indicator, selection target, input method, desired target and
target location fields. Its return is an actual boolean field, rather than
proof of a completed cast or authoritative effect.

The retained SDK's parameter offsets require four-byte packing: the desired
target pointer is unaligned at0x24, the vector starts at0x2c and the return is
at0x38, for0x3c total bytes. `guest_power_order_contract.h` records this authored
ABI and has compile-time size/offset assertions. Its runtime predicate requires
the exact reflected method/native entry and all nine field names, types,
owners, dimensions, sizes, flags, offsets and boolean masks. The complete
declaring method identity is rechecked before returning success.

The header is included by the ordinary entry for compilation. No power order
is dispatched and there is no new power input binding. Runtime verification of
this contract, controlled-pawn power selection, current action-instance
ownership, native cooldown/target acceptance and actual effects remain pending.
The next implementation must resolve power choices from the possessed pawn's
current manager; it must not accept an arbitrary caller-supplied caster or
bypass the normal tactical-order/ActivatePower/server path.

## Explicit guest power request candidate

The ordinary entry exports `ME3SquadCoopPowerScope`, `ME3SquadCoopPowerCount`,
`ME3SquadCoopRequestPower(scope,index)` and `ME3SquadCoopPowerStatus`. Export
callers touch mod-owned queue state only. A scope identifies the sampled possessed
pawn/controller/world/manager, connection/driver/channel, player-registration
token and its current power-array entries/names;
scope changes cancel pending input. Scope0 means unavailable. The generation
never wraps into a previously usable value. The list is bounded to64 entries.
Requests carry a monotonic64-bit enqueue time and expire after2000ms, checked
both when consumed and immediately before the native order. Expiry reports
status2/rejected with reason `power-request-expired`; it never resends an order.
The queue serializes payload publication and timestamp sampling through a
nonblocking SRW lock. A busy producer returns0; a busy Tick defers consumption
without waiting. No engine read or native call occurs while holding that lock.
Status readers still inspect only the atomic payload, and cancellation leaves
the lock-protected timestamp untouched. Scope invalidation independently rejects
an obsolete request published concurrently with cancellation.
The separate local session-panel candidate exposes explicit power requests.
An optional keyboard adapter is described below; gamepad bindings and native
keyboard/gamepad remapping remain unimplemented.

Tick consumes one request only after ordinary guest ownership and an initialized
player context are verified. It rejects invalid indices, native duplicate power
names and downed/dead BioPawn states. Life-state refusal invalidates the catalog
scope and pending input, including when actions or input-ignore fields have not
yet changed. The final catalog reads repeat this check. See
[companion life state](COMPANION-LIFE-STATE.md).
It also rejects stale object identities, missing/unlearned ranks and action instances
still bound to another controller or AI. The reflected manager/power-property
layouts and the complete native order ABI must match. A fresh catalog and
control/scene checks run before dispatch, including after the native combat
getter. The native receiver is the local controller and the supplied caster is
always its possessed Ashley. Optional target fields remain unset for ordinary
native target selection. Accepted dispatches are limited to one per250ms.

Power processing runs after the camera, player-context and collision adapters.
The runtime rereads the current session and scene gate after those adapters,
since collision repair can invoke native script. The power order is the final
operation in the guest frame; no later adapter uses its pre-order context if
native execution starts an action or changes ownership/world. This ordering is
compiled but has not been exercised in the game.

The command invokes only the game's normal SquadOrderUsePower entry. It changes
no ranks, controller caches, cooldowns, permissions, targets or RPC routing.
Native return1 is reported as order acceptance; return0 is native refusal.
Neither is a completed cast, server acceptance, cooldown or target-effect verdict.
Status0/1/2/3/4 means idle/queued/rejected/native-accepted/native-refused.

The runtime catalog skips null native power entries and exposes a compact list
of existing instantiated objects. Non-null unreadable or wrong-class entries
still invalidate the entire sample. Each compact entry retains its native
source position alongside its handle and FName; source-position, membership,
name or identity changes invalidate the scope. Both null and populated source
slots are read again before accepting the sample. An all-null list remains
unavailable. Compact API indices are not native power-action IDs: the native
order continues to use the selected object's validated power name and possessed
pawn, never a compact index as a native action argument.
The runtime does not initialize or rebind those objects. All-empty or stale
controller state remains an observable rejection rather than forced power use.
Normal outputs compile, but no game, request fixture, native order or performance
measurement was run. Complete input mapping, catalog presentation, action-instance
readiness and authoritative effects remain unverified.

## Opt-in guest power slot hotkeys

`ME3_SQUAD_COOP_POWER_HOTKEYS=1` enables a guest-only physical-input adapter.
The planner's guest-only `--power-hotkeys` option emits this flag; it remains
off by default. Ctrl+Alt+1 through Ctrl+Alt+9 request the powers assigned to
nine slots. Empty slots fill from distinct, unambiguous native FNames in the
first available catalog, in catalog order. Each slot retains its complete
two-word FName across scene/catalog/mission changes in the same process.
Later catalogs can fill empty slots, but never replace an occupied assignment.
Reordered catalogs resolve the same name to its current index; absent or
ambiguous powers leave their slots unavailable. No old object pointer is kept.
Bindings reset when the runtime process restarts; no disk persistence or manual
assignment UI is provided. The panel annotates current entries with their actual
Ctrl+Alt slot, independently of the list row number.
The exposed list now excludes unlearned, base-disabled and HUD-hidden entries;
ordinary power validation can still refuse listed powers. The panel is optional, and closing it does not
disable these runtime hotkeys. Remove the startup flag to disable them.

The adapter samples the high/held bit of GetAsyncKeyState on pre-Tick only,
detects a rising chord edge and verifies the foreground window belongs to the
current process. It sends no synthetic input, installs no keyboard hook and
does not register or consume Windows hotkeys. Other bindings may therefore
also respond to the same chord; the preset is opt-in and not yet rebindable.
Windows-modified chords and multiple simultaneous slot chords are ignored.

Input is armed only after consecutive ready samples with the same nonzero
catalog scope and focus, at most500ms apart. Startup, scene/control loss,
focus loss, catalog replacement and longer Tick gaps reset the baseline, so
a held chord across those boundaries does not cast on return. Unavailable
pre-Ticks still sample/disarm input. Holding a chord does not autorepeat.
The adapter queues through `runtime_request_power`; it never overwrites a busy
request or retries a refused/expired order. Existing ownership, scene, rank,
cached-controller, ABI, native targeting and expiry checks still apply.

Native outputs compiled and planner sources syntax parsed only. No hotkey,
focus switch, keyboard layout, native cast, panel or game was exercised.

## Normal keyboard path prerequisite review

A renewed read of the retained exact-image `doHotKey` disassembly corroborates
the eight-slot bound, its controller-relative 16-byte hotkey definitions at
0x92c, and the selected handler icon pointer at0xbd8 + index*8. A null selected
icon takes a refusal path before the power dispatch branch. Consequently a
working mod catalog or a successful explicit power order does not establish
normal keyboard quickslot readiness. Native wheel setup and its assignment
semantics must be validated before adding a normal-input integration. No wheel
cache or quickslot definition is written from this static finding. The gamepad
player-pawn cast remains a separate integration gap.

## Native wheel membership

Owned script inspection completely decoded SFXPowerManager.GetPowerWheelPowers
(170 serialized / 246 VM bytes). It processes display order and selects powers
by IsEnabled(), DisplayInHUD and Rank>0. The base SFXPowerCustomActionBase.IsEnabled
body (14 /22 bytes) simply returns bEnabled. The mod catalog now requires the
corroborated base fields bEnabled mask1 and DisplayInHUD mask0x100 at+0x4dc,
plus finite positive Rank at+0x488. Reflected field names/types/offsets are
checked before reading these membership fields. Non-null unreadable/malformed
entries still invalidate the sample; readable ineligible entries are omitted.

This is base membership filtering, not execution of virtual IsEnabled overrides
or reproduction of native wheel display sorting. The exposed order remains
manager order. Native cooldown, targeting, cached-controller and action checks
remain authoritative. A final fresh catalog rechecks membership before order
dispatch. An entry becoming unavailable changes the compact catalog scope and
invalidates pending input; retained named shortcut assignments remain intact.
Cooldown is not a catalog filter, avoiding shortcut churn while a power recharges.

InitializePowerList (286 /434 bytes) and AddPower (447 /667 bytes) also decoded
completely. InitializePowerList clears/rebuilds manager membership and calls
SetPowerStartingRanks for player-party pawns; it is not a harmless controller
cache refresh. AddPower changes the pawn action-class table and uses
VerifyCAHasBeenInstanced before updating manager membership. Neither is called
by this change. Extracted data remains private and source hashes were unchanged.
Only native compilation was performed; no wheel, shortcut, power or game ran.

Membership layout validation is shared within one catalog read. The first
non-null entry verifies the three base fields; every following entry must
inherit the same actual SFXPowerCustomActionBase Class identity, not just a
matching class name. The declaring-class handle is rechecked during traversal
and before returning the catalog. Class traversal is bounded and rejects cycles.
This avoids repeating the same three property-list searches for every entry.
Each catalog read starts fresh, including final pre-dispatch reads; no layout
cache survives a scene, world change or native call. Live flags/rank, source
membership and object checks remain per entry. No frame-time improvement was
measured and no runtime behavior was exercised for this source optimization.

## Wheel UI initialization boundary

A separate owned-package extraction decoded nine base/PC wheel functions
completely, with matching serialized/VM lengths and aligned explicit branches.
Base InitDisplay is550/738 bytes; base InitPowerIcons508/788. PC InitDisplay
is196/296; PC InitPowerIcons472/684. The PC icon initializer calls its base
initializer, resolves movie variable objects, configures their icon properties
and assigns m_aQuickSlotIcons. It also resolves the drag power/slot objects.
The icons consumed by doHotKey therefore belong to an initialized Scaleform
movie, not merely a power-manager array. Calling an icon initializer without
the correct loaded movie/handler context is not a demonstrated keyboard fix.

Base CleanupReferences116/180 clears its controller, squad, player/hench pawn
and selection references and cleans power icons. PC CleanupReferences128/200
delegates to the base and cleans quickslot icons. Capturing these pointers
without accounting for native movie cleanup would be stale-context handling.

SetupPlayerPowers, SetupQuickSlotKeys and SetupQuickSlotPowers each have only
a two-byte script bridge; their native bodies require separate inspection.
Exact executable name/function registration and thunk tail jumps resolve:

| Method | Name RVA | Table pair RVA | Exec RVA | Direct body RVA |
| --- | --- | --- | --- | --- |
| SetupPlayerPowers | 0x13794b0 | 0x179d180 | 0xb637b0 | 0xc53910 |
| SetupQuickSlotKeys | 0x1379650 | 0x179d260 | 0xb69080 | 0xc54280 |
| SetupQuickSlotPowers | 0x13795e0 | 0x179d250 | 0xb69370 | 0xc54470 |

These are static entry identities for the supported image, not validated
initializers to invoke on an arbitrary handler. Their bodies, movie lifecycle,
current controller/pawn assignment and existing saved quickslot semantics remain
the next integration questions. No UI initializer, cleanup, key assignment or
native body was executed. Package/compression inputs remained unchanged, and
private raw code was not added to this repository.

## Native wheel setup bodies

The three setup bodies were subsequently disassembled from the supported image.
SetupPlayerPowers at0xc53910..0xc5399b checks handler+0x7f8/mask0x10, identified
by the retained SDK as m_bProcessSquadHenchmen. The clear-bit path invokes
virtual+0x4f0 and populates powers for handler+0x710 only; the set-bit path
invokes+0x4e8 and processes player/hench pointers at+0x710/+0x718/+0x720.
For the previously constructor-verified PC handler vtable0x14f6700, these
virtuals resolve to0xc51aa0 and0xc521b0; populate slot+0x500 is0xc539a0.

The non-henchman selector's complete leaf is0xc51aa0..0xc51ae1. It reads the
handler's m_pPlayerSquad at+0x700, gets that squad's CachedPlayerPawn at+0x2a8,
and updates m_pShepardPawn at+0x710 plus the squad-changed bit. It does not
perform a player-pawn class cast in this leaf. This is a concrete connection
between the guest's existing player-squad registration and native wheel pawn
selection. It is not proof that the actual guest wheel selects this branch or
already references that squad. The generic reader's fallback window includes
unrelated functions after the leaf return and is not its body extent.

SetupQuickSlotPowers spans three contiguous unwind fragments
0xc54470..0xc544a3,0xc544a3..0xc545e7,0xc545e7..0xc545ed.
It requires existing handler controller/squad/player pointers, walks eight
cached quickslot icons, reads each controller hotkey definition at+0x92c with
stride16, resolves its pawn, and invokes the existing per-icon setup virtual.
Missing pawn/icon paths preserve native refusal/hiding behavior. It finally
sets m_bEvalQuickSlotPowers at handler+0xc54/mask0x40. The selected pawn resolver,
power population and icon setup callees remain incompletely characterized.

SetupQuickSlotKeys0xc54280..0xc54462 resolves a controller through existing
native helpers, uses its GameModeManager and formats eight key-label entries.
It is not established as the routine that installs gameplay input bindings.
The initial controller resolver must be understood before assuming it selects
the same local guest controller as the handler.

No native setup method was invoked. The current integration needs a live
handler/movie identity, matching guest controller and registered squad, the
appropriate selector branch, existing hotkey definitions and understood
population/icon callees. These findings identify the chain without inventing
quickslot assignments or overwriting campaign story squad membership.

## Quickslot pawn identity and per-icon setup

The pawn resolver directly called by SetupQuickSlotPowers and doHotKey is the
leaf0xc4c8e0..0xc4c956. It compares the supplied two-word name with Actor.Tag
at pawn+0x1a0/+0x1a4. It first considers the handler's player pawn at+0x710;
only when m_bProcessSquadHenchmen is set does it consider+0x718/+0x720.
It returns null if no complete tag matches. It does not search the world,
substitute the possessed pawn or match by power name. The fallback disassembly
window contains adjacent functions after this leaf and is not its body extent.

Consequently native wheel/player context readiness does not prove hotkey
readiness. A saved quickslot definition for Shepard's tag will not resolve to
Ashley merely because the handler now has Ashley as its player pawn. This is
a conditional integration risk, not an observed current guest-profile mismatch.
No profile hotkey tag, pawn Tag or gameplay binding was rewritten.

The adjacent per-icon helper0xc4c960..0xc4c9d0 checks a supplied pawn's existing
PowerManager, calls its native power lookup with the supplied name and requires
enabled/HUD-visible bits0x101 before updating the supplied icon. It refuses
missing pawn/manager/power/icon. This corroborates the base membership filter
already used by the mod's exposed catalog, without establishing every virtual
override or icon-rendering behavior.

The larger populate routine0xc539a0..0xc5427e contains per-player/hench UI
bookkeeping, ProcessEvent calls and reads the supplied pawn's PowerManager.
Its62 call sites are not all semantically resolved; identifying its entry and
manager read does not make it a verified standalone initializer. Calling it
before the appropriate movie objects and handler context exist remains unproven.

The next integration decision must cover existing quickslot character+power
definitions together, current owned pawn, handler selection mode, movie/icon
lifetime and restoration when control changes. Replacing pawn tags or silently
rewriting saved Shepard slots is not a substitute for that lifecycle design.
Only offline image reads were performed for these findings.

## Native quickslot assignment semantics

Exact native registration resolves MoveQuickSlotPowerByName name0x1379720,
table pair0x179d280, exec0xb68cf0 and direct body0xc50570..0xc505d9.
The body searches the controller's eight existing power-name halves at+0x934
with stride16. If the requested name already occurs in another slot, it calls
the assignment virtual to move that slot. It does not instantiate/find a new
power on the currently controlled pawn or establish a new Ashley character
binding merely from a power name.

NewSetQuickSlotPower name0x13797a0, pair0x179d290 and exec0xb68b50 lead to
0xc505e0..0xc5063a, which clears the destination icon's current power and then
dispatches virtual+0x548. The constructor-verified PC handler vtable resolves
that slot to0xc52eb0..0xc53298. This body requires existing controller, squad
and player-pawn references. Its power-drag branch selects an existing power
icon; its alternate branch uses an existing quickslot icon. It derives the
new character tag from the selected icon's pawn and the power name from the
selected instantiated power. It writes the paired hotkey definitions, handles
an existing source slot and refreshes the affected UI icons.

The destination/source/icon indices require independent bounds before any
future use: the native body performs an early slot read with a bound that can
admit8, while the actual slot array has eight entries0..7. Its wrapper also
touches the destination icon before the deeper body's guards. Treating these
methods as arbitrary-name setters would skip required UI and bounds context.

The silent argument suppresses one GUI feedback path, but does not suppress
all work. The controller callback at0xc5d250 emits GenerateTutorialEvent with
event byte0x0b, even after the silent path. Its event name is resolved through
native name-entry global0x17a8a68, entry0x142ddb3 and the exact registration
string, rather than inferred from the call signature. No explicit profile-save
call was identified in the assignment body; indirect UI/helper effects and
later native save behavior have not been exhaustively established.

The integration candidate is therefore scoped assignment from correctly
initialized Ashley power icons, with captured original definitions and native
lifecycle/restoration design, rather than a generic power-name call. This is
still research: no assignment, tutorial event, profile write or game ran.

## Handler bootstrap uses a different squad chain

A targeted static search for stores at the handler controller displacement
found several candidates, not all belonging to wheel objects. Candidate
0xc58410..0xc58b37 was grounded in the PC wheel by its decoded caller
0xc58040..0xc5840a: the caller's instruction0xc58068 invokes it directly,
and the constructor-verified PC handler vtable has0xc58040 at slot+0x3f8.
This establishes a wheel virtual update path, not an arbitrary displacement
match. Other raw store candidates were not treated as wheel methods.

The base path fills m_pPlayerController at+0x6f8 only while null, using existing
native local-controller resolution/cast helpers. It fills m_pPlayerSquad at
+0x700 only while null, through controller helper0xa9dfa0 followed by the
returned pawn's Squad at+0xba8. The helper's ordinary path casts Controller.Pawn
through the already identified BioPawn cast0x551e70; its other branches cover
game modes outside this ordinary path. It does not fetch WorldInfo.m_playerSquad.

This differs materially from the current synthetic guest context: that adapter
registers its local squad in WorldInfo+0x8e8 while deliberately preserving
Ashley's existing Pawn.Squad. The combat getter's positive result therefore
does not prove wheel context readiness. The wheel can retain the story squad
and consequently select its cached Shepard instead of the locally controlled
Ashley. This is an evidence-backed integration possibility; no current live
wheel mismatch was measured.

Both controller and squad are cached until native cleanup clears them; merely
making another world-level squad available does not refresh an already non-null
wheel reference. A future adapter needs its own handler/controller/squad/movie
identity and cleanup policy, plus the existing quickslot character-tag handling.
Do not move Ashley out of the host story squad, replay broad initialization or
reset a live movie solely to make these two pointer chains coincide.

No handler pointer, native input binding or game state was written. The mod's
existing squad/context policy remains unchanged by this research.

## Campaign mapped-power request candidate

The retained SFXGameModeDefault.TryUsePower script is completely decoded
(484 serialized bytes / 724 expanded VM bytes). Actions0,1,2,3,4 and6 read
BioPlayerInput's m_nmMappedPower, m_nmMappedPower2..5 and m_nmMappedPower7.
The script has no case5. These are named mappings, not LEMP CastPower's native
manager-array indices or the mod's compact eligible-power catalog indices.
The retained SDK places the eight-byte name fields at+0x338,+0x340,+0x348,
+0x350,+0x358 and+0x368 respectively.

The ordinary entry now exposes ME3SquadCoopRequestMappedPower(scope,action).
It only queues mod data; its nonzero return means queued, not cast or hit.
The same single pending slot, two-second expiry, scope validation and native
order rate limit apply to direct and mapped requests. Unsupported actions are
rejected at submission. No external caller reads or writes engine objects.

Tick resolves the selected reflected NameProperty against both words of each
current eligible power name. Missing, ineligible or duplicate names are refused
without an ordinal fallback. The catalog includes the owned BioPlayerInput
identity, so replacing that object invalidates pending scopes. Current input
ownership, the selected property layout and name resolution are checked again
around the native combat getter before dispatch through SquadOrderUsePower.
The existing action-owner, scene, life, rank and native ABI gates still apply.

This is an integration candidate for a future normal-input bridge. No physical
gamepad event currently submits this API, and the stock TryUsePower path still
casts SFXPawn_Player. This addition does not fix that stock cast, initialize a
power wheel, create Ashley mappings or rewrite a saved profile. Normal keyboard
and gamepad power operation, casting and target effects remain unverified.
Only offline source review and native compilation were performed.

## Native keyboard command bridge candidate

The ordinary guest can opt into ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS=1 through
the planner's --native-power-hotkeys option. This routes existing UseAbility
commands0..7 to the mod's first eight retained named power slots. Those slots
capture unique eligible names in manager order and retain them across catalog
or mission changes; they are not the saved Shepard quickslot definitions.
Absent or ambiguous names refuse the command rather than selecting a new
power by its changed list position. Panel v6 labels the actual named slot
membership. Its labels do not assume a particular physical key binding.

The bridge replaces one exact native PC wheel vtable slot, not a script Func
pointer or input binding. The owned-image exec thunk0xb69570..0xb695ed decodes
an integer and calls virtual+0x528 with the handler and decoded integer. The
image's constructor-grounded PC wheel vtable0x14f6700 contains0xc5ccc0 there;
the retained SDK declares the virtual as void doHotKey(int). This avoids
replicating FFrame parsing. The thunk's optional debug-token handling remains
stock. Direct byte reads corroborated the vtable target and the implementation
entry signature independently of the short first unwind fragment.

Installation occurs once on game Tick after a valid owned power catalog.
It requires the expected vtable target and entry bytes, uses a pointer CAS
and refuses an existing replacement. The supported executable fingerprint
still gates the enclosing runtime. The resident wrapper forwards stock calls
when disabled, outside the game thread or for another controller. For the
registered local guest's verified wheel/controller, it consumes the native
command and queues only a currently available retained-name power. The queue
performs all ordinary final scene/life/action/controller/ABI checks later on
Tick. It never dispatches a power recursively from the wheel callback.

This bypasses the native hotkey implementation's cached story-pawn/icon
selection for this optional guest path. It does not initialize a missing movie
or handler controller: UseAbility still needs the existing controller-specific
wheel and native combat gate before reaching the virtual. Wheel icons and
saved Shepard definitions are not rewritten to match the mod's slots, so UI
alignment remains unfinished. The panel is the available slot-label surface.
The tiny forwarding hook stays resident until process exit, as the Tick bridge
does; disabling returns its calls to stock behavior.

Keyboard delivery, native acceptance, cooldown, target effects and icon
agreement remain unverified. Gamepad integration is deferred. All three native
targets compiled and the changed Python files syntax parsed; no game, panel,
network session or tests ran.

## Quickslot icon update and restoration findings

Ten selected icon/lifecycle exports decoded with matching serialized and VM
lengths. The substantial SetQuickSlotState script is682/862 bytes, with all
nine explicit branch targets on decoded expression boundaries. Most other
selected exports are native bridges; complete bridge decoding is not complete
native-body coverage. Input package and decompression-library hashes stayed
unchanged. All extracted bodies/readers remain private.

Native registration grounds SFXGUIValue_PowerIcon.SetPower at name0x137aab0,
pair0x179d880, exec0xb58aa0 and direct body0xbcabd0..0xbcaf8e.
The implementation returns immediately when the existing power pointer equals
the supplied pointer. On a change it invokes the old icon's clear virtual,
writes pPower at+0x194 and derives pPawn at+0x19c from the power's m_oPawn.
For a non-null power, it then requires an Outer cast to SFXGUIMovie before
updating localized name/description, resource/icon, full power name and state.
The cast helper0xba0d50 resolves that exact class through0xbc2220; its package
string alone was not treated as a class-name identification.

The missing/wrong movie branch can therefore leave the new power/pawn pointers
written while text/name/icon work was skipped. Reading back pPower alone is
not sufficient acceptance evidence. The null-power branch resets strings,
name, cooldown and state; native string allocation/copying remains engine-owned.
A future adapter must not snapshot and reinstall raw FString buffers.

Class-name construction grounds the quickslot class through0xbc2640,
instance constructor0xbc51b0 and final vtable0x13b29b8. This vtable's
ProcessEvent slot+0x230 is0x0f4f80, not the player controller's0x40d450.
Any eventual reflected UI call requires its own receiver/dispatcher contract.
The clear virtual+0x2b8 is the actual return-only leaf at0x73820; following
padding into adjacent code would invent behavior. ClearIcon is consequently
not a demonstrated cache-restoration operation on this exact quickslot class.
The separate script Cleanup17/25 bytes clears pPower and pPawn only; it does
not reconstruct a previous native icon assignment or localized display state.

The same class resolves state display+0x2c8 to0xbcb240 and cooldown update
+0x2d0 to0xbccfe0. UpdateDisplay's body0xbcd250..0xbcd278 invokes state display
only while dirty, then clears the dirty bit. TickCooldown0xbcc1b0..0xbcc1fa
compares current native cooldown with its cached value and invokes the icon's
cooldown virtual as needed. These are native UI dependencies, not permission
to force gameplay cooldowns or infer a successful cast from an icon change.

The resulting integration requirements are current wheel/movie and icon
identities, ownership of the icon slot, native setter/receiver validation,
retained prior power identity and conditional restoration while the same UI
still exists. Replacement/cleanup must retire that ownership rather than write
to a stale widget. The current keyboard bridge does not perform these writes;
icon agreement remains unfinished. No game or tests ran for this research.
