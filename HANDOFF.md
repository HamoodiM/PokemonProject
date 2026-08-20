# Session Handoff — Battle system built, needs manual click-through verification

Status as of 2026-08-20 (updated same day, second session — two bugs found via real play, fixed).
This file exists so a fresh Claude Code session (or human) has full context without re-deriving it.
Read this before starting new work in this project.

## Two bugs found by the user playing the battle system, both fixed (2026-08-20)

- **`BP_RouteSpawner.IsSpawnLocationValid` runtime crash** ("Accessed None ... CallFunc_Array_Get_Item"):
  its `ForEachLoop` over `WildPokemonList` called `GetActorLocation` on each entry with no
  `IsValid` check — a destroyed/stale wild-pokemon reference crashed it. Fixed by adding the same
  `IsValid` guard `RollSpawnChance` already had (stage 6 of the battle system pruned the list there,
  but this sibling function had its own separate unchecked loop). Dynamically verified in PIE:
  spawned 7 wild pokemon walking the route repeatedly, zero recurrence.
- **Overlapping battles**: a second wild pokemon could chase the player and start its own encounter
  while one was already in progress. Fixed with a new global `bPlayerInBattle` bool on
  `BP_PokemonGameInstance` (set `true` in `BeginEncounter`, `false` in `OnTransitionOutComplete`),
  checked by `BTT_ChasePlayer` (fails/falls through to patrol if true) — patrol
  (`BTT_FindRandomPatrol`) is untouched, so other wild pokemon keep roaming, they just don't chase
  while a battle's active. **Not dynamically verified** — the MCP toolset can't inject player
  input, and teleport-based test movement doesn't reliably fire the overlap that starts an
  encounter (confirmed again this session, same as prior sessions' notes below). Verified via
  exhaustive node-graph inspection only; worth a manual check (walk near a second wild pokemon
  while mid-battle, confirm it doesn't also engage).

## Milestone — Core turn-based battle system (DONE, prior session)

Full design doc: `C:\Users\mohpr\.claude\plans\wobbly-tickling-sonnet.md` (staged plan, all 6
stages complete). Summary of what's now live:

- **Move data pipeline**: `S_TMMove` extended with category/stat-change/status fields; all 12
  existing moves (Charmander/Bulbasaur/Squirtle's Scratch/Growl/Ember/Smokescreen etc.) populated
  with real data in `DT_PokemonSpecies`. `BPFL_Pokemon.MakePokemonFromSpecies` now selects a real,
  level-filtered, PP-tracked moveset (up to 4 moves) instead of hardcoding empty strings, and
  scales stats by level (`BPFL_Battle.ScaleStatsForLevel`) instead of using raw base stats. Wired
  through all 3 creation paths: starter pick (`AC_Dialogue_Pink`), wild spawn
  (`InitializeFromSpecies`), and party-to-battle-actor (`InitializeFromPokemonData`).
- **New struct `S_BattleStatus`**: battle-only transient state (stat stages -6..6, non-volatile
  status condition, flinch) — note its status field is named `StatusCondtion` (typo, missing "i"
  from a manual struct edit — functional, just misspelled; rename later if it bothers you, nothing
  depends on the correct spelling).
- **New `/Game/Pokemon/BPFL_Battle`** function library: type effectiveness (Fire/Water/Grass/
  Normal), stat-stage multipliers, damage calc (STAB + type effectiveness + variance), accuracy
  rolls, stat-stage/status application.
- **Battle UI**: `WBP_BattleMenu_Recovered` now has a working move-selection panel (4 buttons,
  PP-aware enable/disable), a `BattleLog` text line, and `Btn_Bag`/`Btn_Pokemon` disabled (capture
  and party-switch are explicitly out of scope for this checkpoint).
- **Turn loop**: `ExecuteTurn`/`ResolveMove` handle AI move choice, speed-based turn order
  (paralysis halves speed), accuracy rolls, physical/special damage, stat-stage and status-chance
  moves, flinch, poison/burn end-of-turn ticks, and faint detection.
- **`ConcludeBattle(Outcome)`**: on Won/Lost/Fled, syncs the player pokemon's final HP and PP back
  into `BP_PokemonGameInstance.Party[0]` (this didn't exist before — HP/PP were one-way and
  appeared to "reset" between fights), logs the outcome, then reuses the existing
  transition-out/destroy sequence unchanged.
- **`BP_RouteSpawner.WildPokemonList` gap fixed**: `RollSpawnChance` now prunes `!IsValid` entries
  before checking the concurrent-spawn cap, so defeating a wild pokemon actually frees up a spawn
  slot (previously the list only ever grew, so the route "ran dry" after 3 total spawns/session —
  this is the fix promised in the prior session's HANDOFF entry).

### Explicitly deferred (per user decision, see the plan doc's "Scope boundary")

Multi-hit moves, two-turn charge moves, forced switch-out, counter/reflect moves, capture (Bag),
mid-battle party-switch (Pokemon button), EXP/leveling, and a proper "all pokemon fainted"
whiteout/heal flow. `S_TMMove` reserves flag fields (`is Multi Hit`/`is Two Turn`/`forces Switch`/
`is Counter Move`) for the deferred flow-control moves so adding them later won't need another
struct migration — but no execution logic for them exists yet. A "Lost" battle currently ends the
same way as Won/Fled (transition out, no whiteout) — the player's pokemon persists at 0 HP in
`Party` with no healing path yet; a real fix needs a healing location/UI, a separate future feature.

### ⚠️ Not yet verified — needs a manual pass in-editor

The MCP toolset cannot inject clicks or call arbitrary UFUNCTIONs on a live PIE instance (a
limitation hit repeatedly this session and on the title screen previously) — so **no part of this
battle system has been click-tested end to end**. Everything was verified via `get_node_infos`
node-by-node wiring inspection and a clean PIE boot (no compile/runtime errors), which is
thorough but not the same as playing it. Before trusting this is done, manually: trigger an
encounter, use Fight → pick each move type at least once (a damage move, a stat-change move like
Growl, a status-chance move like Bite), confirm HP bars update, get a pokemon to 0 HP both
directions (win and lose), confirm `BattleLog` text reads sensibly, confirm Run works mid-battle,
and confirm a second encounter afterward shows the correct carried-over HP/PP on your party
pokemon (proves the sync-back actually works).

### Known cleanup items from this session (minor, not blocking)

- Manual struct edits were required for stage 1 — the MCP toolset has **no way to add/edit fields
  on a `UserDefinedStruct` asset** (checked every plausible tool path). Confirmed limitation, worth
  remembering for any future struct changes: they need a human in the Editor UI.
- A duplicate/stalled background-agent run during stage 4 left an orphaned, unused function
  `PopulateMoveButtons` on `WBP_BattleMenu_Recovered` (dead code, has a likely PP-display bug, not
  called from anywhere). Removal was blocked by the safety classifier (destructive-removal gate) —
  asked the user for explicit permission, no response yet as of this write-up. Safe to remove once
  confirmed.
- This session repeatedly hit background agents replying with a plausible-sounding "I've
  dispatched a subagent to handle X" summary without making any real MCP tool calls — always
  requiring an explicit resume-and-insist to get real work done. **CLAUDE.md rule 6 now documents
  this** — check `tool_uses` count on completion notifications, and check in on agents quiet for
  15-20+ minutes.

**Tooling state**: Unreal MCP server confirmed working. Note from this session: when delegating to
background subagents, always confirm the resumed/spawned agent actually executed tool calls (check
its `tool_uses`/`duration_ms` in the completion notification) — a couple of runs this session
returned a plausible-sounding text summary without touching the MCP tools at all, requiring an
explicit resume-and-insist before real work happened.

## Feature — Title/loading screen (DONE, this session)

`/Game/Blueprints/UI/WBP_TitleScreen` (new): full-screen title screen shown on game start —
"Pokemon Project" placeholder title top-center, Continue/New Game/About buttons stacked below
(styled to match `WBP_BattleMenu_Recovered`), About opens a collapsed-by-default credits popup with
its own close button. Continue and New Game currently do the exact same thing (no save/load system
exists yet — deliberate placeholder per user decision).

- No `GameMode`/`PlayerController` Blueprint exists in this project — hooked in via
  `BP_KidRed.EventBeginPlay` (the same pawn-based bootstrap pattern already used for
  `WBP_PartyDisplay`), which now creates/adds `WBP_TitleScreen` to the viewport.
- Player input gating: the pre-existing `AddMappingContext(IMC_TopDown)` call was moved out of
  `EventBeginPlay`'s live exec chain into a new `BP_KidRed.EnableGameplayInput` function, called
  only once the title screen is dismissed.
- Crossfade (title fades out while overworld is already rendering underneath): **not** a native
  `UWidgetAnimation`/Sequencer track — the UMG toolset this project's MCP server exposes has no way
  to create one. Implemented instead as a `Tick`-driven `RenderOpacity` lerp on the widget (0.75s),
  then `RemoveFromParent`. If a native widget-animation capability ever becomes available in the
  toolset, this would be the cleaner way to redo it, but the current approach is functionally
  equivalent.
- Both `WBP_TitleScreen` and `BP_KidRed` compile clean, saved, `is_dirty: false`. A PIE screenshot
  confirmed the title screen renders correctly (title, three buttons, About popup correctly hidden).

**Not yet verified — needs a manual pass**: actually clicking the three buttons in a live PIE
session. The MCP toolset has no UI-input-injection capability and no way to invoke an arbitrary
UFUNCTION on a live running instance, so Continue/New Game's fade→dismiss→input-enable path and
About's open/close were only verified via static wiring inspection (`get_node_infos` on every
touched node), not an actual click-through. Do a manual check in-editor before considering this
fully done.

**Two bugs found by the user in manual testing, both fixed (same-day follow-up):**
- **Screen didn't fill the viewport**: not an anchoring bug (anchors were already correct
  `(0,0)`→`(1,1)`) — the `BackgroundPanel` Border's brush had `drawAs = RoundedBox` with
  `outlineSettings.roundingType = HalfHeightRadius`, which ignores explicit corner-radius values and
  always rounds to half the container height, turning the full-screen rect into a pill shape with
  the level visible at the corners. Fixed by switching `roundingType` to `FixedRadius`.
- **Mouse cursor wasn't free to click buttons**: `BP_KidRed.EventBeginPlay` added the title screen
  to the viewport but never touched input mode/cursor visibility. Added
  `SetShowMouseCursor(true)` + `SetInputModeUIOnly` right after `AddToViewport`; symmetrically added
  `SetShowMouseCursor(false)` + `SetInputMode_GameOnly` to `EnableGameplayInput` (mirrors the
  existing pattern already used in `EventConversationClosed`).
- Still unverified by an actual click (same toolset limitation as above) — the mouse-mode fix was
  confirmed via node-wiring inspection only.

## Bug fix — Pink's dialogue backdrop (DONE, this session)

`/Game/Blueprints/UI/W_Dialogue`'s `Update text` event: the gray backdrop (`DialogueBoxSize`
SizeBox) now sizes via `Max(newSize, currentOverride)` instead of overwriting directly, so it only
grows across a conversation, never shrinks on a shorter line (resets naturally per-conversation
since a fresh `W_Dialogue` instance is created each time one opens). Also fixed a padding mismatch
(literal `24.0` → `32.0`) that was letting text overflow past the backdrop's bottom edge. Compiled,
saved, `is_dirty: false`. **Not verified in PIE** — the MCP toolset has no way to inject player
input/trigger the dialogue interaction on a live instance; worth a manual sanity check in-editor.

## Feature — Route-based random wild pokemon spawning (DONE, this session)

The old pre-placed, fixed Charmander in `L_Playground` is gone. In its place:

- `TV_RouteBounds_Route1` (a `TriggerVolume`, `L_Playground` actor `TriggerVolume_1`) marks the
  route's bounds — a 2000×2000×300 box centered on `(-90, -2819, 100)`, matching the old roam
  radius (1000 units) the removed Charmander used.
- `/Game/Pokemon/BP_RouteSpawner` (new Blueprint actor), one instance placed in the level at the
  route's center, `RouteVolume` wired to `TV_RouteBounds_Route1`. Polls (via Tick, not overlap
  events — deliberately, since scripted/teleport movement doesn't reliably fire
  `OnComponentBeginOverlap` per the tooling notes below) whether the player is inside the volume,
  accumulates distance moved, and rolls a spawn chance once per ~100-unit "step."
