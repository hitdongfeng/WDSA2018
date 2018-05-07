#ifndef WDSTYPES_H
#define WDSTYPES_H
#include<stdlib.h>

/*-----------------------------
          全局常量的定义
-----------------------------*/
#define   MAX_LINE   500          /* data.txt文件每行最大字符数 */
#define   MAX_TOKS   50           /* data.txt文件每行最大字段数 */
#define   MAX_CREWS	 3			   /* 工程队数量 */

#define	  MAX_ID	 31            /* ID最大字符数 */
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
	_Reopen,	//阀门开启
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

/* 定义爆管结构体 */
struct Breaks
{
	int isolate_flag;		//爆管隔离标识, 0:未隔离; 1:隔离
	int replace_flag;		//爆管替换标识, 0:未替换; 1:替换
	int reopen_flag;		//阀门开启标识, 0:未开启; 1:开启
	int isolate_time;		//隔离爆管所需要的时间(minutes)
	int replace_time;		//爆管修复时间(hours)
	int num_isovalve;		//隔离爆管所需要关闭的管道数量
	char nodeID[MAX_ID + 1];//模拟爆管所添加的虚拟节点ID(将喷射系数设为0，以关闭爆管流量)
	char pipeID[MAX_ID + 1];//爆管管道ID(将管道状态设置为open，以恢复供水)
	float pipediameter;		//爆管管道直径(mm)
	SFailurePipe *pipes;	//隔离爆管所需要关闭的管道ID
};
typedef struct Breaks SBreaks;

/* 定义漏损管道结构体 */
struct Leaks
{
	int repair_flag;		//漏损修复标识, 0:未修复; 1:修复
	int reopen_flag;		//阀门开启标识, 0:未开启; 1:开启
	int repair_time;		//漏损修复时间(hours)
	char nodeID[MAX_ID + 1];//模拟漏损所添加的虚拟节点ID(将喷射系数设为0，以关闭漏损流量)
	char pipeID[MAX_ID + 1];//漏损管道ID(将管道状态设置为open，以恢复供水)
	float pipediameter;		//漏损管道直径(mm)
};
typedef struct Leaks SLeaks;

/* 定义决策变量结构体 */
struct Decision_Variable
{
	int index;	//管道数组索引,从0开始
	int type;	//管道类型, 1:爆管隔离; 2:爆管替换; 3:漏损修复; 4:开阀 
	struct Decision_Variable *next;	//指向下一个邻接链表结构体
};
typedef struct Decision_Variable* PDecision_Variable;

/* 定义LinkedList链表指针结构体 */
typedef struct _linkedlist
{
	PDecision_Variable head;	/* 指向头节点指针 */
	PDecision_Variable tail;	/* 指向尾节点指针 */
	PDecision_Variable current;	/* 当前指针，用于辅助遍历链表 */
}LinkedList;

/* 定义工程队结构体 */
struct Crew
{
	long cumulative_time;
	LinkedList Plan;
};
typedef struct Crew SCrew;

#endif
