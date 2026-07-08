/*
 * main.cpp - 校园地图查询导引系统 主程序实现
 *
 * 数据结构课程设计任务
 * 采用邻接矩阵存储图，Dijkstra 算法求最短路径
 */

// windows.h：提供 SetConsoleOutputCP 等控制台 API
// locale.h：提供 setlocale，设置 C 运行时区域以支持中文
// iomanip：提供 setprecision / fixed 等输出格式控制符
#include "Graph.h"
#include <windows.h>
#include <locale.h>
#include <iostream>
#include <iomanip>
using namespace std;

/* ========== 基础操作 ========== */

// 初始化图：清空顶点、边计数，将所有边距设为"无穷大"
// 自身到自身距离为0，因为一个地点到自己的距离是0
void InitGraph(Graph* G) {
    G->vexnum = 0;
    G->arcnum = 0;
    for (int i = 0; i < MAX_VERTEX_NUM; i++) {
        G->vexs[i][0] = '\0';                     // 空字符串表示该位置尚无顶点
        for (int j = 0; j < MAX_VERTEX_NUM; j++) {
            // 对角线 = 0，其余 = 无穷大（不可达）
            G->arcs[i][j].distance = (i == j) ? 0.0f : (float)INFINITY_VAL;
            G->arcs[i][j].direction = 0;
            G->arcs[i][j].info[0] = '\0';
        }
    }
}

// 按名称查找顶点下标，线性扫描 O(n)
// 未找到返回 -1，调用方需检查返回值
int FindVertex(Graph G, const char* name) {
    for (int i = 0; i < G.vexnum; i++) {
        if (strcmp(G.vexs[i], name) == 0)        // 字符串匹配成功 → 找到
            return i;
    }
    return -1;                                     // 遍历完未找到 → 不存在
}

/* ========== 文件读写 ========== */

// 从文本文件加载地图数据，文件格式：
//   顶点数 N
//   顶点名1 ... 顶点名N
//   边数 M
//   起点 终点 距离 路名 方向角  （共 M 行）
int LoadMap(Graph* G, const char* filename) {
    FILE* fp = fopen(filename, "r+");//打开可读可写文件
    if (fp == NULL) {                              // 打开失败 → 文件不存在或无权限
        cout << "无法打开文件: " << filename << endl;
        return FALSE;
    }

    InitGraph(G);                                  // 先清空图，避免残留旧数据

    // --- 读取顶点 ---
    int n;
    if (fscanf(fp, "%d", &n) != 1) {               // 第一行是顶点数
        cout << "文件格式错误: 无法读取顶点数" << endl;
        fclose(fp);
        return FALSE;
    }
    fgetc(fp);                                     // 消耗顶点数后的换行符，否则下一行 fgets 会读空

    for (int i = 0; i < n; i++) {
        if (fgets(G->vexs[i], MAX_NAME_LENGTH, fp) == NULL) { // 读取到的顶点少于声明数 → 文件损坏
            cout << "文件格式错误: 顶点数量不足" << endl;
            fclose(fp);
            return FALSE;
        }
        // 去除行尾换行符（Windows 为 \r\n，Unix 为 \n）
        size_t len = strlen(G->vexs[i]);
        if (len > 0 && G->vexs[i][len - 1] == '\n')
            G->vexs[i][len - 1] = '\0';
        if (len > 0 && G->vexs[i][len - 1] == '\r')
            G->vexs[i][len - 1] = '\0';
    }
    G->vexnum = n;

    // --- 读取边 ---
    int m;
    if (fscanf(fp, "%d", &m) != 1) {               // 顶点后必须是边数
        cout << "文件格式错误: 无法读取边数" << endl;
        fclose(fp);
        return FALSE;
    }

    for (int k = 0; k < m; k++) {
        char from[MAX_NAME_LENGTH], to[MAX_NAME_LENGTH];
        float dist;
        char road[MAX_NAME_LENGTH];
        int dir;

        if (fscanf(fp, "%s %s %f %s %d",
            from, to, &dist, road, &dir) != 5) {   // 每行必须5个字段，缺少则文件不完整
            cout << "文件格式错误: 边数据不完整 (第 " << (k + 1) << " 条边)" << endl;
            fclose(fp);
            return FALSE;
        }

        int i = FindVertex(*G, from);
        int j = FindVertex(*G, to);
        if (i == -1 || j == -1) {                  // 边引用了不存在的顶点 → 跳过并警告
            cout << "警告: 边包含未知顶点，已跳过 (" << from << " -> " << to << ")" << endl;
            continue;                              // 跳过这条边，继续读下一条
        }

        // 边有效 → 写入邻接矩阵
        G->arcs[i][j].distance = dist;
        G->arcs[i][j].direction = dir;
        strcpy(G->arcs[i][j].info, road);

#if GRAPH_TYPE == 1                                // 无向图：同时写入反向边，方向角 +180° 取反
        G->arcs[j][i].distance = dist;
        G->arcs[j][i].direction = (dir + 180) % 360;
        strcpy(G->arcs[j][i].info, road);
#endif
    }
    G->arcnum = m;

    fclose(fp);
    cout << "地图加载成功！顶点数: " << G->vexnum << ", 边数: " << G->arcnum << endl;
    return TRUE;
}

