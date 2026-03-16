//// 调整头文件顺序，避免编码/依赖问题
//#include <iostream>
//#include <string>
//#include <vector>
//#include <fstream>
//#include <sstream>
//#include <set>
//#include <unordered_map>
//#include <unordered_set>
//#include <queue>
//#include <chrono>
//#include <map>
//#include <climits>
//#include <algorithm>
//#include <random>
//#include <thread>
//#include <mutex>
//#include <omp.h>
//#include <bitset>
//#include <direct.h>  // Windows下获取当前目录的头文件
//#include <iomanip>   // 用于格式化输出进度
//using namespace std;
//
//// ========== 新增：全局进度监控工具类 ==========
//class ProgressMonitor {
//private:
//    string stageName;       // 当前阶段名称（如"超图构建"）
//    long long total;        // 总任务数
//    long long completed;    // 已完成任务数
//    chrono::steady_clock::time_point start_time; // 阶段开始时间
//    mutex mtx;              // 线程安全锁（防止并行时进度计数错乱）
//    bool first_output;      // 是否首次输出
//
//public:
//    ProgressMonitor() : total(0), completed(0), first_output(true) {}
//
//    // 初始化进度监控
//    void init(const string& name, long long total_tasks) {
//        lock_guard<mutex> lock(mtx);
//        stageName = name;
//        total = total_tasks;
//        completed = 0;
//        start_time = chrono::steady_clock::now();
//        first_output = true;
//        cout << "\n========== 开始阶段：" << stageName << " (总任务数：" << total << ") ==========" << endl;
//    }
//
//    // 完成一个任务，更新进度
//    void update(long long num = 1) {
//        lock_guard<mutex> lock(mtx);
//        completed += num;
//        // 每完成1% 或 首次执行 时输出进度（避免刷屏）
//        if (first_output || (completed % max(1LL, total / 100)) == 0 || completed == total) {
//            first_output = false;
//            printProgress();
//        }
//    }
//
//    // 打印进度（含已耗时、剩余时间预估）
//    void printProgress() {
//        auto now = chrono::steady_clock::now();
//        double elapsed = chrono::duration<double>(now - start_time).count(); // 已耗时（秒）
//        double progress = (total == 0) ? 100.0 : (double)completed / total * 100.0; // 进度百分比
//        double remaining = (progress == 0) ? 0 : elapsed * (100 - progress) / progress; // 剩余时间（秒）
//
//        // 格式化输出：进度条 + 百分比 + 已耗时 + 剩余时间
//        cout << "\r[" << stageName << "] "
//            << "进度: [" << setw(50) << setfill('=') << string((int)(progress / 2), '=') << setfill(' ') << "] "
//            << fixed << setprecision(2) << progress << "% "
//            << "已耗时: " << formatTime(elapsed) << " "
//            << "剩余预估: " << formatTime(remaining) << " ";
//        cout.flush(); // 强制刷新输出缓冲区
//
//        // 阶段完成时输出总耗时
//        if (completed >= total) {
//            cout << "\n[" << stageName << "] 完成！总耗时：" << formatTime(elapsed) << endl;
//        }
//    }
//
//    // 格式化时间（秒 → 时:分:秒）
//    string formatTime(double seconds) {
//        int h = (int)seconds / 3600;
//        int m = ((int)seconds % 3600) / 60;
//        int s = (int)seconds % 60;
//        return to_string(h) + "h" + to_string(m) + "m" + to_string(s) + "s";
//    }
//
//    // 获取已完成数
//    long long getCompleted() { return completed; }
//};
//
//// 全局进度监控实例（不同阶段复用）
//ProgressMonitor g_progress;
//
//typedef pair<int, int> PII;
//unordered_set<int> best;
//
//// ========== 原有比较器保持不变 ==========
//struct myCmp
//{
//    bool operator()(const PII& a, const PII& b) const
//    {
//        if (a.second != b.second)
//            return a.second < b.second;
//        else
//            return a.first < b.first;
//    }
//};
//
//struct myCmp1
//{
//    bool operator()(const PII& a, const PII& b) const
//    {
//        if (a.second != b.second)
//            return a.second > b.second;
//        else
//            return a.first > b.first;
//    }
//};
//
//struct myCmp2
//{
//    bool operator()(const pair<int, double>& a, const pair<int, double>& b) const
//    {
//        if (a.second != b.second)
//            return a.second > b.second;
//        else
//            return a.first > b.first;
//    }
//};
//
//// ========== 优化compute函数（原有逻辑不变） ==========
//int compute(const vector<int>& a, const vector<int>& b)
//{
//    unordered_set<int> map;
//    for (auto x : a)
//    {
//        map.insert(x);
//    }
//
//    int count = 0;
//    for (auto x : b)
//    {
//        if (map.count(x) > 0)
//        {
//            count++;
//        }
//    }
//    return count;
//}
//
//// ========== 修改getGraph：添加进度监控 ==========
//void getGraph(const string& str, vector<vector<int>>& hyperEdge, vector<vector<int>>& incidentHyperedge, int x)
//{
//    string filename = str;
//    ifstream fin(filename, ios::in);
//    if (!fin)
//        throw runtime_error("Could not open file " + str);
//
//    int count = 0;
//    unordered_map<int, vector<int>> tmpnode;
//    string line;
//
//    // 第一步：先统计文件总行数（用于进度预估）
//    cout << "正在统计文件行数...";
//    long long total_lines = 0;
//    while (getline(fin, line)) {
//        if (!line.empty()) total_lines++;
//    }
//    fin.clear(); // 重置文件指针
//    fin.seekg(0); // 回到文件开头
//    cout << "完成！总行数：" << total_lines << endl;
//
//    // 第二步：读取文件，监控进度
//    g_progress.init("读取超图数据", total_lines);
//    while (getline(fin, line))
//    {
//        g_progress.update(); // 每读一行更新进度
//        if (line.empty())
//            continue;
//        istringstream ss(line);
//        int tmp;
//        vector<int> e;
//        while (ss >> tmp)
//        {
//            e.push_back(tmp);
//        }
//        if (e.size() < x)
//            continue;
//        hyperEdge.push_back(e);
//        count++;
//    }
//    fin.close();
//
//    // 第三步：构建邻接矩阵，监控进度
//    long long total_pairs = (long long)hyperEdge.size() * (hyperEdge.size() - 1) / 2; // 总节点对数量
//    g_progress.init("构建超图邻接矩阵", total_pairs);
//    incidentHyperedge.resize(hyperEdge.size());
//    for (int i = 0; i < hyperEdge.size(); i++)
//    {
//        for (int j = i + 1; j < hyperEdge.size(); j++)
//        {
//            g_progress.update(); // 每处理一个节点对更新进度
//            if (compute(hyperEdge[i], hyperEdge[j]) >= x)
//            {
//                incidentHyperedge[i].push_back(j);
//                incidentHyperedge[j].push_back(i);
//            }
//        }
//    }
//}
//
//// ========== 修改ksCoreDecomp：添加进度监控 ==========
//double ksCoreDecomp(const string& file, vector<vector<int>>& hyperEdge, vector<vector<int>>& incidentHyperedge, int x, vector<int>& cE)
//{
//    auto t1 = chrono::steady_clock::now();
//    cE.resize(hyperEdge.size(), 0);
//    vector<int> dE(hyperEdge.size(), 0);
//    vector<bool> visitedEdge(hyperEdge.size(), false);
//
//    for (int i = 0; i < hyperEdge.size(); i++)
//    {
//        dE[i] = static_cast<int>(incidentHyperedge.at(i).size());
//    }
//
//    set<PII, myCmp> Q;
//    for (int i = 0; i < dE.size(); i++)
//    {
//        Q.insert(make_pair(i, dE[i]));
//    }
//
//    // 初始化核分解进度监控
//    long long total_nodes = Q.size();
//    g_progress.init("KS核分解", total_nodes);
//
//    int k = 1;
//    while (!Q.empty())
//    {
//        g_progress.update(); // 每处理一个节点更新进度
//        PII p = *Q.begin();
//        Q.erase(Q.begin());
//        k = max(k, p.second);
//        cout << "\r" << p.first << " " << p.second; // 覆盖进度行临时输出节点信息
//        cout.flush();
//        cE[p.first] = k;
//        visitedEdge[p.first] = true;
//
//        for (auto edge : incidentHyperedge[p.first])
//        {
//            if (visitedEdge[edge])
//                continue;
//            if (Q.erase(make_pair(edge, dE[edge])))
//            {
//                dE[edge]--;
//                Q.insert(make_pair(edge, dE[edge]));
//            }
//        }
//    }
//    cout << endl; // 节点信息输出后换行，恢复进度输出
//
//    auto t2 = chrono::steady_clock::now();
//    double dr_ns = chrono::duration<double, nano>(t2 - t1).count();
//    return dr_ns;
//}
//
//// ========== 修改MinSize：添加基础进度（可选，因逻辑较细） ==========
//int MinSize(vector<vector<int>>& incidentHyperedge, int K, unordered_set<int> C, unordered_set<int> P)
//{
//    unordered_map<int, int> r;
//    set<PII, myCmp1> S;
//    int lb = static_cast<int>(C.size());
//
//    for (auto e : C)
//    {
//        r[e] = K - static_cast<int>(incidentHyperedge[e].size());
//        S.insert(make_pair(e, r[e]));
//    }
//
//    // MinSize内部进度（可选，避免刷屏）
//    static int minsize_count = 0;
//    if (++minsize_count % 1000 == 0) { // 每1000次调用输出一次
//        cout << "\r[MinSize] 已调用 " << minsize_count << " 次 ";
//        cout.flush();
//    }
//
//    while (!S.empty())
//    {
//        PII p = *S.begin();
//        S.erase(S.begin());
//        if (p.second < 0)
//        {
//            break;
//        }
//        lb += p.second;
//
//        set<PII, myCmp1> S1;
//        for (auto e : P)
//        {
//            unordered_set<int> tmp;
//            for (auto x : incidentHyperedge[e])
//            {
//                if (C.count(x))
//                {
//                    tmp.insert(x);
//                }
//            }
//            S1.insert(make_pair(e, static_cast<int>(tmp.size())));
//        }
//
//        int cnt = 0;
//        while (!S1.empty() && cnt < p.second)
//        {
//            PII p1 = *S1.begin();
//            S1.erase(S1.begin());
//            for (auto e : incidentHyperedge[p1.first])
//            {
//                if (C.count(e))
//                    continue;
//                if (S.erase(make_pair(e, r[e])))
//                {
//                    r[e]--;
//                    S.insert(make_pair(e, r[e]));
//                }
//            }
//            cnt++;
//        }
//    }
//    return lb;
//}
//
//// ========== 修改OBBAB：添加递归进度监控 ==========
//// 新增全局变量：记录OBBAB递归次数和总预估次数（简化版，基于P集合大小）
//long long g_obbab_count = 0;
//long long g_obbab_total = 0;
//mutex g_obbab_mtx;
//
//void OBBAB(vector<vector<int>>& hyperEdge, vector<vector<int>>& incidentHyperedge, int K, unordered_set<int> C, unordered_set<int> P)
//{
//    // 递归计数+进度更新
//    {
//        lock_guard<mutex> lock(g_obbab_mtx);
//        g_obbab_count++;
//        // 首次调用时预估总递归次数（简化版：基于P集合大小的2^n，实际可调整）
//        if (g_obbab_total == 0) {
//            g_obbab_total = 1LL << P.size(); // 2^|P|，仅作预估参考
//            g_progress.init("OBBAB递归搜索", g_obbab_total);
//        }
//        // 每100次递归更新一次进度（避免刷屏）
//        if (g_obbab_count % 100 == 0) {
//            g_progress.update(100);
//        }
//    }
//
//    if (MinSize(incidentHyperedge, K, C, P) > static_cast<int>(best.size()))
//        return;
//
//    bool flag = true;
//    for (auto& e : C)
//    {
//        if (static_cast<int>(incidentHyperedge[e].size()) < K)
//        {
//            flag = false;
//            break;
//        }
//    }
//
//    if (flag && static_cast<int>(C.size()) <= static_cast<int>(best.size()))
//    {
//        best = C;
//        return;
//    }
//
//    if (P.empty())
//    {
//        return;
//    }
//
//    set<pair<int, double>, myCmp2> S;
//    for (auto e : P)
//    {
//        double score = 0;
//        for (auto e1 : incidentHyperedge[e])
//        {
//            if (C.count(e1))
//            {
//                score += 1.0 / static_cast<double>(incidentHyperedge[e1].size());
//            }
//        }
//        S.insert(make_pair(e, score));
//    }
//
//    if (S.empty()) return;
//
//    int e = S.begin()->first;
//    P.erase(e);
//    unordered_set<int> C1 = C;
//    C1.insert(e);
//    OBBAB(hyperEdge, incidentHyperedge, K, C1, P);
//    OBBAB(hyperEdge, incidentHyperedge, K, C, P);
//}
//
//// ========== 修改main：添加总进度监控 ==========
//int main()
//{
//    try {
//        // 打印当前工作目录
//        char buffer[256];
//        if (_getcwd(buffer, sizeof(buffer)) != NULL) {
//            cout << "程序当前运行目录：" << buffer << endl;
//        }
//        else {
//            cerr << "警告：获取当前目录失败！" << endl;
//        }
//
//        // 记录程序总开始时间
//        auto total_start = chrono::steady_clock::now();
//
//        int queryVertex = 5;
//        int K = 1;
//        int S = 2;
//        string fileName = "D:\\ExamProject\\Exam1\\HypergraphSearch\\x64\\Debug\\TEST.txt";
//
//        vector<vector<int>> hyperEdge;
//        vector<vector<int>> incidentHyperedge;
//
//        // 阶段1：读取并构建超图
//        getGraph(fileName, hyperEdge, incidentHyperedge, S);
//
//        // 阶段2：KS核分解
//        vector<int> cE;
//        double t = ksCoreDecomp(fileName, hyperEdge, incidentHyperedge, S, cE);
//
//        // 阶段3：准备OBBAB输入
//        unordered_set<int> P;
//        unordered_set<int> C;
//        for (int i = 0; i < hyperEdge.size(); i++)
//        {
//            if (cE[i] >= K)
//            {
//                P.insert(i);
//            }
//        }
//        cout << "\nOBBAB输入P集合大小：" << P.size() << endl;
//        g_obbab_total = 0; // 重置OBBAB进度计数
//        g_obbab_count = 0;
//
//        // 阶段4：OBBAB最优解搜索
//        best = P;
//        OBBAB(hyperEdge, incidentHyperedge, K, C, P);
//        // 补全OBBAB最终进度
//        g_progress.update(g_obbab_total - g_progress.getCompleted());
//
//        // 输出最终结果
//        cout << "\n========== 程序执行完成 ==========" << endl;
//        cout << "最优解大小：" << best.size() << endl;
//        // 输出总耗时
//        auto total_end = chrono::steady_clock::now();
//        double total_elapsed = chrono::duration<double>(total_end - total_start).count();
//        cout << "程序总耗时：" << g_progress.formatTime(total_elapsed) << endl;
//
//    }
//    catch (const exception& e) {
//        cerr << "\n程序异常：" << e.what() << endl;
//        return 1;
//    }
//
//    return 0;
//}


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
#include <mutex>
#include <direct.h>
#include <iomanip>
using namespace std;

