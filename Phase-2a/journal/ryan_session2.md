# Development Journal - Session 2

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 9, 2026, 10 hours

**Objective**:  
Finish the Phase 2a BSP framework, implement BFS, SSSP, and CC, and run the correctness and performance experiments.

**Attempts made:**

After the weighted CSR loader was set up, I added a small BSP framework. The goal was to separate what the graph algorithm does from how vertices are scheduled during each round.

I added the BspAlgorithm interface with three main functions:

  - HasWork
  - Process
  - PostRound

Then I added two runners:

  - BspSerial
  - BspParallel

The serial runner processes every vertex with one thread. The parallel runner splits the vertices into ranges and gives each range to a different std::thread.

**Solutions/Workarounds**:

I implemented BFS, SSSP, and CC as separate classes using the same BSP interface.
For BFS, each vertex checks its in-neighbors and takes the smallest hop distance plus one.
For SSSP, each vertex checks its in-neighbors and takes the smallest weighted distance using the edge weight.
For CC, each vertex checks both in-neighbors and out-neighbors because connected components are weakly connected, so edge direction should not matter.

I used double buffering for the algorithm state. Each algorithm has a current array and a previous array. The Process function reads from the previous array and writes to the current vertex's location in the current array.

This keeps the parallel version safer because two threads do not write to the same vertex location in one round.

**Experiments/Analysis**:

I first tested the implementation on a small weighted graph that I could manually check. The graph included a cycle, a disconnected component, different edge weights, and a path from source 0 to vertex 50.

**Small test output:**

Graph loaded  
vertices: 51  
edges: 11  
threads: 4  
BFS serial ms: 0.001791  
BFS parallel ms: 0.326208  
BFS serial vs parallel differences: 0  
SSSP serial ms: 0.001541  
SSSP parallel ms: 0.239584  
SSSP serial vs parallel differences: 0  
CC serial ms: 0.0015  
CC parallel ms: 0.216041  
CC serial vs parallel differences: 0  
BFS: source=0 -> vertex=50, hops = 2  
SSSP: source=0 -> vertex=50, distance = 15  
Wrote BFS.txt, SSSP.txt, and CC.txt

The serial and parallel versions matched for all three algorithms. The parallel version was slower on the small graph because the graph is tiny and the cost of creating threads is larger than the actual graph work.

The BFS result for vertex 50 was 2 hops because the graph has a path from 0 to 2 to 50.

The SSSP result was 15 because the lower weighted path is 0 to 1 to 3 to 50.

I then ran the full soc-LiveJournal1-weighted.txt dataset. The full dataset loaded with:

vertices: 4847571  
edges: 68993773

I ran the program with 1, 2, 4, and 8 threads.

**1 thread run:**

Graph loaded  
vertices: 4847571  
edges: 68993773  
threads: 1  
BFS serial ms: 423.238  
BFS parallel ms: 415.51  
BFS serial vs parallel differences: 0  
SSSP serial ms: 4762.57  
SSSP parallel ms: 5154.45  
SSSP serial vs parallel differences: 0  
CC serial ms: 1344.97  
CC parallel ms: 1326.08  
CC serial vs parallel differences: 0  
BFS: source=0 -> vertex=50, hops = 3  
SSSP: source=0 -> vertex=50, distance = 63  
Wrote BFS.txt, SSSP.txt, and CC.txt

**2 thread run:**

Graph loaded  
vertices: 4847571  
edges: 68993773  
threads: 2  
BFS serial ms: 459.44  
BFS parallel ms: 325.42  
BFS serial vs parallel differences: 0  
SSSP serial ms: 5674.15  
SSSP parallel ms: 4387.52  
SSSP serial vs parallel differences: 0  
CC serial ms: 1545.87  
CC parallel ms: 976.398  
CC serial vs parallel differences: 0  
BFS: source=0 -> vertex=50, hops = 3  
SSSP: source=0 -> vertex=50, distance = 63  
Wrote BFS.txt, SSSP.txt, and CC.txt

**4 thread run:**

