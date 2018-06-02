/*******************************************************************
Name: GA.c
Purpose: GA算法主函数
Data: 5/3/2018
Author: Qingzhou Zhang
Email: wdswater@gmail.com
********************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "wdstext.h"
#include "GA.h"
#include "wdsfuns.h"
#include "epanet2.h"
#include "mt19937ar.h"
//#define EXTERN extern
#define EXTERN 
#include "wdsvars.h"

int Memory_Allocation()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  Error code
**  Purpose: 分配堆内存
**--------------------------------------------------------------*/
{
	int errcode = 0, err_sum = 0;

	Groups = (Solution**)calloc(Num_group, sizeof(Solution*));
	for (int i = 0; i < Num_group; i++)
	{
		Groups[i] = (Solution*)calloc(1, sizeof(Solution));
		ERR_CODE(MEM_CHECK(Groups[i]));	if (errcode) err_sum++;
	}

	Offspring = (Solution**)calloc(Num_offs, sizeof(Solution*));

	ERR_CODE(MEM_CHECK(Groups));	if (errcode) err_sum++;
	ERR_CODE(MEM_CHECK(Offspring));	if (errcode) err_sum++;

	if (err_sum)
	{
		fprintf(ErrFile, ERR402);
		return (402);
	}

	return errcode;
}

void Free_Solution(Solution* ptr)
/*--------------------------------------------------------------
**  Input:   ptr: Solution结构体指针
**  Output:  None
**  Purpose: 释放Solution结构体内存
**--------------------------------------------------------------*/
{
	/* 释放SerialSchedule链表指针 */
	if (ptr != NULL)
	{
		if (ptr->SerialSchedule != NULL)
		{
			ptr->SerialSchedule->current = ptr->SerialSchedule->head;
			while (ptr->SerialSchedule->current != NULL)
			{
				ptr->SerialSchedule->head = ptr->SerialSchedule->head->next;
				SafeFree(ptr->SerialSchedule->current);
				ptr->SerialSchedule->current = ptr->SerialSchedule->head;
			}
		}

		/* 释放Schedule数组内存 */
		for (int i = 0; i < MAX_CREWS; i++)
		{
			if (ptr->Schedule[i].head != NULL)
			{
				ptr->Schedule[i].current = ptr->Schedule[i].head;
				while (ptr->Schedule[i].current != NULL)
				{
					ptr->Schedule[i].head = ptr->Schedule[i].head->next;
					SafeFree(ptr->Schedule[i].current);
					ptr->Schedule[i].current = ptr->Schedule[i].head;
				}
			}
		}

		SafeFree(ptr);
	}
}

void Free_GAmemory()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  None
**  Purpose: 释放所有内存
**--------------------------------------------------------------*/
{
	/* 释放种群Groups内存 */
	for (int i = 0; i < Num_group; i++)
		Free_Solution(Groups[i]);

	/* 释放子代Offspring内存 */
	for (int i = 0; i < Num_offs; i++)
		Free_Solution(Offspring[i]);

}

int InitialGroups()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  Error code
**  Purpose: 初始化种群
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;
	Chrom_length = 0; /* 个体染色体长度 */
	for (int i = 0; i < Num_group; i++)
	{
		/*  随机生成可见爆管/漏损操作顺序，供工程队从中选取 */
		Groups[i]->SerialSchedule = Randperm();
		/* 打印SerialSchedule结构体数值 */
		Groups[i]->SerialSchedule->current = Groups[i]->SerialSchedule->head;
		while (Groups[i]->SerialSchedule->current != NULL)
		{
			printf("index: %d	type: %d\n", Groups[i]->SerialSchedule->current->index, Groups[i]->SerialSchedule->current->type);

			Groups[i]->SerialSchedule->current = Groups[i]->SerialSchedule->current->next;
		}

		/*  将SerialSchedule链表中的所有指令分配至每个工程队 */
		ERR_CODE(Task_Assignment(Groups[i]->SerialSchedule, Groups[i]->Schedule));

		Groups[i]->C_01 = 0;Groups[i]->C_02 = 0;Groups[i]->C_03 = 0;
		Groups[i]->C_04 = 0;Groups[i]->C_05 = 0;Groups[i]->C_06 = 0;
		Groups[i]->P_Reproduction = 0;Groups[i]->objvalue = 0;

		/*打印 Schedule 结构体数值 */
	for (int j = 0; j < MAX_CREWS; j++)
	{
		printf("\nSchedule[%d]:\n", j);
		Groups[i]->Schedule[j].current = Groups[i]->Schedule[j].head;
		while (Groups[i]->Schedule[j].current != NULL)
		{
			printf("index: %d	type: %d	starttime: %d	endtime: %d\n",
				Groups[i]->Schedule[j].current->pointer->index, Groups[i]->Schedule[j].current->pointer->type,
				Groups[i]->Schedule[j].current->pointer->starttime, Groups[i]->Schedule[j].current->pointer->endtime);
			Groups[i]->Schedule[j].current = Groups[i]->Schedule[j].current->next;
		}
		printf("\n");
	}

		if (errcode) err_count++;
	}

	/* 获取个体染色体长度 */
	Groups[0]->SerialSchedule->current = Groups[0]->SerialSchedule->head;
	while (Groups[0]->SerialSchedule->current != 0)
	{
		Chrom_length++;
		Groups[0]->SerialSchedule->current = Groups[0]->SerialSchedule->current->next;
	}

	/* 计算每个个体的适应度值 */
	for (int i = 0; i < Num_group; i++)
	{
		ERR_CODE(Calculate_Objective_Value(Groups[i]));
		if (errcode)	err_count++;
		printf("Complited: %d / %d\n", i + 1, Num_group);
	}

	Calc_Probablity();/* 计算每个个体的选择概率 */
	BestSolution(); /* 保存当前最优解 */
		
	if (err_count)
		errcode = 419;
	return errcode;
}

