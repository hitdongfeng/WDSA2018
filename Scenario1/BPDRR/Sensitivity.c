/*******************************************************************
Name: Sensitivity.c
Purpose: 对受损管道进行灵敏度分析，用于确定受损管道重要程度
Data: 5/3/2018
Author: Qingzhou Zhang
Email: wdswater@gmail.com
********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "wdstext.h"
#include "wdstypes.h"
#include "wdsfuns.h"
#include "epanet2.h"
#define EXTERN extern
//#define EXTERN 
#include "wdsvars.h"

void Open_inp_file(char *f1, char *f2, char *f3)
/**----------------------------------------------------------------
**  输入:  f1: .inp文件指针
		  f2: .rpt文件指针
		  f3: .out文件指针
**  输出:  无
**  功能:  打开.inp文件.
**----------------------------------------------------------------*/
{
	int errcode = 0;
	ERR_CODE(ENopen(f1, f2, f3));
	if (errcode)
	{
		fprintf(ErrFile, ERR407);
	}
}

int GetDemand(char* f1)
/**----------------------------------------------------------------
**  输入:  f1:inp文件指针;
**  输出:  Error code
**  功能:  获取所有时间步长(小时)需水量节点实际需水量
**----------------------------------------------------------------*/
{
	int s;
	int errcode = 0, errsum = 0;
	long t, tstep;		/* t: 当前时刻; tstep: 水力计算时间步长 */
	float demand;		/* 临时变量，用于存储泄流量 */

	/* run epanet analysis engine */
	Open_inp_file(f1, "BBM_EPS.rpt", "");
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */
	ERR_CODE(ENopenH());	if (errcode > 100) errsum++;	/* Opens the hydraulics analysis system. */
	ERR_CODE(ENinitH(0));	if (errcode > 100) errsum++;	/* Don't save the hydraulics file */

	do 
	{
		ERR_CODE(ENrunH(&t)); if (errcode>100) errsum++;
		s = (t / 3600) % 24; //当前时刻所对应的时段
		if ((t % 3600 == 0) && (t % 3600 < Pattern_length)) /* Begin the restoration */
		{
			for (int i = 0; i < Ndemands; i++)
			{
				ERR_CODE(ENgetnodevalue(i+1, EN_DEMAND, &demand));
				if (errcode > 100) errsum++;
				ActuralBaseDemand[i][s]= demand;
			}
		}
		ERR_CODE(ENnextH(&tstep));	if (errcode>100) errsum++;
	} while (tstep>0);
	
	ERR_CODE(ENcloseH());	if (errcode > 100) errsum++;
	ERR_CODE(ENclose());	if (errcode > 100) errsum++;

	if (errsum > 0) errcode = 412;
	return errcode;
}

