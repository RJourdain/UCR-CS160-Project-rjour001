# Development Journal - Session 2

**Name/NetID**: Ryan Jourdain / rjour001

**Session Date**: May 16, 2026, 7 hours

**Objective**:  
Finish Phase 2c by adding the sparse atomic worklist version and running the LiveJournal and RoadNet datasets.

**Attempts made:**

After the dense atomic version was working, I added the sparse worklist version. The main difference is that the dense version still scans the whole vertex range every round, while the sparse version stores the active vertices in a list.

I wanted the sparse version to avoid wasting time checking vertices that were not active. This is especially useful when the frontier is small.

The harder part was avoiding duplicate work in the sparse list. In the dense bitmap version, duplicates are not a big problem because setting a bitmap entry to 1 more than once still gives the same result. In the sparse version, pushing the same vertex into the next list multiple times would make the next round do extra work.

**Solutions/Workarounds**:

For BFS the sparse worklist was easier because a vertex is discovered once. The thread that wins the atomic update is the only one that adds that vertex to the next worklist.

For SSSP and CC a vertex can improve more than once in the same round. To avoid adding the same vertex too many times, I used the previous round value as the check. The first thread that lowers a vertex from its previous value adds it to the next worklist. Later improvements in the same round can still update the value, but they do not add another copy of the same vertex.

I also kept per-thread next lists. Each thread writes to its own local list first, then the lists get combined after the round. This avoids having every thread push into one shared vector.

I added output for the sparse worklist sizes so I could see how the frontier changed each round.

**Experiments/Analysis**:

I first ran the full soc-LiveJournal1-weighted.txt dataset using 4 threads. The program compares the dense atomic version against the sparse atomic version for BFS, SSSP, and CC.

The command used was:

g++ -std=c++17 -O2 -pthread code/phase2c.cpp -o phase2c  
./phase2c soc-LiveJournal1-weighted.txt 4 | tee phase2c_livejournal_output.txt

The full LiveJournal dataset loaded:

vertices: 4847571  
edges: 68993773  
threads: 4

**LiveJournal BFS results:**

BFS dense atomic 4t ms: 2599.86  
BFS sparse atomic 4t ms: 174.19  
BFS dense vs sparse differences: 0  
BFS sparse worklist sizes: 1 46 2155 101386 1174905 2166715 811128 122063 17877 3176 683 165 28 15 4  
BFS lock-free dense push from source 0 to vertex 50 gave hops = 3  
BFS lock-free sparse push from source 0 to vertex 50 gave hops = 3

The BFS dense and sparse versions matched. The sparse version was much faster because the active frontier started small and only expanded through the graph instead of scanning every vertex every round.

The sparse BFS version was about 14.9 times faster than the dense version in this run.

**LiveJournal SSSP results:**

SSSP dense atomic 4t ms: 9283.04  
SSSP sparse atomic 4t ms: 1706.2  
SSSP dense vs sparse differences: 0  
SSSP sparse worklist sizes: 1 46 2164 102328 1257882 3341181 3986628 3864181 3576074 3238463 2876942 2514521 2168680 1837051 1522658 1232800 972736 741528 549843 387088 259682 165780 100328 57764 31754 ... 20 18 17 10 2  
SSSP from source 0 to vertex 50 gave distance = 63

The SSSP dense and sparse versions also matched. The sparse version helped a lot, but SSSP still took longer than BFS because weighted distances can keep improving over many rounds.

The sparse SSSP version was about 5.4 times faster than the dense version in this run.

**LiveJournal CC results:**

CC dense atomic 4t ms: 3333.83  
CC sparse atomic 4t ms: 1955.75  
CC dense vs sparse differences: 0  
CC sparse worklist sizes: 4847571 4842830 4841053 4822160 4171968 1703021 276464 34435 4529 708 143 10  
CC for vertex 50 gave component = 0

The CC dense and sparse versions matched too. The sparse version was still faster, but the speedup was smaller than BFS and SSSP because CC starts with every vertex active.

The sparse CC version was about 1.7 times faster than the dense version in this run.

**Correctness check for LiveJournal:**

The program generated these local answer files:

BFS_2c.txt  
SSSP_2c.txt  
CC_2c.txt

I compared them against the expected answer files using:

diff -q BFS_2c.txt BFS.txt  
diff -q SSSP_2c.txt SSSP.txt  
diff -q CC_2c.txt CC.txt

The diff commands did not print anything, so the generated files matched the expected files.

**LiveJournal timing summary:**

**BFS:**

  - Dense atomic 4 threads: 2599.86 ms
  - Sparse atomic 4 threads: 174.19 ms
  - Dense vs sparse differences: 0
  - Sparse speedup over dense: about 14.9x

**SSSP:**

  - Dense atomic 4 threads: 9283.04 ms
  - Sparse atomic 4 threads: 1706.2 ms
  - Dense vs sparse differences: 0
  - Sparse speedup over dense: about 5.4x

**CC:**

  - Dense atomic 4 threads: 3333.83 ms
  - Sparse atomic 4 threads: 1955.75 ms
  - Dense vs sparse differences: 0
  - Sparse speedup over dense: about 1.7x

**Speedup comparison using Phase 2b serial lock-free naive push as the baseline:**

**BFS:**

  - Phase 2b dense parallel 4 threads with locks: 3.09x
  - Phase 2c dense parallel 4 threads with atomics: 0.47x
  - Phase 2c sparse parallel 4 threads with atomics: 7.06x

