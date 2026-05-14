# Development Journal - Session 1

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 11, 2026, 5 hours

**Objective**:  
Start Phase 2b by building off my Phase 2a code and changing the algorithms from pull style to push style.

**Attempts made:**

I started by looking back at my Phase 2a code because the graph loader did not really need to change. Phase 2b still uses the weighted graph file with src dst weight, so I kept the same CSR graph structure and loader.

The part that changed was the direction of the algorithm. In Phase 2a, each vertex looked at its in-neighbors and updated itself. That was easier to reason about because each vertex only wrote to its own spot in the distance or label array.

For Phase 2b, I had to switch to push style. That means a vertex with a current value pushes an update out to its neighbors. This took more thought because now two different vertices can try to update the same neighbor in the same round.

I first worked on keeping the structure close to Phase 2a so I was not rewriting everything from scratch. I wanted the Phase 2b code to feel like the next step from my Phase 2a code.

**Solutions/Workarounds**:

I kept the CsrGraph structure from Phase 2a because the same graph data is still useful. The forward CSR is especially important now because push algorithms use outgoing edges.

The forward CSR still uses:

  - offsets
  - edges
  - weights

I also kept the reverse CSR in the graph structure even though Phase 2b mainly uses outgoing edges. Keeping it made the graph loader consistent with Phase 2a and avoided changing the structure too much.

The main change was in the algorithm logic. Instead of treating v as the vertex being updated from its in-neighbors, I treated v as the source vertex that pushes to its out-neighbors.

I set up two push styles:

  - Naive push
  - Dense worklist push

The naive push version is easier to understand because it checks all vertices each round. The dense worklist version should be more efficient because it only checks vertices that are active.

The part I had to be careful with was the parallel push version. Since two vertices can push to the same neighbor, the code needs protection around updates. I used a fixed set of locks instead of making one lock for every single vertex. Each destination vertex maps to one of those locks.

This kept the memory use smaller and made the locking easier to manage.

**Experiments/Analysis**:

For this session, I mostly focused on getting the structure in place before doing the full timing tests.

The important thing I noticed is that pull and push have different problems.

In the pull version from Phase 2a, each vertex writes only to itself. That makes the parallel version cleaner because the thread that owns the vertex is the only one writing there.

In the push version for Phase 2b, a vertex can try to update one of its neighbors, but another vertex might also be trying to update that same neighbor. That is why the parallel push version needs locks.

I also started to see why the dense worklist version should help. In naive push, the program may keep checking vertices that are not useful in that round. With a worklist, the program can focus on the active vertices that actually have updates to push.

I saved the full correctness and timing tests for the next session after finishing BFS, SSSP, and CC.

**Learning outcome**:

I learned that Phase 2b is not just a small change from Phase 2a. The graph structure is mostly the same, but changing from pull to push changes how updates happen.

The biggest thing I learned was why push based parallel code needs extra protection. In Phase 2a, each vertex wrote to itself, but in Phase 2b, multiple vertices can try to write to the same neighbor. That is what makes locks necessary.

I also learned why worklists matter. They help avoid wasting time on vertices that are not active, which should make the dense version faster than the naive version on the full graph.