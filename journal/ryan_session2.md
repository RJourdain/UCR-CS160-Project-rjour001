# Development Journal - Session 2

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 1, 2026, 3 hours

**Objective**:  
Finish the full Phase 1 query requirements by adding K-hop query callbacks, sequential execution, concurrent execution, timing, and check for correctness.

**Attempts made:**

After the CSR loader worked, I added a BFS style traversal for K-hop queries. I needed to make sure the source vertex was excluded from the result. I also needed to avoid revisiting vertices, especially in graphs with cycles.

The two required query types were:

  - Count reachable nodes within K hops.
  - Find the maximum node ID reachable within K hops.

Main traversal:

```cpp
visited[src] = 1;
q.push({src, 0});
```

The source is marked visited at the start so it does not get counted as a reachable result.

**Solutions/Workarounds**:

I added a callback using `std::function`:

```cpp
using QueryCallback = std::function<std::string(const CSRGraph&, int, int)>;
```

I added two callback functions:

  - CountReachable
  - MaxReachable

I added RunTasksSequential to run all queries one at a time.

I added RunTasksParallel using std::thread and an atomic task counter so multiple threads can safely take the next available query.

**Experiments/Analysis**:

I first tested the implementation using a small graph and a small query file where I could manually check the answers.

Small test output:


Graph loaded
vertices: 10
edges: 9
queries: 9
sequential time ms: 0.00625
parallel time ms: 0.48075
threads: 4
sequential expected mismatches: 0
parallel expected mismatches: 0
sequential vs parallel result differences: 0
query 0 src=0 K=1 type=1 result=2 expected=2
query 1 src=0 K=2 type=1 result=3 expected=3
query 2 src=0 K=2 type=2 result=3 expected=3
query 3 src=3 K=3 type=1 result=2 expected=2
query 4 src=3 K=3 type=2 result=4 expected=4

Both the sequential and parallel versions matched the expected answers, and there were no differences between sequential and parallel results. The parallel version was slower on the small test because the input was very small and the thread overhead was larger than the actual work.

I then tested the provided queries20.txt file on soc-Slashdot0902.txt.

queries20.txt output:

Graph loaded
vertices: 82168
edges: 948464
queries: 20
sequential time ms: 6.18325
parallel time ms: 2.23758
threads: 4
sequential expected mismatches: 0
parallel expected mismatches: 0
sequential vs parallel result differences: 0
query 0 src=15795 K=1 type=1 result=20 expected=20
query 1 src=76820 K=3 type=2 result=53604 expected=53604
query 2 src=37194 K=3 type=2 result=-1 expected=-1
query 3 src=44131 K=4 type=2 result=81637 expected=81637
query 4 src=41090 K=2 type=1 result=224 expected=224

The queries20.txt run also had zero mismatches. The parallel version was faster than the sequential version on this test.

I then tested the provided `queries10000.txt` file for performance.

queries10000.txt output:

Graph loaded
vertices: 82168
edges: 948464
queries: 10000
sequential time ms: 2056.96
parallel time ms: 572.77
threads: 4
sequential expected mismatches: 0
parallel expected mismatches: 0
sequential vs parallel result differences: 0
query 0 src=15795 K=1 type=1 result=20 expected=20
query 1 src=76820 K=3 type=2 result=53604 expected=53604
query 2 src=37194 K=3 type=2 result=-1 expected=-1
query 3 src=44131 K=4 type=2 result=81637 expected=81637
query 4 src=41090 K=2 type=1 result=224 expected=224


The queries10000.txt run also had zero mismatches. This was the best performance test because there were many query tasks to split across threads. The parallel version was much faster here because each thread could work on different queries at the same time.

**Learning outcome**:

I learned how to separate the graph traversal from the query logic by using callbacks. I also learned that parallel code is not always faster on very small tests because creating and managing threads has overhead. Parallelism is more useful when there is actually enough work to divide across threads.