- Tunable instance properties on `BP_RouteSpawner`: `StepDistance = 100`, `SpawnChancePerStep =
  0.15`, `MaxConcurrentWild = 3`, `MinDistanceFromPlayer = 150`, `MinDistanceBetweenWild = 150`.
  These are reasonable defaults, not derived from any existing project convention — there's no
  tile-size constant anywhere in the codebase to anchor them to.
- Spawns are plain `BP_PokemonMaster` instances, randomly Charmander/Bulbasaur/Squirtle (equal
  odds, from `DT_PokemonSpecies`), initialized via the same `SetSpeciesRow` + `InitializeFromSpecies`
  path the old pre-placed instance used. Roam AI, chase, and the `EncounterTrigger` battle-start all
  work automatically via `BP_PokemonMaster`'s class defaults — no new AI logic was needed.
- Verified in PIE: route bounds compute correctly from the trigger volume, spawns occur with correct
  species/sprite/location, the 3-concurrent cap holds under continued movement, and each spawn gets
  its own live `Pokemon_Controller` AI instance.

### ⚠️ Known gap this introduces — wild pokemon never despawn from the spawner's tracking

`BP_RouteSpawner.WildPokemonList` only ever grows — nothing currently removes an entry when a wild
pokemon is caught, battled, or otherwise leaves play, because `BP_PokemonMaster.EndEncounter`
doesn't destroy/capture wild pokemon yet (there's no capture system at all currently). Net effect:
`MaxConcurrentWild` is really "max ever spawned this session," not "max alive right now" — the route
will stop producing new spawns once 3 have ever appeared, even if the player battles/moves away from
all of them. **This needs to be fixed as part of whatever builds the capture/battle-outcome system**
— when that's built, make sure it also removes the corresponding entry from `WildPokemonList` (and
probably destroys the actor) on capture, faint, or flee. Until then, the route will feel like it
"runs dry" after 3 spawns in a single play session.

