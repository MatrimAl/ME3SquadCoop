# Optional native guest input initialization

Status: compiled candidate only. No input initializer, profile fixture, game,
keyboard/gamepad action or gameplay test was executed. Normal multiplayer-style
controls remain unverified.

`ME3_SQUAD_COOP_INPUT_BOOTSTRAP=1` opts an ordinary guest into a narrowly scoped
initializer. The session planner's guest-only `--input-bootstrap` flag emits it;
the default is off. The protected probe does not include this initializer.

The trigger is a genuinely empty native PlayerInput.Bindings array on the
currently possessed Ashley, with a registered guest player context. It does not
reset a populated list because a specific action failed. The current local
ownership, client channel, ordinary game mode, input permissions, action/life
state and unpaused world must satisfy existing checks. The initializer never
calls EnableInput, EnableMode, DisableMode or GotoState.

The controller's current BioPlayerInput must belong to that controller. Its
SFXPRI must be live in the current world. ProfileSettings must be an existing
SFXProfileSettings instance with a bounded readable native profile array.
Reflected input, PRI, profile and manager property layouts must match. The
native UpdateInputConfiguration ABI,374-byte expanded script size, ProcessEvent
entry and LoadPCInputConfiguration virtual target must match the inspected
contracts. See [LEMP reference and native loader evidence](LEMP-CONTROL-REFERENCE.md).

The same object identities, current channel/connection/driver, registration
token, empty binding header, profile header and profile-row
fingerprint must persist across four samples at250ms cadence and at least750ms.
An unavailable sample or gap above1 second resets that window. A final complete
scope read must agree immediately before dispatch. The fingerprint is local
transient state and is not logged or persisted.

One attempt is consumed before native execution for each observed
controller/input/manager identity combination. Scene or scope loss does not
rearm an old attempt, nor does an empty post-call result. The process retains
up to128 attempts without eviction; exhaustion stops further initialization.
It does not automatically retry or rebuild a native input object.

The call is the ordinary BioPlayerController.UpdateInputConfiguration path,
which resets and reloads native mappings through the existing profile and PRI
configuration. It also updates native mouse options. The mod supplies no custom
bindings, profile rows, target pawn, input event or replacement settings. It
does not call SavePCInputConfiguration or directly write profile files. Nested
native parser behavior and preservation of actual user rebinds still require
protected gameplay validation.

Execution is pre-Tick, after the context/collision adapters and before the final
power stage. The latter resolves the session/scene again before consuming any
power request. The diagnostic records that the initializer was dispatched and
whether a bounded post-call binding array can be read. A positive count does not
prove key delivery, a completed action, combat eligibility or an actual cast.
No panel protocol or current input bindings are changed by merely enabling the
candidate in a generated plan; the plan itself never launches a game.
