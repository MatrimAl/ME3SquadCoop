# Progress

## 2026-10-24: stop skipping the witnessed post-scene QA firing case

- Retained real-game logs show the fire bot refusing an acknowledged guest
  pawn with local Role2, RemoteRole2 and a simulated Role1/RemoteRole3 weapon.
  The matching paired sample has the native no-fire bit clear. The rejection
  follows the existing ownership, channel, context and equipment guards.
- The QA bot had incorrectly required the pawn's RemoteRole to remain3.
  It now accepts the witnessed counterpart values2/3 while requiring local
  autonomous Role2. Simulated/authority/unowned local pawns, unverified
  counterpart states and the native no-fire permission remain rejected.
  Weapon role requirements and preceding scope guards are preserved.
- The old predicate failed the new offline regression; the corrected predicate
  and existing native DLL harness passed. Three compiled negative controls
  rejected the old RemoteRole filter, simulated local pawn and ignored native
  no-fire permission. This changes QA input eligibility,
  not native roles, weapon state or ordinary player firing behavior.
- The prior skipped scene-return firing case still needs a live run. No
  successful damage, repeatable combat or origin-position repair is claimed.

## 2026-10-24: renew the guest combat context after a same-pawn suspension

- Fixed a local context generation bug: scene/disabled cleanup could remove the
  guest's registered combat context while the same acknowledged pawn remained
  possessed. Ownership-only generation tracking then considered the cleaned
  record current and returned without registering a replacement after gameplay
  resumed. The gate generation now observes enabled ordinary control as well as
  pawn/controller/world identity, allowing the existing cleanup/rebuild path to
  run once on return. It does not alter native ownership or pawn coordinates.
- A temporal native regression failed with the previous ownership-only policy.
  The corrected policy passes repeated held frames, scene and disabled intervals
  with unchanged pawn ownership, return from both intervals, actual ownership
  loss and a pawn change. Both compiled negative controls were rejected: the
  old ownership-only policy and a policy advancing on every held frame. The
  existing offline DLL harness also passed.
- Pending cleanup still finishes before a replacement may be created. Failed
  creation remains suppressed within a control scope; held and blocked frames
  do not repeatedly advance generations or spawn actors.
- This fixes a code path that could leave native combat input context absent.
  No game was launched, so successful scene recovery, shooting, powers, target
  damage and the separate origin-position discontinuity are not claimed fixed.

## 2026-10-24: keep control yielded when dialogue state cannot be read

- Fixed a shared host/guest scene gate that treated failed cinematic-flag and
  conversation reads as zero. An unreadable controller flag, conversation
  pointer, active count or activation flag now keeps control yielded; a negative
  conversation count is also rejected. Readable ordinary gameplay and actual
  scene handling retain their existing behavior. No story/role/position writes
  or speculative coordinate correction were added.
- New native self-test controls failed against the previous gate and passed
  after the fix. All four compiled scene-gate mutations were rejected. The
  existing offline DLL harness passed, including its private low-integrity
  child. This is source-level error-path validation; it does not
  prove this read failure occurred in the retained falling run.
- No game was launched or attached. Reliable movement, hostile damage and full
  dialogue still require later real-game verification with the owner's consent.

## 2026-10-24: expose movement failure beside lifecycle results

- Offline evaluation of the retained failed pair using the existing verifiers
  reports lifecycle acquisition/release/restoration true but common travel
  false and unsafe motion true. Reasons include a below-KillZ sample, sustained
  vertical drop and excessive downward velocity. The maximum observed replica
  position error is about54,025 units. No new game or process access was used.
- The concise pair summary now exposes the existing common-travel verdict and
  unsafe-motion observation beside the lifecycle result. It does not change
  lifecycle semantics, relax movement gates or claim campaign validation.
- No additional movement verifier was added: the existing verifier already
  rejects this real regression. The receive diagnostic remains untested live.

## 2026-10-24: bounded receive observation prepared; no new game test

- Added default-off POSITION_RECEIVE_TRACE diagnostics to the existing protected
  ActorChannel receive wrapper. It observes only the current-world guest Ashley,
  logs at most64 large position or ownership/base changes, and compares stable
  object identity before/after the unchanged native receive call. It does not
  consume packet fields, change position or modify the ordinary player ASI.
- The protected probe can explicitly enable this existing receive hook during
  collision-baseline QA without enabling the separate package-map trace.
- An actual-wrapper fixture on authored memory passed default-off forwarding
  and12 enabled cases, covering position/role changes, identity replacement,
  nonfinite values, record budget, missing marker, wrong role/thread/world and
  unrelated actors. Each case checks one original call, preserved packet/channel
  bytes and incoming/returned LastError. This is offline observer validation,
  not evidence that the game's receive function caused the discontinuity.
- Live testing is deferred at the owner's request. No gameplay correction or
  successful movement/combat claim is made by this instrumentation.

## 2026-10-24: synthetic handoff exposes a position discontinuity

- A new protected pair completed three ownership epochs and AI/disconnect
  restoration, but failed gameplay continuity. The guest Ashley moved about
  54,107 units toward the world origin within 94 ms of the synthetic scene
  release. Its sampled controller detached and network roles changed to zero.
- About 594 ms after the next host acquisition, the host Ashley also jumped
  about 54,025 units toward the origin, lost its previous world base and fell.
  The guest discontinuity preceded the host discontinuity. This establishes
  ordering, not which engine call or packet caused either change.
- The same tracked ActorChannel identity/index survived the initial guest
  jump. Its retained SavedMove still held the original world coordinates;
  after reacquisition a newly generated move held the bad origin coordinates.
  Channel closure and stale saved moves are not established as the initiating
  cause. Receiver handling of unowned pawn location/base state needs inspection.
- No collision repair ran. The discontinuities occurred before the attempted
  ordinary W input, so that key is not their initiating cause. Normal key
  delivery remains unproven; no successful walking or combat claim follows.
- The paired read-only recorder now captures validated PlayerInput identity,
  pressed keys and axes, explicitly labelled asynchronous. Its bounded capture
  starts at actual acquisition so loading cannot consume the useful window.
- The lifecycle verdict is insufficient for gameplay continuity. Originals and
  profiles remained protected, native exceptions were zero and no game remained.
  The memory helper exited1 without a final summary; this is not an unqualified
  passing observer audit. Investigate release/channel/movement state across the
  handoff before broadening collision repair or adding position corrections.

## 2026-10-24: correct the QA input sampling phase

- The input-state record is sampled inside input_bot before session_guest_frame
  writes its movement pulse and before the original engine Tick. Its former
  post_tick_axes name was misleading; zero values do not prove that the pulse
  was overwritten. The record now explicitly names its pre-input/pre-Tick phase.
- The actual W mapping is a command alias chain to Axis aBaseY Speed=1.0.
  Retained pulse-window pawn samples show PlayerWalking, grounded physics and
  no sampled freeze, attachment or move-input suppression. None establishes
  that a normal key reached the input consumer. Distinguish ordinary key
  delivery from the raw-axis QA pulse before changing gameplay state.

## 2026-10-24: mode-0 landing dialogue returns; movement still unresolved

- The corrected protected pair completed three ownership epochs and native
  disconnect/AI return. Its landing conversation was active in 128 valid paired
  samples spanning 13.375 seconds, with both PlayersOnly and the custom focus
  owner flag false. Guest control returned after this scene. This is a mode-0
  scripted landing exchange, not a full mode-7 conversation test.
- No collision repair ran: the observed returned physical shape retained both
  actor/world and blocking collision, while its type fields were custom0.
  Do not expand the repair predicate solely to force an attempt.
- The guest positions were stationary within all three ownership epochs despite
  the QA movement pulse. Input delivery/processing must be distinguished from
  an actual movement restriction before changing collision or movement flags.
- Native exceptions were zero, original protection covered 8,210 files and
  unchanged profiles, no forced/unknown cleanup occurred and no game remained.
  The lifecycle runner returned0, but a process-exit monitor permission error
  made the overall wrapper fail. The observer audit is not an unqualified pass.

## 2026-10-24: focus restoration observed; mode-0 dialogue guard corrected

- The protected host logged an actual native HasFocus result0 adapted to1 at
  the second verified Tick site, cursor280 to281. Paired records then observed
  its focus-owned pause clear and Ashley's first ownership epoch begin. This
  proves the scoped getter path executed; it does not prove campaign support.
- A later mode-0 landing conversation again had PlayersOnly and the custom
  focus-owner flag enabled. The candidate's conversation exclusion prevented
  that focus pause from being restored, stalling the scene. No collision repair
  or usable scene return was demonstrated in this run.
