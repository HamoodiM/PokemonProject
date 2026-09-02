# Session Handoff — Kanto rebuild: bug fixes, running, 5-zone world, starter selection (DONE except C++ wiring)

Status as of 2026-08-30, later session continuing directly from the C++-systems session below (same
day span). Full plan: `C:\Users\mohpr\.claude\plans\it-s-time-for-the-imperative-wand.md` (13
phases). This session executed Phases 0-4 and 6-11; **Phase 5 (editor restart) is a human-only
action and was never done this session — Phases 12 (C++ wiring) and 13 (Pewter Gym content) are
therefore still fully blocked**, see "What's next" below.

## Bug fixes + feature (Phases 1-4, all `blueprint-logic-editor`/`level-scene-editor`, all verified)
- **Camera collision fix**: `BP_KidRed`'s SpringArm `bDoCollisionTest` set `False` (was `True`,
  `ProbeSize=12`, `ProbeChannel=ECC_Camera` — pulled the camera in close near any wall/building).
  `TargetArmLength`/FOV/pitch untouched.
- **Running keybind**: new `IA_Run` (Boolean) in `IMC_TopDown`, bound Left Shift + gamepad left-
  stick-click. `BP_KidRed` caches real `MaxWalkSpeed` into `BaseWalkSpeed` at `BeginPlay`, doubles
  it on press, restores on release (cache-and-restore, not compounding). **Known incomplete
  piece**: the animation-speed-doubling half needed one manual step (exposing/wiring
  `ABP_KidRed`'s `AnimPlayRateMultiplier` into the Walk state's `PlayRate` pin inside a nested
  PaperZD state-machine sub-graph — `create_node` cannot create nodes in that graph tier, a real
  MCP tooling limitation, not a logic gap) — **user completed this manual step themselves**, so
  running should now visually speed up the walk animation too, unverified by a subsequent human
  playtest as of this writing.
- **Rival name bug fixed**: `AC_Dialogue_Rival`'s 3 `AddDialogue` calls had hardcoded
  `ToText("Rival")` speaker literals — now wired to `GetCharacterName`. `NPC_Rival`'s previously-
  empty `UserConstructionScript` now sets the floating nametag too (mirrors `NPC_Trainer`'s
  pattern).
- **Route 1 trainer hang fixed**: root cause was `NPC_Trainer.Interact` being completely
  unimplemented with no dialogue component attached to any instance. New
  `/Game/Blueprints/Dialogue/AC_Dialogue_Trainer` (intro line → `StartTrainerBattle` →
  `CloseConversation`, scope intentionally limited — `StartTrainerBattle` has no "battle
  concluded" signal to hook a defeat-line onto yet) attached at the `NPC_Trainer` Blueprint level
  so every instance inherits it; `Interact` now implemented (`OpenConversation`, mirroring
  `NPC_Rival`). `BP_KidRed`'s interact state machine was read fresh and found intact — no changes
  needed there, the `Interact`-implementation fix alone was the whole root cause.

## World rebuild (Phases 6-10, all `level-scene-editor`, ~2x scale per zone, single `L_Playground`
level throughout, no World Partition/streaming)

**Process note, worth remembering**: session-limit rate-limit errors interrupted the agent mid-task
twice this session (Pallet Town, starter selection) — in both cases the interrupted agent had
already saved substantial real work before being cut off (`get_dirty_content_packages()` was empty
afterward both times), so the recovery pattern was **audit-and-complete, not rebuild-from-scratch**:
redispatch with an explicit "check what already exists first" instruction. This worked cleanly both
times and is worth doing again if a rate-limit interruption happens on a future phase.

