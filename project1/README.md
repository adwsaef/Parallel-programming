# Boolean Circuit Solver

A circuit is represented as a tree.
- **Leaf nodes** must implement a `getValue` function, which may take
some time to compute and returns either a boolean or an error.
- **Non-leaf nodes** can have different types:
- `not`
- `and`
- `or`
- `if` (takes 3 arguments `a, b, c`: evaluates as *if a then b else c*)
- `gt(x)` (true if at least `x + 1` children are true)
- `lt(x)` (true if at most `x - 1` children are true)

The implemented class **ParallelCircuitSolver** provides two functions:
- `solve(circuit)` -- adds the given circuit to the internal set of
circuits being processed, unless calculations are stopped. Returns a
`CircuitValue`.
- `stop()` -- stops all ongoing calculations.

`CircuitValue` exposes a `getValue` function that either returns the
final result of the circuit or throws an exception.

In this project, I did **not** use
`java.util.concurrent.CompletableFuture<T>` or similar utilities.