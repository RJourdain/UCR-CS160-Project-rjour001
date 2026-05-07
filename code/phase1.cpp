#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct CSRGraph {
    int num_vertices;
    std::vector<int> offsets;
    std::vector<int> edges;
};

CSRGraph LoadGraph(const char *filename) {
    std::ifstream fin(filename);

    if (!fin.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        std::exit(1);
    }

    std::vector<std::pair<int, int>> edge_list;
    std::string line;
    int src, dst;
    int max_vertex = -1;

    while (std::getline(fin, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);

        if (!(iss >> src >> dst)) {
            continue;
        }

        edge_list.push_back({src, dst});

        if (src > max_vertex) {
            max_vertex = src;
        }
        if (dst > max_vertex) {
            max_vertex = dst;
        }
    }

    CSRGraph g;
    g.num_vertices = max_vertex + 1;
    g.offsets.resize(g.num_vertices + 1, 0);

    for (int i = 0; i < (int)edge_list.size(); i++) {
        int from = edge_list[i].first;
        g.offsets[from + 1]++;
    }

    for (int i = 1; i <= g.num_vertices; i++) {
        g.offsets[i] = g.offsets[i] + g.offsets[i - 1];
    }

    g.edges.resize(edge_list.size());

    std::vector<int> current = g.offsets;

    for (int i = 0; i < (int)edge_list.size(); i++) {
        int from = edge_list[i].first;
        int to = edge_list[i].second;
        g.edges[current[from]] = to;
        current[from]++;
    }

    return g;
}

std::vector<int> GetReachableWithinK(const CSRGraph& g, int src, int K) {
    std::vector<int> reachable;

    if (src < 0 || src >= g.num_vertices || K <= 0) {
        return reachable;
    }

    std::vector<char> visited(g.num_vertices, 0);
    std::queue<std::pair<int, int>> q;

    visited[src] = 1;
    q.push({src, 0});

    while (!q.empty()) {
        int vertex = q.front().first;
        int depth = q.front().second;
        q.pop();

        if (depth == K) {
            continue;
        }

        for (int i = g.offsets[vertex]; i < g.offsets[vertex + 1]; i++) {
            int next = g.edges[i];

            if (next < 0 || next >= g.num_vertices) {
                continue;
            }

            if (!visited[next]) {
                visited[next] = 1;
                reachable.push_back(next);
                q.push({next, depth + 1});
            }
        }
    }

    return reachable;
}

std::string CountReachable(const CSRGraph& g, int src, int K) {
    std::vector<int> reachable = GetReachableWithinK(g, src, K);
    return std::to_string((int)reachable.size());
}

std::string MaxReachable(const CSRGraph& g, int src, int K) {
    std::vector<int> reachable = GetReachableWithinK(g, src, K);

    if (reachable.size() == 0) {
        return "-1";
    }

    int max_node = reachable[0];

    for (int i = 0; i < (int)reachable.size(); i++) {
        if (reachable[i] > max_node) {
            max_node = reachable[i];
        }
    }

    return std::to_string(max_node);
}

using QueryCallback = std::function<std::string(const CSRGraph&, int, int)>;

struct QueryTask {
    int src;
    int K;
    int query_type;
    std::string expected_result;
    QueryCallback cb;
    std::string result;
};

