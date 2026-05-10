#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
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

class BspAlgorithm {
public:
    virtual ~BspAlgorithm() {}
    virtual bool HasWork() const = 0;
    virtual void Process(int tid, int v, const CsrGraph& g) = 0;
    virtual void PostRound() = 0;
};

void BspSerial(const CsrGraph& g, BspAlgorithm& algo) {
    while (algo.HasWork()) {
        for (int v = 0; v < g.num_vertices; v++) {
            algo.Process(0, v, g);
        }
        algo.PostRound();
    }
}

void BspParallel(const CsrGraph& g, BspAlgorithm& algo, int nthreads) {
    if (nthreads <= 0) {
        nthreads = 1;
    }

    while (algo.HasWork()) {
        std::vector<std::thread> threads;

        for (int tid = 0; tid < nthreads; tid++) {
            int start = (g.num_vertices * tid) / nthreads;
            int end = (g.num_vertices * (tid + 1)) / nthreads;

            threads.push_back(std::thread([&, tid, start, end]() {
                for (int v = start; v < end; v++) {
                    algo.Process(tid, v, g);
                }
            }));
        }

        for (int i = 0; i < (int)threads.size(); i++) {
            threads[i].join();
        }

        algo.PostRound();
    }
}

class Bfs : public BspAlgorithm {
public:
    Bfs(int vertices, int source, int nthreads) {
        dist_.assign(vertices, -1);
        dist_prev_.assign(vertices, -1);
        updated_.assign(nthreads, 0);

        if (source >= 0 && source < vertices) {
            dist_[source] = 0;
            dist_prev_[source] = 0;
        }

        any_updated_ = true;
    }

    bool HasWork() const override {
        return any_updated_;
    }

    void Process(int tid, int v, const CsrGraph& g) override {
        if (dist_prev_[v] != -1) {
            return;
        }

        int best = -1;

        for (int i = g.in_offsets[v]; i < g.in_offsets[v + 1]; i++) {
            int u = g.in_edges[i];

            if (dist_prev_[u] != -1) {
                int candidate = dist_prev_[u] + 1;

                if (best == -1 || candidate < best) {
                    best = candidate;
                }
            }
        }

        if (best != -1) {
            dist_[v] = best;
            updated_[tid] = 1;
        }
    }

    void PostRound() override {
        any_updated_ = false;

        for (int i = 0; i < (int)updated_.size(); i++) {
            if (updated_[i]) {
                any_updated_ = true;
            }
            updated_[i] = 0;
        }

        dist_prev_ = dist_;
    }

    const std::vector<int>& Result() const {
        return dist_;
    }

private:
    std::vector<int> dist_;
    std::vector<int> dist_prev_;
    std::vector<int> updated_;
    bool any_updated_;
};

class Sssp : public BspAlgorithm {
public:
    Sssp(int vertices, int source, int nthreads) {
        const long long INF = std::numeric_limits<long long>::max() / 4;
        dist_.assign(vertices, INF);
        dist_prev_.assign(vertices, INF);
        updated_.assign(nthreads, 0);
        inf_ = INF;

        if (source >= 0 && source < vertices) {
            dist_[source] = 0;
            dist_prev_[source] = 0;
        }

        any_updated_ = true;
    }

    bool HasWork() const override {
        return any_updated_;
    }

    void Process(int tid, int v, const CsrGraph& g) override {
        long long best = dist_prev_[v];

        for (int i = g.in_offsets[v]; i < g.in_offsets[v + 1]; i++) {
            int u = g.in_edges[i];
            int w = g.in_weights[i];

            if (dist_prev_[u] != inf_) {
                long long candidate = dist_prev_[u] + w;

                if (candidate < best) {
                    best = candidate;
                }
            }
        }

        if (best < dist_prev_[v]) {
            dist_[v] = best;
            updated_[tid] = 1;
        }
    }

    void PostRound() override {
        any_updated_ = false;

        for (int i = 0; i < (int)updated_.size(); i++) {
            if (updated_[i]) {
                any_updated_ = true;
            }
            updated_[i] = 0;
        }

        dist_prev_ = dist_;
    }

    const std::vector<long long>& Result() const {
        return dist_;
    }

    long long Inf() const {
        return inf_;
    }

private:
    std::vector<long long> dist_;
    std::vector<long long> dist_prev_;
    std::vector<int> updated_;
    bool any_updated_;
    long long inf_;
};

class Cc : public BspAlgorithm {
public:
    Cc(int vertices, int nthreads) {
        label_.resize(vertices);
        label_prev_.resize(vertices);
        updated_.assign(nthreads, 0);

        for (int i = 0; i < vertices; i++) {
            label_[i] = i;
            label_prev_[i] = i;
        }

        any_updated_ = true;
    }

    bool HasWork() const override {
        return any_updated_;
    }

    void Process(int tid, int v, const CsrGraph& g) override {
        int best = label_prev_[v];

        for (int i = g.in_offsets[v]; i < g.in_offsets[v + 1]; i++) {
            int u = g.in_edges[i];

            if (label_prev_[u] < best) {
                best = label_prev_[u];
            }
        }

        for (int i = g.offsets[v]; i < g.offsets[v + 1]; i++) {
            int u = g.edges[i];

            if (label_prev_[u] < best) {
                best = label_prev_[u];
            }
        }

        if (best < label_prev_[v]) {
            label_[v] = best;
            updated_[tid] = 1;
        }
    }