- A new actual-wrapper regression reproduced this refusal. The focus candidate
  now permits its verified mode-0 Tick caller during dialogue. Possession/input
  handoff guards are unchanged, and a dialogue pause without focus ownership
  still refuses adaptation. The fixture passes 26 cases after failing the new
  dialogue case before the correction. Positive live dialogue return is pending.
- The game run ended with no native exceptions, unchanged original files and
  profiles, no forced cleanup and no remaining game. A process-exit read-only
  monitor permission error prevents calling its overall observer audit clean.

## 2026-10-24: retained blocking bit measured on control return

- The focus candidate installed in both protected game processes. Its first
  actual Tick cursor advanced from71 to72 as predicted. The pair acquired Ashley
  in three ownership epochs and completed scene handoff/disconnect checks, but
  no adapted focus result was logged; focus correction is still unverified.
- Guest control returned with world collision enabled, actor collision absent,
  blocking retained, cylinder response0xaa and type0/replicated0. The paired
  recorder captured 87 valid owned/acknowledged samples on the same positive
  channel. The prior predicate rejected this partial shape, so no repair ran.
- The candidate now admits that one additional measured shape. Its new native
  regression failed before the change and passed afterward, including both
  values of the unrelated adjacent flag. Existing identity, release snapshots,
  channel, ownership and native-call guards are unchanged. All three outputs
  rebuilt, the native protection harness passed and four unsafe variants were
  rejected. A positive live repair and usable walking remain unverified.
- The completed pair recorded zero native exceptions, unchanged originals and
  profiles, no forced/unknown cleanup and no remaining game. UI, paired and
  baseline recorders completed; the memory helper lacked a final summary.

## 2026-10-24: custom focus pause confirmed before possession

- A protected pair measured 39 consecutive host samples over 10.032 seconds
  with PlayersOnly and BioWorldInfo's saved focus-pause flag both enabled,
  no host foreground focus, a stable world and reciprocal guest membership.
  The guest had focus and both pause flags disabled.
- The actual Tick script checks its own HasFocus result and uses PauseGame to
  enter and restore this pause. The disabled stock Engine/HUD focus option
  does not control this SFX path. No browser-wheel or pause flags were changed.
- HasFocus reflection and native bytes matched in both processes. The narrow
  caller-scoped candidate now builds in both entry paths, and its actual-wrapper
  fixture passes 24 positive/negative cases. The native protection harness also
  passes. It has no positive live result yet.
  See [background focus](BACKGROUND-FOCUS.md).
- This pair still never possessed Ashley. Original protection covered 8,210
  files; profiles/source/build were unchanged, native exceptions were zero,
  no forced cleanup was needed and no game remained. Both read-only UI and
  paired recorders completed. The memory helper lacked a final summary.

## 2026-10-24: pre-possession pause identified independently of collision

- The existing bounded read-only paired capture now starts from verified probe
  headers instead of waiting for Ashley possession. No game behavior changed.
  In 738 valid host and 739 valid guest snapshots, the host consistently had
  `WorldInfo.bPlayersOnly` enabled while the guest did not. Both had an unset
  PlayerController cinematic bit and zero active/activating conversations.
- A separate read-only UI capture found a current-world HintBox message handler,
  but its active, modal and force-PlayersOnly flags were all false. That object
  is not evidence of an active owning modal and does not justify dismissing it.
  The pause setter and its correct normal restore path remain under investigation.
- The native connection persisted, but Ashley was never acquired, so the latest
  collision candidate was not exercised. This run provides no collision,
  movement or combat pass. No world flags, UI state or collision were forced.
- The paired recorder completed. Original-file protection covered 8,210 files;
  profiles and the frozen source/build were unchanged, native exceptions were
  zero, no forced or unknown cleanup was needed, and no game remained. The
  memory helper ended without its own final summary; this is not complete
  memory-observer coverage.

## 2026-10-24: measured partial collision return; native mask corrected

- The paired recorder now includes collision type and cylinder response from
  buffers it already reads. It captured 323 valid owned samples after control
  return with world collision enabled, actor/blocking collision absent, cylinder
  response0xaa and type0/replicated0, on the same positive native channel.
  The previous fully-disabled predicate rejected this stable partial state.
- The candidate admits only that additional exact shape. Native type6 restores
  actor/blocking collision while preserving an already enabled world flag;
  the world compare-exchange runs only if that flag is missing.
- Reflection identifies Actor.bBlockActors as mask0x1 at the captured field.
  The old mask also included neighboring0x2, which the native type6 path does
  not restore. That unrelated flag is now excluded from the physical shape and
  preserved. The native fixture's dispatch stand-in was corrected to match the
  actual native writes instead of manufacturing that neighboring flag.
- The fixture passed both neighboring-flag values with world collision already
  present, plus the prior release/identity/ABI refusal cases. Positive live
  collision repair and shared walking still need verification.
  All three native outputs built, the probe harness passed, and four compiled
  negative controls were rejected, including reintroducing the neighboring mask.
- The measurement run recorded zero native exceptions and kept original files
  and profiles protected. Shutdown observation had a permission error; retained
  paired capture completed, but the overall audit is not an unqualified pass.
  A separate read-only mesh capture observed animation ticking enabled throughout;
  it does not justify changing render flags or claim to solve firing.

## 2026-10-23: walking probe exposes stale collision-release state

- A measured protected pair reached actual landing control return and the
  separate synthetic handoff, with guest input after reacquisition. Native
  exceptions were zero, but the repair was refused and usable synchronized
  walking remains unverified. Lifecycle success is not physical gameplay proof.
- The retained-response candidate was observed live with Actor0/block2/cylinder
  0xaa/type0/replicated0 during release. Later paired snapshots changed again
  before reacquisition. Static review found that a subsequently rejected shape
  cleared `pending` but left `released` set. An actual-adapter regression failed
  on that stale flag before the correction. Both flags now clear together;
  refusal reasons identify the existing failing check without extra engine calls.
  A shape mismatch records the already-read before/expected values once.
  All three native outputs rebuilt; the probe harness and actual-adapter
  lifecycle fixture passed, including cancellation and a later valid repair.
- The memory observer measured only 277,741,568 bytes of available system commit
  during startup, with both games allocating roughly 4 GB each. The same run
  reached gameplay without a native exception. No matching System event2004
  was found; memory exhaustion is a lead, not an established crash cause.
- Original-file/profile protection and native cleanup passed; no game remained.
  A process-exit permission error affected the read-only monitor and the memory
  helper exited1 without a final summary. Retained samples remain partial evidence, not a
  claim of complete observer coverage. No further live rerun was made here.

## 2026-10-23: landing return retains a second collision shape and RemoteRole0

- A protected pair completed the actual landing release/reacquisition and the
  separate synthetic handoff. No guest sample fell below KillZ while stationary;
  no collision repair ran. This is not a walking or collision-repair success.
- Read-only paired capture measured the same guest pawn/controller/acknowledged
  pawn and positive channel after control return with LocalRole2/RemoteRole0.
  Its stable released collision was Actor0x20000000, block0, cylinder0x12,
  type8/replicated8: actor collision remained enabled while world/blocking
  collision did not. The previous predicates rejected both this shape and role0.
- The candidate now admits only that exact additional released shape. RemoteRole0
  requires a previously captured healthy baseline; local autonomous role and all
  ownership, world, camera, channel, stable-release and native ABI gates remain.
  The actual adapter fixture passed recovery and neighboring refusal cases;
  role bytes remain unchanged. Live repair is still unverified.
- The existing protected QA mode now permits its existing two-second guest input
  pulse after reacquisition, with following/firing disabled and the host route
  still parked. No new QA mode or timer was added. Physical movement must be
  measured separately from successful possession.
- The stationary run recorded one access violation after ordinary disconnect,
  outside both authored probe and game image. A later Windows error record,
  matched by PID, creation time and executable path, attributes the fatal fault
  to NVIDIA `nvwgf2umx.dll` at RVA0x59a996. A retained live process handle did not
  prove continued game execution. Root cause remains unknown. The final audit verified 8,210 original-file
  locks, unchanged original profiles/source/build, disabled probe/restored action
  and complete cleanup with no forced, unknown or remaining game processes.

## 2026-10-23: retained cylinder response measured; narrow correction prepared

- The released-shape record measured cylinder response0xaa while the engine
  changed Actor collision/type. The former predicate required the cylinder's
  actor-collision bit to clear too. It now also accepts the already validated
  default response0xaa, while still requiring Actor mask0, a stable released
  state and all existing ownership/context/channel/native-ABI guards. Partial
  states retaining world collision remain rejected. Live repair is unverified.
