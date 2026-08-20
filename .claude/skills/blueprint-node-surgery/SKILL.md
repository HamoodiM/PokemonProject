---
name: blueprint-node-surgery
description: How to safely edit an Unreal Blueprint graph via the MCP toolset without corrupting it — when to trust write_graph_dsl and when to fall back to manual node-by-node surgery (create_node + connect_pins). Use whenever editing an existing, non-trivial Blueprint/Widget graph, especially one with more than a handful of nodes, or after changing a variable/struct/function signature that graph depends on.
---

# Blueprint node surgery

Editing a live Blueprint graph through the MCP toolset is not like editing text.
There is no diff, no undo you can casually rely on, and — critically — the DSL
round-trip tools (`read_graph_dsl` / `write_graph_dsl`) have real, reproducible
failure modes on this project that **compile clean and look correct in the text
output while being silently wrong in the actual graph**. This skill is the checklist
for not losing an hour to that.

## The two failure modes, confirmed this session

**1. Signature-change stale pins.** When you change a Blueprint variable's type, an
interface function's parameter, or a plain function's parameter (via
`remove_function_param`/`add_struct_function_param`/`add_struct_variable`/
`remove_variable`), every *existing* node that already referenced the old
signature — the function's own entry node, every call site, every interface
`Message` node — keeps a **stale pin** with the old type, sitting alongside a new,
correctly-typed, unconnected pin (often suffixed `1`, e.g. `Pokemon` +
`Pokemon1`). The stale pin still shows as "connected" to whatever it was wired to
before. Compiling fails with a data-mismatch error ("Only exactly matching
structures are considered compatible") or, worse, succeeds and quietly uses garbage.

  Fix: after any signature change, `get_node_infos` on the entry node and every call
  site. Reconnect consumers from the stale pin to the fresh one (`connect_pins`), then
  `break_pins` the stale connection. Do **not** assume `write_graph_dsl` will do this
  for you — see below.

**2. `write_graph_dsl` on a graph with pre-existing manual wiring, large tuple binds,
or big/legacy content is lossy.** Observed failures, all on a graph that compiled
clean afterward:
  - A `(bind (a b c ...) (Node ...))` argument silently dropped for one branch while
    working in a sibling branch (e.g. `Widget|SetVisibility` losing its `"Visible"`
    literal, `Array|Get` losing its index argument).
  - A manually-wired node (e.g. a `CreateEvent`/`Bind Event` pair set up via
    `set_create_event_function`, which has no DSL representation) getting its
    connection **dropped entirely** on the next `write_graph_dsl` pass over the same
    graph, because the DSL text format cannot encode "this delegate node is bound to
    function X" — round-tripping it forgets the binding.
  - Rewriting a graph that already has an old/legacy generation of nodes in it
    (leftover from earlier sessions) creates a **new, parallel generation** instead of
    replacing the old one — the old nodes go orphaned-but-still-type-broken rather
    than disappearing, and now BOTH generations fail to compile.

  This project's `BeginEncounter` graph had **five** dead generations of the same
  ~70-node subtree from past sessions before this was diagnosed — `write_graph_dsl`
  had been silently leaving corpses behind for a long time.

## The rule of thumb

- **Small, fresh, single-purpose function you're writing from scratch** (no
  pre-existing nodes, no delegate binding, ≤ ~20 nodes): `write_graph_dsl` in one
  shot is fine and fast. Verify with `read_graph_dsl` + `compile_blueprint` after.
- **Anything else** — editing an existing graph, a graph with delegate/event
  bindings, a graph you suspect has legacy cruft, or more than ~30 nodes of new
  content: do it as **manual node surgery**, one logical unit at a time:
  1. `create_node` for each new node (note the returned refPath).
  2. `get_node_infos` on the fresh nodes to get exact pin indices/names — never
     guess pin order from the type signature; input pin order is not always
     `(execute, arg1, arg2, ..., self)` — check every time (`Class|Image|SetBrush`,
     for example, is `(execute, Brush, self)`, not `(execute, self, Brush)`).
  3. `connect_pins` one wire at a time (`output_pin`/`input_pin`, both full `PinID`
     objects with `direction`/`index_id`/`node`).
  4. `set_pin_value` for literals (index constants, enum strings like `"Visible"`/
     `"Collapsed"`).
  5. `compile_blueprint` after each logical chunk (e.g. after each "slot" of a
     repeated pattern), not just once at the very end. Catching a bad wire after 10
     nodes is cheap; catching it after 100 is not.
  6. Spot-check with `get_node_infos` on a couple of the nodes you just wired,
     confirming pins show as actually `connected_pins`-populated, not just present.

## When it's already broken

If `find_nodes(title="")` on a graph returns far more nodes than `read_graph_dsl`'s
text implies exist, you have orphaned generations. To find what's actually live
(reachable from the entry point) versus dead:
1. Pull full `get_node_infos` for every node in the graph (batch it).
2. BFS from the entry node's `Exec`-typed output pins only, following
   `connected_pins`. Everything not reached is either dead-but-harmless (no
   connections at all — leave it) or dead-but-type-broken (has stale connections
   that still fail compilation — `break_pins` those specific connections; don't
   bother deleting the orphaned nodes themselves unless they're actively blocking
   compilation, since deleting shared upstream nodes risks breaking something live
   that also depends on them).
3. Fix only the live path fully (rewire with fresh nodes per the pattern above); for
   dead islands, the minimum safe fix is disconnecting their broken pins, not
   deleting the nodes.

## Delegate binding (event dispatchers) has no DSL form

To bind a Blueprint event dispatcher (`OnSomethingChanged`) to a function/custom
event, you cannot express it in `write_graph_dsl` at all. Do it as manual nodes:
1. `create_node` the `BindEventto<DispatcherName>` node and a
   `EventDispatchers|CreateEvent` node.
2. `connect_pins` the `CreateEvent` node's `OutputDelegate` into the bind node's
   `Delegate` input **first** — `list_compatible_event_functions` errors until that
   connection exists.
3. `list_compatible_event_functions` on the `CreateEvent` node to confirm your
   target function/custom event is in the list.
4. `set_create_event_function` to bind it.
5. Never re-run `write_graph_dsl` over this graph afterward without re-verifying this
   binding survived — see failure mode 2 above.