int Calculate_Objective_Value(Solution* sol)
/*--------------------------------------------------------------
**  Input:   sol: 种群数组指针
**  Output:  Error code
**  Purpose: 计算目标函数值
**--------------------------------------------------------------*/
{
	int errcode = 0, err_sum = 0;	/* 错误编码及计数 */
	int s;							/* 整点时刻 */
	int temvar,temindex;			/* 临时变量 */
	float y,temflow;				/* 临时变量 */
	double sumpdddemand, sumbasedemand; /* 节点pdd需水量、节点基本需水量 */
	long t, tstep;	/* t: 当前时刻; tstep: 水力计算时间步长 */

	/* 设置Schedule结构体链表current指针 */
	for (int i = 0; i < MAX_CREWS; i++)
		sol->Schedule[i].current = sol->Schedule[i].head;
	/* 初始化消火栓累计流量 */
	for (int i = 0; i < Nfirefight; i++)
		Firefighting[i].cumu_flow = 0;
	/* 初始化C_05计数器*/
	for (int i = 0; i < Ndemands; i++)
	{
		Criteria[i].count = 0;
		Criteria[i].flag = -1;
	}
					
	/* run epanet analysis engine */
	Open_inp_file(inpfile, "report.rpt", "");
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */
	ERR_CODE(ENopenH());	if (errcode > 100) err_sum++;	/* Opens the hydraulics analysis system. */
	ERR_CODE(ENinitH(0));	if (errcode > 100) err_sum++;	/* Don't save the hydraulics file */
	
	do 
	{
		ERR_CODE(ENrunH(&t)); if (errcode > 100) err_sum++;
		sumpdddemand = 0.0, sumbasedemand = 0.0;	/* 初始化节点pdd需水量、节点基本需水量 */

		if ((t >= SimulationStartTime && t <= SimulationEndTime) && (t % Time_Step == 0))
		{
			s = (t / 3600) % 24; //当前时刻所对应的时段
			/* 计算C_01 */
			for (int i = 0; i < Nhospital; i++) /* Hospital */
			{
				ERR_CODE(ENgetlinkvalue(Hospitals[i].pipeindex, EN_FLOW, &temflow));
				if (errcode > 100) err_sum++;
				if ((temflow < FLow_Tolerance) || (temflow - ActuralBaseDemand[Hospitals[i].nodeindex - 1][s]) > 0.5)
					temflow = 0.0;

				y = temflow / ActuralBaseDemand[Hospitals[i].nodeindex - 1][s];
				if (y <= 0.5)
					sol->C_01 += Time_Step / 60;
					
			}

			if (t >= RestorStartTime)
			{
				for (int i = 0; i < Nfirefight; i++) /* Firefight nodes */
				{
					if (Firefighting[i].cumu_flow < MAX_Fire_Volume)
					{
						ERR_CODE(ENgetlinkvalue(Firefighting[i].index, EN_FLOW, &temflow));
						if (errcode > 100) err_sum++;
						if ((temflow < FLow_Tolerance) || (temflow - Firefighting[i].fire_flow)>0.5)
							temflow = 0.0;

						y = temflow / Firefighting[i].fire_flow;
						if (y <= 0.5)
							sol->C_01 += Time_Step / 60;
							
					}
				}
			}
			///* 计算C_02、C_04、C_05 */
			for (int i = 0; i < Ndemands; i++)
			{
				ERR_CODE(ENgetlinkvalue(i + Start_pipeindex, EN_FLOW, &temflow));
				if (errcode > 100) err_sum++;
				if ((temflow < FLow_Tolerance) || (temflow - ActuralBaseDemand[i][s]) > 0.5)
					temflow = 0.0;

				if (temflow / ActuralBaseDemand[i][s] <= 0.5)
				{
					sol->C_04 += (double)Time_Step / (Ndemands*60.0);

					Criteria[i].count++;
					if ((Criteria[i].count >= Time_of_Consecutive * (3600 / Time_Step)) && Criteria[i].flag == -1)
					{
						sol->C_05++;
						Criteria[i].flag = 1;
					}
				}
				else
				{
					Criteria[i].count = 0;
				}

				sumpdddemand += (double)temflow;
				sumbasedemand += (double)ActuralBaseDemand[i][s];
			}
			if (sumpdddemand / sumbasedemand <= 0.95)
				sol->C_02 = (int)t/60;
			
			/* 计算C_03 */
			if (sumpdddemand / sumbasedemand < 0.999)
				sol->C_03 += (1.0 - sumpdddemand / sumbasedemand)*(Time_Step/60);


			/* 计算C_06 */
			for (int i = 0; i < Nbreaks; i++) /* 遍历所有爆管 */
			{
				ERR_CODE(ENgetlinkvalue(BreaksRepository[i].flowindex, EN_FLOW, &temflow));
				if (errcode > 100)	err_sum++;
				if ((temflow < FLow_Tolerance))
					temflow = 0.0;
				sol->C_06 += (double)temflow * Time_Step;
			}

			for (int i = 0; i < Nleaks; i++) /* 遍历所有漏损管道 */
			{
				ERR_CODE(ENgetlinkvalue(LeaksRepository[i].flowindex, EN_FLOW, &temflow));
				if (errcode > 100)	err_sum++;
				if ((temflow < FLow_Tolerance))
					temflow = 0.0;
				sol->C_06 += (double)temflow * Time_Step;
			}
/*************************************************************************************************/
			/* 更新消火栓状态 */
			if (t >= RestorStartTime)
			{
				for (int i = 0; i < Nfirefight; i++)
				{
					if (Firefighting[i].cumu_flow >= MAX_Fire_Volume)
					{
						ERR_CODE(ENsetlinkvalue(Firefighting[i].index, EN_SETTING, 0));
						if (errcode > 100) err_sum++;
					}
					else
					{
						ERR_CODE(ENgetlinkvalue(Firefighting[i].index, EN_FLOW, &temflow));
						if (errcode > 100) err_sum++;
						if ((temflow < FLow_Tolerance) || (temflow - Firefighting[i].fire_flow>0.5))
							temflow = 0.0;

						Firefighting[i].cumu_flow += temflow * Time_Step;
					}
				}
			}

			/* 更新管道状态 */
			for (int i = 0; i < MAX_CREWS; i++)
			{
				if ((sol->Schedule[i].current != NULL) && (sol->Schedule[i].current->pointer->endtime == t))
				{
					temindex = sol->Schedule[i].current->pointer->index;
					temvar = sol->Schedule[i].current->pointer->type;
					if (temvar == _Isolate)
					{
						ERR_CODE(Breaks_Adjacent_operation(_Isolate,temindex, EN_STATUS,0,0));
						if(errcode) err_sum++;
					}
					else if (temvar == _Replace)
					{
						ERR_CODE(Breaks_Adjacent_operation(_Reopen, temindex, EN_STATUS, 1, 0));
						if (errcode) err_sum++;
					}
					else if (temvar == _Repair)
					{
						ERR_CODE(Leaks_operation(temindex, EN_STATUS, 1, 0));
						if (errcode) err_sum++;
					}

					sol->Schedule[i].current = sol->Schedule[i].current->next;
				}

			}
		}
		ERR_CODE(ENnextH(&tstep));	if (errcode > 100) err_sum++;
	} while (tstep > 0);

	sol->objvalue = sol->C_01 + sol->C_02 + sol->C_03 + sol->C_04 + sol->C_05 + sol->C_06/1000000;
	//sol->objvalue = sol->C_01;
	//sol->objvalue = sol->C_01 + sol->C_02 + sol->C_03 + sol->C_04 + sol->C_05;

	ERR_CODE(ENcloseH()); if (errcode) err_sum++;
	ERR_CODE(ENclose()); if (errcode) err_sum++; /* 关闭水力模型 */
	if (err_sum)
		errcode = 419;
	return errcode;
}