// ========== 全局配置（对齐论文参数） ==========
typedef pair<int, int> PII;
unordered_set<int> best;       // 最优社区集合
int K;                         // (k,s)-core的k值
int S;                         // (k,s)-core的s值
int queryVertex;               // 查询顶点q

// ========== 超图核心数据结构 ==========
vector<vector<int>> hyperEdge;                // 超边集合：hyperEdge[edge_id] = {v1, v2, ...}
vector<vector<int>> incidentHyperedge;        // 全局超边邻接矩阵（预处理加速）
unordered_map<int, vector<int>> vertex_to_edges; // 顶点→包含该顶点的超边ID

// ========== 进度监控工具（保留工程化特性） ==========
class ProgressMonitor {
private:
    string stageName;
    long long total;
    long long completed;
    chrono::steady_clock::time_point start_time;
    mutex mtx;
    bool first_output;

public:
    ProgressMonitor() : total(0), completed(0), first_output(true) {}

    void init(const string& name, long long total_tasks) {
        lock_guard<mutex> lock(mtx);
        stageName = name;
        total = total_tasks;
        completed = 0;
        start_time = chrono::steady_clock::now();
        first_output = true;
        cout << "\n========== 开始阶段：" << stageName << " (总任务数：" << total << ") ==========" << endl;
    }

