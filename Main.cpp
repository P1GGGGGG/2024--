#include <iostream>
#include <algorithm>
#include <queue>
#include<unordered_map>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <map>
#include <set>

using namespace std;

class solution {
private:
    struct Node {
        int id;
        int width;
        int level;
        int left;  // 儿子
        int right; // 兄弟
        int pt[3];
        int pt2[3];
        int pid;
    };
    int level_num;
    unordered_map<int, int> idx_list;
    unordered_map<int, Node> g; // 存节点
    unordered_map<int, vector<int>> son_map; //存儿子
    unordered_map<int, vector<int>> fa_map; //存父亲
    unordered_map<int, pair<int, int>> mem;     // 存储内存分配情况
    map<pair<int, string>, vector<int>> dic;    // 存储域字典
    map<string, vector<int>> ddic;
    unordered_map<int, vector<int>> pri_map;    // 存储各node的拓扑序
    unordered_map<int, int> inDegree; // 记录每个节点的入度
    queue<Node> q;
    unordered_map<int, set<int>> sset;

public:
    solution(const string path, const string path1, const string path2) {
        init(path);
        get_root();
        mem_alloc();
        get_output1(path1);
        get_output2(path2);

    }

    struct cmp {
        bool operator()(const Node &a, const Node &b) {
            return a.width < b.width;
        }
    };

    int mmax(int a, int b) {
        return a > b ? a : b;
    }

    int mmin(int a, int b) {
        return a < b ? a : b;
    }

// 检查容器起点
    int check_start(int i) {
        if (i >= 240) {
            return i - i % 4;
        } else if (i >= 64) {
            return i - i % 2;
        } else return i;
    }

    int get_start(int width, int pt[]) {
        switch (width) {
            case 1:
                return pt[0];
            case 2:
                return pt[1];
            default:
                return pt[2];
        }
    }

    std::set<int> setIntersection(const std::set<int> &set1, const std::set<int> &set2) {
        std::set<int> intersection;
        std::set_intersection(set1.begin(), set1.end(),
                              set2.begin(), set2.end(),
                              std::inserter(intersection, intersection.begin()));
        return intersection;
    }

    int dfs(Node top) {
        int from = mem[top.id].first, ssize = mem[top.id].second;
        int now_num = 0, total_num = 0;
        for (auto i: son_map[top.id]) {
            if (g[i].level == top.level + 1) {
                total_num++;
                if (fa_map[i].size() == 1 && mem[i].first == from + ssize &&
                    check_start(mem[top.id].first) == check_start(mem[i].first)) {
                    now_num++;
                }
            }
        }
        sset[top.id].insert(top.width);
        if (now_num != total_num || son_map[top.id].empty()) {
            return top.width;
        } else {
            for (auto i: son_map[top.id]) {
                if (g[i].level == top.level + 1) {
                    dfs(g[i]);
                }
            }
            set<int> common;
            for (auto i: son_map[top.id]) {
                if (common.empty())common = sset[i];
                else common = setIntersection(common, sset[i]);
            }
            if (common.empty()) {
                return top.width;
            } else {
                for (auto i: common) {
                    sset[top.id].insert(i + top.width);
                }
                return *common.rbegin() + top.width; //todo
            }
        }
    }

    void ddfs(Node top1, int now_len, int extern_len,  const string s, int is_visit[]) {
        int flag = 0, step = 0;
        now_len += top1.width;
        is_visit[top1.id] = 1;
        int max_step = -1;

        for (auto i: fa_map[top1.id]) {
            max_step = mmax(max_step, idx_list[i]);
        }
        for (auto &it: dic) {
            if (it.first.second == s && step >= max_step) {
                flag = 1;
                it.second.push_back(top1.id);
                break;
            }
            step++;
        }
//        cout << top1.id << ' ' << step << ' ' << max_step << endl;
        if (flag == 0)dic[{step, s}].push_back(top1.id);
        idx_list[top1.id] = step;
        if (now_len == extern_len)return;
        for (auto i: son_map[top1.id]) {
            if (g[i].level == top1.level + 1) {
                ddfs(g[i], now_len, extern_len, s, is_visit);
            }
        }
    }