void Calc_Probablity()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: 每次新产生的群体, 计算每个个体的概率
**--------------------------------------------------------------*/
{
	double total_objvalue = 0.0;
	double TempTotal_P = 0.0;

	for (int i = 0; i < Num_group; i++)
		total_objvalue += Groups[i]->objvalue;

	for (int i = 0; i < Num_group; i++)
	{
		Groups[i]->P_Reproduction = (1.0 / Groups[i]->objvalue)*total_objvalue;
		TempTotal_P += Groups[i]->P_Reproduction;
	}

	for (int i = 0; i < Num_group; i++)
		Groups[i]->P_Reproduction /= TempTotal_P;
}

int Select_Individual()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  个体索引,若为找到，返回错误代码
**  Purpose: 轮盘赌随机从当前总群筛选出一个杂交对象
**--------------------------------------------------------------*/
{
	double selection_P = genrand_real1(); /* 随机生成一个[0,1]随机数 */
	double distribution_P = 0.0;

	for (int i = 0; i < Num_group; i++)
	{
		distribution_P += Groups[i]->P_Reproduction;
		if (selection_P < distribution_P)
			return i;
	}

	return 422;
}

int* Find_Mother_Index(Solution* Father, Solution* Mother)
/*--------------------------------------------------------------
**  Input:   Father: 父代个体, Mother: 母代个体
**  Output:  母代在父代对应变量的索引数组指针
**  Purpose: 返回母代在父代对应变量的索引
**--------------------------------------------------------------*/
{
	int i = 0,j;
	int *Mother_index = (int*)calloc(Chrom_length, sizeof(int));

	Mother->SerialSchedule->current = Mother->SerialSchedule->head;
	while (Mother->SerialSchedule->current != NULL)
	{
		j = 0;
		Father->SerialSchedule->current = Father->SerialSchedule->head;
		while (Father->SerialSchedule->current != NULL)
		{
			if ((Father->SerialSchedule->current->index == Mother->SerialSchedule->current->index)
				&& (Father->SerialSchedule->current->type == Mother->SerialSchedule->current->type))
			{
				Mother_index[i] = j;
				break;
			}
			j++;
			Father->SerialSchedule->current = Father->SerialSchedule->current->next;
		}

		i++;
		Mother->SerialSchedule->current = Mother->SerialSchedule->current->next;
	}
	
	return Mother_index;
}