// 保存地图到文件，格式与 LoadMap 一致
// 无向图时只存上三角（i<j），避免重复存储
int SaveMap(Graph G, const char* filename) {
    FILE* fp = fopen(filename, "w+");
    if (fp == NULL) {                              // 创建/覆盖文件失败
        cout << "无法创建文件: " << filename << endl;
        return FALSE;
    }

    fprintf(fp, "%d\n", G.vexnum);                 // 第一行：顶点数
    for (int i = 0; i < G.vexnum; i++) {
        fprintf(fp, "%s\n", G.vexs[i]);            // 每个顶点一行
    }

    // 统计有效边数（只扫描上三角，无向图下三角是对称的）
    int edgeCount = 0;
    for (int i = 0; i < G.vexnum; i++)
        for (int j = i + 1; j < G.vexnum; j++)
            if (G.arcs[i][j].distance < INFINITY_VAL) // 距离小于无穷大 → 存在边
                edgeCount++;
    fprintf(fp, "%d\n", edgeCount);

    // 写入每条边的信息
    for (int i = 0; i < G.vexnum; i++) {
        for (int j = i + 1; j < G.vexnum; j++) {
            if (G.arcs[i][j].distance < INFINITY_VAL) { // 边存在 → 写出
                fprintf(fp, "%s %s %.0f %s %d\n",
                    G.vexs[i], G.vexs[j],
                    G.arcs[i][j].distance,
                    G.arcs[i][j].info,
                    G.arcs[i][j].direction);
            }
        }
    }

    fclose(fp);
    cout << "地图保存成功！文件: " << filename << endl;
    return TRUE;
}

/* ========== 核心算法 ========== */

// Dijkstra 单源最短路径算法
// 参数：G=图, v0=起点下标, P[]=前驱数组（存每个顶点的前驱下标）, D[]=最短距离数组
// 算法思想：贪心策略，每次从未确定最短路径的顶点中选距离最小的加入已确定集合，
//          然后用它松弛它的邻居
void ShortestPath_DIJ(Graph G, int v0, int P[], float D[]) {
    int final[MAX_VERTEX_NUM];                     // final[v]=TRUE 表示 v 的最短路径已确定

    // --- 初始化 ---
    for (int v = 0; v < G.vexnum; v++) {
        final[v] = FALSE;                          // 所有顶点初始为"未确定"
        D[v] = G.arcs[v0][v].distance;             // 初始距离 = 直接边距离（可能无穷大）
        P[v] = (D[v] < INFINITY_VAL) ? v0 : -1;    // 可达则前驱为 v0，否则标记为 -1
    }
    D[v0] = 0;                                     // 起点到自身距离为 0
    final[v0] = TRUE;                              // 起点已确定
    P[v0] = -1;                                    // 起点无前驱

    // --- 主循环：每次确定一个顶点 ---
    for (int i = 1; i < G.vexnum; i++) {
        float min = INFINITY_VAL;
        int v = -1;

        // 在未确定顶点中，选距离最小的（贪心选择）
        for (int w = 0; w < G.vexnum; w++) {
            if (!final[w] && D[w] < min) {         // 未确定 且 距离更小 → 更新候选
                v = w;
                min = D[w];
            }
        }
        if (v == -1) break;                        // 无可达的未确定顶点 → 剩余顶点均不可达，提前结束

        final[v] = TRUE;                           // 将 v 加入已确定集合

        // 松弛操作：用刚确定的顶点 v 更新其邻居的距离
        for (int w = 0; w < G.vexnum; w++) {
            if (!final[w] && G.arcs[v][w].distance < INFINITY_VAL) { // w 未确定 且 v→w 有边
                float newDist = min + G.arcs[v][w].distance;         // 经过 v 到 w 的新距离
                if (newDist < D[w]) {                                // 新距离更短 → 更新
                    D[w] = newDist;
                    P[w] = v;                                        // w 的前驱改为 v
                }
            }
        }
    }
}