Graph loaded  
vertices: 4847571  
edges: 68993773  
threads: 4  
BFS serial ms: 421.493  
BFS parallel ms: 199.181  
BFS serial vs parallel differences: 0  
SSSP serial ms: 5025.8  
SSSP parallel ms: 2708.42  
SSSP serial vs parallel differences: 0  
CC serial ms: 1283.55  
CC parallel ms: 629.69  
CC serial vs parallel differences: 0  
BFS: source=0 -> vertex=50, hops = 3  
SSSP: source=0 -> vertex=50, distance = 63  
Wrote BFS.txt, SSSP.txt, and CC.txt

**8 thread run:**

Graph loaded  
vertices: 4847571  
edges: 68993773  
threads: 8  
BFS serial ms: 439.412  
BFS parallel ms: 135.562  
BFS serial vs parallel differences: 0  
SSSP serial ms: 5064.28  
SSSP parallel ms: 1823.57  
SSSP serial vs parallel differences: 0  
CC serial ms: 1347.48  
CC parallel ms: 415.473  
CC serial vs parallel differences: 0  
BFS: source=0 -> vertex=50, hops = 3  
SSSP: source=0 -> vertex=50, distance = 63  
Wrote BFS.txt, SSSP.txt, and CC.txt

The serial and parallel difference counts were 0 for BFS, SSSP, and CC in every run. This showed that the parallel version produced the same per-vertex results as the serial version.

After generating BFS.txt, SSSP.txt, and CC.txt, I compared my output files against the expected answer files from the provided dataset folder.

The generated files matched the expected files:

BFS.txt matched the expected BFS.txt  
SSSP.txt matched the expected SSSP.txt  
CC.txt matched the expected CC.txt

This gave another correctness check in addition to the serial vs parallel difference counts being 0.

**Timing results:**

**BFS:**

  - Serial: 423.238 ms
  - 1 thread: 415.51 ms
  - 2 threads: 325.42 ms
  - 4 threads: 199.181 ms
  - 8 threads: 135.562 ms

**SSSP:**

  - Serial: 4762.57 ms
  - 1 thread: 5154.45 ms
  - 2 threads: 4387.52 ms
  - 4 threads: 2708.42 ms
  - 8 threads: 1823.57 ms

**CC:**

  - Serial: 1344.97 ms
  - 1 thread: 1326.08 ms
  - 2 threads: 976.398 ms
  - 4 threads: 629.69 ms
  - 8 threads: 415.473 ms

**Approximate speedup using the 1 thread run serial time as the baseline:**

**BFS:**

  - 1 thread: 1.02x
  - 2 threads: 1.30x
  - 4 threads: 2.12x
  - 8 threads: 3.12x

**SSSP:**

  - 1 thread: 0.92x
  - 2 threads: 1.09x
  - 4 threads: 1.76x
  - 8 threads: 2.61x

**CC:**

  - 1 thread: 1.01x
  - 2 threads: 1.38x
  - 4 threads: 2.14x
  - 8 threads: 3.24x

BFS and CC scaled better than SSSP in these tests. SSSP does more work because weighted shortest paths can keep improving over multiple rounds. BFS uses simple hop distances and CC only propagates smaller labels through the graph.

Inside Process(tid, v, g), the function writes to the current vertex's own location in the state array. Since the parallel runner divides the vertices into separate ranges, each vertex is assigned to only one thread in a round. That means two threads should not write to the same vertex location during the same round.

For BFS, one possible optimization is to skip work after a vertex's distance is already set because BFS distances do not keep improving once discovered. Also, in one BFS round, newly discovered neighbors have the same distance. This could allow the program to process only the active frontier instead of scanning every vertex every round. The same idea does not work as cleanly for SSSP because weighted distances can improve later if a shorter weighted path is found. It also does not work the same way for CC because labels can keep decreasing as smaller component IDs propagate through the graph.

**Learning outcome**:

I learned how BSP works by splitting the algorithm into rounds. Why the algorithm writes only to the current vertex's own state during process. Since the parallel runner gives each vertex to only one thread per round the two threads shouldn't write to the same vertex location at the same time.

I also learned that parallel speedup depends on both the algorithm and the amount of work. The full dataset showed real speedup with more threads but the small graph did not because the overhead was too large.