# Guest controls after weapon and power-slot changes

The stock X alias reaches `SFXGameModeDefault.TrySwapWeapon`, which casts the
controlled pawn to `SFXPawn_Player`. Owned Ashley therefore receives no swap.
The guest adapter forwards that original script once, then dispatches stock
`NextWeapon` only if the actual default-mode receiver, pawn, controller,
inventory, equipped weapon, world, connection/channel and registration token
remain unchanged. Stock combat, cover, action, equipment and network checks
remain in `NextWeapon`. No key is generated and WeaponOnDeck/ammo are not
written. `ME3_SQUAD_COOP_WEAPON_SWAP=0` disables the addition.

Native `MoveQuickSlotPowerByName` searches saved Shepard definitions before
calling the indexed assignment body. The guest bridge instead resolves its
already-decoded power name against the current owned catalog and process-local
guest slot map. The exact caller, native entry and current UI lease are
validated. It leaves saved controller/profile definitions untouched and
repaints through the existing native icon path. Ordinary indexed drops keep
their verified source-icon checks. A structurally valid rejected drop keeps
the last good map instead of invalidating every hotkey; foreign/replaced
widgets still retire their own lease.

The owner confirmed X and power use after weapon switching in `4ff0a2d`.
Moving the power still failed: four indexed requests were refused and the
new slot remained unbound. The next candidate handles decoded `NewSet`
requests before their native destination clear, leaving the final icon update
to the existing idle painter. It records individual refusal conditions rather
than combining caller, flags, menu and catalog failures. This correction has
not been checked in gameplay.

The normal key queue now uses native `DoingDrag` as its drag blocker. A stored
source index with that bit clear is permitted by stock MouseUp and hotkey
handling; it no longer indefinitely blocks queued numbered input. Actual
dragging and unreadable state still block. No native drag field is cleared.

Catalog identity does not include the weapon. The change
does not invent a cause or bypass native cooldown, rank, ammunition, action
or targeting restrictions. Bounded hotkey refusal context now records the
actual slot, current action, native drag state, scope and equipped weapon.

A rare close camera during a power was also reported. Command mode5 was
restoring the existing shoulder lease; an immediate power after leaving the
menu then prevented idle-only acquisition. The next candidate retains an
already-held scale only through the existing verified, unpaused guest
PowerWheel/Command context. It still cannot acquire there or bypass an
actual scene, life/ownership change or native camera policy.

## Metadata cost

The pinned retail build has immutable UClass inheritance and interned names
within an object's lifetime. A bounded positive layout cache avoids repeated
linked-list discovery after an initial five-second full-scan phase. Every hit
rechecks GObjects membership, concrete/declarer/kind/Class identities,
inheritance Super/Children heads and field owner/name/type/link/offset/size/
dimension. Failed checks fall back to the original resolver; failures are not
cached. A derived property kind also retains the original resolver. This is
not an editor/hot-reload metadata cache. Set
`ME3_SQUAD_COOP_LAYOUT_CACHE=0` for the original traversal.

`runtime_layout_profile` emits cumulative full-scan/cache counts and QPC CPU
time every five seconds, with bounded output. Frame-delta samples from
WorldInfo are CPU-side observations, not GPU or Present timing. No FPS gain
is claimed until the new code is measured during comparable gameplay.
In `4ff0a2d`, the owner reported smoother play and the host's measured layout
helper averaged about136us before caching versus17us after. Whole-frame
measurements still varied; a stable frame budget is not established.

## Gameplay checks

1. Use X repeatedly from idle and aim; verify both equipped weapons and normal
   draw/holster, firing and reload behavior on both peers.
2. Use a ranked, available power before and after switching weapons.
3. Move that power to another numbered slot, use its new slot, then move it
   back. Closing/reopening Shift should preserve the guest mapping.
4. Compare evade camera angles to Shepard and confirm real scenes still
   release/return control.
5. Compare performance in the same area while the native timing log records
   its baseline and cache phases. A native fixture alone proves no FPS result.