// 根据前后两段路径的方向角差计算转向提示
// angleFrom：来路方向，angleTo：去路方向
// 返回："前方直行" / "向左转" / "向右转"
const char* GetDirection(int angleFrom, int angleTo) {
    int diff = angleTo - angleFrom;
    if (diff < 0) diff += 360;                     // 归一化到 [0, 359] 范围，确保差值为正
    // 角度差 <45° 或 >315° → 大致同一方向 → 直行
    if (diff < 45 || diff > 315) return "前方直行";
    // 角度差在 [45, 180) → 目的地在左侧 → 左转
    if (diff >= 45 && diff < 180) return "向左转";
    // 其余情况 (180 ~ 315) → 目的地在右侧 → 右转
    return "向右转";
}

/* ========== 查询功能 ========== */

// 列出图中所有地点，带序号方便用户选择
void DisplayAllVertex(Graph G) {
    if (G.vexnum == 0) {                           // 地图为空 → 提示并返回
        cout << "地图为空，请先加载地图数据。" << endl;
        return;
    }
    cout << "\n========== 校园地点列表 ==========" << endl;
    for (int i = 0; i < G.vexnum; i++) {
        cout << "  [" << (i + 1) << "] " << G.vexs[i] << endl;
    }
    cout << "共 " << G.vexnum << " 个地点" << endl;
}

// 查询从指定地点出发到图中所有其他地点的最短距离
// 内部调用 Dijkstra，然后逐项打印
void DistanceQuery(Graph G) {
    if (G.vexnum == 0) {                           // 地图为空 → 无法查询
        cout << "地图为空，请先加载地图数据。" << endl;
        return;
    }

    char name[MAX_NAME_LENGTH];
    cout << "请输入当前地点名称: ";
    cin >> name;

    int v0 = FindVertex(G, name);
    if (v0 == -1) {                                // 地点不存在 → 报错返回
        cout << "未找到地点: " << name << endl;
        return;
    }

    int P[MAX_VERTEX_NUM];
    float D[MAX_VERTEX_NUM];
    ShortestPath_DIJ(G, v0, P, D);                 // 核心：运行 Dijkstra 求得所有最短距离

    cout << "\n========== 从 [" << name << "] 出发的最短距离 ==========" << endl;
    for (int i = 0; i < G.vexnum; i++) {
        if (i == v0) continue;                     // 跳过自己
        if (D[i] < INFINITY_VAL)                   // 可达 → 显示距离
            cout << "  " << G.vexs[i] << ": " << fixed << setprecision(0) << D[i] << " 米" << endl;
        else                                       // 不可达 → 明确提示
            cout << "  " << G.vexs[i] << ": 不可达" << endl;
    }
}

