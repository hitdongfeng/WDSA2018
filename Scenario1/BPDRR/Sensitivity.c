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

int Get_FailurePipe_Attribute()
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
	return errcode;
}

int main(void)
{
	int errcode = 0;

	errcode = readdata("data.txt", "err.txt");
	Open_inp_file("BBM.inp", "1.rpt", "");
	ERR_CODE(Get_FailurePipe_Attribute());

	for (int i = 0; i < Nbreaks; i++)
	{
		printf("nodeindex= %d	pipeindex= %d	emittervalue= %f	", BreaksRepository[i].nodeindex, BreaksRepository[i].pipeindex, BreaksRepository[i].emittervalue);
		for (int j = 0; j < BreaksRepository[i].num_isovalve; j++)
			printf("%d ", BreaksRepository[i].pipes[j].pipeindex);
		printf("\n");
	}
	printf("--------------------\n");

	for (int i = 0; i < Nleaks; i++)
	{
		printf("nodeindex= %d	pipeindex= %d	emittervalue= %f	", LeaksRepository[i].nodeindex, LeaksRepository[i].pipeindex, LeaksRepository[i].emittervalue);
		printf("\n");
	}



	getchar();
	fclose(ErrFile);
	return errcode;
}