    void init(const string path) {
        ifstream inFile(path, ios::in);
        string lineStr;
        int step = 0;
        while (getline(inFile, lineStr)) {
            stringstream ss(lineStr);
            string str;
            vector<string> lineArray;
            step = 0;
            int id, width;
            while (getline(ss, str, ',')) {
                if (step == 0) {
                    id = stoi(str);
                } else if (step == 1) {
                    width = stoi(str) / 8;
                } else {
                    int to = stoi(str);
                    son_map[id].push_back(to);
                    fa_map[to].push_back(id);
                    inDegree[to]++;
                }
                step++;
            }
            g[id] = {id, width, 0, -1, -1, {0, 64, 240}, {0, 64, 240}, -1};
        }
    }

    void get_root() {
        int n = g.size();
        for (int i = 0; i < n; ++i) {
            if (inDegree[i] == 0) {
                q.push(g[i]);
                // 虚拟根节点
                son_map[-1].push_back(i);
                fa_map[i].push_back(-1);
                pri_map[0].push_back(i);
            }
        }
        g[-1] = {-1, 0, -1, -1, -1, {0, 64, 240}};
    }

    void mem_alloc() {
        int n = g.size() - 1;
        int last_level = -1;
        priority_queue<Node, vector<Node>, cmp> pq;
        pair<int, int> last_container = {-1, 0};
        // 更新每个节点的level并更新pri_map
        while (!q.empty()) {
            Node node = q.front();
            q.pop();
            int father_id = node.id;
            for (auto i = 0; i < son_map[father_id].size(); ++i) {
                int son_id = son_map[father_id][i];
                g[son_id].level = mmax(g[son_id].level, g[father_id].level + 1);
                inDegree[son_id]--;
                if (inDegree[son_id] == 0) {
                    q.push(g[son_id]);
                    pri_map[g[son_id].level].push_back(son_id);
                }
            }
        }
        pq.push(g[-1]);
        bool is_continue = true;
        bool is_alloc[n];
        for (int i = 0; i < n; i++) {
            is_alloc[i] = false;
        }

        // todo
        while (is_continue) {
            // 加入下一级
            if (pq.empty()) {
                last_level += 1;
                for (auto i: pri_map[last_level])pq.push(g[i]);
                if (pq.empty())is_continue = false;
            }
            while (!pq.empty()) {
                //  todo 插入逻辑还没有写
                Node top = pq.top();
                pq.pop();
                if (top.id == -1)continue;
                if (is_alloc[top.id])continue;
                int max_pt = -1, max_pt_id = -1;
                int max_pt2 = -1, max_pt2_id = -1;

                // 找到合法的父节点
                for (auto i: fa_map[top.id]) {
                    if (max_pt_id == -1) {
                        max_pt_id = i;
                        max_pt = g[i].pt[2];
                        max_pt2_id = i;
                        max_pt2 = g[i].pt2[2];
                    } else {
                        if (g[i].pt[2] == max_pt) {
                            max_pt_id = g[max_pt_id].level > g[i].level ? max_pt_id : i;
                        } else {
                            if (g[i].pt[2] > max_pt) {
                                max_pt = g[i].pt[2];
                                max_pt_id = i;
                            }
                        }
                        if (g[i].pt2[2] == max_pt2) {
                            max_pt2_id = g[max_pt2_id].level > g[i].level ? max_pt2_id : i;
                        } else {
                            if (g[i].pt2[2] > max_pt2) {
                                max_pt2 = g[i].pt2[2];
                                max_pt2_id = i;
                            }
                        }
                    }
                }

                int start = check_start(max_pt);
                int s_size = start >= 240 ? 4 : start >= 64 ? 2 : 1;
                int fa_id = -1;
                int mode = 0;
                if (start + s_size >= max_pt + top.width) {
                    //插入
                    fa_id = max_pt_id;

                    mode = 0;
                    // 加入当前节点的子节点
                } else {
                    //新开,即当前节点想要的不是给他留的，即等级差不止1或者非独生
                    mode = 1;
                    fa_id = max_pt2_id;
//                top.pt2[2] = max_pt2 + top.width;
//                top.pt[2] = top.pt2[2];
                }
                top.pid = fa_id;
                // 插入树结构
                if (g[fa_id].left == -1) {
                    //自己是第一个子节点 todo 有可能不是给你留的
                    top.right = g[fa_id].left;
                    g[fa_id].left = top.id;
                } else {
                    int ffa_id = g[fa_id].left;
                    if (top.level != g[fa_id].level + 1) {
                        while (g[ffa_id].right != -1)ffa_id = g[ffa_id].right;
                    } else {
                        // 找到正确的插入点
                        while (g[ffa_id].right != -1 && g[ffa_id].width > top.width) ffa_id = g[ffa_id].right;
                    }
                    top.right = g[ffa_id].right;
                    g[ffa_id].right = top.id;
                }

                // 是否需要新开容器
                int tmp_loc = -1;
                if (mode == 0) {
                    start = check_start(max_pt);
                    tmp_loc = max_pt;
                } else {
                    start = check_start(max_pt2);
                    tmp_loc = max_pt2;
                }
                s_size = start >= 240 ? 4 : start >= 64 ? 2 : 1;
                if (start + s_size >= tmp_loc + top.width) {
                    top.pt[2] = tmp_loc + top.width;
                    top.pt2[2] = mode == 0 ? mmax(max_pt2, top.pt[2]) : max_pt2 + top.width;
                    if (top.pt[2] == start + s_size)top.pt[2] = top.pt2[2];
                    mem[top.id] = {tmp_loc, top.width};
                } else {
                    // 此时mode一定等于1
                    top.pt[2] = max_pt2;
                    top.pt2[2] = start + s_size + top.width;
                    mem[top.id] = {start + s_size, top.width};
                }
                g[top.id] = top;
            }
        }
        level_num = last_level;
    }

