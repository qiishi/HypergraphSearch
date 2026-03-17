#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <set>
#include <chrono>

using namespace std;

// 超边数据结构
struct Hyperedge {
    int id;
    vector<int> vertices;
    void sortVertices() { sort(vertices.begin(), vertices.end()); }
};

// 全局变量
vector<Hyperedge> hyperEdges;
vector<vector<int>> adj;
unordered_set<int> bestCommunity;
int K_val, S_val;
double SIM_threshold = 0.0; // 新增：相似度阈值

// 高效交集计算
int computeIntersection(const vector<int>& a, const vector<int>& b) {
    int count = 0;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) { count++; i++; j++; }
        else if (a[i] < b[j]) i++;
        else j++;
    }
    return count;
}

// 优化后的数据加载：集成相似度过滤
void loadGraphOptimized(string filename, int S, double tau) {
    ifstream fin(filename);
    if (!fin) return;

    unordered_map<int, vector<int>> v2e;
    string line;
    int id = 0;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Hyperedge e; e.id = id;
        int v;
        while (ss >> v) e.vertices.push_back(v);

        if (e.vertices.size() >= S) {
            e.sortVertices();
            for (int vtx : e.vertices) v2e[vtx].push_back(id);
            hyperEdges.push_back(e);
            id++;
        }
    }

    adj.resize(hyperEdges.size());
    for (int i = 0; i < (int)hyperEdges.size(); ++i) {
        unordered_map<int, int> candidates;
        for (int vtx : hyperEdges[i].vertices) {
            for (int nid : v2e[vtx]) {
                if (nid > i) candidates[nid]++;
            }
        }
        for (auto const& [nid, intersect_cnt] : candidates) {
            // 计算 Jaccard 相似度
            double sim = (double)intersect_cnt / (hyperEdges[i].vertices.size() + hyperEdges[nid].vertices.size() - intersect_cnt);

            // 同时满足 s-交集 和 相似度阈值
            if (intersect_cnt >= S && sim >= tau) {
                adj[i].push_back(nid);
                adj[nid].push_back(i);
            }
        }
    }
    cout << "图加载完成，相似度阈值: " << tau << "，有效超边数: " << hyperEdges.size() << endl;
}

// 快速核心分解
vector<int> getHCore(int k_threshold) {
    int m = hyperEdges.size();
    vector<int> degree(m);
    set<pair<int, int>> q;
    for (int i = 0; i < m; ++i) {
        degree[i] = adj[i].size();
        q.insert({ degree[i], i });
    }
    vector<int> coreness(m, 0);
    vector<bool> removed(m, false);
    int current_k = 0;
    while (!q.empty()) {
        auto top = *q.begin(); q.erase(q.begin());
        int u = top.second; current_k = max(current_k, top.first);
        coreness[u] = current_k; removed[u] = true;
        for (int v : adj[u]) {
            if (!removed[v]) {
                q.erase({ degree[v], v });
                degree[v]--;
                q.insert({ degree[v], v });
            }
        }
    }
    return coreness;
}

// 下界估计
int computeMinSize(unordered_set<int>& C) {
    int lb = C.size();
    for (int u : C) {
        int in_deg = 0;
        for (int v : adj[u]) if (C.count(v)) in_deg++;
        if (in_deg < K_val) lb += (K_val - in_deg);
    }
    return lb;
}

// OBBAB 相似度加权版本
void OBBAB(unordered_set<int>& C, unordered_set<int>& P) {
    if (!bestCommunity.empty() && computeMinSize(C) >= (int)bestCommunity.size()) return;

    bool is_valid = true;
    for (int u : C) {
        int in_deg = 0;
        for (int v : adj[u]) if (C.count(v)) in_deg++;
        if (in_deg < K_val) { is_valid = false; break; }
    }

    if (is_valid && !C.empty()) {
        if (bestCommunity.empty() || C.size() < bestCommunity.size()) bestCommunity = C;
        return;
    }

    if (P.empty()) return;

    // 启发式选择：结合连接度和相似度分数
    int best_e = -1;
    double max_score = -1.0;
    for (int e : P) {
        double score = 0;
        for (int neighbor : adj[e]) {
            if (C.count(neighbor)) {
                // 计算当前边与社区已选边的相似度贡献
                int inter = computeIntersection(hyperEdges[e].vertices, hyperEdges[neighbor].vertices);
                double sim = (double)inter / (hyperEdges[e].vertices.size() + hyperEdges[neighbor].vertices.size() - inter);
                score += sim; // 相似度累加
            }
        }
        if (score > max_score) { max_score = score; best_e = e; }
    }

    if (best_e == -1) best_e = *P.begin();

    P.erase(best_e);
    C.insert(best_e);
    OBBAB(C, P);
    C.erase(best_e);
    OBBAB(C, P);
    P.insert(best_e);
}

int main() {
    string fileName = "D:\\ExamProject\\Exam1\\HypergraphSearch\\x64\\Debug\\dblp.txt";
    K_val = 3;
    S_val = 2;
    SIM_threshold = 0.3; // 设定相似度阈值（Jaccard）
    int queryV = 1963;

    auto start = chrono::steady_clock::now();

    // 1. 加载并进行相似度初步过滤
    loadGraphOptimized(fileName, S_val, SIM_threshold);

    // 2. 剪枝
    vector<int> coreness = getHCore(K_val);
    unordered_set<int> P_init, C_init;

    for (int i = 0; i < (int)hyperEdges.size(); ++i) {
        if (coreness[i] >= K_val) {
            bool has_q = false;
            for (int v : hyperEdges[i].vertices) if (v == queryV) { has_q = true; break; }
            if (has_q) C_init.insert(i);
            else P_init.insert(i);
        }
    }

    if (C_init.empty()) {
        cout << "查询点不满足相似度及核心度约束条件" << endl;
        return 0;
    }

    // 3. 搜索
    OBBAB(C_init, P_init);

    auto end = chrono::steady_clock::now();
    cout << "\n========== 结果 ==========" << endl;
    cout << "相似度阈值 tau: " << SIM_threshold << endl;
    cout << "最优社区超边数: " << bestCommunity.size() << endl;
    cout << "耗时: " << chrono::duration_cast<chrono::seconds>(end - start).count() << "s" << endl;

    return 0;
}