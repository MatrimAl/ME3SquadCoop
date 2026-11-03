# Power input research

## Current guest controls

Native keyboard power slots and their icons now default on for the ordinary
guest. Explicit zero environment values or the planner's corresponding
`--no-native-power-*` options disable them. The existing power order route now
also has a scoped idle-instance controller-context adapter; it does not create
actions, force a cast or change native rank/cooldown/target checks. The mapped
gamepad candidate below remains opt-in. Historical opt-in descriptions are
superseded by [the current player controls](GUEST-PLAYER-HUD.md). This new
combination has not been exercised in a game.

## Opt-in native mapped-command candidate

The ordinary guest can explicitly enable `ME3_SQUAD_COOP_NATIVE_MAPPED_POWERS=1`
or the planner's `--native-mapped-powers` option. Defaults and existing keyboard
options are unchanged; host plans refuse this option. This is a candidate for
normal `TryUsePower` command delivery, independently of the external shortcuts
and PC wheel hotkey bridge. Actual gamepad/console alias delivery through the
hooked ProcessEvent virtual remains unverified.

Retained owned script identifies `SFXGameModeDefault.TryUsePower` as an exec
function (`0x20202`) with one four-byte `Power` integer, no return value and
724 expanded script bytes. It reads its controller through `Outer`, including
`Outer.PlayerInput` and `Outer.Pawn`; the latter is cast to `SFXPawn_Player`.
The candidate checks that exact method/parameter/native ABI and the native
ProcessEvent entry before replacing the current default mode's forwarding
virtual. It requires the current controller's `GameModeManager2.GameModes[0]`,
mode0, matching Outer, registered owned Ashley and current input identities.

The callback reads already-decoded ProcessEvent parameters and queues actions
0..4 or6 through the existing scoped mapped-power API. It performs read-only
membership/catalog validation and no native engine call. The ordinary Tick
retains final scene/life/rank/controller, mapped-name, combat, expiry and
SquadOrderUsePower checks. Owned unavailable/unsupported requests are consumed
and logged; other methods/controllers, host/disabled runtime and released
control forward the original call once. LastError is preserved around the
filter. There is no FFrame parsing, script-function rewrite, new key polling,
saved mapping change, caster override or forced cast.

The native self-test covers forwarding, ownership/policy refusal and unsupported
command handling; its availability does not establish actual input delivery or
a cast. No game or new reverse-engineering analysis was performed for this
candidate. Native compilation and focused verification are reported separately.

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

## Optional native icon lease candidate

The guest-only --native-power-icons planner option, requiring
--native-power-hotkeys, enables ME3_SQUAD_COOP_NATIVE_POWER_ICONS=1. Both
remain off by default. The keyboard callback records the existing owned wheel
identity. The icon adapter can acquire it for one second after that observation;
it does not search for/load/open a movie. The first real native hotkey command
is therefore required before this candidate can acquire an icon lease.

Acquisition validates the current wheel/controller/world and all eight icon
members, exact quickslot vtables, icon Outer relationships, reflected layouts
and native setter contracts. It retains previous power and pawn identities,
not string-buffer snapshots. SetPower is dispatched on the icon through its
verified0x0f4f80 ProcessEvent, with the reflected eight-byte object parameter
and exact native function metadata/parameter layout. The controller dispatcher
is not used for this receiver.

The adapter assigns at most one slot per pre-Tick, through native SetPower,
and revalidates current context, UI membership and power/pawn/name readback
after every call. Empty slots use the native None name; an equal pointer with
inconsistent caches is refused because SetPower would return without repair.
Unreadable pointers are not interpreted as empty slots. Power dispatch remains
last, after fresh context capture in the guest frame.

An active lease survives transient action/catalog unavailability, allowing the
native icon to keep its cooldown reference. A changed valid catalog, scene,
ownership/registration change, disable, replaced widget or failed readback
retires it. Retirement is bounded to eight setters and checks the current
world/UI and each still-installed power/pawn pair before restoring. If a prior
power/pawn identity expired, a still-owned icon is cleared through the native
setter instead. Foreign UI changes and stale widgets are not overwritten.
After retirement the same observation is consumed; a new native hotkey command
is needed to acquire again, rather than repeatedly fighting native UI changes.