int Get_Conflict_Length(int* Detection_Cross, int* Model_Cross, int Length_Cross)
/*--------------------------------------------------------------
**  Input:   Detection_Cross:当前搜索的个体, 即找冲突的对象
**			 Model_Cross: 另一父代个体
**			 Length_Cross:交叉的变量数量
**  Output:  产生冲突的变量数量
**  Purpose: 找到Father_Cross和Mother_cross中产生冲突的变量数量
**--------------------------------------------------------------*/
{
	int Conflict_Length = 0;
	int flag_Conflict;
	for (int i = 0; i < Length_Cross; i++)
	{
		flag_Conflict = 1;  // 判断是否属于冲突  
		for (int j = 0; j < Length_Cross; j++)
		{
			if (Detection_Cross[i] == Model_Cross[j])
			{
				// 结束第二层循环  
				j = Length_Cross;
				flag_Conflict = 0;  // 该城市不属于冲突  
			}
		}
		if (flag_Conflict)
		{
			Conflict_Length++;
		}
	}
	return Conflict_Length;
}

int *Get_Conflict(int* Detection_Cross, int* Model_Cross, int Length_Cross, int Length_Conflict) 
/*--------------------------------------------------------------
**  Input:   Detection_Cross:当前搜索的个体, 即找冲突的对象
**			 Model_Cross: 另一父代个体
**			 Length_Cross:交叉的变量数量
**			 Length_Conflict: 冲突的变量数量
**  Output:  Error code
**  Purpose: 找到Father_Cross和Mother_cross中产生冲突的变量
**--------------------------------------------------------------*/
{
	int count = 0; /* 计数器 */
	int flag_Conflict; /* 冲突标志, 0:不冲突, 1: 冲突 */
	int *Conflict = (int *)calloc(Length_Conflict, sizeof(int));
	
	for (int i = 0; i < Length_Cross; i++)
	{
		flag_Conflict = 1;  // 判断是否属于冲突  
		for (int j = 0; j < Length_Cross; j++)
		{
			if (Detection_Cross[i] == Model_Cross[j])
			{
				// 结束第二层循环  
				j = Length_Cross;
				flag_Conflict = 0;  // 该城市不属于冲突  
			}
		}
		if (flag_Conflict)
		{
			Conflict[count] = Detection_Cross[i];
			count++;
		}
	}
	return Conflict;
}

