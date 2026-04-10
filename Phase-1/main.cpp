#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <queue>
#include <thread>
#include <functional>
#include <chrono>

// ===================== STRUCTS =====================

struct CSRGraph {
    int num_vertices;
    std::vector<int> offsets;
    std::vector<int> edges;
};

using QueryCallback = std::function<std::string(const CSRGraph&, int, int)>;

struct QueryTask {
    int src;
    int K;
    QueryCallback cb;
    std::string result;
};

// ===================== LOAD GRAPH =====================

CSRGraph LoadGraph(const char *filename) {
    std::ifstream file(filename);
    std::vector<std::pair<int, int>> edge_list;
    std::string line;

    int max_node = -1;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int src, dst;
        iss >> src >> dst;

        edge_list.push_back({src, dst});
        max_node = std::max(max_node, std::max(src, dst));
    }

    int n = max_node + 1;

    CSRGraph g;
    g.num_vertices = n;
    g.offsets.resize(n + 1, 0);

    // count edges
    for (auto &e : edge_list) {
        g.offsets[e.first + 1]++;
    }

    // prefix sum
    for (int i = 1; i <= n; i++) {
        g.offsets[i] += g.offsets[i - 1];
    }

    g.edges.resize(edge_list.size());
    std::vector<int> temp = g.offsets;

    // fill edges
    for (auto &e : edge_list) {
        int src = e.first;
        int dst = e.second;

        int index = temp[src]++;
        g.edges[index] = dst;
    }

    return g;
}

// ===================== K-HOP BFS =====================

std::vector<int> KHop(const CSRGraph& g, int src, int K) {
    std::vector<int> result;

    if (src >= g.num_vertices) return result;

    std::queue<std::pair<int, int>> q;
    std::vector<bool> visited(g.num_vertices, false);

    q.push({src, 0});
    visited[src] = true;

    while (!q.empty()) {
        auto [node, depth] = q.front();
        q.pop();

        if (depth == K) continue;

        for (int i = g.offsets[node]; i < g.offsets[node + 1]; i++) {
            int nei = g.edges[i];

            if (!visited[nei]) {
                visited[nei] = true;
                q.push({nei, depth + 1});
                result.push_back(nei);
            }
        }
    }

    return result;
}

// ===================== CALLBACKS =====================

std::string CountCallback(const CSRGraph& g, int src, int K) {
    auto nodes = KHop(g, src, K);
    return std::to_string(nodes.size());
}

std::string MaxCallback(const CSRGraph& g, int src, int K) {
    auto nodes = KHop(g, src, K);

    if (nodes.empty()) return "-1";

    int mx = *std::max_element(nodes.begin(), nodes.end());
    return std::to_string(mx);
}

// ===================== LOAD QUERIES =====================

std::vector<QueryTask> LoadQueries(const char* filename) {
    std::ifstream file(filename);
    std::vector<QueryTask> tasks;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int src, K, type, expected;
        iss >> src >> K >> type >> expected;

        QueryTask t;
        t.src = src;
        t.K = K;

        if (type == 1)
            t.cb = CountCallback;
        else
            t.cb = MaxCallback;

        tasks.push_back(t);
    }

    return tasks;
}

// ===================== SEQUENTIAL =====================

void RunTasksSequential(const CSRGraph& g, std::vector<QueryTask>& tasks) {
    for (auto& t : tasks) {
        t.result = t.cb(g, t.src, t.K);
    }
}

// ===================== PARALLEL =====================

void Worker(const CSRGraph& g, std::vector<QueryTask>& tasks, int start, int end) {
    for (int i = start; i < end; i++) {
        tasks[i].result = tasks[i].cb(g, tasks[i].src, tasks[i].K);
    }
}

void RunTasksParallel(const CSRGraph& g, std::vector<QueryTask>& tasks, int num_threads) {
    std::vector<std::thread> threads;

    int n = tasks.size();
    int chunk = (n + num_threads - 1) / num_threads;

    for (int t = 0; t < num_threads; t++) {
        int start = t * chunk;
        int end = std::min(start + chunk, n);

        if (start >= n) break;

        threads.emplace_back(Worker, std::ref(g), std::ref(tasks), start, end);
    }

    for (auto& th : threads) {
        th.join();
    }
}

// ===================== MAIN =====================

int main() {
    CSRGraph g = LoadGraph("soc-Slashdot0902.txt");

    auto tasks = LoadQueries("queries20.txt");

    // Sequential
    auto seq_tasks = tasks;
    auto t1 = std::chrono::high_resolution_clock::now();
    RunTasksSequential(g, seq_tasks);
    auto t2 = std::chrono::high_resolution_clock::now();

    // Parallel
    auto par_tasks = tasks;
    auto t3 = std::chrono::high_resolution_clock::now();
    RunTasksParallel(g, par_tasks, 4);
    auto t4 = std::chrono::high_resolution_clock::now();

    auto seq_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto par_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "Sequential time: " << seq_time << " ms\n";
    std::cout << "Parallel time: " << par_time << " ms\n";

    // check correctness
    for (int i = 0; i < seq_tasks.size(); i++) {
        if (seq_tasks[i].result != par_tasks[i].result) {
            std::cout << "Mismatch at task " << i << "\n";
        }
    }

    // example: print neighbors of vertex 6 (for screenshot)
    int v = 6;
    std::cout << "Neighbors of vertex " << v << ": ";
    for (int i = g.offsets[v]; i < g.offsets[v + 1]; i++) {
        std::cout << g.edges[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