This is a compiled candidate, not rendered icon agreement. Existing native
refresh may replace a leased assignment; that path deliberately retires the
lease. Mouse activation, drag/remapping, native widget rendering/cooldown and
scene/disable restoration have not been exercised. No saved hotkey definition,
profile, movie initialization or handler story-pawn cache is written by this
adapter. Native compilation and Python syntax checks completed; no game,
panel, network session or tests ran.

## Mouse quickslot routing and drag coordination

Eleven selected mouse/quickslot exports decoded completely with matching disk
and VM lengths; all38 explicit branch targets aligned with decoded expression
boundaries. Package/library hashes remained unchanged. Native bridge exports
only establish the wrapper, not full native implementation coverage.

HandleQuickSlotMouseUp (173 serialized /237 VM bytes) scans the eight existing
icon IDs and calls doHotKey for the matching slot on its non-drag path. This
reaches the same native virtual already intercepted by the keyboard candidate.
ExInt_IconMouseUp clears m_nDraggingIcon to -1 after that call. Consequently a
pending source index cannot by itself reject the callback, or normal mouse
clicks would be dropped. A queued order is instead checked again on Tick.
This is static routing evidence, not a delivered click or successful cast.

The runtime now reads reflected m_nDraggingIcon at+0xc38 and m_bDoingDrag at
+0xc54 with its exact boolean mask8. The power/key flags at that shared word
describe drag kind and are not treated as reliable activity flags. A pending
source or active drag suspends icon acquisition/assignment; actual dragging
refuses native hotkey submissions. The common power queue refuses execution
while a matching current wheel is pending/dragging or its state is unreadable,
including a final recheck after the native combat getter. Starting an observed
drag invalidates pre-drag queue scope. Icon retirement does not invoke setters
while the native gesture owns transitional widget state.

After a gesture observed across Tick, the adapter retires its lease, restoring
only still-owned idle widgets under its existing identity checks. It does not
repaint over native drop changes. A fresh native hotkey observation is needed
for reacquisition. Gestures completed entirely between samples can still cause
native icon changes; the ordinary membership/value checks then retire rather
than overwrite those changes.

Guest drag/remapping is not implemented. Native NewSetQuickSlotPower clears
the destination through SetPower before virtual+0x548, and the existing native
assignment body updates controller character+power definitions, independently
of the mod's retained name map. Full integration must own that pre-clear and
assignment lifecycle together. The previous proposed assignment-hook patch
was never applied after its approval review hit a usage limit. No assignment
hook, saved-definition write or game ran. Three native targets compiled for
the drag-coordination change; no tests ran.

## Guest quickslot drop candidate

The optional native icon candidate now also owns the PC wheel assignment
virtual at+0x548 while its exact guest wheel has an active icon lease. The
constructor-grounded vtable contains0xc52eb0 there. Installation checks the
original slot and entry bytes and uses pointer CAS. The forwarding wrapper
remains resident until process exit, with stock behavior for other wheels,
disabled runtime, non-game-thread calls and absent guest leases. A replaced
assignment hook retires the icon lease; it is not overwritten.

The accepted caller is the verified NewSetQuickSlotPower exec0xb68b50:
its instruction0xb68c50 calls wrapper0xc505e0, whose tail jump reaches the
assignment virtual, leaving return address0xb68c55. This distinguishes the
decoded UI drop from MoveQuickSlotPowerByName, whose source index comes from
saved Shepard definitions. Unsupported callers are refused for the owned
guest wheel, rather than interpreting that saved source as a guest slot.

NewSet has already cleared the destination before reaching this hook. An
observed native empty power/pawn/None triple is recorded as the lease's current
installed value without discarding the original baseline. If later validation
refuses the drop, idle retirement can therefore restore that cleared slot.
The hook itself makes no native UI or power call.

