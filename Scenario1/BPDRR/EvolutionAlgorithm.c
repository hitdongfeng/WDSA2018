/*******************************************************************
Name: EvolutionAlgorithm.c
Purpose: 进化算法涉及的相关函数
Data: 5/3/2018
Author: Qingzhou Zhang
Email: wdswater@gmail.com
********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "wdstext.h"
#include "wdstypes.h"
#include "wdsfuns.h"
#include "epanet2.h"
#include "mt19937ar.h"
//#define EXTERN extern
#define EXTERN 
#include "wdsvars.h"


Sdamagebranchlist Break, Leak;	/* 可视爆管/漏损管道链表指针 */
int Num_breaks = 0, Num_leaks = 0;	/* 可视爆管/漏损管道数量 */
int Break_count, Leak_count;		/* 可视爆管/漏损管道数量计数 */


void Add_damagebranch_list(Sdamagebranchlist* list, Sdamagebranch *ptr)
/*--------------------------------------------------------------
**  Input:   list: pointer to Sdamagebranchlist chain table
**			 ptr: 需要插入的Sdamagebranch结构体指针
**  Output:  none
**  Purpose: Add a Sdamagebranch struct to the tail of the list
**--------------------------------------------------------------*/
{
	if (list->head == NULL)
	{
		list->head = ptr;
	}
	else
	{
		list->tail->next = ptr;
	}
	list->tail = ptr;
}

int Get_Select_Repository()
/*--------------------------------------------------------------
**  Input:   无
**  Output:  Error Code
**  Purpose: 生成可见爆管/漏损相关信息，供随机选择函数调用
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;	/* 错误编码 */
	Sdamagebranch *ptr;	/* 临时结构体指针 */

	/* 初始化Break, Leak, p结构体 */
	Break.head = NULL; Break.tail = NULL; Break.current = NULL;
	Leak.head = NULL; Leak.tail = NULL; Leak.current = NULL;

	/* 获取可见爆管和漏损管道数量 */
	decisionlist.current = decisionlist.head;
	while (decisionlist.current != NULL)
	{
		if (decisionlist.current->type == _Break)
		{
			ptr = (Sdamagebranch*)calloc(1, sizeof(Sdamagebranch));
			ERR_CODE(MEM_CHECK(ptr));	if (errcode) err_count++;

			ptr->index = decisionlist.current->index;
			ptr->count = 0;
			ptr->next = NULL;
			Add_damagebranch_list(&Break, ptr);
			Num_breaks++;
		}
		else if (decisionlist.current->type = _Leak)
		{
			ptr = (Sdamagebranch*)calloc(1, sizeof(Sdamagebranch));
			ERR_CODE(MEM_CHECK(ptr));	if (errcode) err_count++;

			ptr->index = decisionlist.current->index;
			ptr->count = 0;
			ptr->next = NULL;
			Add_damagebranch_list(&Leak, ptr);
			Num_leaks++;
		}

		else {
			printf("Type of visible demages error: not Break or Leak!\n");
			assert(0); //终止程序，返回错误信息
		}
		decisionlist.current = decisionlist.current->next;
	}
	if (err_count) errcode = 416;

	return errcode;
}

Sdamagebranch* find_visibledamage_index(Sdamagebranchlist* list, int index,int type, int* OperationType)
/*--------------------------------------------------------------
**  Input:   list: pointer to Sdamagebranchlist chain table
**			 index: 索引
**			 number: 爆管/漏损维修操作流程数量,爆管：2; 漏损: 1.
**  Output:  返回指定索引故障操作结构体指针,若找不到指定索引，则返回NULL
**  Purpose: 根据随机索引值，获取指定操作流程
**--------------------------------------------------------------*/
{
	int count = -1;
	list->current = list->head;
	while (list->current != NULL)
	{
		if (type == _Break)
		{
			if (list->current->count < NUM_BreakOperation)
			{
				++count;
				if (count == index)
				{
					if (list->current->count == 0)
						*OperationType = _Isolate;
					else if (list->current->count == 1)
					{
						*OperationType = _Replace;
						Break_count--;
					}
					
					list->current->count++;
					return (list->current);
				}
				
			}
		}
		else 
		{
			if (list->current->count < NUM_LeakOperation)
			{
				++count;
				if (count == index)
				{
					if (list->current->count == 0)
					{
						*OperationType = _Repair;
						Leak_count--;
					}

					list->current->count++;
					return (list->current);
				}
				
			}
		}

		list->current = list->current->next;
	}

	return NULL;
}