- This run captured partial Actor states with response0xaa; the earlier run
  captured complete Actor collision loss without a simultaneous cylinder read.
  Their combination remains a candidate, not a jointly observed live repair.
  All three native outputs rebuilt; the native probe harness and collision
  lifecycle fixture passed, including retained-response recovery and refusal
  cases. These offline checks do not establish working in-game collision.
- Exact native `SetCollisionType` disassembly writes both type bytes and calls
  `UpdateCollision` unconditionally. No force-toggle or extra native method is
  necessary merely because the current physical type already equals6.
- The measurement run failed on its second acquisition with a guest access
  violation in the game executable at RVA0x72b6c. A Windows error record matches
  PID, creation time, executable path and offset. Cause remains unknown; no
  collision repair was called before the fault. Exact authored ASI/PDB retained.
  Final audit verified8,210 locks, unchanged profiles/source/build, disabled
  probe, restored action and complete cleanup without forced termination.

## 2026-10-23: actual landing transition reproduced falling; repair still blocked

- The route reached its actual landing scene, released Ashley to AI, reacquired
  her, and parked at waypoint3. The existing later synthetic handoff produced a
  third acquisition. Native ownership/story restoration and ordinary disconnect
  checks passed, but gameplay failed: guest epoch2 descended below KillZ.
- Paired snapshots show guest actor collision mask0/block2 and roles2/2 after
  the real scene. The adapter had accepted the initial healthy baseline but
  made no repair attempt. The RemoteRole predicate correction alone therefore
  did not resolve falling. Component response/type values during release are
  still needed to identify the remaining physical-shape rejection.
- A small, rate-limited record now logs the released shape already read by the
  adapter, including whether its existing disabled-shape predicate accepts it.
  It performs no new engine calls, physical writes or predicate relaxation.
- Recorded native exceptions and observer errors were zero. All8,210 original
  locks, unchanged profiles/source/build, disabled probe, restored action and
  complete cleanup without forced game termination were verified. The runner's
  passing lifecycle result is not a passing movement/collision result.

## 2026-10-23: stationary handoff passed without reproducing world-collision loss

- The stationary pair accepted a baseline, completed a30-second first lease,
  synthetic release and second acquisition, then restored AI/story state on
  ordinary disconnect. Guest ownership/camera returned for roughly25 seconds.
  The guest retained world collision (`0x40000000`) and local autonomous Role2;
  RemoteRole remained3. No repair attempt occurred. This does not exercise the
  historical fully disabled collision shape or the new RemoteRole2 case.
- Recorded native exceptions were zero. Final audit verified all8,210 original
  locks, unchanged profiles/source/build, disabled probe, restored action and
  complete cleanup with no forced game termination. The monitor again recorded
  an access-denied error, so the broad runner's green lifecycle result is not
  evidence of a clean overall QA pass or working collision recovery.
- Stationary QA now parks at the second acquisition. This lets ordinary route
  input reach the actual landing scene before parking, while retaining the
  existing later synthetic handoff. The earlier failing capture combined these
  two transitions before world collision disappeared. Guest bot input remains
  disabled; no extra timer, mode, role write or partial-shape repair was added.

## 2026-10-23: healthy baseline accepted in-game; stationary recovery prepared

- With the existing extended QA deadlines, Ashley was acquired and the native
  collision adapter accepted her healthy baseline on channel27. A creation-pinned
  read-only snapshot independently captured the variant/sentinel shape. The
  frozen snapshot helper still evaluates the old predicate, so its subset
  rejection is expected and is not the current adapter's verdict.
- Host and guest each produced 60 ownership samples, covering59 seconds.
  Ordinary disconnect restored AI, preserved the host story roster and removed
  native driver membership. No native exception was recorded. The monitor
  encountered an access-denied error; this is not a clean overall QA pass.
  Final audit verified8,210 locks, unchanged profiles/source/build, disabled
  probe, restored action, no forced cleanup and no remaining game process.
- The collision predicate now also accepts the observed guest RemoteRole2
  while requiring local autonomous Role2 and the existing owned/acknowledged
  client context, channel, scene and identity checks. Neither role is written.
  Local simulated/authority roles and other remote values remain rejected.
  Real-adapter offline cases passed; live recovery is still unverified.
- Stationary QA keeps the route parked and guest bot input disabled, while
  restoring the existing timed scene/reacquisition policy. It can now exercise
  recovery without adding another mode or running the later route/cinematic.

## 2026-10-23: focused rerun ended without guest acquisition

- The reduced-observation run completed with zero recorded native exceptions.
  Host listen mode and sustained guest client mode were observed, but Ashley
  was never acquired. Consequently neither the corrected collision baseline
  nor a live collision repair was exercised. Client mode alone is insufficient
  evidence of playable co-op.
- Full timeline review corrects the initial tail-only interpretation: one client
  was present while the host control gate stayed blocked. Discovery later ran
  after the guest's180-second lifetime had ended. This does not establish a
  missing-controller fault or a fix for the preceding access violation.
- All 8,210 original-file/profile locks remained held through child cleanup;
  profiles and frozen source/build were unchanged. The probe was disabled,
  the action restored, and no forced cleanup or remaining game PID was recorded.
- Current priority is reliable squadmate acquisition, collision after control
  return, and authoritative combat. Cross-PC Radmin and packaging are deferred.
  Prefer a small measured gameplay correction over additional infrastructure.

## 2026-10-23: baseline rerun failed during join; focused diagnostics reduced

- The baseline correction rerun failed before guest acquisition or baseline
  evaluation. Windows recorded a guest access violation in `VCRUNTIME140.dll`;
  the probe recorded 174 guest and three host first-chance access violations.
  No live baseline or repair claim follows from this run. The cause remains
  unknown; the collision adapter had not reached its eligible path.
- The focused probe now leaves package-lookup and initial actor receive
  observation hooks uninstalled. Neither is needed for the first-lease shape
  measurement; native package validation and channel handling run directly.
  This removes diagnostic interference without claiming it caused the fault.
- Original-file locks, unchanged profiles, disabled probe and complete process
  cleanup were verified. No forced launcher/child cleanup was recorded.

## 2026-10-23: first-lease collision refusal reproduced and corrected

- The focused run stopped the route at waypoint2 and held Ashley for 59.36
  seconds on the host before a normal guest disconnect. AI and the host story
  roster were restored. No native exception occurred. This stationary scope
  does not validate combat, scene reacquisition or the earlier Bink failure.
- A creation-pinned, read-only snapshot captured the exact rejected baseline.
  Actual and class-default physical masks were identical: actor `0x60000000`,
  block `3`, component `0xaa`, collision type `6`. Both stored replicated type
  `10` (`COLLIDE_MAX`), the initial sentinel rather than a repair argument.
  The component used the Ashley combat variant's archetype, not the class
  default component directly. The former checks incorrectly rejected both.
- Baseline admission now accepts the observed replicated sentinel and pins the
  actual component/pawn archetypes, requiring matching classes and the component
  archetype's ownership by that pawn archetype. Physical equality with the class
  default remains required. Repair still dispatches the captured physical type6;
  scene, ownership, channel and role checks remain unchanged.
- All three native outputs built. Probe/runtime harnesses, real-adapter offline
  lifecycle cases for variant/sentinel and their refusal cases passed; three
  compiled physical-mask/CAS negative controls were rejected. A new live run
  must establish that the corrected baseline arms; no live repair is claimed.

## 2026-10-23: crash module identified; focused first-lease observation prepared

- A Windows Application Error record matches the failed host's PID, creation
  time, executable path and exception RVA exactly. The faulting module was
  `bink2w64_original.DLL`, at RVA `0xc8fe`. This establishes the module, not the
  cause or absence of an upstream caller error. Exact disassembly identifies
  an asynchronous Bink worker-context table dereference on that path. The
  proxy forwards the required imports by name; proxy and original DLL hashes
  match the protected copy's baseline. No DLL replacement is justified.
- A new default-off, protected-probe-only `COLLISION_BASELINE_QA` observation
  stops the host route at its first actual acquisition, suppresses guest bot
  movement/fire and the synthetic scene, and permits only that first lease.
  Native scene/disconnect cleanup remains active. It preserves all acquisition
  and collision guards; the purpose is to measure the rejected class/template
  shape before movement or reacquisition confounds it. This focused test is
  not a combat or complete lifecycle validation.
  All three outputs built and the native probe policy/isolation fixtures
  passed. The private capture wrapper's trigger/provenance tests also passed;
  no live focused run has exercised the new policy yet.

