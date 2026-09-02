# PokemonProject — Project Rules

This is the foundation document for working on this repo. It defines what's always
true: project shape, tooling constraints, and non-negotiable working rules. It is not
a task log — for "what's in progress right now," see [HANDOFF.md](HANDOFF.md).

## What this project is

An Unreal Engine 5 top-down Pokemon-style game. It was originally **Blueprint-only**;
a C++ module (`PokemonProject`, under `Source/PokemonProject/`) was scaffolded on
2026-08-28 as the intended home for gameplay logic going forward (see "C++ module"
below), but as of that date it's still an empty skeleton — **almost all existing game
logic still lives in Blueprints.** Every asset (Blueprints, Widget Blueprints, Structs,
DataTables, levels) is a binary `.uasset`/`.umap` file with no meaningful text diff.
For that Blueprint-side logic, the only way to read or edit it is through the
**Unreal MCP toolset** (`mcp__unreal-mcp__*` / `ToolsetRegistry`), which talks to a
live running Editor instance.

This shapes everything below: for Blueprint work there is no grep-the-source escape
hatch. If the MCP server is down, you cannot verify or edit Blueprint logic — you can
only read `.md` files and edit C++ under `Source/`.

## C++ module

`Source/PokemonProject/` is a normal UBT module (`PokemonProject.Build.cs`,
`PokemonProject.Target.cs` / `PokemonProjectEditor.Target.cs` at `Source/` root,
declared in `PokemonProject.uproject`'s `"Modules"` array). Engine install:
`C:\Program Files\Epic Games\UE_5.8`. It has no bundled system-wide .NET — UnrealBuildTool
needs `DOTNET_ROOT` pointed at the engine's own copy:

```
DOTNET_ROOT="/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/DotNet/10.0/win-x64" \
  "/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe" \
  PokemonProjectEditor Win64 Development \
  -project="C:\UnrealEngineProjects\PokemonProject\PokemonProject.uproject" -waitmutex
```

(There is no `GenerateProjectFiles.bat` in this engine install — use
`UnrealBuildTool.exe -projectfiles -project=... -game -engine` with the same
`DOTNET_ROOT` instead, if `.sln`/`.slnx` need regenerating after adding new files.)

- **Adding a new C++ class**: create the `.h`/`.cpp` under `Source/PokemonProject/`
  yourself (or reuse the Editor's `Tools > New C++ Class` wizard if working live in the
  Editor) — either way, run the build command above afterward to compile it. A running
  Editor instance needs to be closed and reopened (or use Live Coding) to pick up a
  newly-added class; it won't hot-load a brand-new module addition.
- `PokemonProjectEditor.Target.cs` sets `bOverrideBuildEnvironment = true` — this
  editor target intentionally does not share UnrealEditor's build environment, because
  this engine version's default warning-level settings (`UndefinedIdentifierWarningLevel`,
  `ReturnTypeWarningLevel`, etc.) didn't match on first generation. Don't remove that
  line without re-verifying the two targets' settings actually match, or project-file
  generation will throw the same "modifies build environment properties" exception again.
- `*.sln`, `*.slnx`, and `.vsconfig` at the repo root are regenerated build artifacts
  (gitignored) — don't hand-edit them, regenerate via the command above instead.