void Get_FailPipe_keyfacility_Attribute()
/**----------------------------------------------------------------
**  输入:  无
**  输出:  无
**  功能:  获取受损管道和关键基础设置(医院、消火栓)相关属性值
**----------------------------------------------------------------*/
{
	int errcode = 0;
	int nodeindex, pipeindex1, pipeindex2, pipeindex3;
	float emitter;

	for (int i = 0; i < Nbreaks; i++)
	{
		ERR_CODE(ENgetnodeindex(BreaksRepository[i].nodeID, &nodeindex));
		if (errcode) fprintf(ErrFile, ERR408, BreaksRepository[i].nodeID);
		BreaksRepository[i].nodeindex = nodeindex;

		ERR_CODE(ENgetlinkindex(BreaksRepository[i].pipeID, &pipeindex1));
		if (errcode) fprintf(ErrFile, ERR409, BreaksRepository[i].pipeID);
		BreaksRepository[i].pipeindex = pipeindex1;

		ERR_CODE(ENgetlinkindex(BreaksRepository[i].flowID, &pipeindex2));
		if (errcode) fprintf(ErrFile, ERR409, BreaksRepository[i].flowID);
		BreaksRepository[i].flowindex = pipeindex2;

		ERR_CODE(ENgetnodevalue(nodeindex, EN_EMITTER, &emitter));
		if (errcode) fprintf(ErrFile, ERR410);
			
		BreaksRepository[i].emittervalue = emitter;

		for (int j = 0; j < BreaksRepository[i].num_isovalve; j++)
		{
			ERR_CODE(ENgetlinkindex(BreaksRepository[i].pipes[j].pipeID, &pipeindex3));
			if (errcode) fprintf(ErrFile, ERR408, BreaksRepository[i].pipes[j].pipeID);
			BreaksRepository[i].pipes[j].pipeindex = pipeindex3;
		}
	}
	
	for (int i = 0; i < Nleaks; i++)
	{
		ERR_CODE(ENgetnodeindex(LeaksRepository[i].nodeID, &nodeindex));
		if (errcode) fprintf(ErrFile, ERR408, LeaksRepository[i].nodeID);
			
		LeaksRepository[i].nodeindex = nodeindex;

		ERR_CODE(ENgetlinkindex(LeaksRepository[i].pipeID, &pipeindex1));
		if (errcode) fprintf(ErrFile, ERR409,LeaksRepository[i].pipeID);
		LeaksRepository[i].pipeindex = pipeindex1;

		ERR_CODE(ENgetlinkindex(LeaksRepository[i].flowID, &pipeindex2));
		if (errcode) fprintf(ErrFile, ERR409, LeaksRepository[i].flowID);
		LeaksRepository[i].flowindex = pipeindex2;

		ERR_CODE(ENgetnodevalue(nodeindex, EN_EMITTER, &emitter));
		if (errcode) fprintf(ErrFile, ERR410);
			
		LeaksRepository[i].emittervalue = emitter;
	}

	for (int i = 0; i < Nhospital; i++)
	{
		ERR_CODE(ENgetnodeindex(Hospitals[i].nodeID, &nodeindex)); 
		if (errcode) fprintf(ErrFile, ERR408, Hospitals[i].nodeID);

		ERR_CODE(ENgetlinkindex(Hospitals[i].pipeID, &pipeindex1)); 
		if (errcode) fprintf(ErrFile, ERR409, Hospitals[i].pipeID);

		Hospitals[i].nodeindex = nodeindex;
		Hospitals[i].pipeindex = pipeindex1;
	}

	for (int i = 0; i < Nfirefight; i++)
	{
		ERR_CODE(ENgetlinkindex(Firefighting[i].ID, &pipeindex1));
		if (errcode)
		{
			fprintf(ErrFile, ERR409, Firefighting[i].ID);
			break;
		}
		Firefighting[i].index = pipeindex1;
	}
}

int Visible_Damages_initial(long time)
/**----------------------------------------------------------------
**  输入:  time 模拟时刻
**  输出:  Error code
**  功能:  获取模拟开始时可见爆管或漏损管道信息
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum=0;
	long t, tstep;		/* t: 当前时刻; tstep: 水力计算时间步长 */ 
	float flow;	/* 临时变量，用于存储泄流量 */
	

	
	/* 设置epanet计算选项，报告状态信息将耗费大量时间，因此，不输出状态信息 */
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */

	/* run epanet analysis engine */
	ERR_CODE(ENopenH());	if (errcode>100) errsum++;	/* Opens the hydraulics analysis system. */
	ERR_CODE(ENinitH(0));	if (errcode>100) errsum++;	/* Don't save the hydraulics file */

	do 
	{
		ERR_CODE(ENrunH(&t)); if (errcode>100) errsum++;
		if (t == time) /* Begin the restoration */
		{
			/* 遍历所有爆管 */
			for (int i = 0; i < Nbreaks; i++)
			{
				ERR_CODE(ENgetlinkvalue(BreaksRepository[i].flowindex, EN_FLOW, &flow));
				if (BreaksRepository[i].pipediameter >= 150 || flow > 2.5)
					errcode = Add_tail(&IniVisDemages, i, _Break,0,0);
				if (errcode>100)	errsum++;
			}

			/* 遍历所有漏损管道 */
			for (int i = 0; i < Nleaks; i++)
			{
				ERR_CODE(ENgetlinkvalue(LeaksRepository[i].flowindex, EN_FLOW, &flow));
				if (LeaksRepository[i].pipediameter >= 300 || flow > 2.5)
					errcode = Add_tail(&IniVisDemages,i, _Leak,0, 0);
				if (errcode>100)	errsum++;
			}
		}
		ERR_CODE(ENnextH(&tstep));	if (errcode>100) errsum++;
	} while (tstep>0);
	ERR_CODE(ENcloseH());	if (errcode>100) errsum++;

	if (errsum > 0) errcode = 411;
	return errcode;
}