## 2026-10-23: three acquisitions; collision refusal and host exception measured

- The next completed protected pair acquired Ashley three times. Cached and
  current-world drivers matched with one client; remote discovery and native
  membership passed. Earlier blocked samples were ordinary scene gates, not
  evidence of a stale driver. Ownership predicates were not changed between
  the preceding failed acquisition and this run.
- The first lease released to AI after 6.469 seconds during an engine scene;
  the second released after 9.516 seconds through the explicitly synthetic
  gate. Both preserved the host story roster. The third lease had at least
  6.547 seconds of host observations before a native access violation. Clean
  final release/disconnect and combat did not pass.
- Collision baseline admission was refused twice inside its class/template/
  shape checks during the first lease. Later guest samples changed from
  RemoteRole3/Role2 to 2/2, which separately fails the candidate's ordinary
  state requirement. Falling and crossing Kill Z preceded the host exception.
  Neither relaxing role guards nor claiming collision repair is justified.
- The wrapper's incremental observer and final audit completed with correct
  role headers. Original-file locks covered 8,210 files; profiles were unchanged,
  no forced launcher/child cleanup occurred, the probe ended disabled and no
  game remained. One host access violation makes the gameplay run a failure.
- The old exception record lacks a timestamp and module identity; an RVA alone
  cannot attribute it to this ASI. The probe now records monotonic time,
  allocation-query validity and cached game/probe module-base comparisons,
  without loader/symbol lookup or exception recovery in the handler. The next
  real run must establish attribution; this observation change fixes no crash.
  All three outputs rebuilt and the probe harness passed the attribution,
  incomplete-parameter and isolation fixtures. No game has run with the new
  exception metadata yet.

## 2026-10-23: research connection reproduced; squad admission still blocked

- The next protected research pair omitted the ordinary activation DLC and
  runtime ASI. Native listener/client modes and a sustained connection were
  observed, but Ashley was never acquired. The route remained at its ordinary
  host-context gate. Collision baseline/repair, firing and control return were
  therefore not exercised.
- The cached host driver's readable client array was empty in sampled camera
  inventory records. This does not establish a stale driver: the global object
  connection count is not proof of current-world membership. Admission guards
  remain in place while their individual predicates are investigated.
- The wrapper's live observer and final audit exceeded their 8 MiB whole-log
  limit. The inner result recorded unchanged original profiles, zero native
  exceptions, no forced cleanup and a disabled probe. A separate read-only
  recovery supplement hashed and streamed the completed logs and confirmed no
  remaining game processes. It does not replace the missing outer audit or
  convert the failed live observers into a pass.
- Protected-probe admission diagnostics now compare cached and current-world
  driver scope, retain counters from the existing remote-controller scan, and
  report silent pre-acquisition rejections. They do not change possession
  predicates. Sampling is limited to 120 records at two-second intervals.
  All three native outputs built; probe and ordinary runtime harnesses passed,
  including diagnostic budget/rejection fixtures and file-write isolation.
  These offline checks do not establish live acquisition or a gameplay fix.

## 2026-10-23: late ordinary join measured; activation DLC rejection reported

- The protected rerun exercised the new admission policy: five stable samples
  over 1,000 ms, with dispatch at native world time 68.026 seconds. The immediate
  console response was empty. The reflected viewport observer recorded
  `Connecting...` and closed its 60-second window normally. No sustained native
  connection, Ashley acquisition, collision repair or combat was established.
- During this run the owner supplied an in-game corrupt-DLC dialog naming
  `DLC_MOD_ME3SquadCoop`. The screenshot's process was not independently attributed.
  The activation container must not be treated as accepted by the engine. Static
  review confirms that our Mount versions, priority and serialized bytes match
  the supported native reader and official writer. Its absent DLC-root TOC is a
  concrete omission; the exact failure has not been attributed to that omission.
  A conditional native coalesced-file error also reaches the corruption dialog.
- Cleanup retained 8,210 original-file locks, unchanged profiles and zero native
  exceptions. Both native launchers ended at their limits, no game remained,
  and temporary authored ASI/Mount files were removed without forced cleanup.
- Combat investigation returns to the protected research entry, which does not
  install this activation DLC. Ordinary startup and packaging research remain
  separate from Ashley collision, firing/stop and authoritative damage evidence.

## 2026-10-23: stable guest admission and native response diagnostics

- Ordinary join now waits for a stable reciprocal player/story context rather
  than consuming its one request on a transient initial-world control frame.
  The policy observes unchanged identities and advancing native world time;
  it does not force loading, cinematic or pause flags. This changes admission
  behavior without establishing the cause of the preceding failed join.
- The immediate native console response is captured before its existing free.
  A bounded read-only observer also records changes in reflected viewport
  progress strings, explicitly distinguishing stored text from a native event.
  PendingLevel's undocumented private fields are not used.
- All three native outputs built. The ordinary native harness passed readiness,
  string safety and observer lifecycle fixtures; six compiled contract faults
  were rejected. Python discovery ran 257 tests, 2 skipped. These offline checks
  do not prove an ordinary guest connection; a protected game rerun is next.

## 2026-10-23: ordinary guest join attempted without a sustained connection

- A protected pair used the ordinary runtime for both roles, with the probe limited
  to isolation guards. Host startup reached verified status 8 again. The guest's
  one-shot join briefly entered native pending travel, but every valid guest world
  sample (213) remained standalone. No sustained native connection or Ashley
  acquisition was observed. Collision restoration, combat and cross-PC play were
  not verified by this attempt.
- The join was dispatched in the guest's early initial world, whose observed world
  time was about 0.43 seconds. Joining before stable player/story readiness is a
  hypothesis, not an established cause. The exact native rejection/error was not
  retained; a next diagnostic must capture it and distinguish initial travel from
  a genuinely stable ordinary player context without forcing readiness flags.
- The completed run retained 8,210 original-file locks, unchanged profiles and zero
  native exceptions. Both owned processes ended within their bounded launchers;
  temporary authored ASI/Mount files were removed, the probe was disabled, and no
  game process remained. No native source change or new test pass is claimed here.

## 2026-10-23: ordinary host listener exercised in the current mission

- A protected actual-entry run loaded the separate ordinary ASI with the probe
  in guards-only mode. The ordinary entry opened native Listen in the current
  Mars world without reloading it. Its startup result was 8, the independent
  current-world reader observed native net mode 2, and Windows confirmed the
  process owned the configured local Radmin interface/UDP port 17779. No guest
  was launched, and no research networking bootstrap was used.
- The preceding failed actual-entry run exposed a command-buffer address 0x20
  before the native getter target. The reader now derives the target from the
  pinned instruction. Live class inspection also showed that Shepard derives
  from SFXPawn_Player, not the previously assumed BioPlayerPawn. Native fixtures
  now contain the observed hierarchy and reject the old address/class errors.
- Parallel review corrected native guest admission for the client's cached zero
  local port and retained collision observations across ordinary-runtime paused
  scene frames. Meaningful native fixtures passed; guest connection and positive
  collision repair are separate pending game checks.
- All three native outputs rebuilt. Python discovery ran 257 tests, 2 skipped;
  ordinary native and collision lifecycle fixtures passed, and three compiled
  startup contract mutants were rejected. The actual game run retained 8,210
  original locks, unchanged profiles and bounded cleanup. Temporary authored
  ASI/Mount additions were removed and no game process remained. A later
  PlayersOnly observation rules out a claim of uninterrupted ordinary control.

Next: connect a real protected ordinary guest to this listener, then measure
Ashley ownership, equipment and usable scene/control return. Real combat, full
conversations, mission travel, two physical PCs and player installation remain open.

## 2026-10-23: shuttle exit exercised and collision loss isolated