// 路径导引：从起点到终点的最短路径，并给出分步导航指引
// 核心：Dijkstra 求最短路径 → P[] 回溯构建路径 → 逐段输出道路名和转向
void PathGuide(Graph G) {
    if (G.vexnum == 0) {                           // 地图为空 → 返回
        cout << "地图为空，请先加载地图数据。" << endl;
        return;
    }

    char from[MAX_NAME_LENGTH], to[MAX_NAME_LENGTH];
    cout << "请输入起点: ";
    cin >> from;
    cout << "请输入终点: ";
    cin >> to;

    int v0 = FindVertex(G, from);
    int v1 = FindVertex(G, to);

    if (v0 == -1) { cout << "未找到地点: " << from << endl; return; }  // 起点不存在 → 退出
    if (v1 == -1) { cout << "未找到地点: " << to << endl; return; }    // 终点不存在 → 退出
    if (v0 == v1) { cout << "起点和终点相同，距离为 0 米。" << endl; return; } // 同一点 → 无需导航

    int P[MAX_VERTEX_NUM];
    float D[MAX_VERTEX_NUM];
    ShortestPath_DIJ(G, v0, P, D);                 // 求 v0 到全图的最短路径

    if (D[v1] >= INFINITY_VAL) {                   // 起点到终点不可达 → 提示并退出
        cout << "从 " << from << " 到 " << to << " 不可达。" << endl;
        return;
    }

    // 从 P[] 回溯构建路径：从终点 v1 沿前驱走回起点 v0
    // path[] 存储的是逆向路径（终点→起点），所以输出时需要倒序
    int path[MAX_VERTEX_NUM];
    int pathLen = 0;
    int cur = v1;
    while (cur != -1) {                            // -1 表示已到达起点（起点无前驱）
        path[pathLen++] = cur;
        cur = P[cur];                              // 沿前驱回溯
    }
    // 此时 path = [v1, ..., v0]，pathLen-1 是起点下标

    cout << "\n========== 路径导引 ==========" << endl;
    cout << "从 " << from << " 到 " << to << endl;
    cout << "总距离: " << fixed << setprecision(0) << D[v1] << " 米" << endl;

    cout << "\n途经路线:" << endl;
    cout << "  1. 从 " << G.vexs[path[pathLen - 1]] << " 出发"; // 第一步：从起点出发

    // 倒序遍历 path 数组，从起点走到终点
    for (int k = pathLen - 1; k > 0; k--) {
        int a = path[k];                           // 当前段起点
        int b = path[k - 1];                       // 当前段终点
        int curDir = G.arcs[a][b].direction;       // 当前段的方向角

        cout << "\n     " << G.arcs[a][b].info     // 输出路名
            << " (" << fixed << setprecision(0) << G.arcs[a][b].distance << "米)";

        if (k < pathLen - 1) {                     // 不是第一步 → 需要提示转向
            int actualPrevDir = G.arcs[path[k + 1]][a].direction; // 上一段来路的方向
            cout << " -> " << GetDirection(actualPrevDir, curDir);
        }

        cout << "\n  " << (pathLen - k + 1) << ". 到达 " << G.vexs[b]; // 到达下一站
    }
    cout << endl;
}

