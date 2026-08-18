# Session Handoff — Party System + Run Button

Status as of 2026-08-18. Both tasks below are **complete and verified working in PIE**.
This file exists so a fresh Claude Code session (or human) has full context without
re-deriving it. Read this before starting new work in this project.

---

## Task 1 — Party system (DONE, verified)

New assets:
- **`/Game/Blueprints/Interfaces/BPI_PartyHolder`** — Blueprint Interface.
  `AddToParty(Pokemon: S_Pokemon) -> (Success: bool)`
- **`/Game/Blueprints/BP_PokemonGameInstance`** — GameInstance blueprint.
  Implements `BPI_PartyHolder`. Variables: `Party` (array of `S_Pokemon`), `bStarterChosen` (bool guard).
  `AddToParty` impl: if `Party.Length >= 6` return false, else `Add` + return true.
  **Set as the project's Game Instance Class** in Project Settings → Maps & Modes (already done).
- **`/Game/Pokemon/BPFL_Pokemon`** — Blueprint Function Library.
  `MakePokemonFromSpecies(SpeciesRow: Name, SpeciesTable: DataTable, Level: int) -> (Pokemon: S_Pokemon, Stats: S_PokemonStats, Found: bool)`.
  Shared species→struct construction logic (previously duplicated inline).

Modified:
- **`/Game/Pokemon/BP_PokemonMaster.InitializeFromSpecies`** — refactored to call
  `BPFL_Pokemon.MakePokemonFromSpecies` instead of the old inline 18-node Break/Make chain.
  Wild encounters still use `Level = 1`.
- **`/Game/Blueprints/Dialogue/AC_Dialogue_Pink.Dialogue`** — all three starter branches
  (Charmander / Squirtle / Bulbasaur, option indices 0/1/2) now, immediately before the
  existing `SetDialogeTreeIndex(1)`:
  1. `GetGameInstance` → `Cast To BP_PokemonGameInstance` (needed only to read/write the guard bool;
     the cast has a `CastFailed` fallback that still proceeds to close the conversation safely)
  2. If `not bStarterChosen`: `MakePokemonFromSpecies(<species>, DT_PokemonSpecies, Level=5)` →
     `AddToParty` via the **BPI_PartyHolder interface message call** (decoupled — not a direct
     function call on the concrete class) → `SetStarterChosen(true)`
  3. Falls through to the existing `SetDialogeTreeIndex(1)` → `CloseConversation`, unconditionally,
     via a 3-way exec fan-in (success path / already-chosen skip / cast-failed).
  - Option index 3 ("tell me more") is untouched — no party mutation on that path, by design
    (rewinding dialogue must never add a Pokémon).
  - Re-entrancy: `AC_Dialogue_Base` replays the whole `Dialogue` graph top-to-bottom on every
    choice. The `bStarterChosen` guard prevents a duplicate add if a conversation is abandoned
    mid-tree and reopened while `DialogueTreeIndex` is still 0.

**Verified**: live PIE test with a temporary `Print String` diagnostic (since removed) confirmed
`Party.Length == 1` after picking Charmander. Do not trust the Blueprint editor's Details/Class-Defaults
panel for live values during PIE — see gotcha below.

---

## Task 2 — Run button (DONE, verified)

Modified, all in `/Game/Pokemon/BP_PokemonMaster` unless noted:
- **`OnTransitionInComplete`** — now sets `OwningPokemon` on the created battle widget
  (`WBP_BattleMenu_Recovered`) before `AddToViewport`. Previously declared but never set.
- **`/Game/Pokemon/WBP_BattleMenu_Recovered` EventGraph** — `Btn_Run.OnClicked` →
  `OwningPokemon.EndEncounter()`.
