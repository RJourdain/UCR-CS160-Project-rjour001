# Development Journal - Session 1

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 15, 2026, 4 hours

**Objective**:  
Start Phase 2c by building off my Phase 2b push code and changing the parallel update method from locks to lock-free atomic updates.

**Attempts made:**

I started by looking back at my Phase 2b code because Phase 2c is still based on push-style graph algorithms. The graph loader did not need to change much because the dataset is still the same weighted edge file with src dst weight.

The main thing that changed was the synchronization method. In Phase 2b, the parallel push version used locks because multiple vertices could try to update the same destination vertex at the same time. In Phase 2c I had to move away from locks and use atomic updates instead. The graph structure and general push idea stayed the same but the update logic became the important part.

I also worked on the Week 5 lab separately. The lab version is smaller and only focuses on the lock-free dense push BFS result.

**Solutions/Workarounds**:

I kept the same CsrGraph structure from the previous phases. The graph still stores:

  - offsets
  - edges
  - weights
  - in_offsets
  - in_edges
  - in_weights

For Phase 2c, I set up lock-free versions of the push algorithms using atomic compare-and-swap style updates.

The main idea was still the same as Phase 2b:

  - BFS pushes hop distance plus one.
  - SSSP pushes weighted distance plus the edge weight.
  - CC pushes smaller component labels.

The difference is that instead of locking before updating a destination vertex, the code attempts an atomic update. This avoids the explicit mutex lock around every push.

I also set up two worklist styles:

  - Dense atomic push
  - Sparse atomic push

The dense version uses a bitmap-style frontier. The sparse version stores only the active vertices in a list. I expected the sparse version to be faster when the frontier is much smaller than the whole graph.

**Experiments/Analysis**:

For this session, I focused on getting the Phase 2c structure in place and making sure it still followed the same style as Phase 2b.

The part that took the most thought was the update step. In Phase 2b, the lock made it easy to protect the compare and update. In Phase 2c, the atomic update has to keep trying only when the candidate value is actually better than the old value.

I also had to keep the dense and sparse versions organized so they were still comparing the same algorithm result, not two completely different implementations.

The full LiveJournal timing and correctness tests were saved for the next session after the full Phase 2c code and Week 5 lab code were ready.

**Learning outcome**:

I learned that Phase 2c is mostly about synchronization. The biggest thing I learned was that removing locks does not mean the problem disappears. Multiple threads can still try to update the same destination vertex. The difference is that atomics handle that update without using a normal mutex lock.

I also learned why sparse worklists can help. If only a smaller set of vertices are active, then it makes sense to process that active list instead of scanning the whole graph every round.