int Breaks_Adjacent_operation(int type, int index,int code, float status,float emitter)
/**----------------------------------------------------------------
**  输入:  type:类型: 1.Isolate, 4.Reopen
**		  index 爆管在仓库中的索引(以0开始)
**		  code: link parameter code (EN_INITSTATUS,EN_STATUS)
**		  status 管道初始状态, 0:关闭; 1:开启
**		  emitter 爆管节点喷射系数
**  输出:  Error code
**  功能:  关闭或开启爆管附近管道，对爆管进行隔离或复原
**----------------------------------------------------------------*/
{
	int errcode=0, errsum = 0;
	
	//float STATUS,EMITTER;//

	for (int i = 0; i < BreaksRepository[index].num_isovalve; i++)
	{
		ERR_CODE(ENsetlinkvalue(BreaksRepository[index].pipes[i].pipeindex, code, status));
		if (errcode)	errsum++;

		//ERR_CODE(ENgetlinkvalue(BreaksRepository[index].pipes[i].pipeindex, EN_STATUS, &STATUS));//
		//printf("pipeID: %s, status: %f\n", BreaksRepository[index].pipes[i].pipeID, STATUS);//
	}
	ERR_CODE(ENsetnodevalue(BreaksRepository[index].nodeindex, EN_EMITTER, emitter));
	if (errcode)	errsum++;
	//ERR_CODE(ENgetnodevalue(BreaksRepository[index].nodeindex, EN_EMITTER, &EMITTER));//
	//printf("nodeID: %s, emitter: %f\n", BreaksRepository[index].nodeID, EMITTER);//

	if (type == _Reopen)
	{
		ERR_CODE(ENsetlinkvalue(BreaksRepository[index].pipeindex, code, status));
		if (errcode)	errsum++;

		//ERR_CODE(ENgetlinkvalue(BreaksRepository[index].pipeindex, EN_STATUS, &STATUS));//
		//printf("pipeID: %s, status: %f\n", BreaksRepository[index].pipeID, STATUS);//
	}

	if (errsum) errcode = 415;

	return errcode;
}

int Leaks_operation(int index, int code, float status, float emitter)
/**----------------------------------------------------------------
**  输入:  index 爆管在仓库中的索引(以0开始)
**		  code: link parameter code (EN_INITSTATUS,EN_STATUS)
**		  status 管道初始状态, 0:关闭; 1:开启
**		  emitter 漏损节点喷射系数
**  输出:  Error code
**  功能:  修复漏损管道，对漏损管道进行复原
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum = 0; /* 错误编码 */

	//float STATUS, EMITTER;//

	ERR_CODE(ENsetlinkvalue(LeaksRepository[index].pipeindex, code, status));
	if (errcode)	errsum++;
	//ERR_CODE(ENgetlinkvalue(LeaksRepository[index].pipeindex, EN_STATUS, &STATUS));//
	//printf("pipeID: %s, status: %f\n", LeaksRepository[index].pipeID, STATUS);//


	ERR_CODE(ENsetnodevalue(LeaksRepository[index].nodeindex, EN_EMITTER, emitter));
	if (errcode)	errsum++;
	//ERR_CODE(ENgetnodevalue(LeaksRepository[index].nodeindex, EN_EMITTER, &EMITTER));//
	//printf("nodeID: %s, emitter: %f\n", LeaksRepository[index].nodeID, EMITTER);//

	if (errsum) errcode = 421;

	return errcode;
}

