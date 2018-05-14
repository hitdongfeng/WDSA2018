
//该文件定义了BPDRR程序中的全局变量。文件根据变量的类型进行组织，每个变量都有一个简单的说明。

#ifndef WDSVARS_H
#define WDSVARS_H
#include "wdstypes.h"

EXTERN int	Nhospital,		/* 医院设施数量 */
			Nfirefight,		/* 消火栓数量 */
			Nbreaks,		/* 爆管管道数量 */
			Nleaks,			/* 漏失管道数量 */
			Ndecisionvars,	/* 新增变量数量 */
			NvarsCrew1,		/* crew1初始解决策变量 */
			NvarsCrew2,		/* crew2初始解决策变量 */
			NvarsCrew3;		/* crew3初始解决策变量 */

EXTERN FILE *ErrFile,		/* 错误报告文件指针 */
			*InFile,		/* data.txt文件指针 */
			*SenAnalys;		/* 灵敏度分析输出文件指针 */


//EXTERN	PDecision_Variable	Part_init_solution; /* 初始解指针 */
EXTERN	SHospital* Hospitals;			/* 医院设施结构体指针 */
EXTERN	SFirefight* Firefighting;		/* 消火栓结构体指针 */
EXTERN	SBreaks*	BreaksRepository;	/* 爆管仓库指针(用于存储所有爆管) */
EXTERN	SLeaks*		LeaksRepository;	/* 漏损管道仓库指针(用于存储所有漏损管道) */
EXTERN	LinkedList*	Schedule;			/* 工程队调度指针 */
EXTERN	LinkedList	decisionlist;		/* 受损管道管道类型修复指针结构体(用于存储链表指针) */
EXTERN	LinkedList	IniVisDemages;		/* 模拟开始时刻(6:30)可见受损管道数组指针 */
EXTERN	LinkedList	NewVisDemages;		/* 修复过程中新出现的可见受损管道数组指针 */
EXTERN	float**	ActuralBaseDemand;		/* 节点实际需水量数组指针 */
EXTERN	Sercaplist  SerCapcPeriod;		/* 指定时段内每个模拟步长系统供水能力结构体 */






#endif
