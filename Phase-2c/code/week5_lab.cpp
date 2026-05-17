#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

struct CsrGraph {
    int num_vertices;
    std::vector<int> offsets;
    std::vector<int> edges;
    std::vector<int> weights;
};

struct InputEdge {
    int src;
    int dst;
    int weight;
};

CsrGraph LoadGraph(const char *filename) {
    std::ifstream fin(filename);

    if (!fin.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        std::exit(1);
    }

    std::vector<InputEdge> edge_list;
    std::string line;
    int src, dst, weight;
    int max_vertex = -1;

    while (std::getline(fin, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);

        if (!(iss >> src >> dst >> weight)) {
            continue;
        }

        edge_list.push_back({src, dst, weight});

        if (src > max_vertex) {
            max_vertex = src;
        }
        if (dst > max_vertex) {
            max_vertex = dst;
        }
    }

    CsrGraph g;
    g.num_vertices = max_vertex + 1;
    g.offsets.resize(g.num_vertices + 1, 0);

    for (int i = 0; i < (int)edge_list.size(); i++) {
        g.offsets[edge_list[i].src + 1]++;
    }

    for (int i = 1; i <= g.num_vertices; i++) {
        g.offsets[i] = g.offsets[i] + g.offsets[i - 1];
    }

    g.edges.resize(edge_list.size());
    g.weights.resize(edge_list.size());

    std::vector<int> current = g.offsets;

    for (int i = 0; i < (int)edge_list.size(); i++) {
        int from = edge_list[i].src;
        int pos = current[from];
        g.edges[pos] = edge_list[i].dst;
        g.weights[pos] = edge_list[i].weight;
        current[from]++;
    }

    return g;
}

std::vector<long long> BfsLockFreeDense(const CsrGraph& g, int source, int nthreads) {
    if (nthreads <= 0) {
        nthreads = 1;
    }

    long long inf = std::numeric_limits<long long>::max() / 4;
    std::vector<std::atomic<long long>> dist(g.num_vertices);
    std::vector<long long> dist_prev(g.num_vertices, inf);
    std::vector<std::atomic<int>> frontier(g.num_vertices);
    std::vector<std::atomic<int>> next_frontier(g.num_vertices);
    std::vector<int> updated(nthreads, 0);

    for (int i = 0; i < g.num_vertices; i++) {
        dist[i].store(inf);
        frontier[i].store(0);
        next_frontier[i].store(0);
    }

    if (source >= 0 && source < g.num_vertices) {
        dist[source].store(0);
        dist_prev[source] = 0;
        frontier[source].store(1);
    }

    bool any_updated = true;

    while (any_updated) {
        any_updated = false;

        for (int i = 0; i < nthreads; i++) {
            updated[i] = 0;
        }

        auto process_range = [&](int tid, int start, int end) {
            for (int u = start; u < end; u++) {
                if (frontier[u].load() == 0) {
                    continue;
                }

                if (dist_prev[u] == inf) {
                    continue;
                }

                for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                    int v = g.edges[i];
                    long long candidate = dist_prev[u] + 1;
                    long long old = dist[v].load();

                    while (candidate < old) {
                        if (dist[v].compare_exchange_weak(old, candidate)) {
                            updated[tid] = 1;
                            next_frontier[v].store(1);
                            break;
                        }
                    }
                }
            }
        };

        std::vector<std::thread> threads;

        for (int tid = 0; tid < nthreads; tid++) {
            int start = (g.num_vertices * tid) / nthreads;
            int end = (g.num_vertices * (tid + 1)) / nthreads;
            threads.push_back(std::thread(process_range, tid, start, end));
        }

        for (int i = 0; i < (int)threads.size(); i++) {
            threads[i].join();
        }

        for (int i = 0; i < nthreads; i++) {
            if (updated[i]) {
                any_updated = true;
            }
        }

        for (int i = 0; i < g.num_vertices; i++) {
            dist_prev[i] = dist[i].load();
            frontier[i].store(next_frontier[i].load());
            next_frontier[i].store(0);
        }
    }

    std::vector<long long> result(g.num_vertices);

    for (int i = 0; i < g.num_vertices; i++) {
        result[i] = dist[i].load();

        if (result[i] == inf) {
            result[i] = -1;
        }
    }

    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " graph_file" << std::endl;
        return 1;
    }

    CsrGraph g = LoadGraph(argv[1]);
    std::vector<long long> dist = BfsLockFreeDense(g, 0, 4);

    long long answer = -1;

    if (50 >= 0 && 50 < g.num_vertices) {
        answer = dist[50];
    }

    std::cout << "BFS (lock-free dense push): source=0 -> vertex=50, hops = " << answer << std::endl;

    return 0;
}