Sercapacity* GetSerCapcity(long time)
/**----------------------------------------------------------------
**  输入:  time: 模拟时刻
**  输出:  Error code
**  功能:  计算当前时刻系统供水能力
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum = 0;
	int facility_count = 0;
	int s;
	float x;
	double sumpdddemand = 0.0, sumbasedemand = 0.0;
	double y, meankeyfac = 0.0;
	Sercapacity* ptr = (Sercapacity*)calloc(1, sizeof(Sercapacity));
	ERR_CODE(MEM_CHECK(ptr));	if (errcode) errsum++;

	s = (time / 3600) % 24; //当前时刻所对应的时段
	for (int i = 0; i < Ndemands; i++)
	{
		ERR_CODE(ENgetlinkvalue(i + Start_pipeindex, EN_FLOW, &x));
		if (errcode > 100) errsum++;
		
		sumpdddemand += (double)x;
		sumbasedemand += (double)ActuralBaseDemand[i][s];
	}
	ptr->Functionality = sumpdddemand / sumbasedemand;
	for (int i = 0; i <Nhospital; i++)
	{
		ERR_CODE(ENgetlinkvalue(Hospitals[i].pipeindex, EN_FLOW, &x));
		if (errcode > 100) errsum++;

		y = (double)(x / ActuralBaseDemand[Hospitals[i].nodeindex-1][s]);
		if (y > 0.5)
			facility_count++;

		meankeyfac += y;
	}

	for (int i = 0; i < Nfirefight; i++)
	{
		ERR_CODE(ENgetlinkvalue(Firefighting[i].index, EN_FLOW, &x));
		if (errcode > 100) errsum++;

		y = (double)(x / Firefighting[i].fire_flow);
		if (y > 0.5)
			facility_count++;

		meankeyfac += y;
	}

	ptr->MeankeyFunc = meankeyfac / (double)(Nhospital + Nfirefight);
	ptr->Numkeyfac = facility_count;
	ptr->time = time;
	ptr->next = NULL;

	if (errsum > 0)	fprintf(ErrFile, ERR413);

	return ptr;
	
}

int GetSerCapcPeriod(long starttime, long endtime)
/**----------------------------------------------------------------
**  输入:  starttime:起始时刻; endtime:终止时刻
**  输出:  Error code
**  功能:  计算指定时段内每个模拟步长系统的供水能力
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum = 0;
	long t, tstep;	/* t: 当前时刻; tstep: 水力计算时间步长 */
	Sercapacity* ptr;

	/* 设置epanet计算选项，报告状态信息将耗费大量时间，因此，不输出状态信息 */
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */
	/* run epanet analysis engine */
	ERR_CODE(ENopenH());	if (errcode > 100) errsum++;	/* Opens the hydraulics analysis system. */
	ERR_CODE(ENinitH(0));	if (errcode > 100) errsum++;	/* Don't save the hydraulics file */
	do
	{
		ERR_CODE(ENrunH(&t)); if (errcode > 100) errsum++;
		if ((t >= starttime && t <= endtime) && (t % Time_Step == 0))
		{
			ptr = GetSerCapcity(t);
			Add_SerCapcity_list(&SerCapcPeriod, ptr);
		}

		ERR_CODE(ENnextH(&tstep));	if (errcode > 100) errsum++;
	} while (tstep > 0);
	ERR_CODE(ENcloseH());	if (errcode > 100) errsum++;

	if (errsum)	errcode = 414;

	return errcode;
}

int SensitivityAnalysis(long starttime,long endtime)
/**----------------------------------------------------------------
**  输入:  time: 模拟时刻
**  输出:  Error code
**  功能:  对受损可见管道进行灵敏度分析
**----------------------------------------------------------------*/
{
	int count = 0;
	int errcode = 0, errsum=0;
	int index, type;
	float emitter;

	IniVisDemages.current = IniVisDemages.head;
	while (IniVisDemages.current != NULL)
	{
		/* run epanet analysis engine */
		ENsetstatusreport(0);		/* No Status reporting */
		ENsetreport("MESSAGES NO"); /* No Status reporting */
		index = IniVisDemages.current->index;
		type = IniVisDemages.current->type;

		if (type == _Break)
		{
			ERR_CODE(Breaks_Adjacent_operation(_Isolate,index, EN_INITSTATUS,0,0));
			ERR_CODE(GetSerCapcPeriod(starttime, endtime));

			/* 打印每个模拟步长的供水能力计算结果 */
			fprintf(SenAnalys, "BreakPipeId: %s BreakPipeindex: %d\n", BreaksRepository[index].pipeID, BreaksRepository[index].pipeindex);
			SerCapcPeriod.current = SerCapcPeriod.head;
			while (SerCapcPeriod.current != NULL)
			{
				fprintf(SenAnalys, "time: %d	Functionality: %f	Numkeyfac: %d	MeankeyFunc: %f\n",
					SerCapcPeriod.current->time, SerCapcPeriod.current->Functionality,
					SerCapcPeriod.current->Numkeyfac, SerCapcPeriod.current->MeankeyFunc);

				SerCapcPeriod.current = SerCapcPeriod.current->next;
			}
			fprintf(SenAnalys, "\n");

			/* 释放SerCapcPeriod链表指针 */
			SerCapcPeriod.current = SerCapcPeriod.head;
			while (SerCapcPeriod.current != NULL)
			{
				SerCapcPeriod.head = SerCapcPeriod.head->next;
				SafeFree(SerCapcPeriod.current);
				SerCapcPeriod.current = SerCapcPeriod.head;
			}

			/* 还原管道初始状态 */
				emitter = BreaksRepository[index].emittervalue;
				ERR_CODE(Breaks_Adjacent_operation(_Isolate,index, EN_INITSTATUS, 1, emitter));
				printf("Accumulates number of visible breaks: %d\n", count++);
		}
		IniVisDemages.current = IniVisDemages.current->next;
	}

	if (errsum > 0) errcode = 414;
	return errcode;
}