std::vector<QueryTask> LoadQueries(const char *filename) {
    std::ifstream fin(filename);

    if (!fin.is_open()) {
        std::cerr << "Could not open query file: " << filename << std::endl;
        std::exit(1);
    }

    std::vector<QueryTask> tasks;
    std::string line;

    while (std::getline(fin, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        QueryTask task;

        if (!(iss >> task.src >> task.K >> task.query_type >> task.expected_result)) {
            continue;
        }

        if (task.query_type == 1) {
            task.cb = CountReachable;
        } else {
            task.cb = MaxReachable;
        }

        task.result = "";
        tasks.push_back(task);
    }

    return tasks;
}

void RunTasksSequential(const CSRGraph& g, std::vector<QueryTask>& tasks) {
    for (int i = 0; i < (int)tasks.size(); i++) {
        tasks[i].result = tasks[i].cb(g, tasks[i].src, tasks[i].K);
    }
}

void RunTasksParallel(const CSRGraph& g, std::vector<QueryTask>& tasks, int num_threads) {
    if (num_threads <= 0) {
        num_threads = 1;
    }

    std::atomic<int> next_task(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.push_back(std::thread([&]() {
            while (true) {
                int task_id = next_task.fetch_add(1);

                if (task_id >= (int)tasks.size()) {
                    break;
                }

                tasks[task_id].result = tasks[task_id].cb(g, tasks[task_id].src, tasks[task_id].K);
            }
        }));
    }

    for (int i = 0; i < (int)threads.size(); i++) {
        threads[i].join();
    }
}

int CountExpectedMismatches(const std::vector<QueryTask>& tasks) {
    int mismatches = 0;

    for (int i = 0; i < (int)tasks.size(); i++) {
        if (tasks[i].result != tasks[i].expected_result) {
            mismatches++;
        }
    }

    return mismatches;
}

int CountResultDifferences(const std::vector<QueryTask>& a, const std::vector<QueryTask>& b) {
    int differences = 0;

    for (int i = 0; i < (int)a.size() && i < (int)b.size(); i++) {
        if (a[i].result != b[i].result) {
            differences++;
        }
    }

    return differences;
}

void PrintSampleResults(const std::vector<QueryTask>& tasks, int limit) {
    for (int i = 0; i < (int)tasks.size() && i < limit; i++) {
        std::cout << "query " << i
                  << " src=" << tasks[i].src
                  << " K=" << tasks[i].K
                  << " type=" << tasks[i].query_type
                  << " result=" << tasks[i].result
                  << " expected=" << tasks[i].expected_result
                  << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./phase1 graph_file query_file num_threads" << std::endl;
        return 0;
    }

    const char *graph_file = argv[1];
    const char *query_file = argv[2];
    int num_threads = 4;

    if (argc > 3) {
        num_threads = std::atoi(argv[3]);
    }

    CSRGraph graph = LoadGraph(graph_file);
    std::vector<QueryTask> tasks = LoadQueries(query_file);

    std::cout << "Graph loaded" << std::endl;
    std::cout << "vertices: " << graph.num_vertices << std::endl;
    std::cout << "edges: " << graph.edges.size() << std::endl;
    std::cout << "queries: " << tasks.size() << std::endl;

    std::vector<QueryTask> seq_tasks = tasks;
    std::vector<QueryTask> par_tasks = tasks;

    auto seq_start = std::chrono::high_resolution_clock::now();
    RunTasksSequential(graph, seq_tasks);
    auto seq_end = std::chrono::high_resolution_clock::now();

    auto par_start = std::chrono::high_resolution_clock::now();
    RunTasksParallel(graph, par_tasks, num_threads);
    auto par_end = std::chrono::high_resolution_clock::now();

    double seq_ms = std::chrono::duration<double, std::milli>(seq_end - seq_start).count();
    double par_ms = std::chrono::duration<double, std::milli>(par_end - par_start).count();

    int seq_mismatches = CountExpectedMismatches(seq_tasks);
    int par_mismatches = CountExpectedMismatches(par_tasks);
    int result_differences = CountResultDifferences(seq_tasks, par_tasks);

    std::cout << "sequential time ms: " << seq_ms << std::endl;
    std::cout << "parallel time ms: " << par_ms << std::endl;
    std::cout << "threads: " << num_threads << std::endl;
    std::cout << "sequential expected mismatches: " << seq_mismatches << std::endl;
    std::cout << "parallel expected mismatches: " << par_mismatches << std::endl;
    std::cout << "sequential vs parallel result differences: " << result_differences << std::endl;

    PrintSampleResults(seq_tasks, 5);

    return 0;
}