    void update(long long num = 1) {
        lock_guard<mutex> lock(mtx);
        completed += num;
        if (first_output || (completed % max(1LL, total / 100)) == 0 || completed == total) {
            first_output = false;
            printProgress();
        }
    }

    void printProgress() {
        auto now = chrono::steady_clock::now();
        double elapsed = chrono::duration<double>(now - start_time).count();
        double progress = (total == 0) ? 100.0 : (double)completed / total * 100.0;
        double remaining = (progress == 0) ? 0 : elapsed * (100 - progress) / progress;

        cout << "\r[" << stageName << "] "
            << "进度: [" << setw(50) << setfill('=') << string((int)(progress / 2), '=') << setfill(' ') << "] "
            << fixed << setprecision(2) << progress << "% "
            << "已耗时: " << formatTime(elapsed) << " "
            << "剩余预估: " << formatTime(remaining) << " ";
        cout.flush();

        if (completed >= total) {
            cout << "\n[" << stageName << "] 完成！总耗时：" << formatTime(elapsed) << endl;
        }
    }

    string formatTime(double seconds) {
        int h = (int)seconds / 3600;
        int m = ((int)seconds % 3600) / 60;
        int s = (int)seconds % 60;
        return to_string(h) + "h" + to_string(m) + "m" + to_string(s) + "s";
    }

