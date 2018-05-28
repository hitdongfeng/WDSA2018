#ifndef _GA_H_
#define _GA_H_
#include <stdio.h>
#include "wdstypes.h"


/* 定义GA相关参数 */
int Num_group = 100;		/* 群体个体规模 */
int Num_offs = 100;			/* 后代个体数量(Num_son = Num_group + 2) */
int Num_iteration = 10000;	/* 迭代次数 */
double P_mutation = 0.1;	/* 变异概率 */
double P_crossover = 0.8;	/* 交叉概率 */

/* 定义Solution结构体 */
typedef struct {
	int C_01;		/* Criteria 1*/
	int C_02;		/* Criteria 2*/
	double C_03;	/* Criteria 3*/
	double C_04;	/* Criteria 4*/
	int C_05;		/* Criteria 5*/
	double C_06;	/* Criteria 6*/
	double objvalue;	/* 目标函数值 */
	double P_Reproduction;	/* 个体概率 */
	LinkedList* SerialSchedule;  /* 调度指令链表指针(所有调度指令) */
	//LinkedList*	NewVisDemages;	 /* 修复过程中新出现的可见受损管道数组指针 */
	STaskassigmentlist Schedule[MAX_CREWS]; /* 工程队调度指针(包含初始解和新增解) */
}Solution;

Solution** Groups;			/* 存储群体 */  
Solution** Offspring;		/* 存储杂交后的个体 */
int IndexCross_i;			/* 起始交叉点 */
int IndexCross_j;			/* 终止交叉点 */
int Chrom_length;			/* 个体染色体长度 */
int Length_SonSoliton;		/* 遗传产生的孩子的个数 */  
FILE *TemSolution;			/* 存储每一代最优解 */

/* 定义相关函数 */
int Memory_Allocation();			/* 分配堆内存 */
void Free_Solution(Solution*);		/*  释放Solution结构体内存 */
void Free_GAmemory();				/* 释放所有内存 */
int InitialGroups();				/* 初始化种群 */
int Calculate_Objective_Value(Solution*);/* 计算目标函数值 */
void Calc_Probablity();				/* 每次新产生的群体, 计算每个个体的概率 */
int Select_Individual();			/* 轮盘赌随机从当前总群筛选出一个杂交对象 */
int* Find_Mother_Index(Solution*, Solution*);/* 返回母代在父代对应变量的索引 */
int Get_Conflict_Length(int*, int*, int);/* 找到Father_Cross和Mother_cross中产生冲突的变量数量 */
int *Get_Conflict(int*, int*, int, int);/* 找到Father_Cross和Mother_cross中产生冲突的变量 */
Solution* Handle_Conflict(int*, PDecision_Variable*, int*, int*, int);/*  处理冲突子代个体 */
int GA_Cross(Solution*, Solution*);	/* GA交叉操作，从两个个体中产生一个新个体 */
int GA_Variation(int);				/* 对后代进行变异操作 */
void Clone_Group(Solution**, Solution**);/* 对两个个体进行复制操作 */
void BestSolution();				/* 查找最优解 */
void GA_UpdateGroup();				/* 更新种群 */
int GA_Evolution();					/* GA主循环过程 */


#endif // _GA_H_
