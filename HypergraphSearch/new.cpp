// 调整头文件顺序，避免编码/依赖问题
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <map>
#include <climits>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>
#include <omp.h>
#include <bitset>
#include <direct.h>  // Windows下获取当前目录的头文件
#include <iomanip>   // 用于格式化输出进度
using namespace std;

// ========== 新增：全局进度监控工具类 ==========
class ProgressMonitor {
private:
    string stageName;       // 当前阶段名称（如"超图构建"）
    long long total;        // 总任务数
    long long completed;    // 已完成任务数
    chrono::steady_clock::time_point start_time; // 阶段开始时间
    mutex mtx;              // 线程安全锁（防止并行时进度计数错乱）
    bool first_output;      // 是否首次输出

public:
    ProgressMonitor() : total(0), completed(0), first_output(true) {}

    // 初始化进度监控
    void init(const string& name, long long total_tasks) {
        lock_guard<mutex> lock(mtx);
        stageName = name;
        total = total_tasks;
        completed = 0;
        start_time = chrono::steady_clock::now();
        first_output = true;
        cout << "\n========== 开始阶段：" << stageName << " (总任务数：" << total << ") ==========" << endl;
    }

    // 完成一个任务，更新进度
    void update(long long num = 1) {
        lock_guard<mutex> lock(mtx);
        completed += num;
        // 每完成1% 或 首次执行 时输出进度（避免刷屏）
        if (first_output || (completed % max(1LL, total / 100)) == 0 || completed == total) {
            first_output = false;
            printProgress();
        }
    }

    // 打印进度（含已耗时、剩余时间预估）
    void printProgress() {
        auto now = chrono::steady_clock::now();
        double elapsed = chrono::duration<double>(now - start_time).count(); // 已耗时（秒）
        double progress = (total == 0) ? 100.0 : (double)completed / total * 100.0; // 进度百分比
        double remaining = (progress == 0) ? 0 : elapsed * (100 - progress) / progress; // 剩余时间（秒）

        // 格式化输出：进度条 + 百分比 + 已耗时 + 剩余时间
        cout << "\r[" << stageName << "] "
            << "进度: [" << setw(50) << setfill('=') << string((int)(progress / 2), '=') << setfill(' ') << "] "
            << fixed << setprecision(2) << progress << "% "
            << "已耗时: " << formatTime(elapsed) << " "
            << "剩余预估: " << formatTime(remaining) << " ";
        cout.flush(); // 强制刷新输出缓冲区

        // 阶段完成时输出总耗时
        if (completed >= total) {
            cout << "\n[" << stageName << "] 完成！总耗时：" << formatTime(elapsed) << endl;
        }
    }

    // 格式化时间（秒 → 时:分:秒）
    string formatTime(double seconds) {
        int h = (int)seconds / 3600;
        int m = ((int)seconds % 3600) / 60;
        int s = (int)seconds % 60;
        return to_string(h) + "h" + to_string(m) + "m" + to_string(s) + "s";
    }

    // 获取已完成数
    long long getCompleted() { return completed; }
};

// 全局进度监控实例（不同阶段复用）
ProgressMonitor g_progress;

typedef pair<int, int> PII;
unordered_set<int> best;

// ========== 原有比较器保持不变 ==========
struct myCmp
{
    bool operator()(const PII& a, const PII& b) const
    {
        if (a.second != b.second)
            return a.second < b.second;
        else
            return a.first < b.first;
    }
};

struct myCmp1
{
    bool operator()(const PII& a, const PII& b) const
    {
        if (a.second != b.second)
            return a.second > b.second;
        else
            return a.first > b.first;
    }
};

struct myCmp2
{
    bool operator()(const pair<int, double>& a, const pair<int, double>& b) const
    {
        if (a.second != b.second)
            return a.second > b.second;
        else
            return a.first > b.first;
    }
};

// ========== 优化compute函数（原有逻辑不变） ==========
int compute(const vector<int>& a, const vector<int>& b)
{
    unordered_set<int> map;
    for (auto x : a)
    {
        map.insert(x);
    }

    int count = 0;
    for (auto x : b)
    {
        if (map.count(x) > 0)
        {
            count++;
        }
    }
    return count;
}

