# Phase 2a: Parallel Global Queries

## Introduction

In this phase, you will implement three classical graph algorithms (BFS, SSSP, and CC) using the Bulk Synchronous Parallel (BSP) execution model.

BSP divides the computation into rounds. In each round:
1. **Process** every vertex (in parallel when multiple threads are used).
2. **Synchronize**: wait for all threads to finish the round.
3. **Finalize** (single-threaded): combine per-thread results and set up state for the next round.

Repeat until a full round produces no updates.

You will implement a small BSP framework (a serial runner and a parallel runner built on `std::thread`) and plug in three algorithm callbacks. The framework should be designed so that each algorithm is written once and can be executed by either runner unchanged.

## Requirements

1. **Graph Loading**: Extend the CSR representation from Phase 1 to carry:
   - Edge weights
   - A reverse CSR (in-edges) alongside the forward CSR

2. **BSP Framework**: Design a small framework that separates *what an algorithm does per vertex* from *how vertices are scheduled in a round*. Your framework should include:
   - An interface (e.g., an abstract base class) that an algorithm plugs into by providing a per-vertex callback, end-of-round bookkeeping, and a way to tell the runner when to stop.
   - A serial runner.
   - A parallel runner built on `std::thread`.

   Design the interface so that the **same algorithm implementation** runs unchanged under either runner.

3. **Algorithms**: Implement the following three algorithms as callbacks to your framework. Write each algorithm **once**; the same code should run under both runners.
   - **BFS**: shortest hop-distance from a source vertex.
   - **SSSP**: shortest weighted distance from a source vertex (Bellman-Ford style relaxation).
   - **CC**: weakly connected components via label propagation. Treat edges as undirected when propagating labels (each vertex adopts the minimum label among all its neighbors, in either direction).

4. **Correctness**: For each algorithm, the serial and parallel runs must produce identical per-vertex results.

5. **Performance Comparison**: Measure and compare execution times for the serial and parallel versions, and report speedup in the development journal.

## Hints

### Weighted Edge List Format

The input graph is a tab-separated text file with three columns:

```
src	dst	weight
```

Lines starting with `#` are comments. Example:

```
# FromNodeId	ToNodeId	Weight
0	1	194
0	2	221
1	3	76
2	3	175
```

### Answer File Format

Each algorithm writes one line per vertex, sorted by vertex ID. The two columns are separated by a space.

- **BFS**: `vertex_id hop_distance`. Hop distance from the source, or `-1` if unreachable.
- **SSSP**: `vertex_id weighted_distance`. Shortest weighted distance from the source, or `-1` if unreachable.
- **CC**: `vertex_id component_id`. The component ID is the smallest vertex ID in the weakly connected component containing `vertex_id`.

### Data Structures & Interfaces

The following definitions are provided as a starting point. Feel free to adapt or redesign them to better suit your implementation.

```cpp
struct CsrGraph {
    uint32_t num_vertices;
    std::vector<uint32_t> offsets;      // out-edges, size = num_vertices + 1
    std::vector<uint32_t> edges;        // out-neighbor IDs
    std::vector<int> weights;           // edge weights (parallel to edges)
    std::vector<uint32_t> in_offsets;   // in-edges (reverse CSR)
    std::vector<uint32_t> in_edges;     // in-neighbor IDs
    std::vector<int> in_weights;        // reverse edge weights (parallel to in_edges)
};

class BspAlgorithm {
public:
    virtual ~BspAlgorithm() = default;

    // Returns true if another round should be run.
    virtual bool HasWork() const = 0;

    // Process one vertex. tid ∈ [0, nthreads). The serial runner calls with
    // tid=0; the parallel runner calls with each thread's ID.
    virtual void Process(int tid, uint32_t v, const CsrGraph& g) = 0;

    // End of round. Merge per-thread buffers, prepare state for next round.
    virtual void PostRound() = 0;
};

void BspSerial(const CsrGraph& g, BspAlgorithm& algo);
void BspParallel(const CsrGraph& g, BspAlgorithm& algo, int nthreads);
```

### Algorithm Design

Since the same algorithm must run under both runners, design `Process(tid, v, g)` so it only **reads** neighbor state and only **writes** `v`'s own state. The parallel runner assigns each vertex to exactly one thread (static partitioning of `[0, num_vertices)`), so `v`'s state is written by one thread per round.

Every round, every vertex `v` scans its in-edges (via the reverse CSR) and takes the best improvement, if any, from any in-neighbor. The algorithm is **done** when a full round produces no updates.