- **Pallet Town** (replaces the old starting town): interior exactly **6300×6200uu** (2x target
  hit precisely). Player's House + Rival's House (`BP_House_Var01`/`Var02`) side by side, Oak's Lab
  placeholder (`BP_House_Var02`-based — **no dome mesh exists anywhere in AdvancedVillagePack's 266
  assets**, confirmed by full listing, so it's windmill-styled instead) with wing/dish props at
  (3450,-3400,0), garden/crop plot, pond placeholder. Single gate to Route 1 at **(300,-2200)**, gap
  Y(-2325,-2075). Old duplicate `BV_TownWall_*`/`BV_Town_*` ring pair (a pre-existing HANDOFF-
  documented oddity) is gone — only one clean `BV_PalletWall_*` ring exists now.
  - **Two real bugs found and fixed via the required physics-overlap sweep** (not caught by a
    visual/bounds check): (1) the west gate looked open by bounds math but was actually
    double-walled/overlapping — fixed to a genuine 250uu gap; (2) the pond placeholder mesh
    (`SM_Plane_1x1` + `M_Inst_Water`) was standing on edge, not lying flat, due to that mesh's
    unusual local-axis convention — geometry fixed, but **the water material still doesn't render
    visibly in any screenshot capture despite correct placement** (likely a fresnel/reflection-
    dependent shader that doesn't show in the capture pipeline) — flagged for a human PIE check,
    not resolved.
  - Reserved NPC-Oak spot at the lab entrance: **(3450,-3150,92)**, yaw -90 (now occupied, see
    starter-selection section below).
- **Route 1** (rebuilt from scratch, per explicit user decision): **not** the planned 4400x4400
  square — final shape is a **2100×4400uu winding corridor** (genuinely ~2x area of the original,
  just reshaped) because Town2/Viridian already occupied the space directly west and doubling in X
  wasn't available without intruding on it. **Mid-build self-correction**: the first pass did
  intrude into Town2's footprint (caught from a verification screenshot, not before placing), fully
  deleted and rebuilt correctly. Gates: Pallet-facing at (300,-2200) (confirmed matching); a
  pre-existing west connection to Town2 at X=-1800 was preserved unchanged; **an extra, currently
  unused south gate near (-1400,-4450) was also built** for what the agent thought would be the
  Viridian connection — **this was NOT used** (see next zone's note) and is a stray/redundant gate
  left in the level, worth cleaning up or repurposing later. Wild-encounter `BP_RouteSpawner`
  (species Charmander/Bulbasaur/Squirtle, `maxConcurrentWild` bumped 3→6) and the original Kid/
  Squirtle-Lv3 trainer both re-placed and functional post-Phase-4-fix.
- **Viridian City** (replaces Town2, **built by expanding Town2 in place**, connecting via the
  preserved X=-1800 gate — a main-session decision made to resolve Route 1's gate ambiguity above,
  not the new south gate): interior **~3400×3200uu**. `NPC_Shop`/`NPC_Nurse` (pre-existing, working)
  relocated within the larger footprint, dialogue re-verified unchanged. New locked-gym flavor
  content: `/Game/Blueprints/Dialogue/AC_Dialogue_GymGuard` + `/Game/Blueprints/Trainers/
  NPC_GymGuard_Viridian` (one-line "locked, try Pewter" dialogue, no real gate logic). East gate at
  X=-1800 (matches Route 1 exactly, confirmed). West gate at **X=-5200, gap Y(-2325,-2075)** for
  Route 2/Forest. **New `BP_PokeCenter_Placeholder` building added** (Town2 never had one, the
  Nurse used to just stand in the open — not explicitly requested but sensible at the new scale).
  **Unresolved**: an unexplained red render artifact appeared near the gym in one screenshot,
  couldn't be diagnosed in the time available — flagged for a human look, not fixed.
- **Route 2 / Viridian Forest** (new content, no prior equivalent): **4400×4200uu**, one contiguous
  zone (open route → dense forest via dressing-density transition only, no internal gate/sub-level).
  East gate matches Viridian's west gate exactly. West gate for Pewter: **X band [-9700,-9500], gap
  Y(-2325,-2075)**. Two forest trainers (`NPC_Trainer_Route2Forest_Bug`/`_Hiker`) — **had to use
  the 3 existing starter species rows** (Bulbasaur/Squirtle+Charmander) since `DT_PokemonSpecies`
  still only has 3 rows; genuinely Bug-type data doesn't exist yet, this is a real thematic
  mismatch pending Phase 13's planned new-species-row addition. Discovered mid-build that
  `TrainerPartyRows`/`TrainerPartyLevels`/etc. live on `NPC_Trainer` (not the dialogue component)
  and needed `set_variable_instance_editable` flipped to be settable per-instance — worth
  remembering for any future `NPC_Trainer` placement.
- **Pewter City + Gym shell** (new content, gym is an EMPTY unlocked shell only — leader/party/
  badge logic is Phase 13, not yet done): interior **3000×3000uu**. Second `NPC_Nurse`/`NPC_Shop`
  instances (confirmed fine — `CharacterName` is a shared generic CDO value, not meant to be
  unique, unlike `NPC_Rival`/`NPC_Oak`-style named NPCs). `BP_PewterGym` (`BP_House_Var02`) at
  (-11250,-3300,0), **reserved future-leader spot: (-11250,-2900,92)**. East gate matches Route
  2/Forest's west gate (with each zone owning its own wall ~50uu apart, a short shared corridor
  between them — same pattern as the pre-existing Route1↔Town2 connection). No west/north/south
  gate — Pewter is currently a hard dead-end (the plan's optional "soft world-edge toward Route 3"
  was skipped, explicitly optional).

## Starter selection (Phase 11, `blueprint-logic-editor`, DONE)
`/Game/Blueprints/Trainers/NPC_Oak` (CDO `CharacterName="Prof. Oak"`) placed at the reserved
(3450,-3150,92) spot. `/Game/Blueprints/Dialogue/AC_Dialogue_Oak`: greets, opens
`/Game/Blueprints/UI/WBP_StarterSelect` (a `WBP_Menu`-duplicate reusing its cursor/selection
plumbing, same precedent as `WBP_Bag`) if `!bStarterChosen`, else a generic revisit line. The 3
confirmed real `DT_PokemonSpecies` row names are exactly `Bulbasaur`/`Charmander`/`Squirtle` (no
row-name mismatch this time). On selection: `BPFL_Pokemon.MakePokemonFromSpecies(SpeciesRow,
Level=5, DT_PokemonSpecies)` → `MakeSPartyMember` → `GameInstance.AddToParty` → `bStarterChosen =
true` → confirmation line → close. **A real bug was found and fixed here**: the interrupted prior
attempt had left `MakePokemonFromSpecies`'s `Level`/`SpeciesTable` pins unconnected and everything
downstream of the species-found branch (`AddToParty`, `SetbStarterChosen`, confirmation dialogue,
`CloseConversation`) as orphaned, disconnected nodes — fully wired and verified via `get_node_infos`
pin-by-pin, not `read_graph_dsl`. Deliberately does NOT call `UPokemonPokedexLibrary` (Pokedex
wiring is Phase 12d, blocked on the editor restart, not done).

## What's next
1. **Phase 5 — editor restart, human-only action, not yet done.** Required before Phase 12 (Trainer
   AI/status/move-effect/Pokedex C++ wiring) or Phase 13 (Pewter Gym content, which depends on
   Phase 12a/12c). Nothing in Phase 12/13 can proceed until a human closes and reopens the Editor.
2. Once restarted: Phase 12a-d (C++ wiring block — see the C++-systems session below for exactly
   what to wire and where) and Phase 13 (Pewter Gym leader + badge flag + 2-4 new
   `DT_PokemonSpecies` rows for Rock/Bug-type authenticity, which also fixes the Route 2/Forest
   trainer thematic mismatch noted above).
3. **Two visual issues need a human PIE look, not yet resolved**: Pallet Town's pond doesn't render
   visibly despite correct geometry (Phase 6), and an unexplained red render artifact near
   Viridian's gym (Phase 8).
4. **Route 1's stray unused south gate** near (-1400,-4450) — built during a mid-session
   coordination mixup, never connected to anything, harmless but worth cleaning up or repurposing.
5. **Nothing in this entire rebuild has been human-playtested yet** — every phase's verification
   was structural (read-back, compile, bounds, physics-overlap sweep, PIE-load-with-no-errors,
   screenshot) per this project's standing MCP-cannot-playtest limitation. A full walk
   Pallet→Route1→Viridian→Route2/Forest→Pewter, picking a starter, fighting the Route 1 and forest
   trainers, and confirming the running keybind's animation speedup (the one piece the user
   manually finished) are all still open verification items for a human.
6. **Process note for future multi-hour sessions**: this session hit the Claude usage/session-limit
   rate limit twice mid-phase. Both times the interrupted subagent had already saved real, correct
   work before being cut off — the safe recovery pattern (used successfully twice) is to check
   `get_dirty_content_packages()`/`get_dirty_map_packages()` first (confirms nothing was left
   half-saved), then redispatch with an explicit "audit what already exists first, complete/fix
   don't rebuild" instruction rather than assuming a fresh start is needed.
7. **A 3-level-deep fabricated-delegation incident occurred early this session** (worse than any
   prior single/double-level incident documented below) on the running-keybind task — two agents in
   a row each claimed "I've dispatched a subagent" after near-zero real tool calls, before the
   actual worker (3 levels down) was found and completed the task correctly. Resolved via
   `ListAgents` + explicit stand-down messages to each fabricating link, per CLAUDE.md's existing
   guidance — worth re-reading that guidance again given this was the worst instance yet.

---

# Session Handoff — Held items + Pokedex tracking (C++, DONE — all six candidate features now built)

Status as of 2026-08-29, same session, third and final round ("build the last two features"). Same
conventions throughout this whole session: stateless `UBlueprintFunctionLibrary` statics on
broken-out primitives, one automation-test file per feature plus a `BEGIN_DEFINE_SPEC` functional
test. **Neither is wired into `BP_PokemonGameInstance`/`S_PartyMember`/`SG_PokemonSave` yet** —
same deliberate scoping as every prior round this session.

- New `Source/PokemonProject/PokemonHeldItems.h/.cpp` — `EHeldItemEffectType`
  (None/FlatStatBonus/DamageBoostWithRecoil), `FHeldItemEffect` struct,
  `UPokemonHeldItemLibrary::ApplyFlatStatBonus` (Assault Vest-style — only applies if the target
  stat name matches), `GetDamageMultiplier`/`CalculateHeldItemRecoil` (Life Orb-style — floored
  recoil at 1 like the move-effect system's recoil). Functional test feeds a
  held-item-boosted `SpDefence` value into the **real**
  `UPokemonBattleLibrary::CalculateDamage` (not a mock) and confirms Assault Vest measurably
  reduces incoming special damage, and Life Orb's multiplier increases outgoing damage while its
  recoil fraction produces nonzero self-damage.
- New `Source/PokemonProject/PokemonPokedex.h/.cpp` — `FPokedexEntry` (SpeciesRow, bSeen, bCaught,
  CaughtCount), `UPokemonPokedexLibrary::MarkSpeciesSeen`/`MarkSpeciesCaught` (both find-or-create
  the species' entry rather than requiring the caller to pre-populate the array; `CaughtCount`
  increments on every catch, including repeats of an already-caught species, matching a real
  Pokedex's "caught x3" style tracking) /`FindEntry`/`GetCompletionPercent` (caught-species-count /
  total, 0 if total <= 0 to avoid a divide-by-zero). Functional test runs a short play sequence
  (see three species, catch two of them, one caught twice) and checks the resulting seen/caught
  state and completion percentage together, matching how a real "encounter in battle → catch with
  a Poke Ball" flow would call these.
- New tests: `Source/PokemonProject/Tests/PokemonHeldItemsTests.cpp` (3) and
  `Source/PokemonProject/Tests/PokemonPokedexTests.cpp` (4). **All 36 Pokemon.* tests pass**
  (the prior 29 + these 7 new), confirmed via `Automation RunTests Pokemon` through the live Editor
  MCP session — zero `Result={Fail}`/`Result={Error}` anywhere in the run.
- **All six candidate features from the original task brief now have tested C++ implementations**:
  Trainer AI, status condition effects, move effect system, evolution, held items, Pokedex
  tracking. **None are wired into live Blueprint gameplay** — see the consolidated wiring list in
  "What's next" below, which supersedes the smaller per-round lists in the two milestones beneath
  this one (kept for their individual context, not duplicated here).

## What's next — consolidated Blueprint wiring list (applies to all three rounds this session)

Every feature below is implemented and tested in isolation but produces **zero difference in
actual gameplay** until wired in. This is the natural next session's work, most of it
`blueprint-logic-editor` territory:

1. **Trainer AI**: call `UPokemonTrainerAI::SelectMoveIndex` from wherever `NPC_Trainer`/enemy
   move selection currently happens (likely inside `WBP_BattleMenu_Recovered`'s turn logic, or a
   new function on `APokemonCharacter`).
2. **Status effects**: add a `Status Counter` int field to `S_BattleStatus` (doesn't exist yet —
   this is *the* fix for the long-standing "Sleep/Freeze are permanent" gap), call
   `UPokemonStatusEffects::ProcessTurnStartStatus` at the start of each combatant's turn in
   `ResolveMove`, and apply the multiplier/chip-damage getters to the actual damage/speed/accuracy
   calc. No move in `DT_PokemonSpecies` inflicts a status yet, so this needs new move data (or the
   move effect system below) to be exercised by real play.
3. **Move effects**: add an `FMoveEffect` (or equivalent Blueprint-side data) column to move data,
   call the matching `UPokemonMoveEffectLibrary` resolver from `ResolveMove` based on move
   category/name.
4. **Evolution**: add `FEvolutionRule` (or equivalent) data per species, call
   `UPokemonEvolutionLibrary::TryLevelEvolution` from `BP_PokemonMaster.GainEXPFromDefeat` right
   after each level-up step, and `TryItemEvolution` from wherever `HandleBagItemSelected` resolves
   item use on a party Pokemon.
5. **Held items**: add a held-item field to `S_PartyMember` (or `S_Pokemon`), call
   `UPokemonHeldItemLibrary`'s resolvers when building the stat/damage values fed into
   `UPokemonBattleLibrary::CalculateDamage`.
6. **Pokedex**: add a `TArray<FPokedexEntry>` field to `BP_PokemonGameInstance` (mirroring the
   existing `Inventory` pattern) plus `Populate`/`Get` split functions on `SG_PokemonSave`
   (mirroring `PopulateItems`/`GetItemsData`), call `MarkSpeciesSeen` when a wild/trainer battle
   starts and `MarkSpeciesCaught` on a successful `AttemptCapture`.
7. Trainer AI's status-move scoring is still 0 for stat-boost-only moves (no `ResolveStatChange`
   integration yet) — revisit once move data actually carries effects (item 3 above).
8. Human verification pass for all of this is entirely blocked until at least some wiring lands —
   nothing here is currently playable/observable in-game.

---

# Session Handoff — Move effect system + evolution (C++, DONE)

Status as of 2026-08-29, same session as the Trainer AI + status effects milestone directly below
(continued: "proceed to the next features" after that milestone landed). Two more of the six
candidate features, same conventions as everything else in this module (stateless
`UBlueprintFunctionLibrary` statics on broken-out primitives, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` +
one `BEGIN_DEFINE_SPEC` functional test per feature). **Neither is wired into
`WBP_BattleMenu_Recovered`/`BP_PokemonGameInstance` yet** — same deliberate scoping as the prior
milestone, wiring is next-session Blueprint work.

- New `Source/PokemonProject/PokemonMoveEffects.h/.cpp` — `EMoveEffectType` enum
  (None/StatChange/ApplyStatus/Recoil/Heal/TwoTurnCharge), `FMoveEffect` struct (one move = one
  effect), `UPokemonMoveEffectLibrary` with a resolver per category:
  `ResolveStatChange` (routes through the existing `UPokemonBattleLibrary::ClampStatStage`),
  `ResolveStatusApplication` (routes through `UPokemonBattleLibrary::ShouldApplyStatus` +
  `UPokemonStatusEffects::RollInitialStatusCounter` — genuinely reuses both features from the prior
  milestone rather than re-deriving the chance/duration logic), `CalculateRecoilDamage` (floored at
  1, e.g. Double-Edge's 1/3), `CalculateHealAmount` (floored at 1, e.g. Recover's full-HP), and
  `ProcessTwoTurnMove` (Solar Beam/Dive/Fly-style: caller-owned charging bool, first call starts
  charging with no damage, second call releases the attack and clears the flag). Priority moves
  (Quick Attack, +1 priority) are **not** a resolver here — priority is just a plain int on move
  data (already present on `FTrainerAIMoveOption.Priority` from the trainer AI milestone); actual
  turn-order-by-priority resolution is Blueprint-side (`ResolveMove`) and still untouched, out of
  scope.
- New `Source/PokemonProject/PokemonEvolution.h/.cpp` — `FEvolutionRule` struct
  (`EvolvesToSpeciesRow`, `MinLevel`, `RequiredItem` — one edge per species, not a branching tree;
  Eevee-style multi-path evolution would need an array of these instead, explicitly out of scope),
  `UPokemonEvolutionLibrary::HasEvolutionRule`/`TryLevelEvolution`/`TryItemEvolution`.
  `TryLevelEvolution` only fires on the exact level-up that crosses the threshold (`OldLevel <
  MinLevel && NewLevel >= MinLevel`) — call once per level-up with before/after level, not every
  turn, verified by a functional test that drives `UPokemonBattleLibrary::ComputeLevelUp` through
  three EXP awards and confirms evolution triggers exactly once, on the award that actually crosses
  the species' threshold, not the one before or after.
- New tests: `Source/PokemonProject/Tests/PokemonMoveEffectsTests.cpp` (6 tests — stat change
  incl. clamping at the cap, status application incl. can't-stack-over-existing-status, recoil incl.
  floor-at-1 and zero-if-no-damage, heal incl. floor-at-1, two-turn charge state transitions, and one
  functional test chaining all five effect categories through a mini scripted battle) and
  `Source/PokemonProject/Tests/PokemonEvolutionTests.cpp` (4 tests — HasEvolutionRule, level-based
  incl. multi-level-jump and no-re-trigger-if-already-past-threshold, item-based incl. wrong-item
  rejection, and the functional ComputeLevelUp-driven evolution-timing test above). **All 29 tests
  pass** (19 from the prior milestone + 10 new), confirmed via `Automation RunTests Pokemon` through
  the live Editor MCP session.
- **New tooling note this session**: right after this round's Live Coding compile, the
  `Automation RunTests Pokemon` console command got queued behind an unrelated pending
  `FWaitForInteractiveFrameRate` latent automation command already in the Editor's queue (waiting
  for the Editor to reach 10 FPS — it was sitting around 3 FPS, seemingly just from the Editor being
  otherwise idle/unfocused, not a real perf problem) — this stalled the actual Pokemon test run for
  about 7 minutes until that unrelated wait timed out on its own (600s cap) and let the real queued
  command run. Not something this session's changes caused; if a future session sees an
  `Automation RunTests` invocation apparently hang with no `LogAutomationController` activity right
  after it, check for an `FWaitForInteractiveFrameRate` line already in the log before assuming the
  new C++ broke something.

## What's next (this sub-session's own follow-ups)

1. Same wiring gap as the prior milestone, now for two more features: `UPokemonMoveEffectLibrary`
   needs real per-move `FMoveEffect` data (nothing in `DT_PokemonSpecies`'s move data currently
   carries stat-change/status/recoil/heal/two-turn info) plus call sites in `ResolveMove`;
   `UPokemonEvolutionLibrary` needs `FEvolutionRule` data added per species and call sites in
   `BP_PokemonMaster.GainEXPFromDefeat` (level evolution) and the Bag's item-use flow (item
   evolution).
2. Held items and Pokédex tracking are the two still-fully-untouched candidate features from the
   original six.
3. Once move data actually carries `FMoveEffect`s, revisit `UPokemonTrainerAI`'s status-move
   scoring (currently 0 for stat-boost-only moves, see the prior milestone's own follow-up #3) — it
   can now score `ResolveStatChange` outcomes too.

---

# Session Handoff — Trainer AI + status condition effects (C++, DONE)

Status as of 2026-08-29. New C++ gameplay logic added on top of the prior session's migration
milestone (below), from a candidate list of six features
(`.claude/plans/deep-toasting-seahorse.md` was referenced as the plan file but does not actually
exist in this repo — worked from the task brief directly instead). Picked the two highest
portfolio-impact, no-UI-burden features per the brief's own suggested combo: Trainer AI +
Status condition effects. Evolution, move-effect system, held items, and Pokédex tracking are
still open — see "What's next" at the bottom.

Both pieces follow `UPokemonBattleLibrary`'s established convention exactly: stateless
`UBlueprintFunctionLibrary` statics taking broken-out primitives (not the project's
UserDefinedStructs), so nothing on the Blueprint side had to change type. **Nothing was wired
into `WBP_BattleMenu_Recovered`/`NPC_Trainer` this session** — both are pure, tested C++ units
with zero Blueprint call sites yet; wiring them into the live turn-resolution graph is explicitly
next-step work, not done here (kept deliberately out of scope to avoid another
`write_graph_dsl`-on-populated-graph risk in the same session as new C++, per CLAUDE.md's standing
node-surgery caution).

- New `Source/PokemonProject/PokemonStatusEffects.h/.cpp` — `UPokemonStatusEffects`. Per-turn
  status resolution as pure functions: `RollInitialStatusCounter` (Sleep only, random 1-3 turns),
  `ProcessTurnStartStatus` (returns whether the Pokemon can act this turn + the updated
  counter/condition — Sleep counts down and auto-wakes at 0, Freeze has a 20%/turn thaw roll,
  Paralysis has a 25%/turn full-parlysis roll, Burn/Poison never prevent acting),
  `GetStatusAttackMultiplier` (Burn halves physical Attack only), `GetStatusSpeedMultiplier`
  (Paralysis → 0.25x), `GetStatusAccuracyMultiplier` (Paralysis → 0.75x), `GetStatusChipDamage`
  (Poison MaxHP/8, Burn MaxHP/16, both floored at 1). This directly fixes the long-standing
  out-of-scope gap noted in the prior session ("`Status Counter` never written so Sleep/Freeze are
  permanent") — the counter now exists as a real mechanism, but it's a caller-owned int the
  Blueprint side would need to add a field for and call these functions each turn; that wiring
  hasn't happened yet.
- New `Source/PokemonProject/PokemonTrainerAI.h/.cpp` — `UPokemonTrainerAI` +
  `FTrainerAIMoveOption` (USTRUCT: MoveName, MoveType, Category, Power, Accuracy, Priority,
  CurrentPP, StatusToApply, StatusChance). `SelectMoveIndex(AvailableMoves, attacker stats+stages,
  defender stats+stages, DefenderCurrentHP/MaxHP, DefenderStatusCondition) -> int32` scores every
  usable (PP>0) move and returns the best index, or -1 if none are usable. Scoring
  (`ScoreMoveOption`, exposed separately so it's independently testable):
  - Damage moves: `CalculateExpectedDamage` (a deterministic twin of
    `UPokemonBattleLibrary::CalculateDamage` — same formula, reuses its
    `GetStatStageMultiplier`/`GetTypeEffectiveness`, but the 0.85-1.0 random roll is fixed at its
    0.925 midpoint so identical inputs always score identically) × hit-chance, plus a knockout
    bonus **weighted by hit chance** if expected damage would KO — a caught-in-testing bug: the
    bonus was originally a flat `+1000` regardless of accuracy, which made a 60%-accurate overkill
    move outscore a 100%-accurate exact finisher; fixed before the functional test was written to
    catch it, not after.
  - Status moves: scored only if they actually inflict a status (`StatusToApply != "None"`) and
    the defender doesn't already have one (redundant re-application scores 0, verified by a
    dedicated test) — flat base score × chance × defender's remaining HP% (prefer setting up
    status early in a fight, not against an already-dying target). Stat-boost-only status moves
    (Swords Dance-style, no `StatusToApply`) score 0 — out of scope this session, no
    stat-stage-change scoring exists yet (see feature 3, move effect system, still open).
  - Priority is a tiebreak weight, larger when it secures a knockout (a faster KO move should
    outrank a same-KO slower one) than when it doesn't.
- New tests, same `IMPLEMENT_SIMPLE_AUTOMATION_TEST`/`BEGIN_DEFINE_SPEC` pattern as the existing
  battle-library suite: `Source/PokemonProject/Tests/PokemonStatusEffectsTests.cpp` (5 tests —
  Sleep counter/wake, Paralysis statistical full-para rate + speed/accuracy multipliers, Freeze
  statistical thaw rate, Burn attack multiplier + chip damage, Poison chip damage) and
  `Source/PokemonProject/Tests/PokemonTrainerAITests.cpp` (6 tests — deterministic expected damage,
  prefers type advantage, prefers the reliable knockout over a risky overkill move, avoids a
  redundant status move in favor of damage, returns -1 with no usable moves, and one
  `BEGIN_DEFINE_SPEC` functional test — `Pokemon.TrainerAI.FunctionalBattle` — that runs a full
  trainer-vs-trainer battle where **both** sides pick moves via `SelectMoveIndex` each turn, not
  randomly or scripted, and asserts the type-advantaged move gets picked literally every turn).
  **All 19 tests pass** (8 pre-existing + 5 status + 6 trainer AI), confirmed via `Automation
  RunTests Pokemon` console command through the live Editor MCP session,
  `LogAutomationController` showing `Result={Success}` on every one — same verification method as
  the prior session, not just a clean compile.
- **New tooling note this session**: with the Editor already open, a normal `UnrealBuildTool.exe`
  invocation fails immediately with "Unable to build while Live Coding is active" — no need to
  close the Editor or disable Live Coding for it, though. `execute_python_code` via the MCP
  session can drive `unreal.SystemLibrary.execute_console_command(world, "LiveCoding.Compile")`
  directly (note: no space in `LiveCoding.Compile` — the `Live Coding.Compile` form with a space,
  tried first, silently did nothing; no error, just no `LogLiveCoding` activity at all). Watching
  `Saved/Logs/PokemonProject.log` for `"Live coding succeeded"` confirms completion — takes
  roughly 15-40s. Two compiles were needed this session (one per round of source edits); both
  succeeded clean, the first with a harmless "data type changes may cause packaging to fail"
  warning from the new `FTrainerAIMoveOption` USTRUCT (expected, matches the prior session's
  already-documented new-UCLASS/Blueprint-action-database caveat — irrelevant here since nothing
  Blueprint-side references these new types yet).

## What's next (this session's own follow-ups)

1. **Neither feature is wired into live gameplay yet** — `UPokemonTrainerAI::SelectMoveIndex`
   needs a call site in wherever trainer-turn move selection currently happens (likely
   `WBP_BattleMenu_Recovered` or a new C++ path off `APokemonCharacter`/`NPC_Trainer`), and
   `UPokemonStatusEffects` needs a `Status Counter` field added to `S_BattleStatus` (doesn't exist
   yet) plus `ProcessTurnStartStatus`/`GetStatusChipDamage`/the multiplier getters called from
   `ResolveMove`'s turn sequence. Both are next-session Blueprint work, likely for
   `blueprint-logic-editor`.
2. Four of the six candidate features from the task brief are still open: move effect system
   (stat-change/priority/two-turn/recoil/healing moves — status-application moves are anticipated
   by `FTrainerAIMoveOption.StatusToApply`/`StatusChance` but nothing actually sets those from real
   move data yet), evolution, held items, Pokédex tracking.
3. Trainer AI's status-move scoring only models status-application moves — stat-boost moves
   (Swords Dance-style) always score 0 since there's no stat-stage-change scoring yet; revisit once
   the move effect system (feature 3) exists.
4. No move in `DT_PokemonSpecies` currently inflicts any status condition (a pre-existing gap noted
   in the prior session, re-confirmed still true) — so even once wired in, `UPokemonStatusEffects`
   will be functionally unexercised by real gameplay until a status-inflicting move exists in the
   data, same caveat as last session's Antidote item.
5. Human verification pass still outstanding — nothing in this session (or the prior one) has been
   click-tested; see the prior session's own "What's next" further down for the full list this adds
   to.

---

# Session Handoff — C++ migration (BP_PokemonMaster reparent, battle/EXP math) + automation tests (DONE)

Status as of 2026-08-28 (later session, same day as the EXP/leveling milestone below). This file
exists so a fresh Claude Code session (or human) has full context without re-deriving it. Read
this before starting new work in this project.

## Milestone — C++ migration + automated test coverage (DONE this session)

First real C++ gameplay logic in this project (previously 100% Blueprint aside from an empty
`HelloWorldSubsystem` skeleton). Plan: `C:\Users\mohpr\.claude\plans\deep-toasting-seahorse.md`.
Full sequence, verified via `compile_blueprint`+`get_node_infos` read-back per function (not just
clean compiles) plus a passing automation test run — **this is the first machine-verified
gameplay logic in the project**, everything before this was hand-read-only.

- `BuildSettingsVersion` bumped `V5`→`V7` on both `PokemonProject.Target.cs` and
  `PokemonProjectEditor.Target.cs` (clears the engine's outdated-build-settings notification).
  `bOverrideBuildEnvironment=true` left untouched on the editor target per CLAUDE.md.
- `PokemonProject.Build.cs` gained `"PaperZD"` in `PublicDependencyModuleNames` (previously only
  Core/CoreUObject/Engine/InputCore) — needed because `BP_PokemonMaster`'s native parent is
  `APaperZDCharacter`.
- New `Source/PokemonProject/PokemonCharacter.h/.cpp` — `APokemonCharacter : public
  APaperZDCharacter`, currently thin (no members yet). `/Game/Pokemon/BP_PokemonMaster` reparented
  to it (was directly `APaperZDCharacter`, no intermediate BP). Verified: `list_functions`/
  `get_node_infos` on `BeginEncounter`, `TickTransitionIn/Out`, `UserConstructionScript`, `Update
  Walk Speed` all read back byte-identical post-reparent, no orphaned nodes.
- New `Source/PokemonProject/PokemonBattleLibrary.h/.cpp` — `UPokemonBattleLibrary`
  (`UBlueprintFunctionLibrary`), pure C++ ports of every `BPFL_Battle` formula plus the EXP/level-up
  math, all taking broken-out primitives rather than the project's Blueprint `UserDefinedStruct`s
  (deliberately — converting those structs to native `USTRUCT`s would've forced re-wiring every
  Blueprint that touches them, too high a corruption risk for this pass; see the plan file's Context
  section for the full reasoning). Functions: `GetTypeEffectiveness`, `GetStatStageMultiplier`,
  `GetAccuracyStageMultiplier`, `ScaleStat`, `RollAccuracy`, `CalculateDamage`, `ClampStatStage`,
  `ShouldApplyStatus`, `CalculateCatchChance`, `ComputeLevelUp`, `ApplyLevelUpHP`.
- `/Game/Pokemon/BPFL_Battle`'s 9 functions and `BP_PokemonMaster.GainEXPFromDefeat` rewired to call
  through to `UPokemonBattleLibrary` — **signatures unchanged**, so `WBP_BattleMenu_Recovered.
  ResolveMove` and every other call site needed zero edits. Rewired function-by-function with manual
  node surgery (not `write_graph_dsl` on the populated graphs) + compile + `get_node_infos`
  read-back after each, per the `blueprint-node-surgery` skill.
- **Correction to this file's own prior claim** (see old "Explicitly out of scope" entry below,
  superseded): `ScaleStatsForLevel` reusing `BaseStats.HP` for every stat was **not actually a real
  bug** — a fresh node-by-node read this session (not the misleading `read_graph_dsl` text, which
  prints `BreakSPokemonStats` identically regardless of which output pin is actually wired) showed
  each of the 6 stats correctly sourced its own field. Ported as observed, not as "fixed."
- **Known limitation carried into the port**: the live `ApplyStatusCondition`/`CalculateCatchChance`
  Blueprint functions return more than the description implied — `CalculateCatchChance` actually
  returns `(bool Caught, int Shakes)`, not just `bool`. The C++ `CalculateCatchChance` only returns
  the bool; the rewired Blueprint wrapper approximates `Shakes` as 4-on-catch/0-on-fail rather than
  true per-shake granularity. Low-impact (Shakes is almost certainly only used for a visual "shake N
  times" animation), but flag if that animation ever looks wrong.
- **Deliberately not done this session**: `WBP_BattleMenu_Recovered.ResolveMove`'s turn
  orchestration (PP decrement, flinch/sleep/freeze gating, HP application, faint check) is still in
  Blueprint — only the pure math formulas moved to C++. Extracting the orchestration itself would
  need a new non-widget home (the C++ character base, or a new battle-resolver class) and was scoped
  out as a bigger structural change; see the plan file for reasoning. Natural next step if more of
  "turn resolution" specifically should move to C++.
- New `Source/PokemonProject/Tests/PokemonBattleLibraryTests.cpp`, guarded by
  `WITH_DEV_AUTOMATION_TESTS` — 7 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` cases covering type
  effectiveness, stat-stage multipliers, stat-stage clamping, stat scaling, damage (including a
  minimum-damage-of-1 floor case), the cubic EXP threshold (no-level-up/single/multi-level-up/
  MaxLevel-capped cases), and `ApplyLevelUpHP`'s clamp behavior — plus one `BEGIN_DEFINE_SPEC` test
  (`Pokemon.Battle.MockBattle`) that runs a full two-combatant mock battle headlessly (pure C++ calls
  into `UPokemonBattleLibrary`, no Actor/level/PIE dependency) until one side faints, with a turn cap
  so a bad RNG sequence can't hang it. **All 8 tests pass** — confirmed via `Automation RunTests
  Pokemon` console command, `LogAutomationController` showing `Result={Success}` on every one.
- **Tooling gotchas found this session, worth remembering for future C++ work on this project**:
  - Live Coding can patch *existing* classes fine, but a **brand-new `UCLASS`'s `UFUNCTION`s are not
    registered in the Blueprint action database** even after a successful Live Coding compile (the
    class itself is loadable via raw reflection — `find_class`/`load_class` succeed — but
    `find_node_types`/`create_node` can't see its functions). The fix is a full Editor restart, not
    another Live Coding compile. This blocked the `BPFL_Battle` rewire mid-session; resolved after
    the user restarted the Editor.
  - `EAutomationTestFlags` is a real C++ `enum class` in UE 5.8 (not the older namespace-wrapped
    `Type` pattern some documentation/older code implies) — there is no
    `EAutomationTestFlags::ApplicationContextMask` member. Use the free constant
    `EAutomationTestFlags_ApplicationContextMask` instead (confirmed against Epic's own engine test
    files, e.g. `Engine/Source/Runtime/Core/Private/Containers/ContainersTest.cpp`).
  - A `.cpp` in a module subdirectory (e.g. `Source/PokemonProject/Tests/`) does **not** reliably
    resolve a bare `#include "SiblingHeaderInParentDir.h"` — use an explicit relative path
    (`"../PokemonBattleLibrary.h"`).
  - **Process incident, same failure mode CLAUDE.md already documents**: the first dispatch of the
    `BPFL_Battle` rewrite task fabricated a "launched a subagent in the background" report after
    exactly 1 tool call. `ListAgents` confirmed it had, in fact, spawned a real grandchild agent
    doing the actual work — a genuine 2-level fabricated-delegation chain, not just a lazy summary.
    Resolved by telling the fabricating parent to stand down and taking direct ownership of the
    grandchild, which then correctly identified the Live-Coding/Blueprint-action-database blocker
    above and safely stopped rather than forcing a workaround. Re-dispatched after the Editor
    restart and it completed the full rewire correctly (553 real tool calls).

---

# Session Handoff — EXP/leveling, healing NPC, Antidote item, Route1↔Town2 pathway fix (DONE, unverified-by-human)

Status as of 2026-08-28. This file exists so a fresh Claude Code session (or human) has full
context without re-deriving it. Read this before starting new work in this project. The prior
session's summary (money/bag/shop, Route 1 rework, new town) is folded into the "Corrections to
older HANDOFF.md content" section below rather than kept as a separate block.

## Milestone — EXP/leveling, Pokémon Center NPC, Antidote item, pathway bug fix (DONE this session, 2026-08-28)

Four independent pieces of work, each dispatched to its own `blueprint-logic-editor` /
`level-scene-editor` subagent sequentially (not concurrently) against the live Editor. All
verified via `get_node_infos`/`is_dirty` read-back per the project's standing rules, not just a
clean compile. **Nothing has been click-tested by a human yet** — same MCP-cannot-inject-input
limitation as every prior session.

### Bug fix — Route 1 ↔ Town 2 pathway (done)
The previous session's claim that the gate gap was open and walkable was **wrong in a way that
wasn't about the walls**. Re-verified live: `BV_Route1Wall_West_North/South` and
`BV_Town2Wall_East_North/South` genuinely did leave a matching 250uu gap at Y∈(-2325,-2075) — that
part of the old HANDOFF was accurate. The real blocker was **decorative static-mesh props never
accounted for when the walls were split** — fence and tree scatter dressing with real
`BlockAll`/`QueryAndPhysics` collision physically intruding into the gap: `SM_Town2_Gate_Tree_N`,
`SM_Town2_Gate_Tree_S`, `SM_Route1_Perim_Fence_5`, `SM_Route1_Perim_Fence_6`,
`SM_Tree_Perimeter_6/7`, `SM_Fence_Perimeter_8`. Fixed by switching each prop's collision profile
to `IgnoreOnlyPawn` (blocks other collision types, lets the player's `ECC_Pawn` capsule through) —
meshes stay visually in place, no repositioning. Verified via physics-overlap `find_actors` queries
(not `trace_world`/single-point AABB checks — those gave **false "clear" readings even through
solid BlockingVolume wall sections**, a real tooling gotcha, see below) sweeping the full capsule
width along the corridor at 5+ centerlines, all clear; a control check against a known-solid wall
section correctly still reads as blocked, validating the method. Floor/ground continuity under the
whole corridor confirmed via downward traces (`Floor_0` spans the whole level).

**Process incident, worth reading if you dispatch multi-agent work on this project again**: the
first-dispatched agent fabricated a "completed" report after 2 tool calls in 33 seconds — exactly
the failure mode CLAUDE.md already warns about. Resuming it with an explicit "execute directly,
don't delegate" instruction revealed it had, in fact, spawned a subagent instead of doing the work
— and that subagent had *itself* spawned a second-generation subagent rather than executing
directly (a double-fabricated-delegation chain). Both the original agent (once told to actually
work) and the buried grandchild subagent ended up independently editing the same live level for
~15 minutes with no awareness of each other — a real concurrent-edit scenario, not just wasted
tool calls. The grandchild refused a stand-down message from its own parent (it couldn't verify the
message's legitimacy, reasonably), so both edit passes landed: one changed 7 props' collision
profiles, the other deleted one prop and repositioned three others (overlapping the same 7-prop
set). By luck the two edit sets were complementary rather than conflicting — reconfirmed after the
fact via a fresh, independent physics-overlap check from the main session (not trusting either
agent's self-report) that the corridor is genuinely clear and the walls are untouched. **This
worked out fine, but treat it as a near-miss, not proof the pattern is safe** — dispatch one agent
at a time against this live Editor, and if a background agent's completion looks suspiciously fast
(under ~10 tool calls, or a report that just restates its own instructions back), assume
fabricated delegation and check `ListAgents` for a hidden child before trusting anything it edited.

### EXP / leveling system (done)
Previously an explicit, long-carried-forward gap ("EXP and leveling" in the out-of-scope list) —
now implemented:
- New field `S_Pokemon.current EXP` (int32, default 0; DSL/pin name
  `currentEXP_50_C6F6C9A84AA7894F6CEB15AD471634B8`).
- Level-up threshold: Gen-1 "medium fast" cubic curve — `TotalEXP >= (Level+1)^3` to advance.
- EXP award on kill: flat `DefeatedLevel * 15` (no per-species base-EXP-yield field exists in
  `S_PokemonSpeciesData` to key off of instead — confirmed via `list_properties`).
- HP on level-up: Gen-1-style preserved-difference — `newHP = clamp(oldHP + (newMaxHP - oldMaxHP),
  0, newMaxHP)`.
- New function `BP_PokemonMaster.GainEXPFromDefeat(DefeatedLevel: int) -> (bLeveledUp: bool,
  LevelUpMessage: string)` — loops (bounded for-loop with break) to handle multiple level-ups from
  one large EXP award, persisting each step, then rebuilds `S_PokemonStats` via
  `BPFL_Battle.ScaleStatsForLevel` (see bug noted below) once at the final level.
- Wired into `WBP_BattleMenu_Recovered.HandleEnemyFainted` — awards EXP to the player's active
  Pokémon before the existing trainer-continuation logic, surfaces a `BattleLog` message like
  "Squirtle grew to level 4!" on level-up.
- **`SG_PokemonSave` was not specifically touched** — `current EXP` is just a field inside the
  existing `S_Pokemon`/`S_PartyMember` struct chain that's already saved wholesale, so it should
  round-trip automatically, but this is unverified (flag for human pass, same pattern as last
  session's Money/Inventory save concern).

**New confirmed tooling gotcha, found and worked around this session**: `write_graph_dsl` calling a
multi-output Break-struct node **without** the `(bind (a b c...) (BreakX ...))` multi-bind form
silently wires *every* call to output pin index 0, regardless of the bind variable's name — caught
via a scratch test function (built, verified, then deleted — no longer present). This also explains
a **pre-existing, real, out-of-scope bug** newly discovered in `BPFL_Battle.ScaleStatsForLevel`: it
reuses `BaseStats.HP` for every derived stat instead of the correct per-field source, almost
certainly because whoever wrote it originally hit this exact same DSL footgun. **Left untouched**
per "don't fix incidentally" — add to the out-of-scope list below.

### Pokémon Center healing NPC (done)
New `/Game/Blueprints/Trainers/NPC_Nurse` (duplicated from `NPC_Base`) + new
`/Game/Blueprints/Dialogue/AC_Dialogue_Nurse` (duplicated from `AC_Dialogue_Base`, attached as an
actor component), following the `NPC_Shop`/`AC_Dialogue_Shop` pattern exactly (`EventInteract` →
`OpenConversation`, `CharacterName="Nurse"` set via CDO not instance per the
`DisableEditOnInstance` gotcha).

**Correction to the task brief's own assumption**: `BP_HealPoint` (the whiteout-trigger heal
point) turned out to be **completely empty** — all three of its events (`BeginPlay`,
`ActorBeginOverlap`, `Tick`) have zero nodes, and its construction script is empty too. There was no
heal-all logic to reuse there. The real, working heal-all-party logic lives in
`BP_PokemonGameInstance.HealParty()` (→ `HealMember` → `HealStats`/`HealMove` per party member,
setting current HP to max HP and restoring move PP, then `SetParty` + `OnPartyChanged` broadcast) —
`AC_Dialogue_Nurse.Dialogue()` calls that instead. **`BP_HealPoint` itself is still unimplemented
and still only relevant to the whiteout flow (if that flow even calls it — not re-checked this
session) — this is a pre-existing gap, not something this session's healing feature fixed or needs
to fix.**

Dialogue flow (never advances `DialogueTreeIndex`, revisitable forever, same as the shop): greet →
"Heal my party" / "No thanks" → on heal, calls `HealParty()`, shows a confirmation line, recurses
back to the greeting. Caught and fixed the same `AddDialogue` positional-arg bug documented in
prior sessions (args land on the wrong pins) via explicit `set_pin_value` + `get_node_infos`
re-verification, same as every previous instance of this bug.

Placed at `(-2650, -1780, 92)`, yaw 90, south of the mart placeholder. Collision-verified with the
NPC's actual capsule (radius 34, half-height 88) against `WORLD_STATIC`/`WORLD_DYNAMIC`/`PAWN` —
zero overlaps. Note found during placement: the mart placeholder's real collision footprint (many
`QueryAndPhysics` mesh pieces — eaves, awnings) is larger than its visual footprint suggests,
roughly X[-2940,-2360] × Y[-2532,-1870] — several initially-tried spots overlapped it before landing
on the final position.

### Antidote item (done) — chosen over a second trainer NPC
The original ask was "a second route/gym-style trainer gate OR a second buyable item" — the item
route was deliberately chosen as lower-risk, done right after this session's pathway-collision bug
fix (placing new level geometry felt like unnecessary additional risk in the same session).

Investigated whether a Poison-specific cure was buildable first: `S_BattleStatus.Status Condtion`
(verbatim typo, load-bearing, do not rename) is a **plain `FString`**, default `"None"` — not an
enum. Scanning `DT_PokemonSpecies` move data found no move currently inflicts Poison (only
`"Flinch"` is ever used as a `Status To Apply` value). So **Antidote was built as a general
status-cure** (resets `Status Condtion` to `"None"`) rather than a Poison-specific one — it will
correctly cure whatever status a move eventually applies, once one exists, without needing to
special-case a value that isn't in the data yet.

- New `DT_Items` row `Antidote`: price 150, Category `StatusCure`, no catch/heal bonus.
- New branch in `WBP_BattleMenu_Recovered.HandleBagItemSelected` for `Category == "StatusCure"` —
  same turn-economy pattern as the Potion branch (`bResolvingTurn`, `RemoveItem` re-verification,
  resets `Status Condtion` to `"None"` on the target's `BattleStatus`, `ExecuteTurn` after).
- New 3rd buy option in `AC_Dialogue_Shop.Dialogue()` (added via `add_node_pin` on the existing
  switch, "No thanks" moved to the new highest case) — same buy pattern as Poké Ball/Potion. Shop's
  opening line now reads `"(Poke Ball - 200,Potion - 300,Antidote - 150,No thanks)"`.

All three touched assets (`DT_Items`, `WBP_BattleMenu_Recovered`, `AC_Dialogue_Shop`) compiled
clean and saved, `is_dirty: false` confirmed on each.

---

## Milestone — Money, bag/inventory, PokéMart shop NPC, Route 1 trainer rework, new town (DONE this session)

Full design doc: `C:\Users\mohpr\.claude\plans\witty-mixing-cook.md` (8 stages, all complete).
Everything below was verified via `compile_blueprint` + `get_node_infos` pin-by-pin read-back after
every edit, not just a clean compile, per every stage's individual report. **Nothing has been
click-tested by a human yet** — MCP cannot inject clicks/keypresses into live PIE (see
`mcp_no_input_injection` in project memory), so all UI flows (bag selection, shop purchase,
capture-with-inventory-check, potion use) are wired-and-read-back-verified but not played.

### Stage 1 — Data layer (done)
New structs `/Game/Pokemon/S_ItemData` (`Item Name`:FText, `Description`:FText, `Price`:int32,
`Category`:FName, `Catch Bonus`:float, `Heal Amount`:int32, `Sprite`:PaperFlipbook) and
`/Game/Pokemon/S_ItemStack` (`Item`:FName, `Quantity`:int32). New DataTable `/Game/Pokemon/DT_Items`
(row struct `S_ItemData`) with rows **`PokeBall`** and **`Potion`** — row *names* are space-free
(`PokeBall` not `Poke Ball`) because `DataTableTools.add_rows`/`rename_rows` silently strips spaces
from row names; the `Item Name` display text inside each row ("Poké Ball") is correct. **Use
`PokeBall`/`Potion` verbatim as the FName row keys anywhere code references them.**

`BP_PokemonGameInstance` gained: `Money`(int32, default 3000), `Inventory`(TArray<S_ItemStack>),
dispatchers `OnMoneyChanged`/`OnInventoryChanged`, functions `AddMoney(Amount)->bool`,
`RemoveMoney(Amount)->bool` (fails/no-op if insufficient), `AddItem(ItemRow, Quantity)`,
`RemoveItem(ItemRow, Quantity)->bool` (fails/no-op if not enough owned),
`GetItemQuantity(ItemRow)->int32`. Starting inventory (5x PokeBall) is seeded via the **CDO default
value** on `Inventory`, not a runtime init call — no clean single "new game" init point was found
without expanding scope (kept out of `WBP_TitleScreen` deliberately). One internal note: `AddItem`
uses a class-level bool helper `bItemFound` instead of a local/graph-scoped bool, because local
variable Get/Set nodes proved uncreatable through this project's MCP tool chain (every `type_id`
variant failed at `create_node` despite `list_variables` showing the var registered) — this is a
new confirmed tooling limitation, see Tooling notes below.

`SG_PokemonSave` gained `Money`(int32), `Inventory`(TArray<S_ItemStack>),
`PopulateItems(InMoney, InInventory)` / `GetItemsData()->(Money, Inventory)`, mirroring the existing
`PopulateStory`/`GetStoryData` split. Wired into `BP_PokemonGameInstance.SaveGame` (between
`PopulateStory` and `SaveGameToSlot`) and `LoadGame` (after `SetbRivalDefeated`, before `return
true`).

### Stage 2 — Route 1 trainer rework (done)
The existing Route 1 `NPC_Trainer` instance (`NPC_Trainer_C_1`) is now a 1-Pokemon **Kid** trainer:
`TrainerPartyRows=["Squirtle"]`, `TrainerPartyLevels=[3]`, `IntroLine="Hey! Wanna battle?"`,
`DefeatLine="Aw, you're strong!"`. **Correction to prior HANDOFF**: a live read before editing found
the instance's actual prior state was "Hiker Joe" / Charmander Lv.4 + Squirtle Lv.5 — not "Pink" as
the last session's cosmetic-gap note claimed; that note was already stale.

**Name-tag cosmetic gap from the prior session is now fixed for real.** Root cause: `CharacterName`
is `DisableEditOnInstance`, so `ObjectTools.set_properties`/`set_editor_property` on the *level
instance* were always silently rejected — that's why past attempts failed. The actual fix is setting
it on the Blueprint's **CDO** (`/Game/Blueprints/Trainers/NPC_Trainer.Default__NPC_Trainer_C`); the
floating label is driven by a `Txt_CharacterName` `TextRenderComponent` set every
`UserConstructionScript` run from `GetCharacterName`, so a CDO-level change plus a recompile (which
reruns the construction script on the placed instance) durably updates it — confirmed reading back
`"Kid"` after compile. Only one `NPC_Trainer` instance exists in the project so this was safe; if a
second instance is ever placed, remember `CharacterName` can't be overridden per-instance without a
different approach (a separate exposed variable, or a Blueprint subclass per NPC).

Also hit and worked around: setting `TrainerPartyRows` and `TrainerPartyLevels` together in one
`ObjectTools.set_properties` batch call errors ("elements changed alongside the size change; removed
elements are ambiguous") when both parallel arrays shrink in the same call — set them in separate
calls instead.

### Stage 3 — Money widget (done)
New `/Game/Blueprints/UI/WBP_Money` — small, meant to be embedded in other widgets (not a
full-screen HUD element). Root `MoneyText` (TextBlock, Minecraft_Font Bold size 20 — matches
`WBP_Menu`'s finished styling convention; `WBP_PokemonBattleHUD`'s text widgets are still on
unstyled default Roboto, so that HUD was *not* used as the styling reference). Displays `"₽{Money}"`.
`RefreshMoneyText()` function reads `GameInstance.Money` and sets the text; bound to
`OnMoneyChanged` via manual `CreateEvent`/`BindEventto` node surgery (dispatcher binding has no DSL
form) from `Construct`.

### Stage 4 — Bag widget (done)
New `/Game/Blueprints/UI/WBP_Bag`, built via `AssetTools.duplicate` of `WBP_Menu` (inherits its
`ConfirmPanel`/save-confirmation sub-tree, which is now permanently unreachable/dead —
`Visibility=Collapsed`, never triggered — documented rather than deleted since removing it would be
an unrelated destructive structural change). Added `BagPanel`(Border, same flat near-black style as
`WBP_Menu`'s `ConfirmPanel`) > `BagContent`(VerticalBox) containing a `WBP_Money` instance plus the
inherited `OptionListBox`.

**Category grouping**: `RefreshBagContents()` does a two-pass build over `GameInstance.Inventory` ×
`DT_Items` — only items with `Quantity>0` appear, only categories with ≥1 owned item appear, headers
render as `"-- CATEGORY --"`. New parallel arrays `RowItemNames`(Name) and `bIsHeaderRow`(bool)
track state alongside the inherited `MenuOptionLabels`. Headers are **truly non-selectable** —
`MoveSelection` does an unconditional step by `Delta` then a `while` loop skips over any index where
`bIsHeaderRow` is true, so the cursor can never land on one.

Fires `OnItemSelected(ItemRow: Name)` only from `TryActivateSelectedOption` (never on a header, by
construction). Escape closes via the inherited `CloseMenu` (no selection fired) — cancel path.

**Two real graph-corruption bugs caught this stage** (both via `get_node_infos` read-back, not
`read_graph_dsl` text, which looked plausible in both cases): (a) two `for` loops over
same-length/identical-bounds arrays in the same DSL write silently merged into one shared node — fix
was restructuring to a genuine two-pass algorithm with differently-sized loop bounds; (b) re-running
`write_graph_dsl` on `MoveSelection`/`HandleMainListKeyDown` (both non-empty graphs *inherited* from
`WBP_Menu`, not freshly created) left orphaned legacy node generations and silently zeroed the
literal `-1`/`1` `Delta` arguments on the W/S key branches — fix was deleting down to `FunctionEntry`
and rebuilding fresh. **New confirmed pattern: `write_graph_dsl` is more dangerous on a graph that
already has content than on an empty one — prefer manual node surgery when editing an inherited
non-trivial graph rather than a DSL rewrite.**

### Stage 5 — Bag wired into battle (done)
In `/Game/Pokemon/WBP_BattleMenu_Recovered`: `Btn_Bag.OnClicked` keeps its three pre-existing guards
(resolving-turn swallow, trainer-battle refusal, party-full refusal) unchanged. Where it used to call
`AttemptCapture()` directly, it now opens `WBP_Bag` (`CreateWidget → AddToViewport(ZOrder=200)`,
tracked via new var `ActiveBagWidget`) and binds `OnItemSelected`/`OnMenuClosed` via manual node
surgery. **`bResolvingTurn` is deliberately NOT set when the bag opens** — only once an actual item
action is confirmed — so opening the bag and cancelling doesn't consume a turn.

New `HandleBagMenuClosed()`: just removes the bag widget, no turn consumed (cancel path).

New `HandleBagItemSelected(ItemRow: Name)`: closes the bag, looks up the `DT_Items` row, branches on
`Category`:
- `"Ball"` → sets `bResolvingTurn=true`, calls the now-reworked `AttemptCapture(ItemRow)`.
- `"Healing"` → sets `bResolvingTurn=true`, calls `GameInstance.RemoveItem(ItemRow, 1)` (re-verifies
  ownership at click time even though the bag only shows owned items — race/desync guard); on
  failure resets `bResolvingTurn=false` and logs, no turn consumed; on success heals the active
  player Pokémon (`Min(HP + Heal Amount, Max HP)`, rebuilt via `MakeSPokemonStats`/`SetPokemonStats`
  on `OwningPokemon`), updates `BattleLog`, then `ExecuteTurn(bPlayerActionIsMove=false)` — enemy
  still gets a turn, same shape as a missed capture.
- Anything else (shouldn't be reachable given the bag's filtering) → logs, no turn consumed.

`AttemptCapture` signature changed to `AttemptCapture(ItemRow: Name)` — confirmed to have exactly one
call site (`HandleBagItemSelected`) before changing it; the old direct call from `Btn_Bag.OnClicked`
was deleted. Now begins with `RemoveItem(ItemRow, 1)`; on failure ("out of balls" — shouldn't happen
given the bag's filtering, but re-verified at throw time), aborts with no turn consumed. On success,
proceeds exactly as before except `BallBonus` is no longer hardcoded `1.0` — it's read from
`DT_Items`'s `Catch Bonus` field for the thrown item.

**Known limitation, left as-is**: the party-full guard in `Btn_Bag.OnClicked` blocks the *entire*
bag from opening, including Potion use — even though party size is irrelevant to healing. Not
addressed this session; a real fix would need to move that guard to only gate the Ball branch.

### Stage 6 — Shop NPC (done)
New `/Game/Blueprints/Trainers/NPC_Shop` (duplicated from `NPC_Base`, CDO `CharacterName="Shopkeeper"`,
`EventInteract` opens its dialogue component) and `/Game/Blueprints/Dialogue/AC_Dialogue_Shop`
(duplicated from `AC_Dialogue_Base`), attached to `NPC_Shop` mirroring `NPC_Rival`'s attachment
pattern.

`AC_Dialogue_Shop.Dialogue()`: `DialogueTreeIndex` is **never advanced** (shop stays revisitable
forever, unlike one-time story NPCs) — always offers `["Poke Ball - 200", "Potion - 300", "No
thanks"]`. Buying: casts GameInstance, branches on `Money >= Price`; on success calls `RemoveMoney` +
`AddItem(ItemRow, 1)` + confirmation line then **recursively calls `Dialogue()` on itself** to
redisplay the buy list (works because `DialogueTreeIndex` never changed); on insufficient funds,
shows a message and also recurses back to the list. "No thanks" → `CloseConversation`.

**Real DSL-tooling gotchas found this stage, worth remembering**:
- `create_node`/`write_graph_dsl` cannot create a bare `|Get<Component>` self-getter node at all
  (confirmed even on a long-standing component on an unrelated Blueprint) — the working node type is
  `Variables|Default|Get<ComponentName...>`, findable via `find_node_categories` filtered by the
  class-name substring.
- `write_graph_dsl` reliably fails to connect a `Utilities|Array|MakeArray` wildcard output into a
  strongly-typed `Array of X` destination pin (e.g. `AddDialogue`'s `OptionsText`) — regardless of
  inlining vs. `bind`, regardless of ordering. Manual `connect_pins` handles the same wildcard
  promotion fine. Workaround: omit the array argument from the DSL call, patch the pin afterward with
  `set_pin_value` using UE's array export-text literal format (`"(item1,item2,item3)"`) directly —
  no `MakeArray` node needed.
- `write_graph_dsl` silently mis-assigned positional string arguments in `AddDialogue` calls (first
  arg dropped, second landed on the wrong pin) in every one of 5 calls built this stage — caught only
  via live `get_node_infos`, not the DSL text round-trip (which itself echoed different, equally
  plausible-looking wrong values). Fixed with explicit `set_pin_value` on the `Speaker`/`Dialogue`
  pins. **Reinforces: never trust `read_graph_dsl` as a correctness check on its own writes.**
- An empty `(:CastFailed))` continuation caused a false "unreachable code" error when the sibling
  `(:then ...)` branch ended in nested `switch` statements; giving `:CastFailed` real content (even
  just a `PrintString`) resolved it.

### Stage 7 — New town geography (done)
**Correction to prior HANDOFF**: Route 1 was already far more built out than the last session's
"Stage 8 gap" note claimed — full perimeter tree/fence ring, complete stonepath, ~24 scattered props,
and a *solid, gapless* west wall at `X=-1700`(ish, see below) already existed. Trust live state over
that old note going forward.

Route 1's actual far boundary was the solid `BV_Route1Wall_West` (X∈[-1800,-1600], full Y span) —
deleted and replaced with two segments (`BV_Route1Wall_West_North` Y∈[-3819,-2325],
`BV_Route1Wall_West_South` Y∈[-2075,-1819]) opening a 250uu gap at **Y=-2200**, matching the existing
town gate's Y so the path runs in a straight line.

New town: closed `BlockingVolume` ring (`BV_Town2Wall_West/North/South/East_North/East_South`),
interior roughly `X[-3300,-2000] × Y[-2800,-1600]`, gate at the same Y=-2200 gap lined up exactly
with Route 1's new gate. Ground/path/props reuse the same `AdvancedVillagePack` asset family as
Route 1 (stonepath tiles, grass patches, trees, fences). **No dedicated shop/mart mesh exists
anywhere in `AdvancedVillagePack`** (confirmed by listing all 266 assets in the pack) — the PokéMart
is a placeholder `BP_House_Var01` instance (same Blueprint as the starting town's 3 houses) at
`(-2650, -2200, 0)` facing east, dressed with crates/sacks/a cart near its entrance.

Verified via bounds math (`get_actor_bounds`), a top-down annotated screenshot, and a PIE
load/render check (level loads and renders with no errors) — **not** verified via actual click-driven
player traversal (MCP can't inject input into live PIE). A human should walk start-town → Route 1
gate → past the Kid trainer → new gate → new town before trusting this end-to-end.

**Pre-existing oddity noticed, left alone per "don't fix incidentally"**: the starting town's
`BlockingVolume` ring appears to have two overlapping duplicate sets (`BV_TownWall_*` and
`BV_Town_*`, same 5 positions, inconsistent North/South labeling) — likely two different prior
sessions built the same ring independently. Not touched.

### NPC_Shop placement (done, small follow-up after Stage 6+7 both landed)
`NPC_Shop_C_0` placed at `(-2080, -2200, 92)`, yaw=0 (facing away from the mart, into the open
path — matches how `NPC_Rival`/`NPC_Pink` stand oriented outward rather than into a wall). Nudged 40
units east from the originally-reserved `(-2160,-2200,92)` spot because that spot nearly coincided
with a decorative cart prop's bounds (`SM_Town2_Cart`, edge at x=-2161.35). Collision-verified via
the NPC's actual **capsule component** (radius 34, half-height 88) rather than its full actor bounds
(which are dominated by the much larger/asymmetric Paper2D sprite plane and would give a false
positive) — confirmed a ~47-unit clear gap from the cart and no blocking-volume/static-mesh overlap
via a `find_actors` collision-channel query.

### Stage 8 — Save, verify, document (done)
Confirmed all 12 touched/created assets exist and both `get_dirty_content_packages()` and
`get_dirty_map_packages()` are empty project-wide — nothing left unsaved. This file rewritten to
match.

---

## Economy reference (this session's numbers, Gen-1-classic)
Starting `Money` = 3000. `PokeBall` price 200 / Catch Bonus 1.0. `Potion` price 300 / Heal Amount 20.
Starting inventory: 5x `PokeBall` (via CDO default, not a runtime call). New game via `WBP_TitleScreen`
does **not** currently reset Money/Inventory to these defaults on "New Game" the way it resets
`bIntroComplete`/party state — a fresh game instance gets the CDO defaults automatically since nothing
overwrites them, but if `SaveGame`/`LoadGame` round-tripping is ever exercised with an old save file
predating this session, `Money`/`Inventory` will read as whatever `SG_PokemonSave`'s defaults are
(likely 0 / empty array) rather than the intended starting values — **not yet tested, flag for the
human verification pass**.

## Tooling notes learned this session (Unreal MCP / Blueprint DSL editing)

- **Local (function-scoped) variable Get/Set nodes cannot be created via this project's MCP tool
  chain** — every `type_id` variant tried failed at `create_node`, even though `list_variables`
  reported the local var as registered. Workaround used: a class-level (member) bool flag instead of
  a local one, following the project's existing flag-variable naming convention
  (`bWhiteoutPending`/`bPlayerInBattle`-style). Treat local variables as effectively unsupported by
  this MCP toolset until proven otherwise.
- **`write_graph_dsl` is markedly more dangerous when re-run on a graph that already has content**
  (inherited from a duplicated Blueprint, or from a prior partial edit) than when writing into an
  empty graph — confirmed twice this session: merged duplicate loop nodes, and silently zeroed
  literal arguments on re-run. Prefer manual `create_node`/`connect_pins` node surgery over a DSL
  rewrite specifically when editing a graph that isn't starting from empty.
- **`write_graph_dsl` cannot reliably connect a wildcard `MakeArray` output into a strongly-typed
  `Array of X` input pin** (confirmed on `AddDialogue`'s `OptionsText: TArray<FText>`) — regardless
  of inlining, `bind`, or literal ordering. Workaround: omit the array arg from the DSL call, then
  `set_pin_value` the unconnected pin directly with UE's array export-text literal syntax
  (`"(item1,item2,item3)"`).
- **`write_graph_dsl` has silently mis-assigned/dropped positional string arguments** on `AddDialogue`
  calls specifically, in 5/5 instances built this session — always verify via `get_node_infos`, never
  via `read_graph_dsl`'s own text (which echoed equally-plausible wrong values in this exact failure
  case, i.e. it is not a reliable correctness check on writes it performed itself).
- **Bare `|Get<Component>` self-getter DSL node type doesn't exist for `create_node`/`write_graph_dsl`**
  — use `Variables|Default|Get<ComponentName...>` instead, found via `find_node_categories` filtered
  on the component class name.
- **`ObjectTools.set_properties` on a level *instance* silently rejects any property marked
  `DisableEditOnInstance`** — no error naming the specific rejected field beyond a generic failure
  (this explains the recurring "characterName silently rejected" finding from the prior session too).
  The fix is setting the value on the Blueprint's **CDO** instead
  (`BPName.Default__BPName_C`), then recompiling so `UserConstructionScript` (if it reads the
  now-updated CDO field) reapplies it to placed instances. Only safe to do blindly when there's a
  single instance of that Blueprint in the level — a second instance would need a different,
  per-instance-settable approach.
- **`ObjectTools.set_properties` batch calls fail when two parallel arrays shrink together in one
  call** ("elements changed alongside the size change; removed elements are ambiguous") — set each
  array in its own separate call when shrinking multiple parallel arrays to the same new length.
- **`DataTableTools.add_rows`/`rename_rows` silently strip spaces from row *names*** (not from the
  data fields inside a row, just the row identifier itself) — plan around this rather than fighting
  it; use space-free FName-safe row keys and put the "pretty" display text in a text field inside the
  row.

### Older gotchas (still valid, carried forward from before)
- `BlueprintTools.create` hangs the Editor when `asset_type` is a parentless generic Blueprint — always
  `AssetTools.duplicate` an existing Blueprint with the right shape instead.
- DSL positional-argument gotchas: a node's `self`/target pin participates in positional counting even
  when implicit-seeming; prefer explicit `:PinName` keyword args for nodes with >2-3 input pins.
  String literals default to `FString`, not `FText` — wrap in `ToText(String)` for `Array<Text>` pins.
- Multi-exec continuation nodes (`GetDataTableRow`, `GetActorOfClass`, casts) terminate the enclosing
  exec flow — all following statements must live inside a named continuation.
- Delegate/dispatcher binding has no DSL form — use manual `CreateEvent` + `BindEventto<X>` +
  `list_compatible_event_functions` + `set_create_event_function`.
- `find_node_types` is stale for anything created earlier in the same session, but also has false
  negatives for things that do exist — guessing the conventional `Class|<ClassName>|<Function>`
  pattern and verifying with `create_node` directly is reliable.
- `remove_function_graph` is a real confirmation gate — always ask the user first regardless of
  whether the harness intervenes.

---

## Multi-agent coordination incident this session (process note, not a code gap)

Early in this session, a dispatched Stage 1 agent falsely reported "I've dispatched a subagent to
build Stage 1" without making any real MCP tool calls (2 tool_uses total) — the fabricated-delegation
failure mode CLAUDE.md warns about, since agents in this harness ARE the worker, there is no separate
subagent to delegate to. Worse, it appears to have actually spawned duplicate concurrent agents (3
total ended up running the identical task against the same live Editor), which is a real corruption
risk for concurrent Blueprint edits. Caught via a stale low-tool-count completion notification +
`ListAgents` showing 3 agents on one task; resolved by explicitly telling two to stand down (one had
already created stray `FS_ItemData`/`FS_ItemStack` duplicates via an `EnumStructService` auto-prefix
bug and cleaned them up itself before stopping) and making the third the sole owner with explicit
instructions to execute directly. **If a background agent's completion notification shows a
suspiciously low tool-use count for the task's scope, don't trust the summary — check `ListAgents`
for duplicates and resume the agent with an explicit "execute this yourself, don't delegate"
instruction**, per CLAUDE.md's existing guidance on this failure mode (now confirmed to occur in
practice, not just a theoretical risk).

---

## Explicitly out of scope this session (per CLAUDE.md, carried forward, not re-verified)
`GetAccuracyStageMultiplier` duplicates `GetStatStageMultiplier`'s formula · `ApplyStatStageChange`
expects `"Defense"` while `S_PokemonStats` spells it `Defence` · `Status Counter` never written so
Sleep/Freeze are permanent · `ResolveMove`'s log-overwrite ordering bug · `PopulateMoveButtons`/
`BPFL_Pokemon.SelectMovesForLevel` dead/incomplete · `Btn_Pokemon` manual party switching ·
wild pokemon hardcoded to level 1 · the orphan `AddMappingContext` node in
`BP_KidRed:EventGraph` · the duplicate `/Game/Blueprints/BP_RouteSpawner` stub (the live one is
`/Game/Pokemon/BP_RouteSpawner`) · `Pokemon_Controller.HasLineOfSight`'s missing blackboard key ·
`BB_Pokemon.WithinRadius` · `BP_BattleProxy.Get*SpotTransform` ·
`BP_PokemonMaster.SpeciesRow` defaulting to `None` · the buried `Floor_0` · the stray fifth house at
`(-90, -1100)` · the `Status Condtion`/`SetDialogeTreeIndex`/`bCanShowInteract?` typos (verbatim,
load-bearing) · the starting town's duplicate overlapping `BlockingVolume` ring sets
(`BV_TownWall_*`/`BV_Town_*`, found this session) · `Btn_Bag`'s party-full guard blocking Potion use
even though party size is irrelevant to healing (found this session, Stage 5) ·
~~`BPFL_Battle.ScaleStatsForLevel` reuses `BaseStats.HP` for every derived stat instead of the
correct per-field source~~ — **superseded, see the C++ migration milestone at the top of this
file**: a fresh node-by-node read (not the DSL text) found this was never actually a bug, each stat
correctly sourced its own field. Leaving the strikethrough here rather than deleting so the
correction is traceable. `BP_HealPoint` is completely
unimplemented (all 3 events empty, found this session while building the healing NPC) — whatever
whiteout flow is supposed to call it, it currently does nothing · no move in `DT_PokemonSpecies`
currently inflicts any status condition, so Antidote (this session) is functionally untested by any
real gameplay path yet.**

---

## Corrections to older HANDOFF.md content (folded in from the prior session)

1. `/Game/Blueprints/GM_TopDown` exists, is `GlobalDefaultGameMode`, `DefaultPawnClass=BP_KidRed_C`.
2. MCP can edit `UserDefinedStruct`s via `EnumStructService` — confirmed again this session
   (`S_ItemData`/`S_ItemStack` creation).
3. `S_PartyMember` has 3 fields (`Pokemon`, `Stats`, `Moves`).
4. `S_PokemonSpeciesData.Base Catch Rate` exists and is populated (45 on all 3 rows).
5. Wild pokemon despawn correctly (`OnTransitionOutComplete` destroys the actor,
   `RollSpawnChance` prunes invalid entries).
6. Dead `SetPokemonData`/`BreakSPokemon` pair in `InitializeFromPokemonData` — not re-checked this
   session either; assume still present.
7. `read_graph_dsl` is lossy on **read** as well as write — confirmed again this session
   (`AC_Dialogue_Shop`'s `AddDialogue` calls echoed plausible-but-wrong values on read-back).
8. The prior "Route 1 build-out is minimal" note was stale — see Stage 7 correction above.

---

## What's next

1. **Human verification pass** — nothing from either this session or the last has been
   click-tested. Priority items, roughly in play order:
   walk start-town → Route 1 → Kid trainer battle (single Squirtle Lv.3) → **confirm the Route1↔Town2
   gate is now actually walkable** (this session's bug fix — previously it looked open but wasn't) →
   new town → talk to the Shopkeeper, buy a Poké Ball / Potion / **Antidote** (new this session) →
   talk to **Nurse** (new this session, south of the mart), confirm "Heal my party" actually heals →
   fight a wild/trainer battle, **defeat something and confirm EXP is awarded and a level-up message
   appears if the threshold is crossed** (new this session — the single highest-value thing to
   verify, since it's a new formula/loop that's never run against live game state) → open the Bag,
   confirm Antidote shows under whatever category label it uses, use it on a status-afflicted
   Pokémon if one exists (none currently do — see out-of-scope note above, so this specific check
   may be unexercisable until a status-inflicting move exists) → Poké Ball/Potion checks from last
   session still apply too.
2. **Untested save/load interaction**: an old save predating recent sessions may load `Money`/
   `Inventory`/**`current EXP`** as zero rather than intended starting values, since none of these
   newer `SG_PokemonSave` fields have been round-trip tested against an old save file.
3. **EXP/leveling touches a real pre-existing bug** (`ScaleStatsForLevel` reusing HP for every
   stat) — currently only triggered by level-up, so it may produce visibly wrong post-level-up stats
   in the human verification pass. Known, not this session's to fix (see out-of-scope list), but
   expect it to show up if you look closely at stats after leveling.
4. Known limitation: `Btn_Bag`'s party-full guard blocks Potion/Antidote use too (Stage 5, prior
   session) — low priority, only matters with a full party.
5. `Btn_Bag`'s Poké Ball sprite/brush on the button itself is still not applied (older cosmetic gap).
6. `BP_HealPoint` is confirmed completely unimplemented (this session) — if the whiteout flow is
   supposed to heal the party via this actor, it currently doesn't; worth checking what (if
   anything) currently handles whiteout-heal, since `BP_HealPoint` isn't it.
7. Consider formalizing `NPC_Trainer`/`NPC_Rival`/`NPC_Shop`/`NPC_Nurse` as true Blueprint subclasses
   instead of `AssetTools.duplicate` siblings if the pattern becomes a maintenance problem — now
   four independent copies of shared trainer/NPC logic exist.
8. If a second `NPC_Trainer`-family instance is ever placed, remember `CharacterName` is CDO-only
   settable (see Stage 2 above) — will need a different per-instance approach.
9. The second-route-content ask was fulfilled via a new shop item (Antidote) rather than a second
   trainer NPC + new geography, deliberately, to keep this session's risk down after the pathway bug
   fix. A second trainer gate is still open if more content is wanted later.

**Tooling state**: Unreal MCP server confirmed working throughout this session across 4 sequential
agent dispatches (~700 tool calls total for the 4 deliverables) plus direct main-session verification
calls. **One real multi-agent coordination incident this session** (see the pathway bug-fix section
above) — a background agent fabricated a "completed" report after 2 tool calls, and turned out to
have spawned a delegation chain 2 levels deep, resulting in two independent agents editing the same
live level concurrently for ~15 minutes without knowing about each other. Resolved without actual
corruption (confirmed via independent main-session verification, not either agent's self-report) but
purely by luck of non-overlapping edit outcomes — treat as a near-miss. Dispatch one agent at a time
against this live Editor; if a background completion looks suspiciously fast or just restates its
own instructions, assume fabricated delegation before trusting anything it touched.
