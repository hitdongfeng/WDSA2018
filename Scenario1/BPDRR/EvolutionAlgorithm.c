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

	/* 打印SerialSchedule结构体数值 */
	SerialSchedule->current = SerialSchedule->head;
	while (SerialSchedule->current != NULL)
	{
		printf("index: %d	type: %d\n", SerialSchedule->current->index, SerialSchedule->current->type);

		SerialSchedule->current = SerialSchedule->current->next;
	}


	
	getchar();
	return 0;
}