```cpp
struct CsrGraph {
    int num_vertices;
    std::vector<int> offsets;   // size = num_vertices + 1
    std::vector<int> edges;     // out-neighbor IDs
    std::vector<int> weights;   // edge weights (parallel to edges)
};

CsrGraph LoadGraph(const char *filename);

// BFS: hop distance from source; -1 if unreachable.
std::vector<int> BFS(const CsrGraph &g, int source);

// SSSP: shortest weighted distance from source; -1 if unreachable.
std::vector<long long> SSSP(const CsrGraph &g, int source);
```

Extend the `CsrGraph` from Phase 1 to carry edge weights, then implement `BFS` and `SSSP` on the weighted graph `soc-LiveJournal1-weighted.txt` (download link and format are in the [Phase-2a README](README.md)). Use `source = 0`.

The implementation is unrestricted: you may follow the BSP double-buffering approach described in the Phase-2a README, or use any textbook approach (BFS queue, Bellman-Ford, Dijkstra, etc.).

Please submit your code (copy/paste your implementation) and execution result on canvas (an image to show the execution result, which must clearly print the following two lines for `source = 0` and `vertex = 50`):

```
BFS:  source=0 -> vertex=50, hops = <your answer>
SSSP: source=0 -> vertex=50, distance = <your answer>
```

If vertex 50 is unreachable from source 0, print `-1` for the corresponding value.