    long long getCompleted() { return completed; }
};

ProgressMonitor g_progress;
long long g_obbab_count = 0;
mutex g_obbab_mtx;

// ========== 核心工具函数（对齐参考代码） ==========
// 计算两个超边的公共顶点数
int compute(const vector<int>& a, const vector<int>& b) {
    unordered_set<int> mp;
    for (int x : a) mp.insert(x);
    int cnt = 0;
    for (int x : b) if (mp.count(x)) cnt++;
    return cnt;
}

// ========== (k,s)-core验证（核心修复：仅统计C内部邻居） ==========
bool checkCommunity(unordered_set<int>& C) {
    for (int e : C) {
        int cnt = 0;
        for (int e2 : C) {
            if (e == e2) continue;
            // 仅统计C内部、与e有≥S个公共顶点的超边
            if (compute(hyperEdge[e], hyperEdge[e2]) >= S) {
                cnt++;
            }
        }
        // 任意超边在C内的邻居数<K → 无效社区
        if (cnt < K) return false;
    }
    return true;
}

// ========== MinSize下界估计（核心修复：基于C内部邻居） ==========
int MinSize(unordered_set<int>& C) {
    int lb = C.size(); // 初始下界=当前集合大小
    for (int e : C) {
        int cnt = 0;
        // 统计C内部与e相邻的超边数
        for (int e2 : C) {
            if (e == e2) continue;
            if (compute(hyperEdge[e], hyperEdge[e2]) >= S) cnt++;
        }
        // 不足K则补充所需数量到下界
        if (cnt < K) lb += (K - cnt);
    }
    return lb;
}