LinkedList* Randperm()
/**----------------------------------------------------------------
**  输入:  无
**  输出:  返回LinkedList指针
**  功能:  随机生成可见爆管/漏损操作顺序，供工程队从中选取
**----------------------------------------------------------------*/
{
	int errcode = 0;					/* 错误编码 */
	int temptype;						/* 临时变量 */
	int Random_breakindex, Random_leakindex; /* 爆管/漏损随机索引 */
	double randomvalue;					/* 随机浮点数[0,1] */
	Sdamagebranch *ptr;					/* 临时结构体指针 */
	LinkedList* p;						/* 临时结构体指针 */
	
	/* 初始化相关参数 */
	p = (LinkedList*)calloc(1, sizeof(LinkedList));
	p->head = NULL; p->tail = NULL; p->current = NULL;
	Break_count = Num_breaks;
	Leak_count = Num_leaks;

	while (Break_count > 0 || Leak_count > 0)
	{
		if (Break_count > 0 && Leak_count > 0)
		{
			randomvalue = genrand_real1();
			if (randomvalue < Break_Weight_Leak)
			{
				randomvalue = genrand_real1();
				Random_breakindex = (int)floor(randomvalue*(Break_count - 0.0001));
				ptr = find_visibledamage_index(&Break, Random_breakindex, _Break,&temptype);
				errcode = Add_tail(p, ptr->index, temptype, 0, 0);
				if (errcode) fprintf(ErrFile, ERR415);
			}
			else
			{
				randomvalue = genrand_real1();
				Random_leakindex = (int)floor(randomvalue*(Leak_count - 0.0001));
				ptr = find_visibledamage_index(&Leak, Random_leakindex, _Leak, &temptype);
				errcode = Add_tail(p, ptr->index, temptype, 0, 0);
				if (errcode) fprintf(ErrFile, ERR415);
			}
		}
		else if (Break_count > 0)
		{
			randomvalue = genrand_real1();
			Random_breakindex = (int)floor(randomvalue*(Break_count - 0.0001));
			ptr = find_visibledamage_index(&Break, Random_breakindex, _Break, &temptype);
			errcode = Add_tail(p, ptr->index, temptype, 0, 0);
			if (errcode) fprintf(ErrFile, ERR415);
		}
		else
		{
			randomvalue = genrand_real1();
			Random_leakindex = (int)floor(randomvalue*(Leak_count - 0.0001));
			ptr = find_visibledamage_index(&Leak, Random_leakindex, _Leak, &temptype);
			errcode = Add_tail(p, ptr->index, temptype, 0, 0);
			if (errcode) fprintf(ErrFile, ERR415);
		}
	}

	return p;
}

void Add_Taskassigmentlist(STaskassigmentlist* list, Scheduleindex *ptr)
/*--------------------------------------------------------------
**  Input:   list: pointer to STaskassigmentlist chain table
**			 ptr: 需要插入的Scheduleindex结构体指针
**  Output:  none
**  Purpose: Add a Scheduleindex struct to the tail of the list
**--------------------------------------------------------------*/
{
	if (list->head == NULL)
	{
		list->head = ptr;
	}
	else
	{
		list->tail->next = ptr;
	}
	list->tail = ptr;
}

int Find_Replace_Crow(PDecision_Variable ptr)
/*--------------------------------------------------------------
**  Input:   ptr: pointer to Decision_Variable
**  Output:  执行Isolate操作的工程队索引,否则，返回错误值-1.
**  Purpose: 针对爆管replace操作，查找执行Isolate操作的工程队索引
**--------------------------------------------------------------*/
{
	for (int i = 0; i < MAX_CREWS; i++)
	{
		Schedule[i].current = Schedule[i].head;
		while (Schedule[i].current != NULL)
		{
			if (Schedule[i].current->pointer->index == ptr->index)
				return i;
			Schedule[i].current = Schedule[i].current->next;
		}
	}
	return -1;
}

int Find_Finished_Crow()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  返回完成任务的工程队索引,否则，返回错误值-1.
**  Purpose: 返回完成任务的工程队索引，以继续分配下一个指令
**--------------------------------------------------------------*/
{
	int index=-1;
	long mintime=(long)1E+8;
	for (int i = 0; i < MAX_CREWS; i++)
	{
		if (Schedule[i].head == NULL)
			return i;
		else
		{
			if (mintime > Schedule[i].tail->pointer->endtime)
			{
				mintime = Schedule[i].tail->pointer->endtime;
				index = i;
			}	
		}
	}
	return index;
}