- A new completed protected pair cleared both authored shuttle exit anchors and reached the first road node using ordinary movement. The landing obstruction correction is exercised in this run; the remaining route and jump-down traversal are still unverified.
- An actual landing conversation then held the ordinary engine gate for about 13.39 seconds. Its conversation controller and playing line Matinee were observed in mode 0. This is a real engine interruption and release/reacquisition, but it does not meet the separate full mode-7 dialogue requirement. The route stopped after its two-second context allowance; the new neutral scene-wait policy is documented separately and has not yet been rerun.
- A read-only paired transition recorder sampled both processes for 120 seconds. Guest Ashley's world/actor collision cleared during release and remained disabled through subsequent ownership epochs. Both processes later observed her falling. Inventory remained present in this run, so missing inventory is not required for the fall. Native collision restoration after control return is the next fix; captured flag changes alone do not prove its complete cause.
- Firing/stop and shared travel failed their separate evaluators; no hostile damage or full conversation was verified. Pointer ownership, AI restoration and disconnect checks passed, which expressly do not establish usable control while a pawn is falling.
- The final report protected 8,210 original game/profile files, retained unchanged profiles, recorded zero native exceptions and required no forced orphan cleanup. The probe ended disabled, no game process remained, and current native fingerprints matched. Distinct source/build/result/audit files and raw transition records remain private.
- Packaging now authors strict LE3 mount metadata and a DLC-only Mod Manager archive, replacing the fabricated startup PCC and wrong game identifier. Archive/readback and synthetic rollback checks are separate from actual Mod Manager import and engine mounting, which remain unverified. See [activation packaging](DLC-PACKAGING.md).
- The ordinary ASI now implements an explicit one-shot current-world host Listen as well as guest join. It validates native interface selection, the actual bound endpoint, current-map URL ownership, transient two-player capacity and client-free genuine package-map initialization. It never implicitly reloads a mission. The integrated build produced all three outputs; the ordinary native harness and 246 Python tests passed with two skipped. These startup paths still require protected actual-entry game tests; they were not used in the preceding route experiment.

## 2026-10-23: actual drained-hit dispatch and landing obstruction

- The numbered-name correction allowed the protected input route to start. The host moved about 126 units, then stopped against landing-shuttle geometry before the first road node. The four-second stuck guard released the injected input. No jump-down action, hostile encounter or mission progress was established.
- The default-off hit observer installed in the actual supported weapon dispatch and captured two native drained `ProcessInstantHit_Internal` calls for the leased guest weapon. Both original calls returned with valid source scope. Both had a null hit actor and zero hits, so there is no target health/shield or damage evidence. See [hit observation scope](GUEST-HIT-TRACE.md).
- The first ownership epoch received start/stop firing and settled at host/guest consumed counts 2/2, reserves 210/210. After synthetic reacquisition, Ashley's position jumped and fell on both processes; the guest's equipment chain was ineligible. The host returned to the landing only after disconnect/AI restoration. This is an additional unresolved control-return failure, not solely a guest presentation offset; the first epoch does not establish reliable combat or reacquisition.
- The experiment ended before its final paired report was written. Its launcher handle and processes were absent when checked; the enabled research marker was recovered to disabled. Both saved logs contain zero recorded native exceptions and the 16 original profile files match the preserved baseline. This recovery is not a completed lifecycle or clean-shutdown pass, and the preceding run's final report was not reused.
- The separate ordinary entry now has an explicit one-shot guest join request. A protected guards-only probe mode retains profile/file protection without owning Tick, preparing actual ordinary-entry testing. These are source/offline checks; ordinary host startup and actual-entry game validation remain unfinished. The integrated Python suite passed 238 tests with two skipped.

Next: use the measured shuttle exit anchors for ordinary movement, diagnose the second-epoch guest fall, and complete native ordinary host startup. Real enemy damage, full conversations, sustained mission travel, two-PC Radmin and player installation/rollback remain open.

## 2026-10-23: numbered route discovery and separate runtime entry

- A completed protected pair started the listen host after 43.6 seconds and connected a real guest. The route never started and no squadmate lease was acquired, so the run establishes no traversal, firing, dialogue or combat pass. Original-file locks covered 8,210 files; profiles were unchanged, native exceptions were zero, no forced cleanup was needed, the probe was disabled, and no game processes remained.
- A read-only capture found all ten intended route nodes uniquely in the current world, with the expected classes and coordinates. The discovery code compared bare FName strings such as `PathNode` with numbered names such as `PathNode_103`, which could never match. Discovery now reconstructs the suffix from the separate native FName Number field. The native fixture covers the captured numbers, and a compiled off-by-one mutation is rejected. The corrected route still needs a protected game rerun.
- The separate ordinary ASI now builds from its own entry and shares the actual companion ownership, camera and AI-restoration core with the probe. It checks the full executable SHA-256 and has no QA movement, synthetic scene timer, forced map selection or timed disconnect. It currently attaches only to an already established native session: normal listen/join bootstrap and protected validation of this entry remain unfinished. See [runtime integration](RUNTIME-INTEGRATION.md).
- After integrating the reviewed sampler and runtime changes, all three native outputs built, both native harnesses passed, and Python discovery passed 238 tests with two skipped. These offline checks do not establish a playable mod. Actual enemy damage, full conversations, sustained mission travel, two-PC Radmin and player installation/rollback remain open.

## 2026-10-23: read-only combat sampler reviewed and corrected

- An external contribution added the read-only health/shield reader, CLI and 26 fixtures. Review reproduced false decreases when reads failed or different worlds/pawns reused an address; the original evaluator was not reliable damage evidence.
- The corrected evaluator accepts only complete host/standalone Role3 snapshots in stable world, pawn, module and shield identity epochs. Missing data is unknown; identity changes and invalid snapshots break comparison. Array contents and reciprocal component links are rechecked, and read/byte/time budgets bound snapshots.
- Candidate classification now uses exact archetype/controller ancestry. It remains explicitly a candidate classification, not a verified relationship to Shepard. CDO pointer/name and actor lifecycle checks are present; exact UObject flag meanings and unobserved complete identity reuse remain limitations.
- The CLI handles unavailable worlds, verifies exact sandbox executable path and supported SHA, rejects reparse/hardlink ambiguity and refuses output overwrites or output inside the game sandbox.
- Focused offline tests cover the original false positives, valid decreases, regeneration, component replacement, invalid gaps, authority and inventory/array races. No live game was launched for this review; live hostile damage and guest attribution remain unverified. See [sampler scope and validation](AUTHORITATIVE-COMBAT-SAMPLER.md).
- A later 20-second read-only attachment to the separately protected host verified the sandbox executable and SHA and produced 22 host-NetMode-2 records. Twenty-one snapshots were valid and one rejected actual actor-array/streaming changes. It enumerated 1,488 to 1,618 actors with no hostile candidates; this verifies attachment and schema, not hostile health or damage.

## 2026-10-23: route attempt exposes frozen animation ticking

- A separate protected Mars pair enabled the actual `WorldInfo.bPlayersOnly` flag about 36.7 seconds into the world. Ashley's mesh and cover-node tick tags then remained unchanged through about 187.8 seconds. The exact native mesh tick guard skips animation nodes while this world flag is set. Animation ticking outside rendering was already enabled; pause-animation, frozen, saved-pose and no-skeleton flags did not explain this capture. No animation or world flags were changed.
- Native research identified a normal message-box lifecycle that saves and enables `bPlayersOnly`, then restores its prior value when that same message box closes. The active owning message box has not yet been captured. This supports inspecting normal UI ownership before attempting any remedy; it does not establish that a particular tutorial or player action caused the pause.
- Both route attempts rejected the joined landing position before walking or guest acquisition. They therefore provide no new firing, ownership-lifecycle or combat pass. The default-off driver now bounds its first leg by the same 1,500-unit range as its ordinary input calculation, relative to the validated first nav point, and retains the actual grounded start as its first corridor origin. Later corridors and identity, story, camera, ground and duration guards remain in place. Native outputs built, the Windows protection harness passed, and four compiled unsafe-policy variants were rejected. Actual route traversal remains unverified.
- The latest completed run protected 8,210 original game/profile files, retained unchanged profiles, recorded zero native exceptions and required no forced orphan cleanup. The probe was disabled afterward, the native source fingerprint matched, and no game processes remained. Raw captures and distinct run/build records remain private.

Next: identify and normally close the owning modal UI, then exercise the actual input route to a hostile encounter. Consistent firing, enemy damage, real dialogue, two-PC Radmin and end-user installation/rollback remain unverified.

## 2026-10-23: observer-off repeat identifies port-arms deferral