Quickslot-to-quickslot drops resolve the source's retained full power name and
require the source icon to match its current owned pawn/power. A self-drop can
use its retained mapping after the upstream clear. Power-list drops require
the current bounded m_aPowerIcons array, an exact native PowerIcon instance,
its wheel Outer, reflected fields and the owned eligible catalog's unique
name/pointer/pawn match. Stale, foreign, ambiguous and unsupported sources
refuse and retire the lease. Destination bounds are0..7. This downstream
check does not repair the stock wrapper's earlier unchecked icon access.

An accepted drop updates only the process-local guest name map. If the name
already occurs, its source receives the previous destination assignment; this
also preserves an existing optional ninth Ctrl+Alt slot. Queued power commands
and panel catalog scopes are invalidated. The icon baseline remains retained,
and the first idle Tick repaints through the existing native setter, one slot
per Tick. A release-cleanup latch keeps a native drag flag that clears on a
later Tick from prematurely retiring an accepted assignment.

The substituted path does not run the original controller-definition writes,
tutorial callback or GUI sound. No guest mapping is saved across process
restarts. A valid icon lease and existing usable source icons remain required;
this does not initialize missing Ashley power-list icons or a missing movie.
Rejected drops and gestures outside a valid lease do not establish guest
remapping. Drag-to-remove and MoveByName integration are not implemented.
Three native targets compiled and current build fingerprints matched. No game,
mouse gesture, panel, network session or tests ran; rendered drop behavior,
ABI delivery, swap repaint and restoration remain unverified.

## Separate UI membership from cast readiness

The catalog implementation now has two explicit readers. The read-only
membership path validates the existing registered owned pawn/input, live
life state, native manager/channel/registration identities, full power names
and eligible rank/HUD membership. It does not require idle custom actions or
zero IgnoreLookInput. UI leases and drop-source validation use this path.
The command catalog wraps that membership read with idle action/power and
IgnoreLookInput checks before and after the read. Scene, combat getter,
native ownership and final dispatch checks remain unchanged.

An already-acquired UI lease can therefore follow real membership changes
during ordinary transient action/input-ignore intervals. Acquisition still
requires agreement with the published command catalog. The UI reader does
not publish command scope or grant casting permission. Membership consumers
keep their separate scene/ownership/life checks.

There is a broader unresolved native menu lifecycle gap. The retained SDK
names power-wheel mode3, and the owned ExitPowerWheel script independently
confirms DisableMode(3). The current session gate admits only mode0;
guest context adaptation retires a tracked registration when that predicate
fails. A full mode3 wheel therefore cannot be treated as supported by the
existing ordinary-mode drop candidate. This is a source-level incompatibility,
not a newly observed live failure.

Five owned wheel-mode scripts decoded completely: Activated280/412,
Deactivated387/563, EnableMovement29/37, ExitPowerWheel44/60 and ShowMenu21/25
(serialized/VM bytes). Their control flow includes PauseGame(true/false),
base activation/deactivation, ApplyTacticalOrders on deactivation and a
temporary IgnoreMoveInput timer path. These calls have their own conditions;
their presence does not prove a client pause was reached. Complete scripts
do not establish all nested native side effects. Source hashes stayed unchanged.

A proper menu integration must coordinate the actual mode3 instance,
pause behavior, retained ownership/context, UI lifecycle and close-time
tactical orders. Simply exempting every nonordinary mode or keeping a stale
lease would also affect conversations/cinematics. No mode, pause or native
game state was changed for this finding. Three native targets compiled for
the membership/readiness separation; no game or tests ran.

## Co-op command menus without command pause

The owner requests that opening the Shift power menu must not stop co-op
simulation. Owned BioWorldInfo.PauseGame decoded completely (68 serialized /
104 VM bytes). It casts WorldInfo.GRI to SFXGRI, then writes bPlayersOnly from
the supplied bPause only when that cast is null or bPauseForCommand is true.
Thus the game's existing SFXGRI command-pause policy provides a narrower
integration point than forcibly clearing WorldInfo pause state every frame.

