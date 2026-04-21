Implement the **Serial lock-free, naive push** and **Serial lock-free, dense worklist** versions of **BFS only** from the [Phase-2b README](README.md). Both are single-threaded, push-based, and use no locks; the second one adds a dense bitmap worklist. Run them on `soc-LiveJournal1-weighted.txt` with `source = 0` and check that both produce the same hop distance to vertex 50 as your Phase-2a BFS.

Please submit your code (copy/paste both implementations) and execution result on Canvas (an image showing the following two lines for `source = 0` and `vertex = 50`):

```
BFS (naive push):     source=0 -> vertex=50, hops = <your answer>
BFS (dense worklist): source=0 -> vertex=50, hops = <your answer>
```

If vertex 50 is unreachable from source 0, print `-1` for the corresponding value.