- Repeated the protected pair with the unchanged native build and native firing observer disabled. RPC observations remained enabled; snapshot correction remained disabled. The one eligible 250 ms input pulse settled at host/guest consumed counts 0/3, reserves 210/210. After synthetic reacquisition, the guest inventory chain was not eligible for a second pulse. This fails the firing requirement despite passing the separate lifecycle checks.
- The authoritative start-fire request used mode 2 and advanced its script cursor from 0 to 77. The host stayed in `SFXWeapon.Active`, storing pending fire mode 2. `BioPawn.bInPortArms` cleared, but `bPlayingPortArmsAnim` remained set. Exact `SFXWeapon.Active.IsInPortArms` bytecode checks the latter flag, so `BeginFire` defers rather than entering `WeaponFiring`. This differs from the preceding successful run, where the playing flag was clear.
- Read-only native research maps normal flag clearing to `BioAnimNodeBlendAdditiveCover` blend-out and relevance loss. Whether the failing host's mesh/node stops ticking still needs direct measurement. A normal AI capture already had animation ticking enabled outside rendering, so setting that policy without inspecting the failing lease would not establish a fix. No animation, firing-permission or ownership flag was forced.
- Two ownership epochs, synthetic release/reacquisition and native disconnect/AI restoration passed. Protection covered 8,210 original files; profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled, and no game processes remained. The tested native source fingerprint matched at completion. These protections do not turn the failed firing result into a combat pass.

Next: capture the actual leased host mesh/node lifecycle, apply a supported normal-animation remedy, and follow the measured Mars route to a genuine encounter using ordinary player input.

## 2026-10-23: native firing and ammunition consumption observed

- Added a default-off, bounded observer of the supported weapon's native `FireAmmunition` virtual dispatch. Exact native instructions, reflected method ABI, vtable/slot, game thread and current ownership/channel scope are required. Original calls are forwarded once with `LastError` preserved. Incoming RPC snapshots now also record the actual requested fire mode, script cursor, complete state path, burst counters and raw port-arms flags. See [native firing observations](NATIVE-FIRE-OBSERVATION.md).
- All three native outputs built and the Windows harness passed. Python discovery passed 197 tests, 2 skipped. Four compiled RPC-observer faults and five native-firing-observer faults were rejected, covering forwarding, identity, incoming-channel classification, out-of-script cursor rejection and `LastError`; these fixtures do not establish gameplay behavior.
- In the protected pair, with snapshot correction disabled, two 250 ms guest input pulses produced six observed native firing calls on each side. Each matching enter/leave pair advanced consumed count by one. Host incoming start-fire calls requested mode 2, advanced their script cursor from 0 to 77, and changed `SFXWeapon.Active` to `SFXWeapon.WeaponFiring`. The first native firing call occurred inside each start-fire receive; subsequent calls occurred through the ordinary firing loop.
- The two pulses settled at host/guest consumed counts 3/3 and 6/6, reserves 210/210. Counts remained stable in the post-release observation windows. No enemy health/shield decrease or damage attribution was measured. This is one successful bounded firing observation, not reliable combat or a completed weapon fix: the preceding pair received firing RPCs without authoritative consumption, and no intentional firing behavior change was introduced between the runs. Observation/timing effects and initialization differences must be investigated.
- Two ownership epochs, synthetic scene release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Original locks covered 8,210 game/profile files; profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game processes remained. The tested native source fingerprint matched at completion. Detailed build, runtime and call-pair audits remain private.

Next: repeat the firing result with observation disabled, explain the difference from failed runs, and measure authoritative damage against an actual hostile target. Real conversations, sustained progression, two-PC Radmin and player installation/rollback remain open.

## 2026-10-23: guest firing RPC arrival observed on host

- Added default-off `ME3_SQUAD_COOP_FIRE_RPC_TRACE=1` observations around 15 exact-build weapon class/state native script entries, including the separate inactive-state server methods. Installation validates reflected identity, parameter layout, script length and native dispatch; calls retain their original arguments and Windows LastError. Observations require the current reciprocally owned squad weapon and are bounded to 256 records per process. This does not alter firing or consume an RPC.
- Built all three native outputs and passed the Windows harness. Three separately compiled negative controls rejected missing original forwarding, changed self arguments and classification of a different channel as an incoming RPC. Python discovery passed 197 tests, 2 skipped.
- In a protected Mars pair, the snapshot correction adapter was disabled. Two 250 ms guest input pulses produced two `Engine.Weapon.ServerStartFire` and two `SFXGame.SFXWeapon.ServerStopFire` native entries on the host. Each matched the active native receive frame, target function/object and validated weapon channel 21; all four original entry calls returned with valid scope. The weapon state was `Active` before and after each entry. Host consumed count stayed 0; guest counts stabilized at 3 and 6, reserves 210/210.
- This establishes arrival at the selected host RPC entry, not execution of every statement or a full authoritative shot. Internal VM calls can bypass `UFunction.Func`, so missing nested/client entries cannot prove those methods were never called. Separate read-only research also maps the pawn shot counter to a weapon/effects path; a changed shot counter alone does not establish ammunition consumption or enemy damage.
- Two ownership epochs, synthetic scene release/reacquisition, host story preservation and native disconnect/AI restoration passed. Original locks covered 8,210 game/profile files; profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game processes remained. The tested native source fingerprint matched at completion. Raw captures, call records and distinct build/run audits remain private.

Next: distinguish incoming script-body execution from dispatch-only observation, and observe the actual native ammunition-firing path. Real conversations, sustained progression, two-PC Radmin and safe player installation remain open.

## 2026-10-22: guarded host snapshot correction exercised

- Added a default-off research adapter that requests settled authoritative ammunition through existing owner-PlayerController native RPCs. Exact method/parameter/dispatch validation and the active reciprocal native receive channel authorize control messages. Ordinary script calls retain original forwarding. No new socket or package-validation change is involved.
- Requests/replies bind a bounded ID, guest scope generation and selected weapon channel. Only a fresh matching response can update an idle guest weapon whose ownership and local baseline still match. The host reads its actual counters and never writes them. Scope changes invalidate pending state; retries, payloads and observation counts are bounded. See [AMMO-REPLICATION.md](AMMO-REPLICATION.md) for the opt-ins, transport and limits.
- Built all three native outputs and passed the Windows harness. Three separately compiled mutants accepting an old generation, wrong request ID or non-idle response were rejected. Python discovery passed 197 tests, with 2 skipped. New offline regressions require all four exchange stages and exact host snapshot values without assuming synchronized clocks. Successful correction cannot grant a firing/combat verdict.
- A protected paired Mars run completed two requests and responses on weapon channel 29. Guest consumed count was corrected from 1 to authoritative 0 twice, with reserve count 210 unchanged. Guest-local roundtrip times were 281 and 375 ms. **The host consumed no ammunition during either 250 ms pulse.** This verifies bounded counter correction, while authoritative guest firing remains a failure in the same run; it is not reliable combat.
- Two ownership epochs, synthetic scene release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Original-file locks covered 8,194 game files plus 16 profile files. Profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game process remained. The tested build still matched current native sources. Raw captures, distinct build/run fingerprints and the four-stage audit remain private.

Next: locate where authoritative guest firing is lost. Real conversations, stranded-companion catch-up, sustained progression, two-PC Radmin and player installation/rollback remain open. Counter correction alone does not satisfy these requirements.

## 2026-10-22: native ammunition decoding and correction observed

- Added an explicit research opt-in for the actual integer-property network serializer, with exact-build send/receive instruction pins, reflected ammunition metadata, current world, object stamps, reciprocal ownership, connection and weapon channel guards. Receive scratch values are tied to a nested-safe original `ReceivedBunch` frame and compared with final weapon storage. The observer forwards original arguments, result and LastError and does not change ammunition or payloads. It is disabled by default and bounded to 256 records per process.
- Added a conservative offline evidence evaluator and private-log CLI. Encoding, decoding, storage matches and actual changes remain separate. Unsupported/error/ambiguous records cannot create positive application evidence; packet matching and ammunition synchronization always remain unverified.
- Built all three native outputs. The Windows harness passed forwarding, nested-scope and budget fixtures plus existing safety checks. Three compiled mutants removing original forwarding, inheriting the outer receive or exceeding the budget failed. A separate evaluator mutation accepting final-storage mismatches was caught. Python discovery: 190 tests, OK, 2 skipped.
- A protected paired Mars run matched the selected weapon channel 26 on both sides. Six host encodes and six guest decodes were observed: five consumed-count fields and one reserve-count field. Five completed guest receives matched all six decoded values to final storage; one changed consumed count from 4 to 3. No record limit was reached and the offline audit rejected no recorded field/completion events.
- The first 250 ms pulse ended at host/guest 2/3: the guest decoded 2, then a later local increment reached 3 at the recorded release. The next pulse received count 3 while the guest was at 4, producing an observed native downward correction; its final counts matched at 5/5. Both pulses stopped under the existing evaluator; reserves stayed 210/210. The correction path works in this bounded observation, while end-of-fire agreement still fails in the same run. This is diagnosis, not a completed weapon fix.
- Two ownership epochs, synthetic release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Original-file locks covered 8,210 game/profile files; original profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game processes remained. A preceding protected single-process reflection capture also retained protection. Raw field/reflection logs, distinct build/run fingerprints and offline audits remain private.

