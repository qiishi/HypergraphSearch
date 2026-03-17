
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

// 性能优化型：超边数据结构
struct Hyperedge {
    int id;
    vector<int> vertices;
    // 预先排序以便使用 std::set_intersection
    void sortVertices() {
        sort(vertices.begin(), vertices.end());
    }
};

// 全局变量
vector<Hyperedge> hyperEdges;
vector<vector<int>> adj; // 邻接表：adj[i] 存储与超边 i 交集 >= S 的超边 ID
unordered_set<int> bestCommunity;
int K_val, S_val;

// 高效交集计算：利用排序向量
int computeIntersection(const vector<int>& a, const vector<int>& b) {
    int count = 0;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            count++; i++; j++;
        }
        else if (a[i] < b[j]) {
            i++;
        }
        else {
            j++;
        }
    }
    return count;
}

// 优化后的数据加载：使用倒排索引构建邻接关系
void loadGraphOptimized(string filename, int S) {
    ifstream fin(filename);
    if (!fin) return;

    unordered_map<int, vector<int>> v2e;
    string line;
    int id = 0;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Hyperedge e;
        e.id = id;
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
    // 倒排索引加速构建：只检查共享顶点的超边
    for (int i = 0; i < hyperEdges.size(); ++i) {
        unordered_map<int, int> candidates;
        for (int vtx : hyperEdges[i].vertices) {
            for (int neighbor_id : v2e[vtx]) {
                if (neighbor_id > i) candidates[neighbor_id]++;
            }
        }
        for (auto const& [neighbor_id, count] : candidates) {
            if (count >= S) {
                adj[i].push_back(neighbor_id);
                adj[neighbor_id].push_back(i);
            }
        }
    }
    cout << "图加载完成，有效超边数: " << hyperEdges.size() << endl;
}

// 快速 (k,s)-Core 分解
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
        auto top = *q.begin();
        q.erase(q.begin());
        int u = top.second;
        current_k = max(current_k, top.first);
        coreness[u] = current_k;
        removed[u] = true;

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

// 优化的下界估计
int computeMinSize(unordered_set<int>& C, unordered_set<int>& P) {
    int lb = C.size();
    for (int u : C) {
        int internal_deg = 0;
        for (int v : adj[u]) {
            if (C.count(v)) internal_deg++;
        }
        if (internal_deg < K_val) {
            lb += (K_val - internal_deg);
        }
    }
    return lb;
}

// OBBAB 回溯版本：引用传递 + 状态恢复
void OBBAB(unordered_set<int>& C, unordered_set<int>& P) {
    // 1. 剪枝
    if (!bestCommunity.empty() && computeMinSize(C, P) >= bestCommunity.size()) return;

    // 2. 检查凝聚度
    bool is_valid = true;
    for (int u : C) {
        int internal_deg = 0;
        for (int v : adj[u]) if (C.count(v)) internal_deg++;
        if (internal_deg < K_val) { is_valid = false; break; }
    }

    if (is_valid && !C.empty()) {
        if (bestCommunity.empty() || C.size() < bestCommunity.size()) {
            bestCommunity = C;
        }
        return;
    }

    if (P.empty()) return;

    // 3. 启发式选择：选择与 C 连接最紧密的候选超边
    int best_e = -1;
    double max_score = -1.0;
    for (int e : P) {
        double score = 0;
        for (int neighbor : adj[e]) if (C.count(neighbor)) score += 1.0;
        if (score > max_score) { max_score = score; best_e = e; }
    }

    if (best_e == -1) best_e = *P.begin();

    // 4. 分支回溯
    P.erase(best_e);

    // 分支一：加入 C
    C.insert(best_e);
    OBBAB(C, P);
    C.erase(best_e); // 回溯

    // 分支二：不加入 C
    OBBAB(C, P);

    P.insert(best_e); // 恢复 P
}

int main() {
    string fileName = "D:\\ExamProject\\Exam1\\HypergraphSearch\\x64\\Debug\\dblp.txt"; //数据集路径
    K_val = 2; S_val = 2;
    int queryV = 15;

    auto start = chrono::steady_clock::now();

    // Step 1: 优化加载
    loadGraphOptimized(fileName, S_val);

    // Step 2: 核心分解剪枝搜索空间
    vector<int> coreness = getHCore(K_val);

    unordered_set<int> P_init;
    unordered_set<int> C_init;

    // 找出包含查询顶点且满足核心度的超边作为起点
    for (int i = 0; i < hyperEdges.size(); ++i) {
        if (coreness[i] >= K_val) {
            bool has_q = false;
            for (int v : hyperEdges[i].vertices) if (v == queryV) { has_q = true; break; }
            if (has_q) C_init.insert(i);
            else P_init.insert(i);
        }
    }

    if (C_init.empty()) {
        cout << "查询顶点不满足 (k,s)-core 条件" << endl;
        return 0;
    }

    // Step 3: 启动搜索
    OBBAB(C_init, P_init);

    auto end = chrono::steady_clock::now();
    cout << "搜索完成，最优社区大小: " << bestCommunity.size() << endl;
    cout << "总耗时: " << chrono::duration_cast<chrono::seconds>(end - start).count() << "s" << endl;

    return 0;
}