// ========== 修改getGraph：添加进度监控 ==========
void getGraph(const string& str, vector<vector<int>>& hyperEdge, vector<vector<int>>& incidentHyperedge, int x)
{
    string filename = str;
    ifstream fin(filename, ios::in);
    if (!fin)
        throw runtime_error("Could not open file " + str);

    int count = 0;
    unordered_map<int, vector<int>> tmpnode;
    string line;

    // 第一步：先统计文件总行数（用于进度预估）
    cout << "正在统计文件行数...";
    long long total_lines = 0;
    while (getline(fin, line)) {
        if (!line.empty()) total_lines++;
    }
    fin.clear(); // 重置文件指针
    fin.seekg(0); // 回到文件开头
    cout << "完成！总行数：" << total_lines << endl;

    // 第二步：读取文件，监控进度
    g_progress.init("读取超图数据", total_lines);
    while (getline(fin, line))
    {
        g_progress.update(); // 每读一行更新进度
        if (line.empty())
            continue;
        istringstream ss(line);
        int tmp;
        vector<int> e;
        while (ss >> tmp)
        {
            e.push_back(tmp);
        }
        if (e.size() < x)
            continue;
        hyperEdge.push_back(e);
        count++;
    }
    fin.close();

    // 第三步：构建邻接矩阵，监控进度
    long long total_pairs = (long long)hyperEdge.size() * (hyperEdge.size() - 1) / 2; // 总节点对数量
    g_progress.init("构建超图邻接矩阵", total_pairs);
    incidentHyperedge.resize(hyperEdge.size());
    for (int i = 0; i < hyperEdge.size(); i++)
    {
        for (int j = i + 1; j < hyperEdge.size(); j++)
        {
            g_progress.update(); // 每处理一个节点对更新进度
            if (compute(hyperEdge[i], hyperEdge[j]) >= x)
            {
                incidentHyperedge[i].push_back(j);
                incidentHyperedge[j].push_back(i);
            }
        }
    }
}

// ========== 修改ksCoreDecomp：添加进度监控 ==========
double ksCoreDecomp(const string& file, vector<vector<int>>& hyperEdge, vector<vector<int>>& incidentHyperedge, int x, vector<int>& cE)
{
    auto t1 = chrono::steady_clock::now();
    cE.resize(hyperEdge.size(), 0);
    vector<int> dE(hyperEdge.size(), 0);
    vector<bool> visitedEdge(hyperEdge.size(), false);

    for (int i = 0; i < hyperEdge.size(); i++)
    {
        dE[i] = static_cast<int>(incidentHyperedge.at(i).size());
    }

    set<PII, myCmp> Q;
    for (int i = 0; i < dE.size(); i++)
    {
        Q.insert(make_pair(i, dE[i]));
    }

    // 初始化核分解进度监控
    long long total_nodes = Q.size();
    g_progress.init("KS核分解", total_nodes);

    int k = 1;
    while (!Q.empty())
    {
        g_progress.update(); // 每处理一个节点更新进度
        PII p = *Q.begin();
        Q.erase(Q.begin());
        k = max(k, p.second);
        cout << "\r" << p.first << " " << p.second; // 覆盖进度行临时输出节点信息
        cout.flush();
        cE[p.first] = k;
        visitedEdge[p.first] = true;

        for (auto edge : incidentHyperedge[p.first])
        {
            if (visitedEdge[edge])
                continue;
            if (Q.erase(make_pair(edge, dE[edge])))
            {
                dE[edge]--;
                Q.insert(make_pair(edge, dE[edge]));
            }
        }
    }
    cout << endl; // 节点信息输出后换行，恢复进度输出

    auto t2 = chrono::steady_clock::now();
    double dr_ns = chrono::duration<double, nano>(t2 - t1).count();
    return dr_ns;
}

// ===== 新增：计算超边相似度 (Jaccard) =====
double hyperedgeSimilarity(const vector<int>& a, const vector<int>& b)
{
    unordered_set<int> s(a.begin(), a.end());
    int inter = 0;

    for (int x : b)
        if (s.count(x)) inter++;

    int uni = a.size() + b.size() - inter;

    if (uni == 0) return 0;
    return (double)inter / uni;
}


// ===== 找到包含 query node 的超边 =====
vector<int> findIncidentEdges(int q, vector<vector<int>>& hyperEdge)
{
    vector<int> res;

    for (int i = 0;i < hyperEdge.size();i++)
    {
        for (int v : hyperEdge[i])
        {
            if (v == q)
            {
                res.push_back(i);
                break;
            }
        }
    }

    return res;
}


// ===== 相似超边扩展 =====
unordered_set<int> similarEdgeExpansion(
    vector<int>& seedEdges,
    vector<vector<int>>& hyperEdge,
    double threshold)
{
    unordered_set<int> result(seedEdges.begin(), seedEdges.end());

    queue<int> Q;
    for (int e : seedEdges) Q.push(e);

    long long total = hyperEdge.size() * hyperEdge.size();
    g_progress.init("相似超边扩展", total);

    while (!Q.empty())
    {
        int cur = Q.front();
        Q.pop();

        for (int i = 0; i < hyperEdge.size(); i++)
        {
            g_progress.update();

            if (result.count(i)) continue;

            double sim = hyperedgeSimilarity(hyperEdge[cur], hyperEdge[i]);

            if (sim >= threshold)
            {
                result.insert(i);
                Q.push(i);
            }
        }
    }

    return result;
}