    void PostRound() override {
        any_updated_ = false;

        for (int i = 0; i < (int)updated_.size(); i++) {
            if (updated_[i]) {
                any_updated_ = true;
            }
            updated_[i] = 0;
        }

        label_prev_ = label_;
    }

    const std::vector<int>& Result() const {
        return label_;
    }

private:
    std::vector<int> label_;
    std::vector<int> label_prev_;
    std::vector<int> updated_;
    bool any_updated_;
};

template <typename Func>
double TimeMs(Func f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int CountDifferences(const std::vector<int>& a, const std::vector<int>& b) {
    int differences = 0;

    for (int i = 0; i < (int)a.size() && i < (int)b.size(); i++) {
        if (a[i] != b[i]) {
            differences++;
        }
    }

    return differences;
}

int CountDifferencesLong(const std::vector<long long>& a, const std::vector<long long>& b) {
    int differences = 0;

    for (int i = 0; i < (int)a.size() && i < (int)b.size(); i++) {
        if (a[i] != b[i]) {
            differences++;
        }
    }

    return differences;
}

void WriteIntResults(const std::string& filename, const std::vector<int>& result) {
    std::ofstream fout(filename);

    for (int i = 0; i < (int)result.size(); i++) {
        fout << i << " " << result[i] << std::endl;
    }
}

void WriteSsspResults(const std::string& filename, const std::vector<long long>& result, long long inf) {
    std::ofstream fout(filename);

    for (int i = 0; i < (int)result.size(); i++) {
        if (result[i] == inf) {
            fout << i << " -1" << std::endl;
        } else {
            fout << i << " " << result[i] << std::endl;
        }
    }
}

std::string PathJoin(const std::string& dir, const std::string& file) {
    if (dir.size() == 0 || dir == ".") {
        return file;
    }

    if (dir[dir.size() - 1] == '/') {
        return dir + file;
    }

    return dir + "/" + file;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: ./phase2a graph_file [threads] [output_dir]" << std::endl;
        return 1;
    }

    const char *graph_file = argv[1];
    int threads = 4;
    std::string output_dir = ".";

    if (argc >= 3) {
        threads = std::atoi(argv[2]);
    }

    if (argc >= 4) {
        output_dir = argv[3];
    }

    if (threads <= 0) {
        threads = 1;
    }

    CsrGraph g = LoadGraph(graph_file);

    std::cout << "Graph loaded" << std::endl;
    std::cout << "vertices: " << g.num_vertices << std::endl;
    std::cout << "edges: " << g.edges.size() << std::endl;
    std::cout << "threads: " << threads << std::endl;

    Bfs bfs_s(g.num_vertices, 0, 1);
    double bfs_serial_ms = TimeMs([&]() {
        BspSerial(g, bfs_s);
    });

    Bfs bfs_p(g.num_vertices, 0, threads);
    double bfs_parallel_ms = TimeMs([&]() {
        BspParallel(g, bfs_p, threads);
    });

    Sssp sssp_s(g.num_vertices, 0, 1);
    double sssp_serial_ms = TimeMs([&]() {
        BspSerial(g, sssp_s);
    });

    Sssp sssp_p(g.num_vertices, 0, threads);
    double sssp_parallel_ms = TimeMs([&]() {
        BspParallel(g, sssp_p, threads);
    });

    Cc cc_s(g.num_vertices, 1);
    double cc_serial_ms = TimeMs([&]() {
        BspSerial(g, cc_s);
    });

    Cc cc_p(g.num_vertices, threads);
    double cc_parallel_ms = TimeMs([&]() {
        BspParallel(g, cc_p, threads);
    });

    int bfs_diffs = CountDifferences(bfs_s.Result(), bfs_p.Result());
    int sssp_diffs = CountDifferencesLong(sssp_s.Result(), sssp_p.Result());
    int cc_diffs = CountDifferences(cc_s.Result(), cc_p.Result());

    std::cout << "BFS serial ms: " << bfs_serial_ms << std::endl;
    std::cout << "BFS parallel ms: " << bfs_parallel_ms << std::endl;
    std::cout << "BFS serial vs parallel differences: " << bfs_diffs << std::endl;

    std::cout << "SSSP serial ms: " << sssp_serial_ms << std::endl;
    std::cout << "SSSP parallel ms: " << sssp_parallel_ms << std::endl;
    std::cout << "SSSP serial vs parallel differences: " << sssp_diffs << std::endl;

    std::cout << "CC serial ms: " << cc_serial_ms << std::endl;
    std::cout << "CC parallel ms: " << cc_parallel_ms << std::endl;
    std::cout << "CC serial vs parallel differences: " << cc_diffs << std::endl;

    int vertex = 50;
    int bfs_answer = -1;
    long long sssp_answer = -1;

    if (vertex >= 0 && vertex < g.num_vertices) {
        bfs_answer = bfs_s.Result()[vertex];

        if (sssp_s.Result()[vertex] != sssp_s.Inf()) {
            sssp_answer = sssp_s.Result()[vertex];
        }
    }

    std::cout << "BFS:  source=0 -> vertex=50, hops = " << bfs_answer << std::endl;
    std::cout << "SSSP: source=0 -> vertex=50, distance = " << sssp_answer << std::endl;

    WriteIntResults(PathJoin(output_dir, "BFS.txt"), bfs_s.Result());
    WriteSsspResults(PathJoin(output_dir, "SSSP.txt"), sssp_s.Result(), sssp_s.Inf());
    WriteIntResults(PathJoin(output_dir, "CC.txt"), cc_s.Result());

    std::cout << "Wrote BFS.txt, SSSP.txt, and CC.txt" << std::endl;

    return 0;
}