Solution* Handle_Conflict(int* ConflictSolution, PDecision_Variable*pointer, int *Detection_Conflict, int *Model_Conflict, int Length_Conflict)
/*--------------------------------------------------------------
**  Input:   ConflictSolution:需要解决的冲突对象
**			 pointer: 存储父代个体流程操作指针
**			 Detection_Conflict: 存储冲突位置的父代个体
**			 Model_Conflict: 存储冲突位置的母代个体
**			 Length_Conflict: 冲突的变量数量
**  Output:  处理好的子代个体指针
**  Purpose: 处理冲突子代个体
**--------------------------------------------------------------*/
{
	int errcode = 0;
	int flag; /* 冲突标识 */
	int index;
	int temp=0; /* 临时变量 */
	PDecision_Variable ptr; /* 临时变量 */
	Solution* Offspring;
	LinkedList* p;	/* 临时结构体指针 */
	/* 初始化相关参数 */
	Offspring = (Solution*)calloc(1, sizeof(Solution));
	p = (LinkedList*)calloc(1, sizeof(LinkedList));
	ERR_CODE(MEM_CHECK(Offspring));	if (errcode) {fprintf(ErrFile, ERR402);}
	ERR_CODE(MEM_CHECK(p));	if (errcode) {fprintf(ErrFile, ERR402);}
	Offspring->C_01 = 0; Offspring->C_02 = 0; Offspring->C_03 = 0;
	Offspring->C_04 = 0; Offspring->C_05 = 0; Offspring->C_06 = 0;
	Offspring->P_Reproduction = 0; Offspring->objvalue = 0;
	p->head = NULL; p->tail = NULL; p->current = NULL;

	for (int i = 0; i < Length_Conflict; i++)
	{
		flag = 0;
		index = 0;

		/* [0, IndexCross_i) 寻找冲突 */
		for (index = 0; index < IndexCross_i; index++)
		{
			if (Model_Conflict[i] == ConflictSolution[index])
			{
				flag = 1;
				break;
			}
		}
		/* 第一段没找到, 找剩余的部分(除了交换的基因段外) */
		if (!flag)
		{
			/* [IndexCross_i + 1, Chrom_length) 寻找冲突 */
			for (index = IndexCross_j + 1; index < Chrom_length; index++)
			{
				if (Model_Conflict[i] == ConflictSolution[index])
					break;
			}
		}
		ConflictSolution[index] = Detection_Conflict[i];
	}

	for (int i = 0; i < Chrom_length; i++)
	{
		if (pointer[ConflictSolution[i]]->type != _Repair)
		{
			for (int j = i+1; j < Chrom_length; j++)
			{
				if ((pointer[ConflictSolution[i]]->index == pointer[ConflictSolution[j]]->index)
					&& (pointer[ConflictSolution[j]]->type != _Repair))
				{
					if (pointer[ConflictSolution[i]]->type > pointer[ConflictSolution[j]]->type)
					{
						temp= ConflictSolution[i];
						ConflictSolution[i] = ConflictSolution[j];
						ConflictSolution[j] = temp;
					}
					break;
				}
			}
		}
	}

	/* 编码转换为子代个体 */
	for (int i = 0; i < Chrom_length; i++)
	{
		ptr = pointer[ConflictSolution[i]];
		ERR_CODE(Add_tail(p,ptr->index,ptr->type,0,0));
		if (errcode) {fprintf(ErrFile, ERR423);}
	}
	/*  将SerialSchedule链表中的所有指令分配至每个工程队 */
	Offspring->SerialSchedule = p;
	ERR_CODE(Task_Assignment(Offspring->SerialSchedule,Offspring->Schedule));

	return Offspring;
}

int GA_Cross(Solution* Father, Solution* Mother)
/*--------------------------------------------------------------
**  Input:   Father: 父代个体, Mother: 母代个体
**  Output:  Error code
**  Purpose: GA交叉操作，从两个个体中产生一个新个体
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0; /* 错误编码 */
	int temp;	/* 临时变量 */
	int Length_Cross;    /* 交叉的个数 */
	int *Father_index, *Mother_index;
	PDecision_Variable* Father_pointer;
	int *Father_cross, *Mother_cross; /* 交叉基因段 */
	int *Conflict_Father, *Conflict_Mother;  /* 存储冲突的位置 */   
	int Length_Conflict;	/* 冲突的个数 */ 

	/* 分配内存 */
	Father_index = (int*)calloc(Chrom_length, sizeof(int));
	Father_pointer = (PDecision_Variable*)calloc(Chrom_length, sizeof(PDecision_Variable));
	ERR_CODE(MEM_CHECK(Father_index));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(Father_pointer));	if (errcode) err_count++;
	for (int i = 0; i < Chrom_length; i++)
		Father_index[i] = i;
	Mother_index = Find_Mother_Index(Father, Mother);

	temp = 0;
	Father->SerialSchedule->current = Father->SerialSchedule->head;
	while (Father->SerialSchedule->current != NULL)
	{
		Father_pointer[temp] = Father->SerialSchedule->current;
		temp++;
		Father->SerialSchedule->current = Father->SerialSchedule->current->next;
	}