**SSSP:**

  - Phase 2b dense parallel 4 threads with locks: 1.72x
  - Phase 2c dense parallel 4 threads with atomics: 0.60x
  - Phase 2c sparse parallel 4 threads with atomics: 3.29x

**CC:**

  - Phase 2b dense parallel 4 threads with locks: 0.89x
  - Phase 2c dense parallel 4 threads with atomics: 0.77x
  - Phase 2c sparse parallel 4 threads with atomics: 1.31x

The 2c dense atomic version was not automatically faster than the Phase 2b locked dense version in my results. I expected atomics to help more, but the dense version still scans the whole graph every round and still has atomic update overhead. The biggest improvement came from using the sparse worklist, not just from replacing locks with atomics.

The biggest difference was in BFS. This makes sense because BFS starts from one source and the active set moves through the graph in frontier levels. The sparse worklist avoids checking a lot of inactive vertices.

SSSP also improved with sparse worklists, but it still has more rounds and more repeated updates because weighted distances can improve later.

CC improved the least on LiveJournal because the first sparse frontier starts with all vertices. Since every vertex starts active, the sparse version does not save as much work at the beginning compared to BFS.

**RoadNet-CA test:**

I also ran the RoadNet-CA dataset to compare the dense and sparse atomic versions on a different kind of graph.

The command used was:

./phase2c roadNet-CA.txt 4 | tee phase2c_roadnet_output.txt

The RoadNet-CA dataset loaded:

vertices: 1971281  
edges: 11066428  
threads: 4

**RoadNet BFS results:**

BFS dense atomic 4t ms: 35901.4  
BFS sparse atomic 4t ms: 50.9183  
BFS dense vs sparse differences: 0  
BFS sparse worklist sizes: 1 3 5 8 12 17 29 32 35 37 43 50 51 52 61 59 67 84 116 155 158 193 211 228 257 ... 65 47 32 20 8  
BFS lock-free dense push from source 0 to vertex 50 gave hops = 63  
BFS lock-free sparse push from source 0 to vertex 50 gave hops = 63

The RoadNet BFS sparse version was much faster than the dense version. The sparse frontier stayed very small compared to the whole graph, so scanning every vertex every round was very expensive for the dense version.

The sparse BFS version was about 705 times faster than the dense version on RoadNet.

**RoadNet SSSP results:**

SSSP dense atomic 4t ms: 50528  
SSSP sparse atomic 4t ms: 1900.35  
SSSP dense vs sparse differences: 0  
SSSP sparse worklist sizes: 1 3 5 8 13 18 32 35 45 53 69 83 88 97 110 114 129 167 225 306 358 439 509 599 677 ... 19 9 5 2 1  
SSSP from source 0 to vertex 50 gave distance = 8016

The RoadNet SSSP sparse version was also much faster than dense. The sparse version was about 26.6 times faster in this run.

**RoadNet CC results:**

CC dense atomic 4t ms: 38657.2  
CC sparse atomic 4t ms: 2445.2  
CC dense vs sparse differences: 0  
CC sparse worklist sizes: 1971281 1799803 1643191 1528763 1442303 1368862 1300458 1234976 1171974 1112682 1056919 1006995 961341 920358 883217 850475 821061 795643 770738 748081 728680 712274 697610 684169 672681 ... 112 79 52 28 8  
CC for vertex 50 gave component = 0

The RoadNet CC sparse version was faster too, but CC still started with all vertices active. The sparse version was about 15.8 times faster than the dense version on RoadNet.

**RoadNet timing summary:**

**BFS:**

  - Dense atomic 4 threads: 35901.4 ms
  - Sparse atomic 4 threads: 50.9183 ms
  - Dense vs sparse differences: 0
  - Sparse speedup over dense: about 705x

**SSSP:**

  - Dense atomic 4 threads: 50528 ms
  - Sparse atomic 4 threads: 1900.35 ms
  - Dense vs sparse differences: 0
  - Sparse speedup over dense: about 26.6x

**CC:**

  - Dense atomic 4 threads: 38657.2 ms
  - Sparse atomic 4 threads: 2445.2 ms
  - Dense vs sparse differences: 0
  - Sparse speedup over dense: about 15.8x

RoadNet showed an even bigger difference between dense and sparse than LiveJournal. My guess is that this is because road networks are more spread out and the BFS frontier stays small for many rounds. Dense scanning wastes a lot of time checking inactive vertices, while sparse worklists only process the active part of the graph.

**Week 5 lab result:**

I also ran the Week 5 lab file separately because the lab only asks for the lock-free dense push BFS result.

The Week 5 lab command was:

g++ -std=c++17 -O2 -pthread code/week5_lab.cpp -o week5_lab  
./week5_lab soc-LiveJournal1-weighted.txt | tee week5_lab_output.txt

The Week 5 lab program printed:

BFS lock-free dense push from source 0 to vertex 50 gave hops = 3
This matched the BFS result from the full Phase 2c program.

**Learning outcome**:

I learned that sparse worklists are useful because they focus on the active part of the graph instead of scanning every vertex every round.

I also learned that sparse worklists need more care than dense bitmaps. A dense bitmap naturally handles duplicates, but a sparse vector can accidentally add the same vertex multiple times if the code is not careful.

The biggest thing I learned is that lock-free atomic updates and sparse worklists solve different problems. Atomic updates remove the mutex overhead, while the sparse worklist reduces the amount of scanning.

The RoadNet test made this more obvious because the sparse version was way faster than the dense version. That helped me see that the graph structure matters a lot. A sparse worklist helps the most when only a small part of the graph is active in each round.