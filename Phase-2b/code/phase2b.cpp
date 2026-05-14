#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct CsrGraph {
    int num_vertices;
    std::vector<int> offsets;
    std::vector<int> edges;
    std::vector<int> weights;
    std::vector<int> in_offsets;
    std::vector<int> in_edges;
    std::vector<int> in_weights;
};

struct InputEdge {
    int src;
    int dst;
    int weight;
};

struct RunResult {
    double ms;
    std::vector<long long> result;
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
    g.in_offsets.resize(g.num_vertices + 1, 0);

    for (int i = 0; i < (int)edge_list.size(); i++) {
        int from = edge_list[i].src;
        int to = edge_list[i].dst;
        g.offsets[from + 1]++;
        g.in_offsets[to + 1]++;
    }

    for (int i = 1; i <= g.num_vertices; i++) {
        g.offsets[i] = g.offsets[i] + g.offsets[i - 1];
        g.in_offsets[i] = g.in_offsets[i] + g.in_offsets[i - 1];
    }

    g.edges.resize(edge_list.size());
    g.weights.resize(edge_list.size());
    g.in_edges.resize(edge_list.size());
    g.in_weights.resize(edge_list.size());

    std::vector<int> current = g.offsets;
    std::vector<int> in_current = g.in_offsets;

    for (int i = 0; i < (int)edge_list.size(); i++) {
        int from = edge_list[i].src;
        int to = edge_list[i].dst;
        int w = edge_list[i].weight;

        int pos = current[from];
        g.edges[pos] = to;
        g.weights[pos] = w;
        current[from]++;

        int in_pos = in_current[to];
        g.in_edges[in_pos] = from;
        g.in_weights[in_pos] = w;
        in_current[to]++;
    }

    return g;
}

RunResult RunPush(const CsrGraph& g, int algorithm, bool dense, bool parallel, int nthreads, int lock_count) {
    if (nthreads <= 0) {
        nthreads = 1;
    }

    if (!parallel) {
        nthreads = 1;
    }

    long long inf = std::numeric_limits<long long>::max() / 4;
    std::vector<long long> value(g.num_vertices, inf);
    std::vector<long long> value_prev(g.num_vertices, inf);
    std::vector<unsigned char> frontier;
    std::vector<unsigned char> next_frontier;
    std::vector<int> updated(nthreads, 0);
    std::vector<std::mutex> locks;

    if (parallel) {
        if (lock_count <= 0) {
            lock_count = 1024;
        }
        locks = std::vector<std::mutex>(lock_count);
    }

    if (algorithm == 3) {
        for (int i = 0; i < g.num_vertices; i++) {
            value[i] = i;
            value_prev[i] = i;
        }
    } else {
        value[0] = 0;
        value_prev[0] = 0;
    }

    if (dense) {
        frontier.assign(g.num_vertices, 0);
        next_frontier.assign(g.num_vertices, 0);

        if (algorithm == 3) {
            for (int i = 0; i < g.num_vertices; i++) {
                frontier[i] = 1;
            }
        } else {
            frontier[0] = 1;
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    bool any_updated = true;

    while (any_updated) {
        any_updated = false;

        for (int i = 0; i < nthreads; i++) {
            updated[i] = 0;
        }

        auto try_update = [&](int tid, int v, long long candidate) {
            if (parallel) {
                std::lock_guard<std::mutex> guard(locks[v % locks.size()]);

                if (candidate < value[v]) {
                    value[v] = candidate;
                    updated[tid] = 1;

                    if (dense) {
                        next_frontier[v] = 1;
                    }
                }
            } else {
                if (candidate < value[v]) {
                    value[v] = candidate;
                    updated[tid] = 1;

                    if (dense) {
                        next_frontier[v] = 1;
                    }
                }
            }
        };

        auto process_range = [&](int tid, int start, int end) {
            for (int u = start; u < end; u++) {
                if (dense && frontier[u] == 0) {
                    continue;
                }

                if (algorithm != 3 && value_prev[u] == inf) {
                    continue;
                }

                if (algorithm == 1) {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u] + 1;
                        try_update(tid, v, candidate);
                    }
                } else if (algorithm == 2) {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u] + g.weights[i];
                        try_update(tid, v, candidate);
                    }
                } else {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u];
                        try_update(tid, v, candidate);
                    }

                    for (int i = g.in_offsets[u]; i < g.in_offsets[u + 1]; i++) {
                        int v = g.in_edges[i];
                        long long candidate = value_prev[u];
                        try_update(tid, v, candidate);
                    }
                }
            }
        };

        if (parallel) {
            std::vector<std::thread> threads;

            for (int tid = 0; tid < nthreads; tid++) {
                int start = (g.num_vertices * tid) / nthreads;
                int end = (g.num_vertices * (tid + 1)) / nthreads;
                threads.push_back(std::thread(process_range, tid, start, end));
            }

            for (int i = 0; i < (int)threads.size(); i++) {
                threads[i].join();
            }
        } else {
            process_range(0, 0, g.num_vertices);
        }

        for (int i = 0; i < nthreads; i++) {
            if (updated[i]) {
                any_updated = true;
            }
        }

        value_prev = value;

        if (dense) {
            frontier.swap(next_frontier);
            std::fill(next_frontier.begin(), next_frontier.end(), 0);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::vector<long long> result = value;

    if (algorithm != 3) {
        for (int i = 0; i < (int)result.size(); i++) {
            if (result[i] == inf) {
                result[i] = -1;
            }
        }
    }

    return {ms, result};
}