/* 随机产生交叉位置，保证 IndexCross_i < IndexCross_j */
	IndexCross_i = (int)floor(genrand_real1()*(Chrom_length-0.0001));
	IndexCross_j = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	while ((IndexCross_i == IndexCross_j) || abs(IndexCross_i - IndexCross_j) == (Chrom_length - 1))
	{
		IndexCross_j = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	}
	if (IndexCross_i > IndexCross_j)
	{
		temp = IndexCross_i;
		IndexCross_i = IndexCross_j;
		IndexCross_j = temp;
	}

	/* 交叉基因段 */
	Length_Cross = IndexCross_j - IndexCross_i+1; /* 交叉的变量数量 */
	Father_cross = (int*)calloc(Length_Cross, sizeof(int)); /* 父代遗传基因段 */
	Mother_cross = (int*)calloc(Length_Cross, sizeof(int)); /* 母代遗传基因段 */
	ERR_CODE(MEM_CHECK(Father_cross));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(Father_cross));	if (errcode) err_count++;
	temp = 0;
	for (int i = IndexCross_i; i <= IndexCross_j; i++)
	{
		Father_cross[temp] = Father_index[i];
		Mother_cross[temp] = Mother_index[i];
		temp++;
	}

	// 开始交叉 - 找到Father_Cross和Mother_cross中产生冲突的变量
	Length_Conflict = Get_Conflict_Length(Father_cross, Mother_cross, Length_Cross);
	Conflict_Father = Get_Conflict(Father_cross, Mother_cross, Length_Cross, Length_Conflict);
	Conflict_Mother = Get_Conflict(Mother_cross, Father_cross, Length_Cross, Length_Conflict);

	/*  Father and Mother 交换基因段 */
	for (int i = IndexCross_i; i <= IndexCross_j; i++)
	{
		temp = Father_index[i];
		Father_index[i] = Mother_index[i];
		Mother_index[i] = temp;
	}

	/* 解决父代和母代交叉后的冲突问题 */
	Solution* Descendant_ONE = Handle_Conflict(Father_index, Father_pointer, Conflict_Father, Conflict_Mother, Length_Conflict);
	Solution* Descendant_TWO = Handle_Conflict(Mother_index, Father_pointer, Conflict_Mother, Conflict_Father, Length_Conflict);

	Offspring[Length_SonSoliton++] = Descendant_ONE;
	Offspring[Length_SonSoliton++] = Descendant_TWO;

	/* 释放内存 */
	SafeFree(Father_index);
	SafeFree(Mother_index);
	SafeFree(Father_pointer);
	SafeFree(Father_cross);
	SafeFree(Mother_cross);
	SafeFree(Conflict_Father);
	SafeFree(Conflict_Mother);

	if (err_count)
		errcode = 424;
	return errcode;
}

int GA_Variation(int Index_Offspring)
/*--------------------------------------------------------------
**  Input:   Index_Offspring: 后代个体索引
**  Output:  Error code
**  Purpose: 对后代进行变异操作
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0; /* 错误编码 */
	int temp;	/* 临时变量 */
	int IndexVariation_i, IndexVariation_j;
	int *Index;
	PDecision_Variable* Index_pointer;
	LinkedList* p;	/* 临时结构体指针 */
	Solution* Offptr;
	/* 初始化相关参数 */
	Offptr = (Solution*)calloc(1, sizeof(Solution));
	p = (LinkedList*)calloc(1, sizeof(LinkedList));
	ERR_CODE(MEM_CHECK(Offptr));	if (errcode) {fprintf(ErrFile, ERR402);}
	ERR_CODE(MEM_CHECK(p));	if (errcode) {fprintf(ErrFile, ERR402);}
	Offptr->C_01 = 0; Offptr->C_02 = 0; Offptr->C_03 = 0;
	Offptr->C_04 = 0; Offptr->C_05 = 0; Offptr->C_06 = 0;
	Offptr->P_Reproduction = 0; Offptr->objvalue = 0;
	p->head = NULL; p->tail = NULL; p->current = NULL;

	/* 分配内存 */
	Index = (int*)calloc(Chrom_length, sizeof(int));
	Index_pointer = (PDecision_Variable*)calloc(Chrom_length, sizeof(PDecision_Variable));
	ERR_CODE(MEM_CHECK(Index));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(Index_pointer));	if (errcode) err_count++;
	for (int i = 0; i < Chrom_length; i++)
		Index[i] = i;

	temp = 0;
	Offspring[Index_Offspring]->SerialSchedule->current = Offspring[Index_Offspring]->SerialSchedule->head;
	while (Offspring[Index_Offspring]->SerialSchedule->current != NULL)
	{
		Index_pointer[temp] = Offspring[Index_Offspring]->SerialSchedule->current;
		temp++;
		Offspring[Index_Offspring]->SerialSchedule->current = Offspring[Index_Offspring]->SerialSchedule->current->next;
	}

	/* 随机产生两个随机数表示两个变异的位置, 并进行位置交换 */
	IndexVariation_i = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	IndexVariation_j = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	while ((IndexVariation_i == IndexVariation_j))
	{
		IndexVariation_j = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	}

	/* 交换位置 */
	temp = Index[IndexVariation_i];
	Index[IndexVariation_i] = Index[IndexVariation_j];
	Index[IndexVariation_j] = temp;

	for (int i = 0; i < Chrom_length; i++)
	{
		if (Index_pointer[Index[i]]->type != _Repair)
		{
			for (int j = i + 1; j < Chrom_length; j++)
			{
				if ((Index_pointer[Index[i]]->index == Index_pointer[Index[j]]->index)
					&& (Index_pointer[Index[j]]->type != _Repair))
				{
					if (Index_pointer[Index[i]]->type > Index_pointer[Index[j]]->type)
					{
						temp = Index[i];
						Index[i] = Index[j];
						Index[j] = temp;
					}
					break;
				}
			}
		}
	}
	/* 编码转换为子代个体 */
	for (int i = 0; i < Chrom_length; i++)
	{
		ERR_CODE(Add_tail(p, Index_pointer[Index[i]]->index, Index_pointer[Index[i]]->type, 0, 0));
		if (errcode) {fprintf(ErrFile, ERR423);}
	}
	/*  将SerialSchedule链表中的所有指令分配至每个工程队 */
	Offptr->SerialSchedule = p;
	ERR_CODE(Task_Assignment(Offptr->SerialSchedule, Offptr->Schedule));

	Free_Solution(Offspring[Index_Offspring]);
	Offspring[Index_Offspring] = Offptr;

	SafeFree(Index);
	SafeFree(Index_pointer);

	if (err_count)
		errcode = 425;

	return errcode;
}

