#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
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
    std::vector<int> worklist_sizes;
};

void AddEdge(std::vector<InputEdge>& edge_list, int src, int dst, int weight, int& max_vertex) {
    edge_list.push_back({src, dst, weight});

    if (src > max_vertex) {
        max_vertex = src;
    }
    if (dst > max_vertex) {
        max_vertex = dst;
    }
}

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

        if (!(iss >> src >> dst)) {
            continue;
        }

        if (iss >> weight) {
            AddEdge(edge_list, src, dst, weight, max_vertex);
        } else {
            weight = ((src * 31 + dst * 17) % 400) + 1;
            AddEdge(edge_list, src, dst, weight, max_vertex);
            AddEdge(edge_list, dst, src, weight, max_vertex);
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

bool AtomicLower(std::vector<std::atomic<long long>>& value, int v, long long candidate, long long previous_value, int algorithm, bool& first_update) {
    long long old = value[v].load();

    while (candidate < old) {
        long long before = old;

        if (value[v].compare_exchange_weak(old, candidate)) {
            if (algorithm == 1) {
                first_update = true;
            } else if (before == previous_value) {
                first_update = true;
            } else {
                first_update = false;
            }

            return true;
        }
    }

    first_update = false;
    return false;
}

RunResult RunDenseAtomic(const CsrGraph& g, int algorithm, int nthreads) {
    if (nthreads <= 0) {
        nthreads = 1;
    }

    long long inf = std::numeric_limits<long long>::max() / 4;
    std::vector<std::atomic<long long>> value(g.num_vertices);
    std::vector<long long> value_prev(g.num_vertices, inf);
    std::vector<std::atomic<int>> frontier(g.num_vertices);
    std::vector<std::atomic<int>> next_frontier(g.num_vertices);
    std::vector<int> updated(nthreads, 0);

    for (int i = 0; i < g.num_vertices; i++) {
        value[i].store(inf);
        frontier[i].store(0);
        next_frontier[i].store(0);
    }

    if (algorithm == 3) {
        for (int i = 0; i < g.num_vertices; i++) {
            value[i].store(i);
            value_prev[i] = i;
            frontier[i].store(1);
        }
    } else if (g.num_vertices > 0) {
        value[0].store(0);
        value_prev[0] = 0;
        frontier[0].store(1);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
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

                if (algorithm != 3 && value_prev[u] == inf) {
                    continue;
                }

                if (algorithm == 1) {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u] + 1;
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            updated[tid] = 1;

                            if (first_update) {
                                next_frontier[v].store(1);
                            }
                        }
                    }
                } else if (algorithm == 2) {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u] + g.weights[i];
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            updated[tid] = 1;

                            if (first_update) {
                                next_frontier[v].store(1);
                            }
                        }
                    }
                } else {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u];
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            updated[tid] = 1;

                            if (first_update) {
                                next_frontier[v].store(1);
                            }
                        }
                    }

                    for (int i = g.in_offsets[u]; i < g.in_offsets[u + 1]; i++) {
                        int v = g.in_edges[i];
                        long long candidate = value_prev[u];
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            updated[tid] = 1;

                            if (first_update) {
                                next_frontier[v].store(1);
                            }
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
            value_prev[i] = value[i].load();
            frontier[i].store(next_frontier[i].load());
            next_frontier[i].store(0);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::vector<long long> result(g.num_vertices);

    for (int i = 0; i < g.num_vertices; i++) {
        result[i] = value[i].load();

        if (algorithm != 3 && result[i] == inf) {
            result[i] = -1;
        }
    }

    return {ms, result, {}};
}