int CountDifferences(const std::vector<long long>& a, const std::vector<long long>& b) {
    int differences = 0;

    for (int i = 0; i < (int)a.size() && i < (int)b.size(); i++) {
        if (a[i] != b[i]) {
            differences++;
        }
    }

    return differences;
}

void PrintTopInDegrees(const CsrGraph& g) {
    std::vector<std::pair<int, int>> top;

    for (int v = 0; v < g.num_vertices; v++) {
        int degree = g.in_offsets[v + 1] - g.in_offsets[v];
        top.push_back({degree, v});
    }

    std::sort(top.begin(), top.end(), std::greater<std::pair<int, int>>());

    std::cout << "top in-degrees:" << std::endl;

    for (int i = 0; i < 3 && i < (int)top.size(); i++) {
        std::cout << "vertex " << top[i].second << " in-degree " << top[i].first << std::endl;
    }
}

void RunAlgorithm(const CsrGraph& g, int algorithm, const std::string& name, int nthreads) {
    RunResult serial_naive = RunPush(g, algorithm, false, false, 1, 0);
    RunResult serial_dense = RunPush(g, algorithm, true, false, 1, 0);
    RunResult parallel_naive_1 = RunPush(g, algorithm, false, true, 1, 1024);
    RunResult parallel_naive_n = RunPush(g, algorithm, false, true, nthreads, 1024);
    RunResult parallel_dense_1 = RunPush(g, algorithm, true, true, 1, 1024);
    RunResult parallel_dense_n = RunPush(g, algorithm, true, true, nthreads, 1024);

    std::cout << name << " serial naive ms: " << serial_naive.ms << std::endl;
    std::cout << name << " serial dense ms: " << serial_dense.ms << std::endl;
    std::cout << name << " naive parallel 1t ms: " << parallel_naive_1.ms << std::endl;
    std::cout << name << " naive parallel " << nthreads << "t ms: " << parallel_naive_n.ms << std::endl;
    std::cout << name << " dense parallel 1t ms: " << parallel_dense_1.ms << std::endl;
    std::cout << name << " dense parallel " << nthreads << "t ms: " << parallel_dense_n.ms << std::endl;

    std::cout << name << " serial naive vs serial dense differences: " << CountDifferences(serial_naive.result, serial_dense.result) << std::endl;
    std::cout << name << " serial naive vs naive parallel differences: " << CountDifferences(serial_naive.result, parallel_naive_n.result) << std::endl;
    std::cout << name << " serial naive vs dense parallel differences: " << CountDifferences(serial_naive.result, parallel_dense_n.result) << std::endl;

    std::cout << name << " dense parallel " << nthreads << "t K=1024 ms: " << parallel_dense_n.ms << std::endl;

    RunResult k16384 = RunPush(g, algorithm, true, true, nthreads, 16384);
    RunResult k262144 = RunPush(g, algorithm, true, true, nthreads, 262144);

    std::cout << name << " dense parallel " << nthreads << "t K=16384 ms: " << k16384.ms << std::endl;
    std::cout << name << " dense parallel " << nthreads << "t K=262144 ms: " << k262144.ms << std::endl;

    if (algorithm == 1) {
        std::cout << "BFS (naive push):     source=0 -> vertex=50, hops = " << serial_naive.result[50] << std::endl;
        std::cout << "BFS (dense worklist): source=0 -> vertex=50, hops = " << serial_dense.result[50] << std::endl;
    } else if (algorithm == 2) {
        std::cout << "SSSP: source=0 -> vertex=50, distance = " << serial_naive.result[50] << std::endl;
    } else {
        std::cout << "CC: vertex=50, component = " << serial_naive.result[50] << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " graph_file num_threads" << std::endl;
        return 1;
    }

    int nthreads = std::atoi(argv[2]);

    if (nthreads <= 0) {
        nthreads = 1;
    }

    CsrGraph g = LoadGraph(argv[1]);

    std::cout << "Graph loaded" << std::endl;
    std::cout << "vertices: " << g.num_vertices << std::endl;
    std::cout << "edges: " << g.edges.size() << std::endl;
    std::cout << "threads: " << nthreads << std::endl;

    PrintTopInDegrees(g);

    RunAlgorithm(g, 1, "BFS", nthreads);
    RunAlgorithm(g, 2, "SSSP", nthreads);
    RunAlgorithm(g, 3, "CC", nthreads);

    return 0;
}