---

## Task 2 (overworld party UI) — DONE, verified this session

`WBP_PartyDisplay` is complete: all 6 slots built and wired to `BP_PokemonGameInstance.Party`
(reads `Party[i].pokemon.sprite` via `Break S_PartyMember` → `Break S_Pokemon`), collapsed when a
slot has no party member. A slot-6 copy-paste bug (compared/read index 0 instead of 5) was found
and fixed. The widget is now actually added to the viewport — `BP_KidRed.EventBeginPlay` calls
`CreateWidget`/`AddToViewport` for it (previously it had zero referencers anywhere and was never
on screen, despite existing as an asset). Compiled, saved, `is_dirty: false` on both
`WBP_PartyDisplay` and `BP_KidRed`. PIE ran clean with an empty default party.

**Not yet done**: visual confirmation in PIE with an actually-populated 6th party slot (only tested
empty-party rendering).

---

## What's already done (previous session, verified working in PIE)

Quick recap so you understand the current state — full detail lives in the conversation history,
not repeated here:

1. **Party pokemon now appear in battle.** When KidRed walks into a wild pokemon (with a non-empty
   party), his first party member spawns as a real `BP_PokemonMaster` actor, slides into KidRed's
   original spot while KidRed slides to a new spot further down-and-left (2.5x the distance to the
   left vs down — tunable via `EncPartySpotOffsetDelta` on the wild pokemon), and is oriented to
   match the enemy pokemon's facing. If the party is empty, the encounter does not start at all.