// ========== OBBAB递归搜索（严格对齐参考代码逻辑） ==========
void OBBAB(unordered_set<int> C, unordered_set<int> P) {
    // 进度计数
    {
        lock_guard<mutex> lock(g_obbab_mtx);
        g_obbab_count++;
        if (g_obbab_count % 1000 == 0) {
            cout << "\r[OBBAB] 递归次数：" << g_obbab_count << " 最优解大小：" << (best.empty() ? 0 : best.size()) << " ";
            cout.flush();
        }
    }

    // 剪枝：下界≥当前最优解则返回
    if (!best.empty() && MinSize(C) >= (int)best.size()) {
        return;
    }

    // 验证社区有效性，更新最优解（找最小有效社区）
    if (checkCommunity(C)) {
        if (best.empty() || (int)C.size() < (int)best.size()) {
            best = C;
            cout << "\r[OBBAB] 找到更优解，大小：" << best.size() << " ";
            cout.flush();
        }
        return; // 找到更小的有效社区，无需继续递归
    }

    // P为空则终止
    if (P.empty()) return;

    // 选择P中第一个超边（参考代码逻辑，可扩展为score排序）
    int e = *P.begin();
    P.erase(e);

    // 分支1：将e加入C
    unordered_set<int> C1 = C;
    C1.insert(e);
    OBBAB(C1, P);

    // 分支2：不加入e
    OBBAB(C, P);
}

