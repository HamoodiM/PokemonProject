# PokemonProject — Project Rules

This is the foundation document for working on this repo. It defines what's always
true: project shape, tooling constraints, and non-negotiable working rules. It is not
a task log — for "what's in progress right now," see [HANDOFF.md](HANDOFF.md).

## What this project is

An Unreal Engine 5 top-down Pokemon-style game. **Blueprint-only** — there is no C++
source module. Every asset (Blueprints, Widget Blueprints, Structs, DataTables,
levels) is a binary `.uasset`/`.umap` file with no meaningful text diff. The only way
to read or edit game logic is through the **Unreal MCP toolset** (`mcp__unreal-mcp__*`
/ `ToolsetRegistry`), which talks to a live running Editor instance.

This shapes everything below: there is no grep-the-source escape hatch. If the MCP
server is down, you cannot verify or edit anything — you can only read `.md` files.

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
