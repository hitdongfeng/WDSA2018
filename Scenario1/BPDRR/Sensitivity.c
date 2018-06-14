/********************************************************************************************
Name: Sensitivity.c
Purpose: Sensitivity analysis for damaged pipes to determine the importance of damaged pipes
Data: 5/3/2018
Author: Qingzhou Zhang
Email: wdswater@gmail.com
*********************************************************************************************/

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
**  Input:  f1: .inp file
		  f2: .rpt file
		  f3: .out file
**  Output:  none
**  Purpose:  Open the .inp file.
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
**  Input:  f1:inp file;
**  Output:  Error code
**  Purpose:  Get the nodal actual demand for each integer moment
**----------------------------------------------------------------*/
{
	int s;
	int errcode = 0, errsum = 0;
	long t, tstep;		/* t: current time; tstep: time step */
	float demand;		/* temporary variable */

	/* run epanet analysis engine */
	Open_inp_file(f1, "BBM_EPS.rpt", "");
	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */
	ERR_CODE(ENopenH());	if (errcode > 100) errsum++;	/* Opens the hydraulics analysis system. */
	ERR_CODE(ENinitH(0));	if (errcode > 100) errsum++;	/* Don't save the hydraulics file */

	do 
	{
		ERR_CODE(ENrunH(&t)); if (errcode>100) errsum++;
		s = (t / 3600) % 24; //current integer time
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
**  Input:  none
**  Output:  none
**  Purpose:  Get property for damaged pipes and critical facilities (hospitals and hydrants) settings
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
**  Input:  time: simulation time (sec)
**  Output:  Error code
**  Purpose:  Get the visible damages at the beginning of restoration
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum=0;
	long t, tstep;		
	float flow;
	

	
	/* Setting the epanet calculation option, reporting state information will take a significant amount of time, so do not output status information */
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
			/* Scan all breaks */
			for (int i = 0; i < Nbreaks; i++)
			{
				ERR_CODE(ENgetlinkvalue(BreaksRepository[i].flowindex, EN_FLOW, &flow));
				if (BreaksRepository[i].pipediameter >= 150 || flow > 2.5)
					errcode = Add_tail(&IniVisDemages, i, _Break,0,0);
				if (errcode>100)	errsum++;
			}

			/* Scan all leaks */
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
**  Input:  type:类型: 1.Isolate, 4.Reopen
**		  index: Broken pipe index in breaksRepository(start from 0)
**		  code: link parameter code (EN_INITSTATUS,EN_STATUS)
**		  status: pipe initial status, 0:close; 1:open
**		  emitter: emitter coefficient
**  Output:  Error code
**  Purpose:  Iisolate or restore the damage by closing or reopening the pipes related the damage
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
**  Input:  index: leak pipe index in leaksRepository(start from 0)
**			code: link parameter code (EN_INITSTATUS,EN_STATUS)
**			status: pipe initial status, 0:close; 1:open
**			emitter 漏损节点喷射系数
**  Output:  Error code
**  Purpose:  emitter coefficient
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum = 0; /* Error  code */

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
**  Input:  time: simulation time
**  Output:  Error code
**  Purpose:  Calculate system water supply capacity at current time
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

	s = (time / 3600) % 24; 
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
**  Input:  starttime:start time; endtime:end time
**  Output:  Error code
**  Purpose:   Calculate system water supply capacity at specified time period
**----------------------------------------------------------------*/
{
	int errcode = 0, errsum = 0;
	long t, tstep;	
	Sercapacity* ptr;

	
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
**  Input:  time: simulation time
**  Output:  Error code
**  Purpose:  Sensitivity analysis for damaged visible pipes
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

			/* Print the calculate results at each time step */
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

			/*Free SerCapcPeriod */
			SerCapcPeriod.current = SerCapcPeriod.head;
			while (SerCapcPeriod.current != NULL)
			{
				SerCapcPeriod.head = SerCapcPeriod.head->next;
				SafeFree(SerCapcPeriod.current);
				SerCapcPeriod.current = SerCapcPeriod.head;
			}

			/* Set initial state for each pipe */
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
** This main function is used to calculate the water supply capacity of each simulated step system during a specified period of time
** Used to debug the function GetSerCapcPeriod
**************************************************/
//#define GSCP
#ifdef GSCP
int main(void)
{
	int errcode = 0;		//Error code
	FILE *file;				//output file
	long starttime = 1800;	//Simulation start time (sec)
	long endtime = 86400;	//Simulation end time (sec)
	
	/* read ata.txt */
	errcode = readdata("data.txt", "err.txt");
	if (errcode) { fprintf(ErrFile, ERR406); return (406); }

	/* Get the nodal base demand at each integer time step */
	errcode = GetDemand("BBM_EPS.inp");
	if (errcode) { fprintf(ErrFile, ERR412); return (412); }

	/* Open inp file */
	Open_inp_file("BBM_Scenario1.inp", "BBM_Scenario1.rpt", "");

	/* Get property for damaged pipes and critical facilities (hospitals and hydrants) settings */
	Get_FailPipe_keyfacility_Attribute();

	ENsetstatusreport(0);		/* No Status reporting */
	ENsetreport("MESSAGES NO"); /* No Status reporting */

	/* Calculate system water supply capacity at specified time period */
	ERR_CODE(GetSerCapcPeriod(starttime, endtime));
	if (errcode) { fprintf(ErrFile, ERR414); return (414); }
	
	/* Print the calculate results at each time step */
	if ( (file = fopen("GSCP.txt", "wt")) == NULL )
	{
		printf("Can not open the GSCP.txt file!\n");
		assert(0); 
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

	/* Free memory */
	//Emptymemory();

	/* close .txt file */
	fclose(InFile);
	fclose(ErrFile);
	fclose(file);
	getchar();

	return 0;
}
#endif

//#define SA
#ifdef SA
int main(void)
{
	int errcode = 0;		//Error code 
	long starttime = 1800;	//Simulation start time (sec)
	long endtime = 86400;	//Simulation end time (sec)

	/* read ata.txt */
	errcode = readdata("data.txt", "err.txt");
	if (errcode) { fprintf(ErrFile, ERR406); return (406); }

	/* Get the nodal base demand at each integer time step */
	errcode = GetDemand("BBM_EPS.inp");
	if (errcode) { fprintf(ErrFile, ERR412); return (412); }
	
	/* Open inp file */
	Open_inp_file("BBM_Scenario1.inp", "BBM_Scenario1.rpt", "");

	/* Get property for damaged pipes and critical facilities (hospitals and hydrants) settings */
	Get_FailPipe_keyfacility_Attribute();
	
	/* Get the visible damages at the beginning of restoration */
	ERR_CODE(Visible_Damages_initial(1800));
	if (errcode) { fprintf(ErrFile, ERR411); return (411); }

	if ((SenAnalys = fopen("SenAnalysis.txt", "wt")) == NULL)
	{
		printf("Can not open the SenAnalysis.txt file!\n"); assert(0);
	} 
	fprintf(SenAnalys, "Report start time: %d	Report end time: %d\n", starttime, endtime);

	ERR_CODE(SensitivityAnalysis(starttime, endtime));
	if (errcode) { fprintf(ErrFile, ERR414); return (414); }

	/* Free memory */
	//Emptymemory();

	/* close .txt file */
	fclose(InFile);
	fclose(ErrFile);
	fclose(SenAnalys);
	printf("Successful\n");
	getchar();
	return errcode;
}
#endif