// 最近地点查询：找出离当前位置最近的 1 个或 N 个地点
void NearestSpot(Graph G) {
    if (G.vexnum == 0) {                           // 地图为空 → 返回
        cout << "地图为空，请先加载地图数据。" << endl;
        return;
    }

    char name[MAX_NAME_LENGTH];
    cout << "请输入当前地点名称: ";
    cin >> name;

    int v0 = FindVertex(G, name);
    if (v0 == -1) {                                // 地点不存在 → 报错退出
        cout << "未找到地点: " << name << endl;
        return;
    }

    int P[MAX_VERTEX_NUM];
    float D[MAX_VERTEX_NUM];
    ShortestPath_DIJ(G, v0, P, D);                 // 先求到所有地点的最短距离

    // 找最近的一个地点：遍历所有顶点，跳过自身，记录最小距离
    int nearest = -1;
    float minDist = INFINITY_VAL;
    for (int i = 0; i < G.vexnum; i++) {
        if (i != v0 && D[i] < minDist) {           // 不是自己 且 距离更小 → 更新
            minDist = D[i];
            nearest = i;
        }
    }

    if (nearest == -1) {                           // 没有任何可达的其他地点
        cout << "从 " << name << " 无法到达任何其他地点。" << endl;
        return;
    }

    cout << "\n========== 最近地点查询 ==========" << endl;
    cout << "距 " << name << " 最近的地点是: " << G.vexs[nearest]
        << " (" << fixed << setprecision(0) << minDist << " 米)" << endl;

    cout << "\n是否查看最近 N 个地点？(y/n): ";
    char choice;
    cin >> choice;
    if (choice == 'y' || choice == 'Y') {          // 用户想看前N个 → 排序后展示
        int n;
        cout << "请输入 N（个数）: ";
        cin >> n;

        // 用结构体绑定下标和距离，方便排序
        typedef struct { int index; float dist; } DistItem;
        DistItem items[MAX_VERTEX_NUM];
        int count = 0;
        for (int i = 0; i < G.vexnum; i++) {
            if (i != v0 && D[i] < INFINITY_VAL) {  // 只收集可达的地点
                items[count].index = i;
                items[count].dist = D[i];
                count++;
            }
        }

        // 冒泡排序：按距离升序排列，数据量小（≤30）冒泡够用且代码简单
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - 1 - i; j++) {
                if (items[j].dist > items[j + 1].dist) { // 前面的距离更大 → 交换
                    DistItem tmp = items[j];
                    items[j] = items[j + 1];
                    items[j + 1] = tmp;
                }
            }
        }

        int showCount = (n < count) ? n : count;   // 不能超过实际数量
        cout << "\n最近的 " << showCount << " 个地点:" << endl;
        for (int i = 0; i < showCount; i++) {
            cout << "  " << (i + 1) << ". " << G.vexs[items[i].index]
                << " - " << fixed << setprecision(0) << items[i].dist << " 米" << endl;
        }
    }
    // 用户选择"n" → 不展示前N个，直接返回
}
/* ========== 地图维护功能 ========== */

// 向图中添加一个新地点
// 检查上限（MAX_VERTEX_NUM）和重名
void AddVertex(Graph* G) {
    if (G->vexnum >= MAX_VERTEX_NUM) {             // 数量已达上限 → 拒绝添加
        cout << "地点数量已达上限 (" << MAX_VERTEX_NUM << ")！" << endl;
        return;
    }

    char name[MAX_NAME_LENGTH];
    cout << "请输入新地点名称: ";
    cin >> name;

    if (FindVertex(*G, name) != -1) {              // 名称已存在 → 拒绝重复
        cout << "地点 " << name << " 已存在！" << endl;
        return;
    }

    strcpy(G->vexs[G->vexnum], name);              // 写入顶点名称数组
    // 初始化新顶点在邻接矩阵中的行和列：所有距离设为无穷大，方向设为0
    for (int i = 0; i <= G->vexnum; i++) {
        G->arcs[G->vexnum][i].distance = INFINITY_VAL; // 新地点到其他地点：不可达
        G->arcs[G->vexnum][i].direction = 0;
        G->arcs[G->vexnum][i].info[0] = '\0';
        G->arcs[i][G->vexnum].distance = INFINITY_VAL; // 其他地点到新地点：不可达
        G->arcs[i][G->vexnum].direction = 0;
        G->arcs[i][G->vexnum].info[0] = '\0';
    }
    G->arcs[G->vexnum][G->vexnum].distance = 0;    // 自己到自己距离为0
    G->vexnum++;                                   // 顶点计数器+1

    cout << "地点 " << name << " 添加成功！" << endl;
}

