#ifndef _GA_H_
#define _GA_H_
#include "wdstypes.h"


/* 定义GA相关参数 */
int Num_group = 100;		/* 群体个体规模 */
int Num_offs = 102;		/* 后代个体数量(Num_son = Num_group + 2) */
int Num_iteration = 1000;	/* 迭代次数 */
double P_mutation = 0.01;	/* 变异概率 */
double P_crossover = 0.8;	/* 交叉概率 */

/* 定义Solution结构体 */
typedef struct {
	double objvalue;	/* 目标函数值 */
	LinkedList* SerialSchedule;  /* 调度指令链表指针(所有调度指令) */
	STaskassigmentlist Schedule[MAX_CREWS]; /* 工程队调度指针(包含初始解和新增解) */
	double P_Reproduction;	/* 个体概率 */
}Solution;

Solution* Groups;			/* 存储群体 */  
Solution* Offspring;		/* 存储杂交后的个体 */


/* 定义相关函数 */
int Memory_Allocation();			/* 分配堆内存 */
void InitialGroups();				/* 初始化种群 */


#endif // _GA_H_