Next: determine the appropriate authoritative end-of-fire reconciliation path and test it against the observed late local increment. Real dialogue, stranded-companion catch-up, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: bounded current-world channel discovery

- Replaced the channel resolver's full object snapshot and nested guest driver scan with the current world's native NetDriver and bounded peer/channel lists. Exact-build instruction pins, live World/Level/WorldInfo identities, actor/world binding and matching before/after scope snapshots authorize this path. No native method is called and no engine state is written by the resolver.
- Kept all-peer ambiguity rejection, connection/driver reciprocity, exact channel vtable checks, channel-index bounds and reciprocal table membership. Missing or malformed scope returns unknown. Pawn-only and weapon-only public entry points remain distinct; published cache hints cannot bypass enumeration.
- Built all three native outputs. The native harness passed valid/invalid scope, public-type, duplicate peer/channel, native-pin and membership fixtures plus existing safety checks. Three separately compiled mutation twins removing world binding, reciprocal table validation or ambiguity rejection were caught. Python discovery: 178 tests, OK, 2 skipped.
- In a protected normal-duration paired Mars run, the last cumulative channel snapshots fell from host 3.696 seconds over 439 calls to 0.224 seconds over 564 calls, and guest 2.762 seconds over 2,654 calls to 1.244 seconds over 2,071 calls. Maximum calls fell from 318/668 ms to 24/27 ms. Counts, scene timing and machine scheduling differ, so this is measured resolver cost rather than a controlled FPS/GPU benchmark; remaining peaks are not zero. See [LOOKUP-PERFORMANCE.md](LOOKUP-PERFORMANCE.md).
- Both sides matched pawn channel 27 and weapon channel 24 within this connection. Two ownership epochs, synthetic scene release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Recorded firing intervals were 250 and 312 ms. Both pulses consumed ammunition and stopped; counts matched at 3/3, then differed at 7/8, with reserves 210/210. The ammunition problem remains open.
- Original-file locks covered 8,210 game/profile files. The report recorded unchanged original profiles, zero native exceptions, no forced orphan cleanup, a disabled probe and no remaining game processes. Distinct run/build/trace/profile evidence remains private. No real conversation, mission completion, two-PC Radmin or player-installation claim was added.

Next: inspect authoritative ammunition field transmission and client prediction/correction for the matched weapon channel. Startup repeatability, stranded-companion catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: measured function-discovery stall reduction

- Added explicitly opted-in, bounded wall-clock profiling for function discovery, channel resolution and Tick/diagnostic scopes. GPU timing is not measured. A protected 100-second Mars baseline found repeated function discovery dominating diagnostic cost: 54 `GetHealthPct` resolutions accumulated 20.537 seconds, with an 829 ms maximum. The diagnostic scope accumulated 38.836 seconds in its last emitted snapshot.
- Added a 64-entry game-thread discovery cache. Every hit revalidates current object-table membership and the function, owner and Function-class identities, including complete FNames. Changed identities fall back to discovery; function results are never cached. Caller-side validation and native game behavior remain intact.
- Repeated the same protected 100-second action: 63 `GetHealthPct` resolutions accumulated 0.306 seconds, including one initial discovery; 62 warm hits averaged 41.7 microseconds and peaked at 230 microseconds. The diagnostic scope accumulated 1.263 seconds. Callback counts and scene timing differed, so this is resolver/diagnostic evidence, not a controlled FPS benchmark. See [LOOKUP-PERFORMANCE.md](LOOKUP-PERFORMANCE.md).
- Built all three native outputs. The Windows harness passed the new synthetic function/owner/class/table-membership guards and existing safety fixtures. Separate compiled mutants truncating FName identity or removing table membership were rejected. Python discovery: 178 tests, OK, 2 skipped.
- A subsequent protected paired run passed two ownership epochs, synthetic scene release/reacquisition and native disconnect/AI restoration. Both recorded press/release intervals were 250 ms; both pulses consumed ammunition on host and guest and stopped under the existing evaluator. Counts matched after the first pulse (3/3), then differed after the second (5/6), with reserves 210/210. Reliable ammunition reconciliation remains unverified.
- The change trace matched pawn channel 27 and weapon channel 31 on both sides within this connection. Recorded change-sample intervals were 15ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¾ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã¢â‚¬Å“31 ms in this run, compared with multi-second gaps in the previous instrumented run. These observations do not establish a complete shot history or absence of all stalls. Separate channel-resolution measurements still had cold peaks around 318 ms on host and 668 ms on guest; this remaining cost has not been fixed.
- The two single-process measurements and paired run retained original-file protection. Completed reports recorded unchanged original profiles and zero native exceptions. The paired run protected 8,210 original game/profile files, required no forced orphan cleanup, disabled the probe and left no game processes running. Raw profiles and distinct build/run fingerprints remain private.

Next: inspect remaining cold channel-resolution cost and actual ammunition field updates without removing identity guards. Startup repeatability, catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: read-only ammunition change tracing

- Added opt-in post-Tick ammunition observations for reciprocally controlled Ashley on host and guest. Baselines restart on object/context changes; subsequent records capture observed consumed/reserve changes, actual pawn/weapon channel indices, local identities, roles, actor flags and scheduling fields. Counters, replication flags and scheduling are never written by this recorder.
- Extended the validated channel resolver with a weapon-only entry point while retaining the existing pawn-only public contract. Ambiguous/missing channels remain unknown. The recorder is limited to 128 observations plus one limit notice per process across all contexts.
- Built all three native outputs and passed the Windows harness, including the new trace filter test and original-file write-denial fixtures. Python discovery: 178 tests, OK, 2 skipped. Separate compiled mutations exceeding the sample budget or merging new contexts were caught by the native self-test.
- The first bounded pair did not acquire Ashley after a slow intro and generated no trace observations. This was not a successful runtime trace test. One extended bounded retry acquired twice and passed synthetic release/reacquisition plus native disconnect/AI restoration.
- The extended run recorded three host and four guest trace observations, with two context baselines on each side and no budget exhaustion. Both sides resolved Ashley to channel 15 and her weapon to channel 9 within that connection. The first pulse changed host consumed count to 4 and guest count to 7; reserves stayed 210. Thus the differing counts were observed for the same validated weapon channel, rather than inferred from matching class names.
- The first pulse stopped under the existing evaluator but failed ammunition agreement. The second input attempt produced no observed consumption; the overall firing/stop gate remained false. Logged press/release timestamps were 2,172 ms and 515 ms apart despite the requested 250 ms duration. These timestamps do not independently establish exactly when native input changed. Trace sample gaps reached 2,344 ms on host and 2,485 ms on guest. This is sampled evidence with significant gaps, not a complete shot history or a clean timing benchmark. Instrumentation and other diagnostic overhead have not been excluded as contributors.
- Host and guest sampled update frequency 100; this is configured frequency, not measured delivery cadence. Host update timestamps advanced, but post-Tick flags were sampled outside replication and cannot prove whether a particular connection's owner/dirty condition passed. Packet field transmission, delivery and prediction reconciliation remain unknown.
- Both completed runs protected 8,210 original game/profile files, recorded unchanged original profiles and zero native exceptions, required no forced orphan cleanup, and disabled the probe after shutdown. No game processes remained after the extended run. Raw evidence and build fingerprints remain private.

Next: measure diagnostic/channel-lookup costs and trace actual per-connection field updates before attempting an ammunition correction. Startup repeatability, catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: ammunition condition inspection and reserve-agreement fix

- Fixed a false-positive ammunition comparison: equal consumed-round counts no longer pass `post_release_ammo_agreement` when host and guest reserve counts differ. Reports now retain both sides' reserve samples. Stopping and ammunition agreement remain separate findings.
- Added an offline, read-only reflection-capture recognizer for the observed owner/dirty/authority replication condition. Both ammunition fields matched it in a fresh private capture. The output omits addresses and script bytes and cannot claim runtime synchronization or a dirty-gate failure. See [AMMO-REPLICATION.md](AMMO-REPLICATION.md).
- The captured condition includes the owning connection; it does not exclude the guest categorically. Static inspection also showed that `Actor.SetNetUpdateTime` only changes scheduling, rather than marking dirty state. Actual per-connection gate evaluation, field transmission and prediction reconciliation remain unverified.
- Ran another protected two-process Mars session with unchanged native code. Two ownership epochs, synthetic handoff/reacquisition and native disconnect/AI restoration passed. The guest had 131 consecutive client-mode samples spanning 131.062 seconds before intentional exit.
- Both firing pulses consumed host and guest ammunition and stopped within the evaluator's release grace period. This time the final consumed counts matched (1/1, then 6/6), with matching reserves (210/210). The previous run disagreed. This is a positive bounded observation, not a repeatable weapon fix or full-combat pass. The updated reserve comparison was applied offline after the run.
- Original locks covered 8,210 game/profile files; profiles remained unchanged, native exceptions were zero, no forced orphan cleanup occurred and the probe was disabled after shutdown.
- Python discovery: 178 tests, OK, 2 skipped. Changed expressions/operands/metadata, missing or duplicate fields and malformed private content are rejected by synthetic tests. A mutation removing reserve comparison was caught by the new regression. No native rebuild was needed because native sources were unchanged.