/*************************************************
** 此main函数用于计算指定时段内每个模拟步长系统的供水能力
** 用于调试函数GetSerCapcPeriod
**************************************************/
//#define GSCP
#ifdef GSCP
int main(void)
{
	int errcode = 0;		//错误编码 
	FILE *file;				//输出结果文件指针
	long starttime = 1800;	//模拟开始时刻(秒)
	long endtime = 86400;	//模拟结束时刻(秒)
	
	if ((Temfile = fopen("Temfile.txt", "wt")) == NULL)
	{
		printf("Can not open the GSCP.txt file!\n");
		assert(0); //终止程序，返回错误信息
	}

	/* 读取data.txt数据 */
	errcode = readdata("data.txt", "err.txt");
	if (errcode) { fprintf(ErrFile, ERR406); return (406); }

	/* 获取各节点每个时间步长(h)的基本需水量 */
	errcode = GetDemand("BBM_EPS.inp");
	if (errcode) { fprintf(ErrFile, ERR412); return (412); }

	/* 打开inp文件 */
	Open_inp_file("BBM_Scenario1.inp", "BBM_Scenario1.rpt", "");

	/* 获取爆管/漏损管道喷射节点索引、喷射系数、管道索引; 医院及消火栓节点、管道索引 */
	Get_FailPipe_keyfacility_Attribute();

	/* 设置epanet计算选项，报告状态信息将耗费大量时间，因此，不输出状态信息 */
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */

	/* 计算指定时段内每个模拟步长系统的供水能力 */
	ERR_CODE(GetSerCapcPeriod(starttime, endtime));
	if (errcode) { fprintf(ErrFile, ERR414); return (414); }
	
	/* 打印每个模拟步长的供水能力计算结果 */
	if ( (file = fopen("GSCP.txt", "wt")) == NULL )
	{
		printf("Can not open the GSCP.txt file!\n");
		assert(0); //终止程序，返回错误信息
	}
	fprintf(file, "Report start time: %d	Report end time: %d\n", starttime, endtime);
	SerCapcPeriod.current = SerCapcPeriod.head;
	while (SerCapcPeriod.current != NULL)
	{
		fprintf(file,"time: %d	Functionality: %f	Numkeyfac: %d	MeankeyFunc: %f\n", 
				SerCapcPeriod.current->time,SerCapcPeriod.current->Functionality,
			  SerCapcPeriod.current->Numkeyfac,SerCapcPeriod.current->MeankeyFunc);
		
		SerCapcPeriod.current = SerCapcPeriod.current->next;
	}

	/* 释放内存 */
	Emptymemory();

	/* 关闭txt文件 */
	fclose(InFile);
	fclose(ErrFile);
	fclose(file);
	fclose(Temfile);
	getchar();

	return 0;
}
#endif

//#define SA
#ifdef SA
int main(void)
{
	int errcode = 0;	//错误编码 
	long starttime = 1800;	//模拟开始时刻(秒)
	long endtime = 86400;	//模拟结束时刻(秒)

	/* 读取data.txt数据 */
	errcode = readdata("data.txt", "err.txt");
	if (errcode) { fprintf(ErrFile, ERR406); return (406); }

	/* 获取各节点每个时间步长(h)的基本需水量 */
	errcode = GetDemand("BBM_EPS.inp");
	if (errcode) { fprintf(ErrFile, ERR412); return (412); }
	
	/* 打开inp文件 */
	Open_inp_file("BBM_Scenario1.inp", "BBM_Scenario1.rpt", "");

	/* 获取爆管/漏损管道喷射节点索引、喷射系数、管道索引; 医院及消火栓节点、管道索引 */
	Get_FailPipe_keyfacility_Attribute();
	
	/* 获取模拟开始时可见爆管或漏损管道信息 */
	ERR_CODE(Visible_Damages_initial(1800));
	if (errcode) { fprintf(ErrFile, ERR411); return (411); }

	if ((SenAnalys = fopen("SenAnalysis.txt", "wt")) == NULL)
	{
		printf("Can not open the SenAnalysis.txt file!\n"); assert(0);
	} //终止程序，返回错误信息
	fprintf(SenAnalys, "Report start time: %d	Report end time: %d\n", starttime, endtime);

	ERR_CODE(SensitivityAnalysis(starttime, endtime));
	if (errcode) { fprintf(ErrFile, ERR414); return (414); }

	/* 释放内存 */
	Emptymemory();

	/* 关闭txt文件 */
	fclose(InFile);
	fclose(ErrFile);
	fclose(SenAnalys);
	printf("Successful\n");
	getchar();
	return errcode;
}
#endif