// 删除一个地点，同时删除与之相关的所有道路
// 采用"覆盖法"：用最后一个顶点填充被删除顶点的位置，避免移动大量数据
void DeleteVertex(Graph* G) {
    if (G->vexnum == 0) {                          // 地图为空 → 无法删除
        cout << "地图为空，无可删除的地点。" << endl;
        return;
    }

    char name[MAX_NAME_LENGTH];
    cout << "请输入要删除的地点名称: ";
    cin >> name;

    int idx = FindVertex(*G, name);
    if (idx == -1) {                               // 地点不存在 → 报错返回
        cout << "未找到地点: " << name << endl;
        return;
    }

    // 统计涉及该地点的边数，用于更新 arcnum
    int removedEdges = 0;
    for (int i = 0; i < G->vexnum; i++) {
        if (i != idx) {
            if (G->arcs[idx][i].distance < INFINITY_VAL) removedEdges++;
            if (G->arcs[i][idx].distance < INFINITY_VAL) removedEdges++;
        }
    }

    // "覆盖法"删除：如果删除的不是最后一个顶点，用最后一个顶点覆盖它
    if (idx != G->vexnum - 1) {                    // 不是最后一个 → 需要搬移
        strcpy(G->vexs[idx], G->vexs[G->vexnum - 1]); // 名称覆盖
        // 邻接矩阵：将最后一行的边复制到被删行，最后一列的边复制到被删列
        for (int i = 0; i < G->vexnum; i++) {
            G->arcs[idx][i] = G->arcs[G->vexnum - 1][i]; // 行复制
            G->arcs[i][idx] = G->arcs[i][G->vexnum - 1]; // 列复制
        }
        G->arcs[idx][idx].distance = 0;            // 修复被覆盖行的对角线
    }
    // 如果 idx == G->vexnum-1 → 直接缩减 vexnum 即可，无需移动

    G->vexnum--;                                   // 顶点数减1
    G->arcnum -= removedEdges;                     // 减去删除顶点涉及的所有边

    cout << "地点 " << name << " 删除成功！" << endl;
}

// 修改地点名称：先查重名再修改
void ModifyVertex(Graph* G) {
    if (G->vexnum == 0) {                          // 地图为空 → 返回
        cout << "地图为空。" << endl;
        return;
    }

    char oldName[MAX_NAME_LENGTH], newName[MAX_NAME_LENGTH];
    cout << "请输入要修改的地点名称: ";
    cin >> oldName;

    int idx = FindVertex(*G, oldName);
    if (idx == -1) {                               // 地点不存在 → 报错返回
        cout << "未找到地点: " << oldName << endl;
        return;
    }

    cout << "请输入新名称: ";
    cin >> newName;

    if (FindVertex(*G, newName) != -1) {           // 新名称已被占用 → 拒绝
        cout << "地点 " << newName << " 已存在！" << endl;
        return;
    }

    strcpy(G->vexs[idx], newName);                 // 修改名称
    cout << "地点名称修改成功: " << oldName << " -> " << newName << endl;
}

// 添加一条道路（边）
// 检查起点终点是否存在、道路是否已存在
void AddRoad(Graph* G) {
    if (G->vexnum == 0) {                          // 没有地点 → 无法修路
        cout << "地图为空，请先添加地点。" << endl;
        return;
    }

    char from[MAX_NAME_LENGTH], to[MAX_NAME_LENGTH];
    cout << "请输入起点: ";
    cin >> from;
    cout << "请输入终点: ";
    cin >> to;

    int i = FindVertex(*G, from);
    int j = FindVertex(*G, to);

    if (i == -1) { cout << "未找到地点: " << from << endl; return; }  // 起点不存在
    if (j == -1) { cout << "未找到地点: " << to << endl; return; }    // 终点不存在

    if (G->arcs[i][j].distance < INFINITY_VAL) {   // 道路已存在 → 拒绝重复添加
        cout << "从 " << from << " 到 " << to << " 的道路已存在！" << endl;
        return;
    }

    char roadName[MAX_NAME_LENGTH];
    float dist;
    int dir;

    cout << "请输入道路名称: ";
    cin >> roadName;
    cout << "请输入距离（米）: ";
    cin >> dist;
    cout << "请输入方向角（0-359度）: ";
    cin >> dir;

    // 写入邻接矩阵
    G->arcs[i][j].distance = dist;
    G->arcs[i][j].direction = dir;
    strcpy(G->arcs[i][j].info, roadName);

#if GRAPH_TYPE == 1                                // 无向图：同步写入反向边
    G->arcs[j][i].distance = dist;
    G->arcs[j][i].direction = (dir + 180) % 360;   // 反向方向角 = 正向 + 180°
    strcpy(G->arcs[j][i].info, roadName);
#endif

    G->arcnum++;                                   // 边计数器+1
    cout << "道路添加成功！" << endl;
}