- No test coverage or CI exists for this module yet. If you add meaningful C++ gameplay
  logic, prefer writing it in a way that's actually unit-testable (pure functions over
  `AActor`/`UObject` state where reasonable) — this project's Blueprint side has no
  equivalent and has paid for it (see `HANDOFF.md`'s many "unverified by human" notes).

## Before doing any Blueprint/UMG/level work

1. **Confirm the MCP server is connected.** Call `list_toolsets` (or equivalent).
   If it errors or the tools aren't there, stop and tell the user to reconnect it —
   do not attempt file-system workarounds, and do not guess at what a graph currently
   contains.
2. **Never trust memory of a graph's state across sessions, or even across your own
   edits.** Structs, variable types, and function signatures on this project change
   under you (literally, this session — struct-typed variables get re-typed, nodes
   get orphaned). Always `read_graph_dsl` / `list_variables` / `get_node_infos` fresh
   before editing something you didn't just create yourself.
3. **Route the work to the right specialist.** This project's Unreal work splits
   cleanly into three domains, each with its own subagent under `.claude/agents/`:
   - `blueprint-logic-editor` — Blueprint graphs, functions, events, interfaces,
     variables, struct/data migrations.
   - `umg-widget-builder` — UMG Widget Blueprint trees, styling, widget properties.
   - `level-scene-editor` — level/actor placement, lighting, viewport verification.
   Use them for anything nontrivial in their domain rather than reinventing the
   approach inline — they carry the hard-won gotchas so you don't rediscover them.
4. For the specific mechanics of editing a Blueprint graph without corrupting it
   (the single easiest way to waste an entire session), see the
   `blueprint-node-surgery` skill under `.claude/skills/`. Read it before writing or
   rewriting any non-trivial graph.
5. **If a task needs a subagent or skill that doesn't exist yet, create it** rather
   than working around the gap inline. Add the new agent under `.claude/agents/` or
   skill under `.claude/skills/` following the existing ones' format, then use it.
   This keeps the specialist coverage in rule 3 complete as the project grows instead
   of letting one-off inline work silently substitute for it.
6. **Check in on background subagents that have gone quiet.** Blueprint/UMG editing
   sessions on this project routinely run long (many minutes to over an hour of real
   tool calls). If a background agent hasn't reported back within roughly 15-20
   minutes of dispatch, send it a status check-in (a plain message asking what it's
   done, what's left, and whether it's blocked) rather than silently waiting longer or
   assuming it's stalled. Also watch for the specific failure mode this project has
   hit repeatedly: an agent replies with a plausible-sounding summary ("I've
   dispatched a subagent to handle X...") without having made any real MCP tool
   calls — there is no separate subagent to delegate to, agents ARE the worker. If a
   completion notification shows a suspiciously low `tool_uses` count for the scope of
   the task, resume the agent and explicitly tell it to execute the work itself.
7. **Hard rule: only the main session may dispatch subagents. A dispatched subagent
   must never spawn a child agent of its own, for any reason.** This project has hit
   real, confirmed concurrent-edit corruption from this exact failure mode more than
   once (not just wasted tool calls — two agents independently building overlapping
   widget subtrees on the same live Blueprint at the same time, caught via
   `WidgetService.validate()` duplicate-name errors). Every subagent prompt dispatched
   on this project must include an explicit instruction that it is the worker, not a
   coordinator, and must execute all tool calls itself. If a subagent's task turns out
   to be bigger than expected, it should say so and hand control back to the main
   session — never spawn its own helper. If `ListAgents` ever shows more live agents
   than the main session directly dispatched, treat it as an active corruption risk:
   stop all of them immediately, identify which one has the fullest/most correct
   context, tell every other one to stand down and make no further edits, and have the
   surviving one re-read the live asset state fresh (don't trust either agent's memory
   of what state it's in) before any cleanup or continuation.

## Project layout conventions

- `/Game/Pokemon/` — core gameplay: `BP_PokemonMaster`, `S_Pokemon`, `S_PokemonStats`,
  `S_PartyMember`, `BPFL_Pokemon`, battle widgets, party UI.
- `/Game/Blueprints/` — game instance, dialogue system, shared interfaces
  (`BPI_PartyHolder`, `BPI_Interactable`), player controller/pawn Blueprints.
- `/Game/Blueprints/Trainers/` — player and NPC pawns (`BP_KidRed`, `NPC_Pink`, ...).
- `/Game/AdvancedVillagePack/` — third-party environment art/Blueprints. Treat as
  vendored; prefer overriding class defaults or placed-instance properties over
  restructuring these Blueprints.
- `/Game/Levels/L_Playground` — the one active level.
- Struct field display names contain spaces (e.g. `S_Pokemon.current Level`,
  `S_PokemonStats.max HP`) even though DSL/node pin names render them without spaces
  and with a hash suffix (`CurrentLevel_45_8AD93...`). Don't guess field names —
  confirm via `list_properties`/`get_node_type_pins` before relying on them.

## Non-negotiable working rules

- **Compile after every structural edit** (`compile_blueprint`), not just at the end
  of a task. Corruption compounds silently otherwise.
- **Save and verify** (`save_assets`, then confirm `is_dirty: false`) before ending
  any work on an asset — don't leave dirty assets across a session boundary.
- **A clean `compile_blueprint` does not mean the graph is correct.** This project's
  DSL tooling has silently dropped arguments, mis-wired pins, and duplicated nodes
  while still compiling clean. After any nontrivial graph write, read back the
  specific nodes you touched with `get_node_infos` and confirm the actual pin
  connections match your intent — don't trust the DSL text round-trip alone (see the
  `blueprint-node-surgery` skill for why).
- **Don't "fix" pre-existing issues incidentally.** This codebase has known dead code
  and orphaned references from past editing sessions (see `HANDOFF.md`'s "explicitly
  out of scope" section, kept current there, not duplicated here). Leave them unless
  the task is specifically about them.
- **Ask before destructive structural removals** (`remove_function_graph` and
  similar) — the harness's safety classifier will usually stop you anyway, but treat
  it as a real confirmation gate, not a formality to route around.