int Task_Assignment()
/**----------------------------------------------------------------
**  输入:  无
**  输出:  Error Code
**  功能:  将SerialSchedule链表中的所有指令分配至每个工程队
**----------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;
	int crowindex;	/* replace操作工程队索引 */
	
	Scheduleindex* ptr;	/* 临时指针 */

	/* 若有初始解，先添加初始解 */
	if (NvarsCrew1 > 0 || NvarsCrew2 > 0 || NvarsCrew1 > 0)
	{
		for (int i = 0; i < MAX_CREWS; i++)
		{
			ExistSchedule[i].current = ExistSchedule[i].head;
			while (ExistSchedule[i].current != NULL)
			{
				ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
				ERR_CODE(MEM_CHECK(ptr));	if (errcode) err_count++;

				ptr->pointer = ExistSchedule[i].current;
				ptr->next = NULL;
				Add_Taskassigmentlist(&Schedule[i], ptr);
				ExistSchedule[i].current = ExistSchedule[i].current->next;
			}
		}
	}

	for (int i = 0; i < MAX_CREWS; i++)
	{
		printf("Schedule[%d]:\n", i);
		Schedule[i].current = Schedule[i].head;
		while (Schedule[i].current != NULL)
		{
			printf("index: %d	type: %d	starttime: %d	endtime: %d\n",
				Schedule[i].current->pointer->index, Schedule[i].current->pointer->type,
				Schedule[i].current->pointer->starttime, Schedule[i].current->pointer->endtime);
			Schedule[i].current = Schedule[i].current->next;
		}
		printf("\n");
	}
	
	/* 将SerialSchedule链表中的所有指令分配至每个工程队 */
	SerialSchedule->current = SerialSchedule->head;
	while (SerialSchedule->current != NULL)
	{
		/* 针对爆管Isolate操作,分配至相应的工程队 */
		if (SerialSchedule->current->type == _Isolate)
		{
			crowindex = Find_Finished_Crow();
			if (crowindex < 0) err_count++;
			
			if (Schedule[crowindex].head == NULL)
			{
				SerialSchedule->current->starttime = RestorStartTime;
				SerialSchedule->current->endtime = BreaksRepository[SerialSchedule->current->index].isolate_time;
			}
			else
			{
				SerialSchedule->current->starttime= Schedule[crowindex].tail->pointer->endtime;
				SerialSchedule->current->endtime = SerialSchedule->current->starttime + BreaksRepository[SerialSchedule->current->index].replace_time;
			}
				ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
				ptr->pointer = SerialSchedule->current;
				ptr->next = NULL;
				Add_Taskassigmentlist(&Schedule[crowindex], ptr);
		}
		/* 针对爆管replace操作，查找执行Isolate操作的工程队索引 */
		else if (SerialSchedule->current->type == _Replace)
		{
			crowindex = Find_Replace_Crow(SerialSchedule->current);
			if (crowindex < 0) err_count++;

			SerialSchedule->current->starttime = Schedule[crowindex].tail->pointer->endtime;
			SerialSchedule->current->endtime = SerialSchedule->current->starttime + BreaksRepository[SerialSchedule->current->index].replace_time;
			
			ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
			ptr->pointer = SerialSchedule->current;
			ptr->next = NULL;
			Add_Taskassigmentlist(&Schedule[crowindex], ptr);
		}

		/* 针对漏损repair操作，分配至相应的工程队 */
		if (SerialSchedule->current->type == _Repair)
		{
			crowindex = Find_Finished_Crow();
			if (crowindex < 0) err_count++;

			if (Schedule[crowindex].head == NULL)
			{
				SerialSchedule->current->starttime = RestorStartTime;
				SerialSchedule->current->endtime = LeaksRepository[SerialSchedule->current->index].repair_time;
			}
			else
			{
				SerialSchedule->current->starttime = Schedule[crowindex].tail->pointer->endtime;
				SerialSchedule->current->endtime = SerialSchedule->current->starttime + LeaksRepository[SerialSchedule->current->index].repair_time;
			}
			ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
			ptr->pointer = SerialSchedule->current;
			ptr->next = NULL;
			Add_Taskassigmentlist(&Schedule[crowindex], ptr);
		}

		SerialSchedule->current = SerialSchedule->current->next;
	}

	if (err_count)
		errcode = 417;
	return errcode;
}


int main(void)
{
	int errcode = 0;	//错误编码 
	


	/* 读取data.txt数据 */
	errcode = readdata("data.txt", "err.txt");
	if (errcode) { fprintf(ErrFile, ERR406); return (406); }

	/* 打开inp文件 */
	Open_inp_file("BBM_Scenario1.inp", "BBM_Scenario1.rpt", "");

	/* 获取爆管/漏损管道喷射节点索引、喷射系数、管道索引; 医院及消火栓节点、管道索引 */
	Get_FailPipe_keyfacility_Attribute();

	/* 生成可见爆管/漏损相关信息，供随机选择函数调用 */
	errcode = Get_Select_Repository();
	if (errcode) { fprintf(ErrFile, ERR416); return (416); }

	SerialSchedule = Randperm();

	///* 打印SerialSchedule结构体数值 */
	//SerialSchedule->current = SerialSchedule->head;
	//while (SerialSchedule->current != NULL)
	//{
	//	printf("index: %d	type: %d\n", SerialSchedule->current->index, SerialSchedule->current->type);

	//	SerialSchedule->current = SerialSchedule->current->next;
	//}

	errcode = Task_Assignment();
	
	getchar();
	return 0;
}