// ========== 读取超图数据（补充进度监控+统计信息） ==========
void getGraph(string file) {
    ifstream fin(file);
    if (!fin) throw runtime_error("无法打开文件：" + file);

    string line;
    int edge_id = 0;

    // 第一步：统计总行数（进度预估）
    cout << "正在统计文件行数...";
    long long total_lines = 0;
    while (getline(fin, line)) {
        if (!line.empty()) total_lines++;
    }
    fin.clear();
    fin.seekg(0);
    cout << "完成！总行数：" << total_lines << endl;

    // 第二步：读取超边+构建vertex_to_edges
    g_progress.init("读取超图数据", total_lines);
    while (getline(fin, line)) {
        g_progress.update();
        if (line.empty()) continue;

        stringstream ss(line);
        int v;
        vector<int> e;
        while (ss >> v) e.push_back(v);

        // 过滤顶点数<S的超边
        if (e.size() < S) continue;

        hyperEdge.push_back(e);
        // 构建顶点→超边映射
        for (int x : e) vertex_to_edges[x].push_back(edge_id);
        edge_id++;
    }
    fin.close();

    // 第三步：构建全局邻接矩阵（加速后续计算）
    long long total_pairs = (long long)hyperEdge.size() * (hyperEdge.size() - 1) / 2;
    g_progress.init("构建超边邻接矩阵", total_pairs);
    incidentHyperedge.resize(hyperEdge.size());
    long long total_adj_edges = 0; // 统计总邻接边数（数据诊断）
    for (int i = 0; i < hyperEdge.size(); i++) {
        for (int j = i + 1; j < hyperEdge.size(); j++) {
            g_progress.update();
            if (compute(hyperEdge[i], hyperEdge[j]) >= S) {
                incidentHyperedge[i].push_back(j);
                incidentHyperedge[j].push_back(i);
                total_adj_edges++;
            }
        }
    }

    // 输出关键统计信息（数据诊断）
    cout << "\n=== 数据诊断信息 ===" << endl;
    cout << "总超边数：" << hyperEdge.size() << endl;
    cout << "总邻接边数（全局）：" << total_adj_edges << endl;
    cout << "平均每个超边的邻接数：" << (double)total_adj_edges * 2 / hyperEdge.size() << endl;
}