```
Process(tid, v, g):
    best = current state of v
    updated = false
    for u in in_neighbors(v):
        candidate = value derived from state_prev[u]
                    //   BFS:  state_prev[u] + 1
                    //   SSSP: state_prev[u] + weight(u, v)
                    //   CC:   state_prev[u]
        if candidate improves best:
            best = candidate
            updated = true
    if updated:
        state of v = best
        tl_updated[tid] = 1
```

**Signaling convergence.** Each thread sets `tl_updated[tid] = 1` when it updates at least one vertex during the round. `PostRound()` reduces the per-thread flags into a single `any_updated` boolean; `HasWork()` returns that value, so the loop exits as soon as a round settles everything.

**Double buffering.** A vertex's value can be improved more than once (SSSP finds shorter paths, CC finds smaller labels). If thread A reads `state[u]` while thread B is simultaneously writing `state[u]`, the read is unsafe. To prevent this, keep two arrays: `state_` (live, written by `Process()`) and `state_prev_` (read by `Process()`, a snapshot of the values at the end of the previous round). `PostRound()` copies `state_` into `state_prev_` so the next round reads a stable view.

> **Note on CC**: CC computes weakly connected components, so labels should propagate across every edge regardless of direction. For CC, `Process()` must scan both in-neighbors and out-neighbors.

### Pseudocode

The following pseudocode illustrates the overall workflow. It is provided as a reference, and you may organize your code differently.

```
// 1. Load weighted graph with reverse edges
graph = LoadGraph("edge.txt")

// 2. Run BFS with both runners. The same Bfs class is used by both.
bfs_s = Bfs(graph.num_vertices, source=0, nthreads=1)
BspSerial(graph, bfs_s)

bfs_p = Bfs(graph.num_vertices, source=0, nthreads=4)
BspParallel(graph, bfs_p, nthreads=4)

// 3. Verify that serial and parallel produced identical results
assert bfs_s.distances == bfs_p.distances

// 4. Repeat for SSSP (source=0) and CC
// 5. Measure times, report speedup, write BFS.txt / SSSP.txt / CC.txt
```

## Testing & Verification

1. **Small test cases**: Build small graphs (5–10 vertices) where you can manually compute expected BFS distances, SSSP shortest paths, and CC labels. Include:
   - Disconnected components (for CC)
   - Cycles (to test convergence)
   - Varying edge weights (to distinguish SSSP from BFS)

2. **Serial-parallel consistency**: Verify that serial and parallel produce identical per-vertex results for all three algorithms.

3. **Provided dataset**: Download `soc-LiveJournal1-weighted.txt` from the [shared folder](https://drive.google.com/drive/folders/1MR0FuR0UxRCIFYBpEQ9e4Bx8s4qA_U7F?usp=sharing). This is a directed social network graph:
   - 4,847,571 vertices, 68,993,773 edges
   - Tab-separated, with integer edge weights in [1, 400]
   - Use `head -n 5 <file-name>` to have a quick look at the format of the provided dataset.
   - Add it (and the answer files) to `.gitignore` so it is not committed to GitHub.

4. **Answer files**: The shared folder also contains expected answer files (`BFS.txt`, `SSSP.txt`, `CC.txt`) for the provided dataset. Compare your output against these files to verify correctness. BFS and SSSP use source vertex 0.

## Experiments

Run the following experiments on `soc-LiveJournal1-weighted.txt` and report in your development journal. Use source vertex 0 for BFS and SSSP.

1. **Serial execution time** for BFS, SSSP, and CC.

2. **Speedup table**: For each algorithm, report:

   | Algorithm | Serial | 1 thread | 2 threads | 4 threads | 8 threads |
   |---|---|---|---|---|---|
   | BFS | | | | | |
   | SSSP | | | | | |
   | CC | | | | | |

### Discussion

Answer the following questions in your development journal:

1. Inside `Process(tid, v, g)`, which memory location does the function write to? Using the parallel runner's vertex partitioning, explain why two threads can never write to the same location in one round.

2. Compare the parallel speedup you measured for BFS, SSSP, and CC. Which algorithm scales best, and what property of the algorithm explains the difference?

3. In BFS, `dist_[v]` stores the current shortest hop distance from the source to vertex `v`. Once `dist_[v]` has been set, later rounds can never change it. How could you use this fact to skip work on already-settled vertices? Why would the same trick not work for SSSP or CC? (Implementing this optimization is not required, but strongly encouraged: you should observe a significant speedup over the baseline BFS.)