# Development Journal - Session 1

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 7, 2026, 4 hours

**Objective**:  
Set up Phase 2a by extending the Phase 1 CSR graph loader into a weighted CSR graph loader with reverse edges.

**Attempts made:**

I started from the same CSR graph loading idea used in Phase 1. The Phase 1 loader only needed to read src dst, but Phase 2a uses a weighted edge list with src dst weight.

The main changes were that the graph now needs to store edge weights and also store the reverse graph. The reverse graph is needed because the BSP algorithms check the in-neighbors of each vertex during each round.

I kept the same loader style from Phase 1 and the earlier lab:

  - Read each line with getline.
  - Skip blank lines and comments.
  - Use istringstream to read the columns.
  - Store the edges first.
  - Count offsets.
  - Prefix sum the offsets.
  - Copy the offsets into current arrays and fill the final edge arrays.

I also had to think about the graph as two CSR graphs at the same time. The forward graph stores outgoing edges, and the reverse graph stores incoming edges.

**Solutions/Workarounds**:

I made a new CsrGraph structure with forward and reverse CSR arrays.

The forward CSR uses:

  - offsets
  - edges
  - weights

The reverse CSR uses:

  - in_offsets
  - in_edges
  - in_weights

I used one input edge list first, then built both the forward graph and reverse graph from the same data. This kept the code close to the Phase 1 methodology instead of changing the loader into a completely different approach.

For each edge src -> dst with a weight, I stored it in the forward CSR as an outgoing edge from src. I also stored the same edge in the reverse CSR as an incoming edge to dst.

**Experiments/Analysis**:

For this session I focused on just getting the weighted graph loader setup for the full Phase 2a algorithms.

The main thing was making sure every original edge src -> dst was stored in the forward CSR, and also stored as dst <- src in the reverse CSR.
Which is important because BFS and SSSP process each vertex by checking incoming edges. If the reverse CSR is wrong, then the algorithms would not see the correct in-neighbors during each BSP round.

CC also needs both incoming and outgoing edges because connected components are weakly connected. That means edge direction should not matter when labels are propagated.

The actual algorithm testing was saved for the next session after the BSP framework and algorithm classes were added.

**Learning outcome**:

I learned that reverse CSR is important because it lets each vertex update only its own value during a BSP round while reading values from its neighbors.

I also learned that building the forward and reverse graph at load time makes the later BSP algorithms simpler, because BFS and SSSP can directly scan incoming edges instead of searching through the whole graph.