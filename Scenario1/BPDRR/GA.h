#ifndef _GA_H_
#define _GA_H_
#include "wdstypes.h"


/* 定义GA相关参数 */
int Num_group = 100;		/* 群体个体规模 */
int Num_offs = 102;			/* 后代个体数量(Num_son = Num_group + 2) */
int Num_iteration = 1000;	/* 迭代次数 */
double P_mutation = 0.01;	/* 变异概率 */
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
	LinkedList	NewVisDemages;	 /* 修复过程中新出现的可见受损管道数组指针 */
	STaskassigmentlist Schedule[MAX_CREWS]; /* 工程队调度指针(包含初始解和新增解) */
}Solution;



Solution* Groups;			/* 存储群体 */  
Solution* Offspring;		/* 存储杂交后的个体 */


/* 定义相关函数 */
int Memory_Allocation();			/* 分配堆内存 */
int InitialGroups();				/* 初始化种群 */


#endif // _GA_H_