Next: trace live authoritative ammunition changes through the owning actor channel and distinguish missed correction from later client prediction. Catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: connection interval reporting and protected rerun

### Completed reporting change

- Connection evidence now retains the longest observed client-mode interval separately from the trailing interval. An ordinary disconnect ends the active-at-end verdict without erasing an earlier observation. Failure messages remain visible; historical observation is not a disconnect-cleanup, cross-PC or campaign verdict.
- The paired summary adds `sustained_connection_observed`. The existing `connection_states_verified` and `persistent_client_mode_verified` checks retain their active-at-end meaning. Reconnects and sample gaps over 2.5 seconds cannot be combined into one interval; invalid or non-increasing frame clocks cannot verify continuity.
- Python discovery: 169 tests, OK, 2 skipped. Seven added regressions cover active/disconnected states, reconnect boundaries, sampling gaps, invalid clocks, errors and missing timing. An in-memory mutation that erased completed interval history was rejected by the disconnect regression.
- Offline replay of three private guest traces distinguished one still-active connection from two intentionally disconnected sessions. The latter retained 119.641 and 124.234 seconds of observed continuous client mode while correctly failing the active-at-end check. Raw traces remain private.

### Actual game rerun

- Ran the native world-floor guard build in a bounded, protected two-process Mars test. Two ownership epochs, synthetic scene release/reacquisition, preserved host story control and native disconnect/AI restoration passed the lifecycle evaluator. No real full-conversation samples were observed.
- The guest had 125 consecutive client-mode observations spanning 124.234 seconds before intentional disconnect. The new evaluator was applied offline after the run; the running orchestrator had loaded the preceding reporting code.
- Ashley was already nearby when control was acquired. No out-of-range scenario or native catch-up call occurred. This rerun therefore **does not verify the world-floor catch-up fix**; a targeted stranded-companion scenario remains required.
- Two guest firing pulses consumed ammunition on both host and guest, and consumption stabilized after release within the evaluator's one-second grace period. Consumed-round counts still disagreed: host/guest 4/6 after the first pulse and 8/9 after the second. This is bounded firing/stop evidence, not an equipment synchronization or full-combat pass; enemy damage remains unverified.
- Original-file locks covered 8,194 game files plus 16 profile files. Original profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, and the probe was disabled after shutdown. No game processes remained.

Next: exercise catch-up with a genuinely stranded squadmate and diagnose authoritative/client ammunition disagreement. Real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: world-floor catch-up guard and firing audit

- A later bounded pair exercised two ownership epochs, synthetic scene release/reacquisition and native disconnect/AI restoration. This does not establish real dialogue or repeatable campaign progression.
- Its 250 ms guest input consumed one predicted round locally, while the authoritative host count stayed zero. Firing/stop-firing verification now requires observed consumption on both sides; stable zero counts cannot pass. Client-only prediction remains an unresolved combat failure.
- A read-only live capture observed a player floor normal of `[0, 0, 1]` with a null supporting actor (`Base`). Catch-up no longer requires a supporting actor: it validates finite health, velocity and floor data, while preserving walking, world, squad and ownership checks and the game's normal navigation/catch-up routine. No position is directly written.
- The guard change has native fixture coverage for ordinary world-floor readiness and invalid, steep, falling, non-finite and dead-player cases. An in-game rerun of this specific guard change is still required; it is not a confirmed fix for every stranded-companion case.
- The bounded pair recorded zero native exceptions, unchanged original profiles, no forced orphan cleanup and a disabled probe after shutdown. Its generic persistent-connection summary was false after the intentional disconnect despite the separate lifecycle evidence passing; this reporting discrepancy needs investigation.

Next: repeat the catch-up scenario, investigate the absent authoritative shot and inventory restoration after reacquisition, then exercise actual dialogue, cross-PC Radmin and installation/rollback gates. No gameplay release is newly claimed.

## 2026-10-22: public-source runtime checks and acquisition diagnosis

The public checkout now uses the renamed Windows native harness correctly; the old launcher path prevented protected boots from this checkout.

### Changes

- Replaced the ineffective ProcessEvent query of intrinsic GetStateName with an exact-build, read-only state-frame reader. Native code bytes, live state objects and stable frame/node identities are checked; unsupported or unreadable states remain unknown. No state is changed by this reader.
- Added bounded acquisition-wait reasons and firing-method resolution diagnostics without relaxing possession, story, distance, equipment or bytecode guards.
- Included firing/stop-firing and real-dialogue evidence in paired-run reports. Dialogue verification now also requires observed ownership before the scene; a release/reacquire label alone cannot pass it.

### Actual runtime results

- A protected 35-second boot from the public source checkout passed. The original profile remained unchanged and the probe was disabled after the run.
- Two subsequent isolated paired runs connected through native networking, but neither acquired the guest squadmate. They did not exercise firing or real dialogue and are not successful session lifecycle tests.
- In the diagnostic run, acquisition reported `companion-out-of-range` 35 times. The host and squadmate became separated during the Mars intro transition. The native state reader observed `PlayerWalking` and AI `Idle`, rather than the previous misleading `None` result.
- The diagnostic run recorded zero native exceptions, unchanged original profiles, no forced orphan cleanup and a disabled probe after shutdown.

Earlier bounded possession/lifecycle observations remain documented in VALIDATION.md. The latest failure shows those results are not yet repeatable for this startup route. No campaign, combat, cross-PC Radmin or release-package milestone is newly claimed.

### Next investigation

Inspect the normal squad-follow/catch-up path and its actual ground/readiness conditions at the intro transition, then repeat ownership and equipment tests. Do not remove the distance guard or directly relocate actors to turn this failure into a pass. Reliable acquisition is required before the weapon and real-dialogue gates can be exercised.


### Relative-location field comparison (2026-10-25)

A protected pair recorded a large guest `RelativeLocation` change inside the
original native receive call. The field observer and receive entry/exit snapshot
agree, while world `Location` was unchanged in that call. This does not prove
that the field caused a later position jump. Nearby host field samples were
normal, but timestamps alone cannot identify the exact sender packet. Shared
progression, full dialogue and combat still failed validation. Original files
and profiles were preserved; read-only helper failures prevent an all-pass audit.

The opt-in observer now also records host Ashley `RelativeLocation` serialization
through the exact native sender call site and the existing writer scalar hook.
It requires a live current-world Ashley, the pinned writer/package-map tables
and a non-loading archive. It forwards the original once and never changes
payload, actor position or archive cursor. Authored-memory tests check sender
identity rejection, scalar observation, forwarding and error preservation;
these tests are not live packet matching or a gameplay fix.


### Ordinary-action test deadlock

The preceding route settled at80.75054 units from the real jump-down top with
actual Touching, zero velocity and neutral input. Braking retained neutral input
through82 units, but readiness rejected anything above80, so the bot waited
until its10-second bound. The retained native selector checks Touching, node
and reachspec eligibility and direction scores; no80-unit comparison was found.
That80-unit threshold was an authored QA heuristic. Readiness now shares the
existing82-unit braking margin, retaining actual contact, facing, speed and
settling checks. The bot still sends ordinary `PC_Action`; native selection and
traversal are untouched. Regression checks cover the recorded80.75054 case and
reject missing contact, excessive speed and insufficient settling. Live
traversal remains unverified.


A subsequent protected pair recorded matching sender/receiver `RelativeLocation`
scalars and the expected rounded value, then later a guest `Velocity` decode
followed by archive errors. The healthy field does not reproduce the earlier
large relative-position change. That run still failed shared progression and
the guest fell below world kill Z. Native exceptions were zero and original
profiles remained unchanged; the memory helper exited1. Host `Velocity` is now
included in the same bounded observer to compare the failing field directly.
Equal field values alone do not identify a packet by its header.