The ordinary runtime now disables bPauseForCommand in a verified native host
or guest network world. The retained SDK places it at+0x378, mask4; current
reflection checks owner/type/offset/size/dimension and the exact boolean mask.
WorldInfo.GRI at+0x6a8, the live GRI/world identities and writable aligned
storage are verified. A CAS clears only that bit. Starting the policy requires
an ordinary unpaused frame: a menu that was already paused must retain its
stock close/unpause path instead of being stranded by a mid-pause policy change.

The baseline bit is retained and conditionally restored on disable/scope loss
only while the same current world/GRI still exists and the installed bit is
still clear. A disposed/replaced world is not written. Already-disabled native
policy requires no write/restoration. No PlayersOnly, Pauser, conversation,
input-ignore, animation or time-dilation field is cleared. The policy affects
the native command-pause route for the co-op session; it is not a global
replacement for all pause mechanisms. Missing/unverified GRI refuses policy
installation, so this is not guaranteed Shift-menu behavior yet.

## Retain an existing UI context in an unpaused power wheel

With the optional native icon candidate, a pre-existing owned icon/context
lease can now be retained in exact native power-wheel mode3. Current manager
and GameModes/CurrentMode reflection, the mode3 object's class/Outer, live
world/controller/pawn, registration token, channel/catalog and synthetic or
existing squad caches are verified. The ordinary environmental gate is shared
without altering the observed mode. Cinematic flags, PlayersOnly/pending,
Pauser, active conversation and unreadable state still refuse UI retention.

This exception skips context teardown only for an already-registered matching
menu lease; it does not initialize a context inside a menu. The icon/drop
consumers can use that read-only UI eligibility. Gameplay dispatch continues
to require mode0 and its original native combat/action checks. Other modes
and invalid menu identities still take scene cleanup. The command-pause policy
above is independent of optional icon/keyboard flags and runs for both roles.

All three native targets compiled and current source/build fingerprints
matched. No game, Shift input, pause transition, network session or tests ran.
Native close-time ApplyTacticalOrders, full power-list initialization and
two-player control/rendering through menu transitions remain unverified.

## Full-wheel selection uses group caches, not the icon pawn

Owned selection-path inspection decoded five exports completely, with matching
serialized/VM lengths and eight explicit branch targets aligned. Source
package/library hashes stayed unchanged. SelectCurrentWheelItem50/66 dispatches
mode1 to SelectCurrentPower and mode2 to SelectCurrentWeapon. Its native power
bridge resolves through registration name0x1379320/pair0x179d140/exec0xb638e0
to implementation0xc52930. These observations do not establish live delivery.

The implementation spans five contiguous unwind fragments:
0xc52930..0xc529d1,0xc529d1..0xc529f3,0xc529f3..0xc52a7c,
0xc52a7c..0xc52b46 and0xc52b46..0xc52be1. The first short fragment alone is
not the whole method. It obtains the selected caster and related power-index
array through virtual+0x508. The constructor-grounded PC wheel vtable resolves
that virtual to0xc4d740..0xc4d85a.

That helper validates selected index+0x780 against m_aPowerIcons count+0x464,
then searches the three integer-index groups at+0x3c4,+0x3d4,+0x3e4. Membership
in the first group selects cached m_pShepardPawn+0x710. The other two groups
are considered only with m_bProcessSquadHenchmen mask0x10 at+0x7f8 and select
cached henchman pawns+0x718/+0x720. It returns the matching group-array address
together with that cached pawn. It never selects the caster from the icon's
pPawn+0x19c or the controller's current Pawn directly.

SelectCurrentPower takes the chosen icon from m_aPowerIcons, checks its native
state, then processes other group icons in state3. Helper0xc5d170 dispatches
CancelLastOrderedPower with the chosen caster and each relevant power name;
the event name is grounded in global0x17a8700, name-entry0x13efbae. The native
path later calls the already identified SquadOrderUsePower0xaa38b0 directly,
using the selected icon's full power name and the group-derived caster.
It does not pass through the optional doHotKey queue bridge. The selected icon
is then activated. Activate's native leaf0xbbbd60..0xbbbd6f only marks dirty and
sets desired state3; adjacent code in a fallback512-byte window is unrelated.