// ===== 构建节点集合 =====
unordered_set<int> buildNodeSet(
    unordered_set<int>& edges,
    vector<vector<int>>& hyperEdge)
{
    unordered_set<int> nodes;

    for (int e : edges)
        for (int v : hyperEdge[e])
            nodes.insert(v);

    return nodes;
}


// ===== 结构约束过滤 =====
unordered_set<int> structuralFilter(
    unordered_set<int>& nodes,
    vector<vector<int>>& hyperEdge,
    int K)
{
    unordered_map<int, int> deg;

    for (int e = 0;e < hyperEdge.size();e++)
    {
        for (int v : hyperEdge[e])
        {
            if (nodes.count(v))
                deg[v]++;
        }
    }

    unordered_set<int> res;

    for (auto& p : deg)
        if (p.second >= K)
            res.insert(p.first);

    return res;
}


// ===== membership计算 =====
double membershipScore(
    int node,
    unordered_set<int>& community,
    vector<vector<int>>& hyperEdge)
{
    int total = 0;
    int inside = 0;

    for (auto& e : hyperEdge)
    {
        bool contain = false;

        for (int v : e)
            if (v == node) contain = true;

        if (!contain) continue;

        total++;

        bool all = true;
        for (int v : e)
            if (!community.count(v))
                all = false;

        if (all) inside++;
    }

    if (total == 0) return 0;

    return (double)inside / total;
}


// ===== membership pruning =====
unordered_set<int> membershipPruning(
    unordered_set<int>& nodes,
    vector<vector<int>>& hyperEdge,
    double mu)
{
    unordered_set<int> res;

    g_progress.init("membership剪枝", nodes.size());

    for (int v : nodes)
    {
        g_progress.update();

        double score = membershipScore(v, nodes, hyperEdge);

        if (score >= mu)
            res.insert(v);
    }

    return res;
}


// ===== 新社区搜索算法 =====
unordered_set<int> communitySearch(
    int queryNode,
    vector<vector<int>>& hyperEdge)
{
    double simThreshold = 0.3;
    int K = 2;
    double mu = 0.3;

    cout << "\n===== 新框架社区搜索开始 =====" << endl;

    // Step1
    auto incident = findIncidentEdges(queryNode, hyperEdge);

    cout << "query相关超边数量 " << incident.size() << endl;

    // Step2
    auto simEdges = similarEdgeExpansion(
        incident,
        hyperEdge,
        simThreshold);

    cout << "相似超边数量 " << simEdges.size() << endl;

    // Step3
    auto nodes = buildNodeSet(simEdges, hyperEdge);

    cout << "候选节点数 " << nodes.size() << endl;

    // Step4
    auto structural = structuralFilter(nodes, hyperEdge, K);

    cout << "结构过滤后 " << structural.size() << endl;

    // Step5
    auto finalCommunity = membershipPruning(
        structural,
        hyperEdge,
        mu);

    cout << "membership剪枝后 " << finalCommunity.size() << endl;

    return finalCommunity;
}

// ========== 修改main：添加总进度监控 ==========
int main()
{
    try {

        char buffer[256];
        if (_getcwd(buffer, sizeof(buffer)) != NULL)
            cout << "程序当前运行目录：" << buffer << endl;

        auto total_start = chrono::steady_clock::now();

        int queryVertex = 5;
        int S = 3;

        string fileName =
            "D:\\ExamProject\\Exam1\\HypergraphSearch\\x64\\Debug\\NDCC.txt";

        vector<vector<int>> hyperEdge;
        vector<vector<int>> incidentHyperedge;

        // ===== 阶段1 读取超图 =====
        getGraph(fileName, hyperEdge, incidentHyperedge, S);

        // ===== 阶段2 KS Core (可选) =====
        vector<int> cE;
        ksCoreDecomp(fileName, hyperEdge, incidentHyperedge, S, cE);

        // ===== 阶段3 新社区搜索 =====
        auto community =
            communitySearch(queryVertex, hyperEdge);

        // ===== 输出结果 =====
        cout << "\n========== 社区搜索完成 ==========" << endl; 

        cout << "社区大小: " << community.size() << endl;

        cout << "社区节点: ";
        for (auto v : community)
            cout << v << " ";

        cout << endl;

        auto total_end = chrono::steady_clock::now();
        double total_elapsed =
            chrono::duration<double>(total_end - total_start).count();

        cout << "程序总耗时: "
            << g_progress.formatTime(total_elapsed)
            << endl;

    }
    catch (const exception& e)
    {
        cerr << "\n程序异常：" << e.what() << endl;
        return 1;
    }

    return 0;
}