RunResult RunSparseAtomic(const CsrGraph& g, int algorithm, int nthreads) {
    if (nthreads <= 0) {
        nthreads = 1;
    }

    long long inf = std::numeric_limits<long long>::max() / 4;
    std::vector<std::atomic<long long>> value(g.num_vertices);
    std::vector<long long> value_prev(g.num_vertices, inf);
    std::vector<int> current;
    std::vector<int> worklist_sizes;

    for (int i = 0; i < g.num_vertices; i++) {
        value[i].store(inf);
    }

    if (algorithm == 3) {
        current.reserve(g.num_vertices);

        for (int i = 0; i < g.num_vertices; i++) {
            value[i].store(i);
            value_prev[i] = i;
            current.push_back(i);
        }
    } else if (g.num_vertices > 0) {
        value[0].store(0);
        value_prev[0] = 0;
        current.push_back(0);
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    while (!current.empty()) {
        worklist_sizes.push_back((int)current.size());
        std::vector<std::vector<int>> next_thread(nthreads);

        auto process_range = [&](int tid, int start, int end) {
            for (int index = start; index < end; index++) {
                int u = current[index];

                if (algorithm != 3 && value_prev[u] == inf) {
                    continue;
                }

                if (algorithm == 1) {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u] + 1;
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            if (first_update) {
                                next_thread[tid].push_back(v);
                            }
                        }
                    }
                } else if (algorithm == 2) {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u] + g.weights[i];
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            if (first_update) {
                                next_thread[tid].push_back(v);
                            }
                        }
                    }
                } else {
                    for (int i = g.offsets[u]; i < g.offsets[u + 1]; i++) {
                        int v = g.edges[i];
                        long long candidate = value_prev[u];
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            if (first_update) {
                                next_thread[tid].push_back(v);
                            }
                        }
                    }

                    for (int i = g.in_offsets[u]; i < g.in_offsets[u + 1]; i++) {
                        int v = g.in_edges[i];
                        long long candidate = value_prev[u];
                        bool first_update = false;

                        if (AtomicLower(value, v, candidate, value_prev[v], algorithm, first_update)) {
                            if (first_update) {
                                next_thread[tid].push_back(v);
                            }
                        }
                    }
                }
            }
        };

        std::vector<std::thread> threads;

        for (int tid = 0; tid < nthreads; tid++) {
            int start = ((int)current.size() * tid) / nthreads;
            int end = ((int)current.size() * (tid + 1)) / nthreads;
            threads.push_back(std::thread(process_range, tid, start, end));
        }

        for (int i = 0; i < (int)threads.size(); i++) {
            threads[i].join();
        }

        std::vector<int> next;
        int total = 0;

        for (int tid = 0; tid < nthreads; tid++) {
            total += next_thread[tid].size();
        }

        next.reserve(total);

        for (int tid = 0; tid < nthreads; tid++) {
            next.insert(next.end(), next_thread[tid].begin(), next_thread[tid].end());
        }

        current.swap(next);

        for (int i = 0; i < (int)current.size(); i++) {
            int v = current[i];
            value_prev[v] = value[v].load();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::vector<long long> result(g.num_vertices);

    for (int i = 0; i < g.num_vertices; i++) {
        result[i] = value[i].load();

        if (algorithm != 3 && result[i] == inf) {
            result[i] = -1;
        }
    }

    return {ms, result, worklist_sizes};
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

void WriteAnswerFile(const char *filename, const std::vector<long long>& result) {
    std::ofstream fout(filename);

    for (int i = 0; i < (int)result.size(); i++) {
        fout << i << " " << result[i] << "\n";
    }
}

void PrintSizes(const std::string& name, const std::vector<int>& sizes) {
    std::cout << name << " sparse worklist sizes:";

    for (int i = 0; i < (int)sizes.size(); i++) {
        if (i < 25 || i >= (int)sizes.size() - 5) {
            std::cout << " " << sizes[i];
        } else if (i == 25) {
            std::cout << " ...";
        }
    }

    std::cout << std::endl;
}

void RunAlgorithm(const CsrGraph& g, int algorithm, const std::string& name, int nthreads) {
    RunResult dense = RunDenseAtomic(g, algorithm, nthreads);
    RunResult sparse = RunSparseAtomic(g, algorithm, nthreads);

    std::cout << name << " dense atomic 4t ms: " << dense.ms << std::endl;
    std::cout << name << " sparse atomic 4t ms: " << sparse.ms << std::endl;
    std::cout << name << " dense vs sparse differences: " << CountDifferences(dense.result, sparse.result) << std::endl;

    PrintSizes(name, sparse.worklist_sizes);

    if (algorithm == 1) {
        std::cout << "BFS (lock-free dense push): source=0 -> vertex=50, hops = " << dense.result[50] << std::endl;
        std::cout << "BFS (lock-free sparse push): source=0 -> vertex=50, hops = " << sparse.result[50] << std::endl;
        WriteAnswerFile("BFS_2c.txt", sparse.result);
    } else if (algorithm == 2) {
        std::cout << "SSSP: source=0 -> vertex=50, distance = " << sparse.result[50] << std::endl;
        WriteAnswerFile("SSSP_2c.txt", sparse.result);
    } else {
        std::cout << "CC: vertex=50, component = " << sparse.result[50] << std::endl;
        WriteAnswerFile("CC_2c.txt", sparse.result);
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

    RunAlgorithm(g, 1, "BFS", nthreads);
    RunAlgorithm(g, 2, "SSSP", nthreads);
    RunAlgorithm(g, 3, "CC", nthreads);

    std::cout << "Wrote BFS_2c.txt, SSSP_2c.txt, and CC_2c.txt" << std::endl;

    return 0;
}
