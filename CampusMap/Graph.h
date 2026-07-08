#pragma once
/*
 * Graph.h - 校园地图查询导引系统 核心数据结构定义
 *
 * 采用邻接矩阵存储图，支持无向图/有向图切换
 * 参考《数据结构课程设计任务指导书》
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

 /* ========== 宏定义 ========== */
// GRAPH_TYPE 决定图的类型：1=无向图（边双向对称），2=有向图（边单向）
// 用宏而非变量是为了在编译期确定，避免运行时判断开销
#define GRAPH_TYPE 1
// 最大地点数设为30，数组静态分配避免动态内存管理复杂性
#define MAX_VERTEX_NUM 30
// 名称最大40字符，足够容纳中英文地名，同时控制内存占用
#define MAX_NAME_LENGTH 40
// 无穷大取值：用一个大数表示"不可达"，选一个特殊数字方便调试时识别
#define INFINITY_VAL 6503148
#define TRUE 1
#define FALSE 0

/* ========== 图数据结构 ========== */

// 边（弧）结构体：每条边记录距离、方向角、道路名称
// direction 用0~359度表示与X轴夹角，方便计算转向提示
typedef struct ArcCell {
    float distance;                         // 边的长度（米）
    int direction;                          // 路径与X轴夹角 (0~359度)
    char info[MAX_NAME_LENGTH];             // 道路名称
} ArcCell, AdjMatrix[MAX_VERTEX_NUM][MAX_VERTEX_NUM];
// AdjMatrix 是 ArcCell 的二维数组别名，即邻接矩阵类型

// 图结构体：用邻接矩阵存储，O(1)时间查询任意两点间的边
// 选择邻接矩阵而非邻接表的原因：校园地点少（≤30），矩阵更简洁且支持 O(1) 边查询
typedef struct {
    char vexs[MAX_VERTEX_NUM][MAX_NAME_LENGTH];  // 顶点名称二维数组
    AdjMatrix arcs;                               // 邻接矩阵
    int vexnum;                                   // 当前顶点数
    int arcnum;                                   // 当前边数
} Graph;

/* ========== 函数声明 ========== */

// --- 基础操作 ---
void InitGraph(Graph* G);                       // 初始化图为空状态，清零计数并设所有边为无穷
int FindVertex(Graph G, const char* name);      // 按名称查找顶点下标，未找到返回-1

// --- 文件读写 ---
int LoadMap(Graph* G, const char* filename);    // 从文件加载地图，返回成功/失败
int SaveMap(Graph G, const char* filename);     // 保存地图到文件

// --- 核心算法 ---
// Dijkstra 单源最短路径：从 v0 出发，P[]记录前驱顶点，D[]记录最短距离
void ShortestPath_DIJ(Graph G, int v0, int P[], float D[]);
// 根据前后两段路的方向角计算转向提示（直行/左转/右转）
const char* GetDirection(int angleFrom, int angleTo);

// --- 查询功能 ---
void DisplayAllVertex(Graph G);     // 列出所有地点
void DistanceQuery(Graph G);        // 查询某地点到全图各点的最短距离
void PathGuide(Graph G);            // 两点间路径导引（带转向提示）
void NearestSpot(Graph G);          // 查询最近地点（支持前N个）

// --- 地图维护 ---
void AddVertex(Graph* G);           // 添加新地点
void DeleteVertex(Graph* G);        // 删除地点（级联删除相关道路）
void ModifyVertex(Graph* G);        // 修改地点名称
void AddRoad(Graph* G);             // 添加新道路
void DeleteRoad(Graph* G);          // 删除道路
void ModifyRoad(Graph* G);          // 修改道路属性（名称/距离/方向）
void DataMaintenanceMenu(Graph* G); // 数据维护子菜单

// --- 菜单 ---
void PrintMainMenu();               // 打印主菜单
void Run(Graph* G);                 // 主循环：读选择→分发→循环直到退出

#endif // GRAPH_H