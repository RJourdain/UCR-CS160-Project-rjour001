# Development Journal - Session 2

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 13, 2026, 8 hours

**Objective**:  
Finish Phase 2b by testing the push-based BFS, SSSP, and CC implementations on the full LiveJournal dataset.

**Attempts made:**

After getting the Phase 2b structure set up, I finished the push versions for BFS, SSSP, and CC.

The main thing I was trying to check was whether the push versions still gave the same answers as the baseline versions. Since push updates go out to neighbors, I wanted to make sure the faster dense worklist version was not changing the final result.

I tested three comparisons for each algorithm:

  - Serial naive vs serial dense
  - Serial naive vs naive parallel
  - Serial naive vs dense parallel

I also tested a few different K values for the dense worklist version because I wanted to see if the block size changed the runtime.

**Solutions/Workarounds**:

For BFS, I checked that both push versions still found the same hop distance from source 0.
For SSSP, I checked that the weighted distance still matched even though the dense worklist skips inactive vertices.
For CC, I checked that the component labels matched because CC can take multiple rounds for the smallest label to spread through the graph.

The dense worklist version was the main improvement. Instead of scanning every vertex every round like the naive version, it focuses on active vertices that actually have updates to push.


**Experiments/Analysis**:

I ran the program on the full soc-LiveJournal1-weighted.txt dataset using 4 threads.

The graph loaded with:

vertices: 4847571  
edges: 68993773  
threads: 4

The program printed the top in-degree vertices:

vertex 10029 in-degree 13906  
vertex 8737 in-degree 13434  
vertex 2914 in-degree 12422

**BFS results:**

BFS serial naive ms: 1230.37  
BFS serial dense ms: 261.032  
BFS naive parallel 1t ms: 4213.64  
BFS naive parallel 4t ms: 3397.8  
BFS dense parallel 1t ms: 553.102  
BFS dense parallel 4t ms: 398.635  
BFS serial naive vs serial dense differences: 0  
BFS serial naive vs naive parallel differences: 0  
BFS serial naive vs dense parallel differences: 0  
BFS dense parallel 4t K=1024 ms: 398.635  
BFS dense parallel 4t K=16384 ms: 379.695  
BFS dense parallel 4t K=262144 ms: 417.583  
BFS naive push: source=0 -> vertex=50, hops = 3  
BFS dense worklist: source=0 -> vertex=50, hops = 3

The BFS outputs matched for every version. The dense serial version was much faster than serial naive because it avoided a lot of unnecessary scanning. The dense parallel version was also better than naive parallel.

For BFS, K=16384 gave the best dense worklist time in this run.

**SSSP results:**

SSSP serial naive ms: 5615.76  
SSSP serial dense ms: 1869.74  
SSSP naive parallel 1t ms: 16572.5  
SSSP naive parallel 4t ms: 13497.1  
SSSP dense parallel 1t ms: 4380.02  
SSSP dense parallel 4t ms: 3263.6  
SSSP serial naive vs serial dense differences: 0  
SSSP serial naive vs naive parallel differences: 0  
SSSP serial naive vs dense parallel differences: 0  
SSSP dense parallel 4t K=1024 ms: 3263.6  
SSSP dense parallel 4t K=16384 ms: 2756.77  
SSSP dense parallel 4t K=262144 ms: 3368.05  
SSSP: source=0 -> vertex=50, distance = 63

The SSSP outputs also matched across all versions. The dense version helped a lot, but SSSP still took longer than BFS. This makes sense because weighted distances can keep improving over multiple rounds.

For SSSP, K=16384 gave the best dense worklist time in this run.

**CC results:**

CC serial naive ms: 2553.04  
CC serial dense ms: 1262.73  
CC naive parallel 1t ms: 9869.79  
CC naive parallel 4t ms: 7348.09  
CC dense parallel 1t ms: 3789.86  
CC dense parallel 4t ms: 2858.58  
CC serial naive vs serial dense differences: 0  
CC serial naive vs naive parallel differences: 0  
CC serial naive vs dense parallel differences: 0  
CC dense parallel 4t K=1024 ms: 2858.58  
CC dense parallel 4t K=16384 ms: 2391.29  
CC dense parallel 4t K=262144 ms: 2735.42  
CC: vertex=50, component = 0

The CC outputs matched too. The dense version was faster than naive, but CC still needed several rounds because the smallest labels have to spread through the graph.

For CC, K=16384 gave the best dense worklist time in this run.

**Timing summary:**

**BFS:**

  - Serial naive: 1230.37 ms
  - Serial dense: 261.032 ms
  - Naive parallel 1 thread: 4213.64 ms
  - Naive parallel 4 threads: 3397.8 ms
  - Dense parallel 1 thread: 553.102 ms
  - Dense parallel 4 threads: 398.635 ms

**SSSP:**

  - Serial naive: 5615.76 ms
  - Serial dense: 1869.74 ms
  - Naive parallel 1 thread: 16572.5 ms
  - Naive parallel 4 threads: 13497.1 ms
  - Dense parallel 1 thread: 4380.02 ms
  - Dense parallel 4 threads: 3263.6 ms

**CC:**

  - Serial naive: 2553.04 ms
  - Serial dense: 1262.73 ms
  - Naive parallel 1 thread: 9869.79 ms
  - Naive parallel 4 threads: 7348.09 ms
  - Dense parallel 1 thread: 3789.86 ms
  - Dense parallel 4 threads: 2858.58 ms

**Dense worklist K test:**

**BFS:**

  - K=1024: 398.635 ms
  - K=16384: 379.695 ms
  - K=262144: 417.583 ms

**SSSP:**

  - K=1024: 3263.6 ms
  - K=16384: 2756.77 ms
  - K=262144: 3368.05 ms

**CC:**

  - K=1024: 2858.58 ms
  - K=16384: 2391.29 ms
  - K=262144: 2735.42 ms

The biggest thing I noticed is that dense worklist helped more than just adding threads to the naive version. The naive parallel version was slower than expected because push updates create more overhead, especially when many vertices try to update neighbors at the same time.

The dense worklist version worked better because it reduced the amount of useless scanning. Out of the K values I tested, K=16384 was the best for BFS, SSSP, and CC on my machine.

**Learning outcome**:

I learned that push-based algorithms are useful but they are not automatically faster just because they are parallel. The naive parallel version had a lot of overhead, while the dense worklist version was more efficient because it focused on active vertices.

I also learned that tuning the worklist block size mattes. For my tests, K=16384 worked best for all three algorithms, but that could change depending on the machine.