void Clone_Group(Solution** group, Solution** son)
/*--------------------------------------------------------------
**  Input:  group:种群个体指针, son: 后代个体指针
**  Output:  None
**  Purpose: 对两个个体进行复制操作
**--------------------------------------------------------------*/
{
	Solution* temp;
	temp = *group;
	*group = *son;
	*son = temp;
}

void BestSolution()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  None
**  Purpose: 查找最优解
**--------------------------------------------------------------*/
{
	int index = 0;
	double Optimalvalue;
	Optimalvalue = Groups[0]->objvalue;
	for (int i = 1; i < Num_group; i++)
	{
		if (Optimalvalue > Groups[i]->objvalue)
		{
			Optimalvalue = Groups[i]->objvalue;
			index = i;
		}
	}

	/* 打印 Schedule 结构体数值 */
	for (int j = 0; j < MAX_CREWS; j++)
	{
		fprintf(TemSolution,"Schedule[%d]:\n", j);
		Groups[index]->Schedule[j].current = Groups[index]->Schedule[j].head;
		while (Groups[index]->Schedule[j].current != NULL)
		{
			fprintf(TemSolution,"index: %d	type: %d	starttime: %d	endtime: %d\n",
				Groups[index]->Schedule[j].current->pointer->index, Groups[index]->Schedule[j].current->pointer->type,
				Groups[index]->Schedule[j].current->pointer->starttime, Groups[index]->Schedule[j].current->pointer->endtime);
			Groups[index]->Schedule[j].current = Groups[index]->Schedule[j].current->next;
		}
		fprintf(TemSolution, "\n");
	}
	/* 打印BestSolution */
	fprintf(TemSolution, "C_01= %d, C_02= %d, C_03= %f, C_04= %f, C_05= %d, C_06= %f, Sum= %f\n",
		Groups[index]->C_01, Groups[index]->C_02, Groups[index]->C_03, Groups[index]->C_04, Groups[index]->C_05,
		Groups[index]->C_06, Groups[index]->objvalue);
	fprintf(TemSolution, "\n***********************************\n");

}

void GA_UpdateGroup()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  None
**  Purpose: 更新种群
**--------------------------------------------------------------*/
{
	Solution* tempSolution;
	/* 先对子代 - Offspring 依据适应度值进行排序 - 降序[按目标值从小到大] */  
	for (int i = 0; i < Num_offs; i++)
	{
		for (int j = Num_offs - 1; j > i; j--)
		{
			if (Offspring[i]->objvalue > Offspring[j]->objvalue)
			{
				tempSolution = Offspring[i];
				Offspring[i] = Offspring[j];
				Offspring[j] = tempSolution;
			}
		}
	}

	/* 更新种群 */
	for (int i = 0; i < Num_offs; i++)
	{
		for (int j = 0; j < Num_group; j++)
		{
			if (Offspring[i]->objvalue < Groups[j]->objvalue)
			{
				Clone_Group(&Groups[j], &Offspring[i]);
				break;
			}
		}
	}

	/* 子代删除 */
	for(int i=0;i<Num_offs;i++)
		Free_Solution(Offspring[i]);

	BestSolution();
}

