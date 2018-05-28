#ifndef WDSTYPES_H
#define WDSTYPES_H
#include<stdlib.h>

/*-----------------------------
          全局常量的定义
-----------------------------*/

#define	  Ndemands	 4201		/* 管网需水量节点数量(需水量>0) */
#define	  Start_pipeindex	6440/* pdd模型管道索引起始值(pdd模型节点水量为0，由对应的管道流量代替) */
#define	  RestorStartTime 1800	/* 开始修复时刻(秒) */
#define   SimulationStartTime 0  /* 水力模拟开始时刻 */
#define	  SimulationEndTime 259200 /* 水力模拟结束时刻 */

#define   MAX_CREWS	 3				/* 工程队数量 */
#define   MAX_LINE   500			/* data.txt文件每行最大字符数 */
#define   MAX_TOKS   50				/* data.txt文件每行最大字段数 */
#define	  MAX_ID	 31				/* ID最大字符数 */
#define	  Time_Step	 900			/* 水力模拟时间步长(秒) */
#define	  Pattern_length 24			/* 用水量时间模式长度 */
#define	  Break_Weight_Leak	0.5		/* 分配任务时，爆管与漏损选取权重 */
#define	  NUM_BreakOperation 2		/* 爆管维修操作流程数量(隔离+替换) */
#define	  NUM_LeakOperation 1		/* 漏损维修操作流程数量(维修) */
#define	  MAX_Fire_Volume 756000	/* 每个消火栓总供水量(L) */
#define	  NUM_Criteria	6			/* 评价准则数量 */
#define	  Time_of_Consecutive 8 		/* 节点连续缺水时间(小时) */
#define	  FLow_Tolerance	1e-3	/* 流量容差 */


#define   DATA_SEPSTR    " \t\n\r" /* data.txt文件字段分割符 */
#define   U_CHAR(x) (((x) >= 'a' && (x) <= 'z') ? ((x)&~32) : (x)) /* 字母转大写 */
#define   MEM_CHECK(x)  (((x) == NULL) ? 402 : 0 )   /* 内存分配不成功会返回空值NULL，显示错误代码402(没有充分的内存),否则返回0(无错误) */
#define   ERR_CODE(x)  (errcode = ((errcode>100) ? (errcode) : (x))) /* 函数错误类型检查,如果错误代码大于100,则被认为是严重错误 */

static void SafeFree(void **pp)          /* 安全释放内存函数：safeFree函数调用实际释放内存的free函数 */
{                                        /* 单纯的free函数执行后，内存被释放，但仍然有可能包含原值，指针仍指向原地址，会导致迷途指针 */
	if (pp != NULL && *pp != NULL)
	{
		free(*pp);
		*pp = NULL;
	}
}
#define SafeFree(p) SafeFree((void**)&(p))

/*-----------------------------
		全局枚举变量定义
-----------------------------*/

/* 定义故障管道状态 */
enum Pipe_Status 
{
	_Isolate=1,	//爆管隔离
	_Replace,	//爆管替换
	_Repair,	//漏损管道修复
	_Reopen		//阀门开启
};

/* 定义受损管道类型 */
enum Demage_type
{
	_Break=1,	//爆管
	_Leak		//漏损
};


/*-----------------------------
		全局数据结构体定义
-----------------------------*/

/* 定义故障管道结构体 */
struct FailurePipe
{
	char pipeID[MAX_ID + 1];	//故障管道ID
	int pipeindex;				//故障管道索引(以1开始)
};
typedef struct FailurePipe SFailurePipe;

/* 定义医院设施结构体 */
struct Hospital
{
	char nodeID[MAX_ID + 1];	//医院节点ID
	char pipeID[MAX_ID + 1];	//医院对应管道ID(用于模拟用水量)
	int	nodeindex;				//医院节点索引
	int pipeindex;				//医院对应管道索引
};
typedef struct Hospital SHospital;

/* 定义消火栓结构体 */
struct Firefight
{
	char ID[MAX_ID + 1];	//消火栓管道ID
	int index;				//消火栓管道索引(以1开始)
	float fire_flow;		//设计消防流量
	float cumu_flow;		//累计消防流量
};
typedef struct Firefight SFirefight;