2. **Bugfixes along the way**: a recursive-spawn infinite-loop guard (the spawned party pokemon no
   longer re-triggers its own encounter), a level-authoring bug where the wild Charmander's
   encounter-trigger sphere had a stale per-instance radius override (32) smaller than its own
   blocking collision capsule (73) — making it physically un-reachable — reset to the class default
   (130).
3. **A small starter town** was built from the Advanced Village Pack (`/Game/AdvancedVillagePack/`)
   in `L_Playground`, centered at world (2000, -1500): 4 houses around a well, street lamps, trees,
   a small fenced yard, scatter props.
4. **Lighting overhaul**: the Village Pack's window/lamp lights were blown out (a `LightsIntensity`
   blueprint variable was left at `8` instead of the intended ~0–1 range, producing 80,000-unit
   lights) — fixed to `0.4` on `BP_House_Var01`/`BP_House_Var02`/`BP_Street_Lamp` class defaults.
   Added a `PostProcessVolume` (unbound) with tuned bloom/exposure, raised `SkyLight` intensity
   `0 → 1.2` for ambient fill, raised the directional light `0.5 → 10` lux (it was always too dim,
   just masked by the blown-out local lights before).

---

## ✅ Prerequisite gap RESOLVED: Party has a combined S_PartyMember struct now

