
//该文件定义了BPDRR程序中的全局变量。文件根据变量的类型进行组织，每个变量都有一个简单的说明。

#ifndef WDSVARS_H
#define WDSVARS_H
#include "wdstypes.h"

EXTERN int	Nbreaks,		/* 爆管管道数量 */
			Nleaks,			/* 漏失管道数量 */
			Ninivariables;	/* 初始解变量数量 */




EXTERN	PDecision_Variable	Part_init_solution; /* 初始解指针 */
EXTERN	SBreaks*	BreaksRepository;	/* 爆管仓库指针(用于存储所有爆管) */
EXTERN	SLeaks*		LeaksRepository;	/* 漏损管道仓库指针(用于存储所有漏损管道) */
EXTERN	SCrew*		Schedule;			/* 工程队调度指针 */









#endif