// 删除一条道路
void DeleteRoad(Graph* G) {
    if (G->vexnum == 0) {                          // 没有图 → 返回
        cout << "地图为空。" << endl;
        return;
    }

    char from[MAX_NAME_LENGTH], to[MAX_NAME_LENGTH];
    cout << "请输入起点: ";
    cin >> from;
    cout << "请输入终点: ";
    cin >> to;

    int i = FindVertex(*G, from);
    int j = FindVertex(*G, to);

    if (i == -1) { cout << "未找到地点: " << from << endl; return; }  // 起点不存在
    if (j == -1) { cout << "未找到地点: " << to << endl; return; }    // 终点不存在

    if (G->arcs[i][j].distance >= INFINITY_VAL) {  // 道路不存在 → 无法删除
        cout << "从 " << from << " 到 " << to << " 的道路不存在！" << endl;
        return;
    }

    // 将距离重置为无穷大（逻辑删除），同时清空方向和名称
    G->arcs[i][j].distance = INFINITY_VAL;
    G->arcs[i][j].direction = 0;
    G->arcs[i][j].info[0] = '\0';

#if GRAPH_TYPE == 1                                // 无向图：同样删除反向边
    G->arcs[j][i].distance = INFINITY_VAL;
    G->arcs[j][i].direction = 0;
    G->arcs[j][i].info[0] = '\0';
#endif

    G->arcnum--;                                   // 边计数器-1
    cout << "道路 (" << from << " -> " << to << ") 删除成功！" << endl;
}

// 修改道路属性：输入"-"或"-1"表示保留原值不修改
void ModifyRoad(Graph* G) {
    if (G->vexnum == 0) {                          // 地图为空 → 返回
        cout << "地图为空。" << endl;
        return;
    }

    char from[MAX_NAME_LENGTH], to[MAX_NAME_LENGTH];
    cout << "请输入起点: ";
    cin >> from;
    cout << "请输入终点: ";
    cin >> to;

    int i = FindVertex(*G, from);
    int j = FindVertex(*G, to);

    if (i == -1) { cout << "未找到地点: " << from << endl; return; }  // 起点不存在
    if (j == -1) { cout << "未找到地点: " << to << endl; return; }    // 终点不存在

    if (G->arcs[i][j].distance >= INFINITY_VAL) {  // 道路不存在 → 无法修改
        cout << "从 " << from << " 到 " << to << " 的道路不存在！" << endl;
        return;
    }

    cout << "当前道路信息: " << G->arcs[i][j].info
        << ", 距离: " << fixed << setprecision(0) << G->arcs[i][j].distance
        << "米, 方向角: " << G->arcs[i][j].direction << "度" << endl;

    char roadName[MAX_NAME_LENGTH];
    float dist;
    int dir;

    // 修改道路名称：输入"-"表示保持不变，否则更新
    cout << "请输入新道路名称（输入 - 保持原值）: ";
    cin >> roadName;
    if (strcmp(roadName, "-") != 0) {              // 不是"-" → 用户输入了新名称
        strcpy(G->arcs[i][j].info, roadName);
#if GRAPH_TYPE == 1
        strcpy(G->arcs[j][i].info, roadName);      // 无向图同步反向边
#endif
    }
    // 如果是"-" → 跳过，保持原值不变

    // 修改距离：输入 -1 表示保持不变
    cout << "请输入新距离（输入 -1 保持原值）: ";
    cin >> dist;
    if (dist >= 0) {                               // >=0 → 用户输入了有效距离
        G->arcs[i][j].distance = dist;
#if GRAPH_TYPE == 1
        G->arcs[j][i].distance = dist;             // 无向图同步
#endif
    }
    // 如果是 -1 → 保持原值不变

    // 修改方向角：输入 -1 表示保持不变
    cout << "请输入新方向角（输入 -1 保持原值）: ";
    cin >> dir;
    if (dir >= 0 && dir < 360) {                   // 有效角度范围 [0,359]
        G->arcs[i][j].direction = dir;
#if GRAPH_TYPE == 1
        G->arcs[j][i].direction = (dir + 180) % 360; // 反向角 = 正向 + 180°
#endif
    }
    // 如果是 -1 或超出范围 → 保持原值不变

    cout << "道路 (" << from << " -> " << to << ") 修改成功！" << endl;
}