The consequence is concrete: retaining a registered Ashley context, correcting
quickslot icons or replacing a selected icon's power/pawn is insufficient to
establish the full-wheel caster. The cached squad/pawn bootstrap and integer
groups must agree with the owned power list, including all related pending
orders. Blindly returning Ashley from the pawn helper could cancel or issue
powers belonging to another group. A proper adapter must validate and scope
the whole selected group and its initialization/restoration lifecycle.

No caster helper, power-list group, cached pawn or tactical order was changed
for this research. Existing quickslot/drop and command-pause candidates remain
as previously compiled. Full-wheel selection and close-time tactical order
delivery remain incomplete and unverified; no game or tests ran.

## Owned full-wheel caster/group candidate

The optional icon lease now requires a checked forwarding hook at the PC
wheel caster virtual+0x508, whose constructor-grounded target is0xc4d740.
Installation checks the original slot and entry bytes and uses pointer CAS.
Other callers/wheels, inactive leases and disabled runtime retain the stock
helper. The substituted path accepts only the exact SelectCurrentPower call
returning at0xc5295e. It makes no native gameplay/UI call itself.

For that leased guest selection, the adapter checks current menu/ordinary UI
eligibility, world/controller/pawn/registration identities and the current
eligible power catalog. It validates the reflected selected-index, power-icon
array and three-index-group struct layouts, bounded signed indices and a
single group containing the selected index. The native henchman-group policy
is still respected. Empty henchman groups are valid; unreadable or malformed
groups are not treated as empty.

Every icon in the chosen group must be a current native PowerIcon belonging
to this wheel, with a unique eligible full power name and exact current power
instance. Its pPawn and the action's m_oPawn/m_oPC/m_oAI must agree with Ashley,
the current guest controller and no AI controller. Duplicate, foreign, stale
or incompletely rebound entries refuse the group. Headers, index values,
icon/power identities, caches, selection and current UI context are rechecked
before returning the owned pawn and the existing native group-array address.

The adapter does not rewrite group arrays or cached story-pawn fields. Native
SelectCurrentPower retains its normal state, cancellation, targeting, tactical
order and icon-activation flow after this return. Because the entire group is
owned, its cancellation pass cannot knowingly consume a foreign squad member's
power names. The mod's explicit command queue remains mode0-only; this helper
instead supplies context to the game's existing native wheel order path.

This closes one caster-selection integration point, not full-wheel readiness.
The existing full power-list icons/groups must already contain the owned Ashley
powers; this change does not initialize or convert a Shepard list. Without
that prerequisite the helper refuses rather than pretending the wheel is ready.
The hook remains forwarding/resident until exit, and a replaced hook retires
the optional icon lease. Three native targets compiled and build fingerprints
matched. No game, selected power, cancellation, cast effect or tests ran.

## Host power menu retains an existing guest lease

Disabling command pause alone did not cover host ownership policy: the shared
host loop still released a guest whenever the local host left mode0. The
ordinary runtime now supplies a read-only retention predicate for a verified
host power-wheel mode3, only while a guest lease is already active. Admission
of a new guest remains mode0-only. The protected probe leaves the predicate
unset and keeps its original scene policy.

Retention requires the same live host/Shepard/world/story squad, a healthy
active lease without pending cleanup/fault, admitted listen-server scope,
the verified current SFXGRI command-pause policy still disabled, and the actual
mode3 object's class/Outer in the current reflected mode-manager array. The
ordinary environmental checks still reject real pause, PlayersOnly/pending,
cinematic flags, active conversations and unreadable state. Array/mode and
environment are checked again before returning eligibility.

The host loop still performs its normal travel, story, disconnect and reciprocal
guest ownership checks. Equipment startup receives ordinary=false during this
menu; retaining ownership does not authorize another equipment initialization.
Other scenes still release. Three native targets compiled and fingerprints
matched. No game, Shift transition, guest movement/combat or tests ran.