/* 定义爆管结构体 */
struct Breaks
{
	int isolate_time;		//隔离爆管所需要的时间(minutes)
	int replace_time;		//爆管修复时间(hours)
	int num_isovalve;		//隔离爆管所需要关闭的管道数量
	int nodeindex;			//虚拟节点索引（以1开始）
	int pipeindex;          //爆管管道索引（以1开始）
	int flowindex;			//爆管流量管道索引(以1开始)
	char nodeID[MAX_ID + 1];//模拟爆管所添加的虚拟节点ID(将喷射系数设为0，以关闭爆管流量)
	char pipeID[MAX_ID + 1];//爆管管道ID(将管道状态设置为open，以恢复供水)
	char flowID[MAX_ID + 1];//爆管流量管道ID
	float pipediameter;		//爆管管道直径(mm)
	float emittervalue;     //虚拟节点喷射系数
	SFailurePipe *pipes;	//隔离爆管所需要关闭的管道ID
};
typedef struct Breaks SBreaks;

/* 定义漏损管道结构体 */
struct Leaks
{
	int repair_flag;		//漏损修复标识, 0:未修复; 1:修复
	int reopen_flag;		//阀门开启标识, 0:未开启; 1:开启
	int repair_time;		//漏损修复时间(hours)
	int nodeindex;			//虚拟节点索引（以1开始）
	int pipeindex;          //漏损管道索引（以1开始）
	int flowindex;			//漏损流量管道索引(以1开始)
	char nodeID[MAX_ID + 1];//模拟漏损所添加的虚拟节点ID(将喷射系数设为0，以关闭漏损流量)
	char pipeID[MAX_ID + 1];//漏损管道ID(将管道状态设置为open，以恢复供水)
	char flowID[MAX_ID + 1];//漏损流量管道ID
	float pipediameter;		//漏损管道直径(mm)
	float emittervalue;     //虚拟节点喷射系数
};
typedef struct Leaks SLeaks;

/* 定义决策变量结构体 */
struct Decision_Variable
{
	int index;		//管道数组索引,从0开始
	int type;		//管道类型, 1:爆管隔离; 2:爆管替换; 3:漏损修复; 4:开阀 
	long starttime;	//操作起始时间
	long endtime;	//操作结束时间
	struct Decision_Variable *next;	//指向下一个邻接链表结构体
};
typedef  struct Decision_Variable* PDecision_Variable;

/* 定义LinkedList链表指针结构体 */
typedef struct _linkedlist
{
	PDecision_Variable head;	/* 指向头节点指针 */
	PDecision_Variable tail;	/* 指向尾节点指针 */
	PDecision_Variable current;	/* 当前指针，用于辅助遍历链表 */
}LinkedList;

/* 定义系统供水能力结构体 */
typedef struct _sercapacity
{
	double Functionality;	/* 系统整体供水能力 */
	int Numkeyfac;			/* 满足供水能力的关键基础设施数量 */
	double MeankeyFunc;		/* 基础设施平均供水能力 */
	long time;				/* 模拟时刻 */
	struct _sercapacity *next;
}Sercapacity;

/* 定义Sercaplist链表指针结构体 */
typedef struct _sercaplist
{
	Sercapacity* head;		/* 指向头节点指针 */
	Sercapacity* tail;		/* 指向尾节点指针 */
	Sercapacity* current;	/* 当前指针，用于辅助遍历链表 */
}Sercaplist;

/* 定义damagebranch结构体，在randperm函数中调用 */
typedef struct _damagebranch
{
	int index;
	int count;
	struct _damagebranch *next;
}Sdamagebranch;

/* 定义damagebranchlist链表指针结构体 */
typedef struct _damagebranchlist
{
	Sdamagebranch* head;	/* 指向头节点指针 */
	Sdamagebranch* tail;	/* 指向尾节点指针 */
	Sdamagebranch* current;	/* 当前指针，用于辅助遍历链表 */
}Sdamagebranchlist;

/* 定义调度指令索引结构体 */
typedef struct _Scheduleindex
{
	PDecision_Variable pointer; /* SerialSchedule链表指定位置指针 */
	struct _Scheduleindex *next;/* 指向下一个结构体指针 */
}Scheduleindex;

/* 定义调度指令索引链表指针 */
typedef struct _Taskassigmentlist
{
	Scheduleindex* head;	/* 指向头节点指针 */
	Scheduleindex* tail;	/* 指向尾节点指针 */
	Scheduleindex* current;	/* 当前指针，用于辅助遍历链表 */
}STaskassigmentlist;

/* 定义Criteria结构体, C_05评估准则数组指针(用于计数) */
typedef struct _Criteria
{
	int flag;  /* 标志 */
	int count; /* 计数器 */
}SCriteria;


#endif