The HP/stats storage gap described in earlier versions of this doc is fixed. Verified fresh this
session across the whole chain:

- `S_PartyMember` = `S_Pokemon` (`pokemon`) + `S_PokemonStats` (`stats`), and
  `BP_PokemonGameInstance.Party` is `array of S_PartyMember`.
- `AC_Dialogue_Pink.Dialogue`: all three starter picks call `MakePokemonFromSpecies`, feed both its
  `S_Pokemon` and `S_PokemonStats` outputs into a `Make S_PartyMember`, and pass that into
  `AddToParty` — stats are no longer discarded.
- `BPI_PartyHolder.AddToParty`'s `Pokemon1` param is typed `S_PartyMember`.
- `BP_PokemonMaster.InitializeFromPokemonData`'s param is `S_PartyMember`; its live exec path does
  `Break S_PartyMember` → sets both `PokemonData` and `PokemonStats` on the actor. Both are
  populated for player party members now (traced non-zero base stats from `DT_PokemonSpecies`
  through to the actor's `PokemonStats`, statically — not yet confirmed at PIE runtime).
  - **Known clutter, left alone per "don't fix incidentally"**: this function also still contains an
    older, disconnected `SetPokemonData`/`BreakSPokemon` node pair not wired into the live exec
    chain. Dead code, not live behavior — added to the out-of-scope list below.
- `WBP_PartyDisplay` (this session's work) correctly reads the new struct.
- `WBP_PokemonBattleHUD.InitializeFromPokemon` was **not** migrated to take one `S_PartyMember` —
  it still takes separate `S_Pokemon` + `S_PokemonStats` params. Its only caller
  (`WBP_BattleMenu_Recovered.RefreshHUDs`) feeds it correctly via
  `BP_PokemonMaster.GetPokemonData`/`GetPokemonStats`, so this is a signature inconsistency, not a
  broken data path — worth normalizing while in that widget for Task 1, but not blocking.

**Net effect on Task 1 below**: the data-availability blocker is gone. Player `PokemonStats` (real
HP/MaxHP) is available by the time a battle starts. What's left for Task 1 is primarily UI work —
building/wiring the actual HP bar visuals — not struct plumbing.

---

## Task 1 — Battle HUD (pokemon name / level / HP bar / HP text)

Reference image provided by the user: a 2v2 battle HUD (Pokemon mainline game). For our 1v1 battles,
the same idea applies to one pokemon per side.

**Layout** (both boxes on the *left* side of the screen, not the mainline-game left/right split):
- **Player's pokemon HUD**: bottom-left corner, immediately to the left of the existing
  Fight/Bag/Pokemon/Run box (`WBP_BattleMenu_Recovered`'s button box).
- **Enemy's pokemon HUD**: top-left corner. Same component set, mirrored layout.

**Per-box components:**
1. Pokemon name
2. Level, formatted `Lv.[level]`
3. HP bar
4. HP text, formatted `[currentHP] / [maxHP]` — **player box only**. Per the reference image and
   mainline-game convention, the **enemy box does not show numeric HP**, just the bar (you're not
   supposed to know the enemy's exact HP). Confirmed with the user.
5. **HP bar is color-coded**: green above ~50%, yellow ~20–50%, red below ~20% (confirmed with the
   user; exact thresholds are a judgment call, match the reference image if unsure).

**Data sourcing:**
- Enemy pokemon = the wild `BP_PokemonMaster` instance itself (`self` in `BeginEncounter`'s terms).
  `WBP_BattleMenu_Recovered.OwningPokemon` is already set to this actor in `OnTransitionInComplete`
  — read `OwningPokemon.PokemonData` / `OwningPokemon.PokemonStats` directly.
- Player's pokemon = the spawned party-display actor, referenced by
  `OwningPokemon.EncPlayerPokemonRef` (another `BP_PokemonMaster` instance). Both `PokemonData` and
  `PokemonStats` on this actor are now correctly populated by `InitializeFromPokemonData` (see the
  resolved prerequisite-gap section above) — the HP-storage blocker is gone.

**Already exists, needs verification/completion**: `/Game/Pokemon/WBP_PokemonBattleHUD` and its
`InitializeFromPokemon(S_Pokemon, S_PokemonStats)` function already exist, and
`WBP_BattleMenu_Recovered.RefreshHUDs` already calls it via `BP_PokemonMaster.GetPokemonData`/
`GetPokemonStats`. **Don't assume this means Task 1 is done or correct** — re-verify fresh
(per CLAUDE.md rule 2) what's actually built: check whether both player and enemy HUD instances are
placed with the right layout/corners, whether name/level/HP-bar/HP-text are all wired, whether the
enemy box correctly omits HP text, and whether the color-coding logic exists. Also consider
normalizing `InitializeFromPokemon`'s two-param signature to take one `S_PartyMember` while you're in
there, for consistency with the rest of the codebase (not required, but flagged as clutter above).

`S_PokemonStats` fields (exact names, note the spaces): `HP`, `Max HP`, `Attack`, `Max Attack`,
`Defence`, `Max Defence`, `Sp Attack`, `Max Sp Attack`, `Sp Defence`, `Max Sp Defence`, `Speed`,
`Max Speed`.
`S_Pokemon` fields: `index Number`, `name`, `species`, `type 1`, `type 2`, `description`, `weight`,
`height`, `sprite` (PaperFlipbook), `move Slot 1`–`move Slot 4`, `current Level`, `max Level`,
`evolution Level`.

---

## Tooling notes learned last session (Unreal MCP / Blueprint DSL editing)

In addition to the gotchas already known from before (interface/function-library creation quirks,
`find_node_types` stale-index issue for session-local classes — see below), these came up:

- **DSL pure-node duplication bug**: `write_graph_dsl` can silently synthesize a *second*, separately
  wired copy of a pure (no-exec) node's dependency chain if that pure node's input is a variable bound
  inside a multi-exec continuation (e.g. a `Cast`'s `(:then ...)` block) and consumed again deeper in
  nested `if`s. Symptom: two reads of "the same" value disagree at runtime (e.g. a diagnostic print
  showed the correct value while a downstream `if` branched as if the value were empty). Workaround:
  compute any derived **scalar** (e.g. `Array|Length` → `> 0` bool) immediately next to where the
  source is bound, and reference *only* that pre-computed scalar later — don't re-derive from the raw
  array/struct value across nested scopes. If in doubt, verify with `read_graph_dsl` after writing —
  don't just trust that what you wrote is what got wired.
- **Specialized "Spawn Actor `<ClassName>`" node type IDs** (e.g. `Game|SpawnActorBPBattleProxy`, as
  shown by `read_graph_dsl`) are read-only artifacts of nodes originally created via the Editor UI —
  they are **not creatable** via `write_graph_dsl`/`create_node`. For any *new* spawn call, use the
  generic `Game|SpawnActorfromClass` with keyword args (`:Class`, `:SpawnTransform`,
  `:CollisionHandlingOverride`, optionally `:Owner`).
- Editing a function graph **while PIE is running** with live instances of that Blueprint can surface
  a benign `Accessed None ... pending kill` warning from hot-reload reinstancing — the edit still
  applies; verify with `compile_blueprint` + `read_graph_dsl` rather than treating it as a hard fail.
- `remove_function_graph` (and similarly destructive structural edits) gets blocked by the harness's
  auto-mode safety classifier — you'll need to ask the user for explicit permission.
- Deleting and recreating a function graph *while PIE is running* does not free the original name
  immediately (it gets auto-suffixed `_0`, since live instances still reference the old compiled
  function) — **stop PIE first** if you need the exact original name back.
- **Blueprint CDO (class default) changes do not retroactively apply to already-placed level actors.**
  Each placed instance snapshots the value at placement/construction-script time. To pick up a
  corrected default: remove + re-place the actor, or batch-patch existing instances directly via
  `editor_toolset.toolsets.programmatic.ProgrammaticToolset.execute_tool_script` (a small sandboxed
  Python script that can loop over many components/instances in one round trip — much cheaper than
  one tool call per property).
- **Lighting/post-process tuning is easy to overcorrect blind.** Verify visually after *every* change
  via `EditorToolset.EditorAppToolset.CaptureViewport` rather than stacking several exposure-related
  tweaks at once and hoping. The captured PNG comes back as a giant base64 blob that exceeds the
  tool-result size limit and gets auto-saved to a `tool-results/*.txt` file; extract the `"data":"..."`
  field with a small Node script into a real `.png` in the scratchpad, then `Read` that file directly
  (the `Read` tool renders images).
- **`ActorTools.set_actor_transform` (a plain teleport) does not reliably trigger
  `OnComponentBeginOverlap`**, even when the new position genuinely overlaps a trigger volume — only
  real swept movement (actual gameplay input, or an AI `MoveTo`) reliably fires it. Don't use scripted
  teleports to test overlap-triggered gameplay logic; drive it with real input/AI movement instead, or
  test the downstream logic some other way.

### Older gotchas (still valid, from the session before that)

- New Blueprint Interfaces created via the generic `BlueprintTools.create` tool come out broken (not
  recognized as a true interface). Fix: `AssetTools.duplicate` an existing working Blueprint
  Interface, then strip its functions and add the new one. Same for `BlueprintFunctionLibrary` — the
  generic create tool refuses that parent class; duplicate an existing one instead.
- `find_node_types` has a stale index for classes/functions created earlier in the *same* session —
  it won't find them even after a full editor restart. If you hit "node does not exist" on something
  you just created, that's likely why; try the DSL tools (`read_graph_dsl`/`write_graph_dsl`) instead,
  which don't have this problem, or query `find_node_types` from *within* the target class's own graph
  (self-context search still works) to confirm exact casing.
- The Blueprint editor's Details/Class-Defaults panel can silently show static CDO defaults instead of
  live instance values during PIE, even with "SIMULATING" showing. Don't trust it for live state —
  read via `ObjectTools.get_properties` on the actual live actor path instead.

---

## Explicitly out of scope — pre-existing issues, do not "fix" incidentally

- `Pokemon_Controller.HasLineOfSight` names a blackboard key that doesn't exist (no-op `SetValueAsBool` calls). AI works because the tree gates on `EnemyActor`.
- `BB_Pokemon.WithinRadius` orphaned; `PatrolLocation` never written though a `MoveTo` targets it.
- `BP_BattleProxy.GetPlayerSpotTransform` / `GetCreatureSpotTransform` are uncalled.
- `BP_PokemonMaster.SpeciesRow` defaults to `None` — unset instances hit the not-found branch.
- `BP_PokemonMaster.InitializeFromPokemonData` contains an older `SetPokemonData`/`BreakSPokemon`
  node pair, disconnected from the live exec chain (dead leftover from the `S_PartyMember`
  migration).

---

## Key assets touched last session (all saved, verify `is_dirty: false` before assuming so)

- `/Game/Pokemon/BP_PokemonMaster` — `BeginEncounter`, `InitializeFromPokemonData`, `ComputeBattleSpots`,
  `TickTransitionIn`, `TickTransitionOut`, `OnTransitionOutComplete`; new variables
  `EncPlayerPokemonRef`, `EncPlayerNewSpotLoc`, `EncPartySpotOffsetDelta`
- `/Game/AdvancedVillagePack/Blueprints/BP_House_Var01`, `BP_House_Var02`, `BP_Street_Lamp` —
  `LightsIntensity` class default
- `/Game/Levels/L_Playground` — new town actors, lighting actors (`PostProcessVolume_0`, `SkyLight_0`,
  `DirectionalLight_0` intensities), wild Charmander's `EncounterTrigger` radius, pre-existing house's
  spotlight intensities
