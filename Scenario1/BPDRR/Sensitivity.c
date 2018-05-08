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
		return (407);
	}
}

void Get_FailurePipe_Attribute()
/**----------------------------------------------------------------
**  输入:  无
**  输出:  无
**  功能:  获取受损管道相关属性值
**----------------------------------------------------------------*/
{
	int errcode = 0;
	int nodeindex,pipeindex1, pipeindex2;
	float emitter;

	for (int i = 0; i < Nbreaks; i++)
	{
		ERR_CODE(ENgetnodeindex(BreaksRepository[i].nodeID, &nodeindex));
		if (errcode){
			fprintf(ErrFile, ERR408, BreaksRepository[i].nodeID);
			return (408);
		}
		BreaksRepository[i].nodeindex = nodeindex;

		ERR_CODE(ENgetlinkindex(BreaksRepository[i].pipeID, &pipeindex1));
		if (errcode) {
			fprintf(ErrFile, ERR409, BreaksRepository[i].pipeID);
			return (409);
		}
		BreaksRepository[i].pipeindex = pipeindex1;

		ERR_CODE(ENgetnodevalue(nodeindex, EN_EMITTER, &emitter));
		if (errcode) {
			fprintf(ErrFile, ERR410);
		}
		BreaksRepository[i].emittervalue = emitter;

		for (int j = 0; j < BreaksRepository[i].num_isovalve; j++)
		{
			ERR_CODE(ENgetlinkindex(BreaksRepository[i].pipes[j].pipeID, &pipeindex2));
			if (errcode) {
				fprintf(ErrFile, ERR408, BreaksRepository[i].pipes[j].pipeID);
				return (408);
			}
			BreaksRepository[i].pipes[j].pipeindex = pipeindex2;
		}
	}
	
	for (int i = 0; i < Nleaks; i++)
	{
		ERR_CODE(ENgetnodeindex(LeaksRepository[i].nodeID, &nodeindex));
		if (errcode) {
			fprintf(ErrFile, ERR408, LeaksRepository[i].nodeID);
			return (408);
		}
		LeaksRepository[i].nodeindex = nodeindex;

		ERR_CODE(ENgetlinkindex(LeaksRepository[i].pipeID, &pipeindex1));
		if (errcode) {
			fprintf(ErrFile, ERR409,LeaksRepository[i].pipeID);
			return (409);
		}
		LeaksRepository[i].pipeindex = pipeindex1;

		ERR_CODE(ENgetnodevalue(nodeindex, EN_EMITTER, &emitter));
		if (errcode) {
			fprintf(ErrFile, ERR410);
		}
		LeaksRepository[i].emittervalue = emitter;
	}
}

int Add_Visdemage_tail(LinkedList *list, int type, int index,long time)
/*--------------------------------------------------------------
**  Input:   list: pointer to LinkedList array
**			 type: 受损管道类型, 1:爆管; 2:漏损
**			 index: 管道在仓库数组中的索引(以0开始)
**			 time:	Times of demage that is visible
**  Output:  none
**  Purpose: Add a visible demanges struct to the tail of the list
**--------------------------------------------------------------*/
{
	int errcode = 0; /* 初始化错误代码 */
	SCvisible *p;	/* 临时变量，用于存储可见爆管或漏损管道信息 */
	p = (SCvisible*)calloc(1, sizeof(SCvisible));
	ERR_CODE(MEM_CHECK(p));	if (errcode) return 402;
	p->time = time;
	p->type = type;
	p->Repoindex = index;
	p->next = NULL;

	if (list->head == NULL)
	{
		list->head = p;
	}
	else
	{
		list->tail->next = p;
	}
	list->tail = p;

	return errcode;
}


int Visible_Damages_initial()
/**----------------------------------------------------------------
**  输入:  无
**  输出:  Error code
**  功能:  获取模拟开始时可见爆管或漏损管道信息
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum=0;
	long t, tstep;		/* t: 当前时刻; tstep: 水力计算时间步长 */ 
	float flow;	/* 临时变量，用于存储泄流量 */
	

	//run epanet analysis engine
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */
	ERR_CODE(ENopenH());	if (errcode>100) errsum++;	/* Opens the hydraulics analysis system. */
	ERR_CODE(ENinitH(0));	if (errcode>100) errsum++;	/* Don't save the hydraulics file */

	do 
	{
		ERR_CODE(ENrunH(&t)); if (errcode>100) errsum++;
		if (t == 1800) /* Begin the restoration */
		{
			/* 遍历所有爆管 */
			for (int i = 0; i < Nbreaks; i++)
			{
				ERR_CODE(ENgetnodevalue(BreaksRepository[i].nodeindex, EN_DEMAND, &flow));
				if (BreaksRepository[i].pipediameter >= 150 || flow > 2.5)
					errcode = Add_Visdemage_tail(&IniVisDemages, _Break, i, 1800);
				if (errcode>100)	errsum++;
			}

			/* 遍历所有漏损管道 */
			for (int i = 0; i < Nleaks; i++)
			{
				ERR_CODE(ENgetnodevalue(LeaksRepository[i].nodeindex, EN_DEMAND, &flow));
				if (LeaksRepository[i].pipediameter >= 300 || flow > 2.5)
					errcode = Add_Visdemage_tail(&IniVisDemages, _Leak, i, 1800);
				if (errcode>100)	errsum++;
			}
			
		}
		ERR_CODE(ENnextH(&tstep));	if (errcode>100) errsum++;
	} while (tstep>0);
	ERR_CODE(ENcloseH());	if (errcode>100) errsum++;

	if (errsum > 0) errcode = 411;
	return errcode;
}






int main(void)
{
	int errcode = 0;

	errcode = readdata("data.txt", "err.txt");
	if (errcode){
		fprintf(ErrFile, ERR406);
		return (406);
	}
	Open_inp_file("BBM_Scenario1.inp", "BBM_Scenario1.rpt", "");
	Get_FailurePipe_Attribute();

	ERR_CODE(Visible_Damages_initial());
	if (errcode) {
		fprintf(ErrFile, ERR411);
		return (411);
	}



	getchar();
	fclose(ErrFile);
	return errcode;
}