// ========== 主函数（严格对齐论文流程） ==========
int main() {
    try {
        // 打印当前工作目录
        char buffer[256];
        if (_getcwd(buffer, sizeof(buffer)) != NULL) {
            cout << "程序当前运行目录：" << buffer << endl;
        }
        else {
            cerr << "警告：获取当前目录失败！" << endl;
        }

        // ========== 论文核心参数配置 ==========
        string file = "D:\\ExamProject\\Exam1\\HypergraphSearch\\x64\\Debug\\NDCC.txt";       // 数据集路径
        queryVertex = 5;                // 查询顶点q
        K = 3;                          // k值（建议先从2开始测试）
        S = 2;                          // s值（建议先从2开始测试）

        // ========== Step1：读取超图数据 ==========
        getGraph(file);

        // ========== Step2：验证查询顶点是否存在 ==========
        if (!vertex_to_edges.count(queryVertex)) {
            cout << "\n错误：查询顶点 " << queryVertex << " 不存在于超图中！" << endl;
            return 0;
        }
        vector<int> start_edges = vertex_to_edges[queryVertex];
        cout << "\n=== 查询顶点信息 ===" << endl;
        cout << "查询顶点：" << queryVertex << endl;
        cout << "包含该顶点的超边数：" << start_edges.size() << endl;

        // ========== Step3：初始化所有超边集合 ==========
        unordered_set<int> all_edges;
        for (int i = 0; i < hyperEdge.size(); i++) all_edges.insert(i);

        // ========== Step4：遍历所有包含q的超边，启动OBBAB ==========
        best.clear();
        g_obbab_count = 0;
        cout << "\n========== 开始OBBAB搜索 ==========" << endl;
        for (int e : start_edges) {
            unordered_set<int> C;
            unordered_set<int> P = all_edges;
            C.insert(e);    // 初始集合={e}（必须包含q的超边）
            P.erase(e);     // P = 所有超边 \ {e}
            OBBAB(C, P);
        }

        // ========== Step5：输出最终结果 ==========
        cout << "\n\n========== 最终结果 ==========" << endl;
        cout << "参数配置：k=" << K << ", s=" << S << ", 查询顶点=" << queryVertex << endl;
        cout << "最优社区大小：" << best.size() << endl;

        if (!best.empty()) {
            cout << "最优社区包含的超边及顶点：" << endl;
            for (int e : best) {
                cout << "超边" << e << ": ";
                for (int v : hyperEdge[e]) cout << v << " ";
                // 验证是否包含查询顶点
                bool has_q = (find(hyperEdge[e].begin(), hyperEdge[e].end(), queryVertex) != hyperEdge[e].end());
                cout << "(包含查询顶点：" << (has_q ? "是" : "否") << ")";
                // 验证该超边在社区内的邻居数
                int inner_neighbors = 0;
                for (int e2 : best) {
                    if (e == e2) continue;
                    if (compute(hyperEdge[e], hyperEdge[e2]) >= S) inner_neighbors++;
                }
                cout << " (内部邻居数：" << inner_neighbors << ")";
                cout << endl;
            }
        }
        else {
            cout << "提示：未找到满足条件的社区，可能原因：" << endl;
            cout << "1. k/s参数过大，超图稀疏；" << endl;
            cout << "2. 包含查询顶点的超边无足够内部邻居；" << endl;
            cout << "3. 建议降低k/s值（如k=1, s=1）测试。" << endl;
        }

    }
    catch (const exception& e) {
        cerr << "\n程序异常：" << e.what() << endl;
        return 1;
    }

    return 0;
}