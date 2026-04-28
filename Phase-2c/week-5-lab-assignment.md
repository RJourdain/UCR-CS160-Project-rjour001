Implement the **Lock-free dense push** version of **BFS only** from the [Phase-2c README](README.md). Start from your Phase 2b parallel dense BFS (the version that uses a striped lock pool around every write to `dist[v]`) and replace the locked critical section with an atomic compare-and-swap on `dist[v]`. The dense bitmap frontier and the BSP runner are unchanged. Run with 4 threads on `soc-LiveJournal1-weighted.txt` with `source = 0` and check that the hop distance to vertex 50 matches your Phase 2a / 2b BFS.

Please submit your code (copy/paste the implementation) and execution result on Canvas (an image showing the following line for `source = 0` and `vertex = 50`):

```
BFS (lock-free dense push): source=0 -> vertex=50, hops = <your answer>
```

If vertex 50 is unreachable from source 0, print `-1`.