- **`EndEncounter`** — `RemoveFromParent` + clear `EncBattleWidgetRef`; `SetViewTargetWithBlend`
  back to the player over `EncTransitionDuration`; `EncTransitionAlpha = 0`;
  `EncTransitioningOut = true`. Does **not** restore input (that happens at the very end of the
  out-transition, so the player can't walk/fight the camera blend mid-transition).
- **`EventGraph.EventTick`** — added a second, independent branch alongside the existing
  `EncTransitioningIn` one: `if EncTransitioningOut → TickTransitionOut(DeltaSeconds)`.
- **`TickTransitionOut`** — mirrors `TickTransitionIn`'s smoothstep ease curve
  (`alpha² × (3 − 2×alpha)`) with lerp endpoints swapped (player: `EncPlayerSpotLoc` →
  `EncPlayerStartLoc`; creature: `EncCreatureSpotLoc` → `EncPokemonStartLoc`), keeping the same
  "X/Y from target, Z from this transition's start" trick so actors slide along the ground.
  On `alpha >= 1` calls `OnTransitionOutComplete`.
- **`OnTransitionOutComplete`** — order matters, implemented exactly as specced:
  1. `EncTransitioningOut = false`
  2. `SetActorRotation(player, EncPlayerStartRot)`
  3. Re-add `IMC_TopDown` mapping context to the player
  4. `SetShowMouseCursor(false)` → `SetInputMode_GameOnly`
  5. `EncInBattle = false`
  6. `DestroyActor(EncBattleProxyRef)` — only now, after the camera blend has had time to finish, so there's no camera pop
  7. `DestroyActor(self)` — despawns the encountered Pokémon

---

## Important tooling gotcha (read before doing more Blueprint scripting)

The Unreal MCP toolset's node-search/discovery function (`find_node_types`, and by extension the
live Blueprint editor's own right-click "add node" search) has a **stale index for classes/functions
created within the current session** — it does not find them, even after a full editor restart.
This affects Blueprint Function Library statics, Interface message calls, and Casts to any
blueprint class created this session.

**Workaround that reliably works**: call `create_node` directly with the exact type ID string,
even though `find_node_types` won't surface it first. The pattern is:
- Cross-class function call: `Class|<StrippedClassName>|<FunctionName>` (e.g. `Class|BPPokemonGameInstance|AddtoParty`)
- Interface message call: `Class|<StrippedInterfaceName>|<FunctionName>(Message)` (e.g. `Class|BPIPartyHolder|AddtoParty(Message)`)
- Cast: `Utilities|Casting|CastTo<ClassNameWithUnderscore>` (e.g. `Utilities|Casting|CastToBP_PokemonGameInstance`)
- To learn the exact function-name casing, first query `find_node_types` from *within* the
  target class's own graph (self-context search still works) — the correct casing is guaranteed to be shown.
- New Blueprint Interfaces created via the generic `BlueprintTools.create` tool come out broken
  (not recognized as a true interface, won't show in "Implement Interface" pickers). The fix used
  here: `AssetTools.duplicate` an existing working Blueprint Interface, then strip its functions
  and add the new one. Same applies to `BlueprintFunctionLibrary` — the generic create tool
  explicitly refuses that parent class ("Cannot create a blueprint based on the class
  'BlueprintFunctionLibrary'"); duplicate an existing one instead (e.g. an engine/plugin sample FL)
  and clear + repopulate its functions.
- When debugging via the Blueprint editor's Details/Class-Defaults panel during PIE: it can silently
  show static CDO defaults instead of the live instance's values, even with the "SIMULATING"
  watermark showing. Don't trust it for live state — add a temporary `Print String` reading the
  actual value and check the Output Log instead (`EditorToolset.LogsToolset.GetLogEntries`), then
  remove the diagnostic afterward.

---

## Explicitly out of scope — pre-existing issues, do not "fix" incidentally

- `Pokemon_Controller.HasLineOfSight` names a blackboard key that doesn't exist (no-op `SetValueAsBool` calls). AI works because the tree gates on `EnemyActor`.
- `BB_Pokemon.WithinRadius` orphaned; `PatrolLocation` never written though a `MoveTo` targets it.
- `BP_BattleProxy.GetPlayerSpotTransform` / `GetCreatureSpotTransform` are uncalled.
- `BP_PokemonMaster.SpeciesRow` defaults to `None` — unset instances hit the not-found branch.

---

## Verification checklist (all passed)

- [x] Every touched blueprint compiles clean (no new Compiler Results entries)
- [x] `is_dirty == false` on all touched assets
- [x] `InitializeFromSpecies` still produces an identical `S_Pokemon` for a wild creature after refactor
- [x] Picking each of the three starters adds exactly one entry to `Party`
- [x] "Tell me more" then re-choosing does not double-add (guard bool)
- [x] Run returns control, restores the player to their pre-encounter spot, and removes the creature
- [x] No camera pop when the proxy is destroyed (proxy destroyed only after the blend has time to finish)

---

## Touched assets (all saved, `is_dirty: false`)

- `/Game/Blueprints/Interfaces/BPI_PartyHolder`
- `/Game/Blueprints/BP_PokemonGameInstance`
- `/Game/Pokemon/BPFL_Pokemon`
- `/Game/Pokemon/BP_PokemonMaster`
- `/Game/Blueprints/Dialogue/AC_Dialogue_Pink`
- `/Game/Pokemon/WBP_BattleMenu_Recovered`