/* ========== 数据维护子菜单 ========== */

// 地图维护的二级菜单，do-while 循环直到用户选择"0 返回"
void DataMaintenanceMenu(Graph* G) {
    int choice;
    do {
        cout << "\n===== 地图数据维护 =====" << endl;
        cout << "  1. 增加地点" << endl;
        cout << "  2. 删除地点" << endl;
        cout << "  3. 修改地点名称" << endl;
        cout << "  4. 增加道路" << endl;
        cout << "  5. 删除道路" << endl;
        cout << "  6. 修改道路属性" << endl;
        cout << "  0. 返回主菜单" << endl;
        cout << "请选择: ";
        cin >> choice;

        switch (choice) {
        case 1: AddVertex(G); break;               // 增加地点
        case 2: DeleteVertex(G); break;            // 删除地点
        case 3: ModifyVertex(G); break;            // 修改名称
        case 4: AddRoad(G); break;                 // 增加道路
        case 5: DeleteRoad(G); break;              // 删除道路
        case 6: ModifyRoad(G); break;              // 修改道路属性
        case 0: break;                             // 返回主菜单（由 while 条件退出）
        default: cout << "无效选择，请重新输入。" << endl; // 非法输入 → 提示重试
        }
    } while (choice != 0);                         // 选择0则退出循环
}

/* ========== 菜单系统 ========== */

// 打印主菜单，用 ASCII 画框提升可读性
void PrintMainMenu() {
    cout << "\n"
        << "+========================================+" << endl
        << "|      校园地图查询导引系统              |" << endl
        << "+========================================+" << endl
        << "|  1. 调用地图（从文件加载）             |" << endl
        << "|  2. 显示全部地点                       |" << endl
        << "|  3. 距离查询                           |" << endl
        << "|  4. 路径导引                           |" << endl
        << "|  5. 最近景点查询                       |" << endl
        << "|  6. 地图数据维护                       |" << endl
        << "|  7. 保存地图到文件                     |" << endl
        << "|  0. 退出系统                           |" << endl
        << "+========================================+" << endl
        << "请选择: ";
}

// 主运行循环：显示菜单 → 读取选择 → 分发处理 → 循环直到用户选0退出
// choice 用 char 而非 int，防止输入字母时 cin 进入错误状态
void Run(Graph* G) {
    char choice;
    char filename[MAX_NAME_LENGTH];

    do {
        PrintMainMenu();
        cin >> choice;

        switch (choice) {
        case '1':                                  // 加载地图文件
            cout << "请输入地图文件名: ";
            cin >> filename;
            LoadMap(G, filename);
            break;
        case '2':                                  // 显示全部地点
            DisplayAllVertex(*G);
            break;
        case '3':                                  // 距离查询
            DistanceQuery(*G);
            break;
        case '4':                                  // 路径导引
            PathGuide(*G);
            break;
        case '5':                                  // 最近景点
            NearestSpot(*G);
            break;
        case '6':                                  // 进入数据维护子菜单
            DataMaintenanceMenu(G);
            break;
        case '7':                                  // 保存地图
            cout << "请输入保存文件名: ";
            cin >> filename;
            SaveMap(*G, filename);
            break;
        case '0':                                  // 退出系统
            cout << "感谢使用校园地图查询导引系统，再见！" << endl;
            break;
        default:                                   // 无效输入 → 提示重新选择
            cout << "无效选择，请重新输入。" << endl;
        }
    } while (choice != '0');                       // 输入'0'退出循环
}

/* ========== 主函数 ========== */

// 程序入口：创建图对象 → 初始化 → 进入主循环
int main() {

    Graph G;
    InitGraph(&G);                                 // 初始化空图
    Run(&G);                                       // 进入菜单主循环
    return 0;
}