int GA_Evolution()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  Error code;
**  Purpose: GA主循环过程
**--------------------------------------------------------------*/
{
	int errcode = 0, err_sum = 0;	/* 错误代码 */
	int iter = 0;	/* 迭代次数计数器 */
	int M;	/* 交叉次数 */
	int Father_index, Mother_index;	/* 父代、母代个体索引 */
	double Is_Crossover;	/* 交叉概率随机数 */
	double Is_mutation;		/* 变异概率随机数 */

	while (iter < Num_iteration)
	{
		fprintf(TemSolution, "Iteration: %d\n", iter + 1); /* 写入结果报告文件 */
		printf("********Iteration: %d / %d **********\n", iter + 1, Num_iteration); /* 输出至控制台 */
		
		/* 1.选择 */
		Father_index = Select_Individual();
		Mother_index = Select_Individual();

		/* 防止Father和Mother都是同一个个体 -> 自交( 父母为同一个个体时, 母亲重新选择, 直到父母为不同的个体为止 ) */
		while (Mother_index == Father_index)
		{
			Mother_index = Select_Individual();
		}

		/* 2.交叉, 存储在全局变脸 Offspring[] 数组 - 通过M次杂交, 产生2M个新个体, 2M >= Num_group */
		M = Num_group - Num_group / 2;
		Length_SonSoliton = 0;	/* 遗传产生的个体个数, 置零重新累加 */

		while (M)
		{
			Is_Crossover = genrand_real1();
			if (Is_Crossover <= P_crossover)
			{
				ERR_CODE(GA_Cross(Groups[Father_index], Groups[Mother_index]));
				if (errcode) err_sum++;
				M--;
			}
		}

		/* 3.变异, 针对 Offspring[] 数组 */
		//total_objvalue = 0.0;

		for (int IndexVariation = 0; IndexVariation < Length_SonSoliton; IndexVariation++)
		{
			Is_mutation = genrand_real1();
			if (Is_mutation <= P_mutation)
			{
				ERR_CODE(GA_Variation(IndexVariation));
				if (errcode) err_sum++;
			}

			/*  经过变异处理后,重新计算适应度值  */
			ERR_CODE(Calculate_Objective_Value(Offspring[IndexVariation]));
			if (errcode) err_sum++;

			//total_objvalue += Offspring[IndexVariation]->objvalue;
			printf("Complited: %d / %d\n", IndexVariation + 1, Length_SonSoliton);
		}
		
		/* 4.更新个体, 参与对象: 父代+子代 */
		GA_UpdateGroup();
		Calc_Probablity();
		iter++;
	}

	if (err_sum)
		errcode = 426;
	return errcode;
}



#define _GA
#ifdef _GA

int main(void)
{
	int errcode = 0;	/* 错误编码 */ 
	inpfile = "BBM_Scenario1.inp";	/* 水力模型inp文件指针 */
	time_t T_begin = clock();

	/* 读取data.txt数据 */
	ERR_CODE(readdata("data.txt", "err.txt"));
	if (errcode) fprintf(ErrFile, ERR406);

	/* 获取各节点每个时间步长(h)的基本需水量 */
	errcode = GetDemand("BBM_EPS.inp");
	if (errcode) fprintf(ErrFile, ERR412);

	/* 打开inp文件 */
	Open_inp_file(inpfile, "report.rpt", "");

	/* 获取爆管/漏损管道喷射节点索引、喷射系数、管道索引; 医院及消火栓节点、管道索引 */
	Get_FailPipe_keyfacility_Attribute();
	/* 关闭水力模型文件 */
	ERR_CODE(ENclose());	if (errcode > 100) fprintf(ErrFile, ERR420);;

	/* 生成可见爆管/漏损相关信息，供随机选择函数调用 */
	ERR_CODE(Get_Select_Repository());
	if (errcode) fprintf(ErrFile, ERR416);

	/* 为初始解分配内存 */
	ERR_CODE(Memory_Allocation());
	if (errcode) fprintf(ErrFile, ERR417);

	/* 打开最优解存储文件 */
	if ((TemSolution = fopen("TemSolution.txt", "wt")) == NULL)
	{
		printf("Can not open the TemSolution.txt!\n");
		assert(0); //终止程序，返回错误信息
	}
	
	/* 种群初始化 */
	ERR_CODE(InitialGroups());
	if (errcode) fprintf(ErrFile, ERR418);

	/* GA主循环过程 */
	ERR_CODE(GA_Evolution());
	if (errcode) fprintf(ErrFile, ERR426);

	/*释放内存*/
	//Emptymemory();
	//Free_GAmemory();
	fclose(InFile);
	fclose(ErrFile);
	fclose(TemSolution);
	time_t Tend = clock();

	getchar();
	return 0;
}

#endif