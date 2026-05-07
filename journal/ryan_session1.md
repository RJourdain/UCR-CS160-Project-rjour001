# Development Journal - Session 1

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: April 30, 2026, 2 hours

**Objective**:  
Complete the Phase 1 graph loading requirement by extending the earlier lab CSR loader.

**Attempts made:**

I started from the Week 3 lab version of LoadGraph, which read the graph line by line, skipped empty/comment lines, stored edges in a `vector<pair<int,int>>`, and then converted the edge list into CSR.

The main thing I had to check was that Phase 1 graph files can have comment lines starting with #.

I also wanted the loader to work if the graph file has extra columns. For Phase 1, only the first two columns matter because they are `src dst`.

**Solutions/Workarounds**:

I kept the same CSR structure from the lab assignment.

I kept the Week 3 methodology for loading the graph:

  - Read each line with getline.
  - Skip blank lines and comments.
  - Use `istringstream` to read src and dst.
  - Store the pair in edge_list.
  - Count offsets.
  - Prefix sum the offsets.
  - Copy the offsets into current and fill the final edges array.

This kept the graph loading code close to the lab version instead of changing it into a completely different approach.

**Experiments/Analysis**:

For this session, I focused on getting the graph loader code organized and making sure the CSR structure matched the lab methodology.

The main check was making sure the loader could read the input file format correctly and build the two CSR arrays:

  - `offsets`
  - `edges`

The actual query testing was saved for the next session after the K-hop traversal and callbacks were added.

**Learning outcome**:

I learned that CSR uses one array for offsets and one array for neighbors, so once the graph is loaded, K-hop traversal can use `offsets[v]` to `offsets[v + 1]` to loop through the outgoing neighbors of a vertex.