    void get_output1(const string path1) {
        int n = g.size() - 1;
        std::ofstream outFile(path1, ios::app);
        string s;
        for (int i = 0; i < n; ++i) {
            s = "";
            s += to_string(i) + ",";
            int begin = mem[i].first, len = mem[i].second;
            for (int j = 0; j < len; ++j) {
                s += to_string(begin);
                begin++;
                if (j < len - 1)s += ",";
                else s += "\n";
            }
            outFile << s;
        }
        outFile.close();
    }

    void get_output2(const string path2) {
        int n = g.size() - 1;
        string s;
        s = "";
        //更新域字典
        ofstream File2(path2, ios::app);
        int is_visit[n];
        for (int i = 0; i < n; ++i) {
            is_visit[i] = 0;
        }

        for (int i = 0; i < level_num; ++i) {
            for (auto j = 0; j < pri_map[i].size(); j++) {
                q.push(g[pri_map[i][j]]);
            }
        }
        while (!q.empty()) {
            Node top = q.front();
            q.pop();
            if (is_visit[top.id] == 1) continue;
            int kkey[4] = {-1, -1, -1, -1};
            s = "";
            int from = mem[top.id].first;
            int extern_len = dfs(top);

            for (int i = 0; i < extern_len; ++i, from++) {
                kkey[from % 4] = from;
            }
            for (int i: kkey) {
                if (i == -1)s += "-,";
                else s += to_string(i) + ",";
            }
            ddfs(top, 0, extern_len, s, is_visit);
        }
//        for (auto &it: dic) {
//            s = "";
//            s += it.first.second;
//            for (int i = 0; i < it.second.size(); i++) {
//                ddic[s].push_back(it.second[i]);
//            }
//        }
//        for (auto &it: ddic) {
//            s = "";
//            s += it.first;
//            for (int i = 0; i < it.second.size(); ++i) {
//                s += to_string(it.second[i]);
//                if (i < it.second.size() - 1)s += ",";
//                else s += "\n";
//            }
//            File2 << s;
//        }
        for (auto &it: dic) {
            s = "";
            s += it.first.second;
            for (int i = 0; i < it.second.size(); ++i) {
                s += to_string(it.second[i]);
                if (i < it.second.size() - 1)s += ",";
                else s += "\n";
            }
            File2 << s;
        }
        File2.close();
    }
};

void fileProcess(const string path) {
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        std::cerr << "Cannot open directory: " << path << std::endl;
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        std::string fullPath = path + "/" + name;
        struct stat stat_buf;
        if (stat(fullPath.c_str(), &stat_buf) != 0) {
            std::cerr << "Cannot stat file: " << fullPath << std::endl;
            return;
        }
        // Determine whether it is a folder
        if (S_ISDIR(stat_buf.st_mode)) {
            fileProcess(fullPath);
        } else if (S_ISREG(stat_buf.st_mode)) {
            // todo 打开文件
            string path1 = path + "/output1.csv", path2 = path + "/output2.csv";
            std::ofstream file1(path1);
            file1.close();
            std::ofstream file2(path2);
            file2.close();
            solution(fullPath, path1, path2);
        }
    }
    closedir(dir);
}

int main() {
    string path = "./data";
    fileProcess(path);
}