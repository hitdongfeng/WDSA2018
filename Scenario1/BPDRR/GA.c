/*******************************************************************
Name: GA.c
Purpose: GA Main Function 
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
**  Purpose: Memory allocation
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
**  Input:   ptr: pointer to Solution
**  Output:  None
**  Purpose: Free the memory of a solution
**--------------------------------------------------------------*/
{
	/* Free SerialSchedule list */
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

		/* Free Schedule array */
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
**  Purpose: Free all memory allocated on the heap
**--------------------------------------------------------------*/
{
	/* Free population memory */
	for (int i = 0; i < Num_group; i++)
		Free_Solution(Groups[i]);

	/* Free Offspring memory */
	for (int i = 0; i < Num_offs; i++)
		Free_Solution(Offspring[i]);

}

int InitialGroups()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  Error code
**  Purpose: Initialize the population
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;
	Chrom_length = 0; /* Chromosome length */
	for (int i = 0; i < Num_group; i++)
	{
		/* Generate randomly visible break / leak operation sequence for engineering team to choose */
		Groups[i]->SerialSchedule = Randperm();

		/* Assign serial instructions to each crew */
		ERR_CODE(Task_Assignment(Groups[i]->SerialSchedule, Groups[i]->Schedule));

		Groups[i]->C_01 = 0;Groups[i]->C_02 = 0;Groups[i]->C_03 = 0;
		Groups[i]->C_04 = 0;Groups[i]->C_05 = 0;Groups[i]->C_06 = 0;
		Groups[i]->P_Reproduction = 0;Groups[i]->objvalue = 0;

		if (errcode) err_count++;
	}

	/* Get the length of chromosome length */
	Groups[0]->SerialSchedule->current = Groups[0]->SerialSchedule->head;
	while (Groups[0]->SerialSchedule->current != 0)
	{
		Chrom_length++;
		Groups[0]->SerialSchedule->current = Groups[0]->SerialSchedule->current->next;
	}

	/* Calculate the fitness for each individual */
	for (int i = 0; i < Num_group; i++)
	{
		ERR_CODE(Calculate_Objective_Value(Groups[i]));
		if (errcode)	err_count++;
		printf("Complited: %d / %d\n", i + 1, Num_group);
	}

	Calc_Probablity();/* Calculate the selection probality for each individual */
	BestSolution(); /* Save the current best solution */
		
	if (err_count)
		errcode = 419;
	return errcode;
}

int Calculate_Objective_Value(Solution* sol)
/*--------------------------------------------------------------
**  Input:   sol: Population arrray
**  Output:  Error code
**  Purpose: Objective evalutation
**--------------------------------------------------------------*/
{
	int errcode = 0, err_sum = 0;	/* Error code & error count */
	int s;							/* Integer time */
	int temvar,temindex;			/* Temporary int variables */
	float y,temflow;				/* Temporary float variables */
	double sumpdddemand, sumbasedemand; /* pressure driven demand and nodal basedemand */
	long t, tstep;	/* t: Current time; tstep: hydraulic simulation time step */

	/* Set the current pointer of Schedule struct */
	for (int i = 0; i < MAX_CREWS; i++)
		sol->Schedule[i].current = sol->Schedule[i].head;
	/* Initialize firefight cumulative flow */
	for (int i = 0; i < Nfirefight; i++)
		Firefighting[i].cumu_flow = 0;
	/* Initialize C_05 count */
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
		sumpdddemand = 0.0, sumbasedemand = 0.0;	/* Initialize pressure driven demand and nodal basedemand */

		if ((t >= SimulationStartTime && t <= SimulationEndTime) && (t % Time_Step == 0))
		{
			s = (t / 3600) % 24; //Integer time
			/* Calculate C_01 */
			for (int i = 0; i < Nhospital; i++) /* Hospital */
			{
				ERR_CODE(ENgetlinkvalue(Hospitals[i].pipeindex, EN_FLOW, &temflow));
				if (errcode > 100) err_sum++;
				if ((temflow < FLow_Tolerance) || (temflow - ActuralBaseDemand[Hospitals[i].nodeindex - 1][s]) > 0.5)
					temflow = 0.0;

				y = temflow / ActuralBaseDemand[Hospitals[i].nodeindex - 1][s];
				if (y <= 0.5)
				{
					sol->C_01 += Time_Step / 60;
				}
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
						{
							sol->C_01 += Time_Step / 60;
						}
					}
				}
			}
			/* Calculate C_02, C_04 and C_05 */
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

			/* Calculate C_03 */
			if (sumpdddemand / sumbasedemand < 0.999)
			{
				sol->C_03 += (1.0 - sumpdddemand / sumbasedemand)*(Time_Step / 60);
			}
				


			/* Calculate C_06 */
			for (int i = 0; i < Nbreaks; i++) /* Scan all breaks */
			{
				ERR_CODE(ENgetlinkvalue(BreaksRepository[i].flowindex, EN_FLOW, &temflow));
				if (errcode > 100)	err_sum++;
				if ((temflow < FLow_Tolerance))
					temflow = 0.0;
				sol->C_06 += (double)temflow * Time_Step;
			}

			for (int i = 0; i < Nleaks; i++) /* Scan all leaks */
			{
				ERR_CODE(ENgetlinkvalue(LeaksRepository[i].flowindex, EN_FLOW, &temflow));
				if (errcode > 100)	err_sum++;
				if ((temflow < FLow_Tolerance))
					temflow = 0.0;
				sol->C_06 += (double)temflow * Time_Step;
			}
/*************************************************************************************************/
			/* Update hydrant status */
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

			/* Update pipe status */
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
	sol->objvalue = 0*sol->C_01 + 0.15*sol->C_02 + 0.15*sol->C_03 + 0.2*sol->C_04 + 0.4*sol->C_05 + 0.1*sol->C_06/1000000;
	//sol->objvalue = sol->C_01;

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
**  Purpose:  Calcuiate the selection probability of offspring
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
**  Output:  individual index, if not, return error code
**  Purpose: Select a individual through roulette options
**--------------------------------------------------------------*/
{
	double selection_P = genrand_real1(); /* Generating a random number between [0,1] randomly. */
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
**  Input:   Father: A individual in the Parent, Mother: A individual in the mother 
**  Output:  index of a individual in a parent
**  Purpose: Finding the corresponding index of a individual in a parent
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
**  Input:   Detection_Cross: Detection objective
**			 Model_Cross: Target individual
**			 Length_Cross: Cross length
**  Output:  the number of conflicting variables
**  Purpose: Get the number of conflicting variables in Father_cross and Mother_cross
**--------------------------------------------------------------*/
{
	int Conflict_Length = 0;
	int flag_Conflict;
	for (int i = 0; i < Length_Cross; i++)
	{
		flag_Conflict = 1;  // Determine if there is a conflict 
		for (int j = 0; j < Length_Cross; j++)
		{
			if (Detection_Cross[i] == Model_Cross[j])
			{
				// End the second loop  
				j = Length_Cross;
				flag_Conflict = 0;  // Not a conflict  
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
**  Input:   Detection_Cross: Detection objective
**			 Model_Cross: Target individual
**			 Length_Cross: Cross length
**			 Length_Conflict: Conflict length
**  Output:  Error code
**  Purpose: Get the conflicting variables in Father_cross and Mother_cross
**--------------------------------------------------------------*/
{
	int count = 0; /* count */
	int flag_Conflict; /* Conflict flag, 0: No, 1: Yes */
	int *Conflict = (int *)calloc(Length_Conflict, sizeof(int));
	
	for (int i = 0; i < Length_Cross; i++)
	{
		flag_Conflict = 1;  // Determine if there is a conflict   
		for (int j = 0; j < Length_Cross; j++)
		{
			if (Detection_Cross[i] == Model_Cross[j])
			{
				 // End the second loop
				j = Length_Cross;
				flag_Conflict = 0;  // Not a conflict    
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
**  Input:   ConflictSolution: Conflict Solution
**			 pointer: point to operation in Parent
**			 Detection_Conflict: Detection objective
**			 Model_Conflict: Target individual
**			 Length_Conflict: Conflict length
**  Output:  Solved offspring
**  Purpose: Dealing with the conflict individuals
**--------------------------------------------------------------*/
{
	int errcode = 0;
	int flag;	/* Conflict flag, 0: No, 1: Yes */
	int index;
	int temp=0; /* Temporary variable */
	PDecision_Variable ptr; /* Temporary variable */
	Solution* Offspring;
	LinkedList* p;	/* Temporary struct pointer */
	/* Initializing related parameters */
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

		/* [0, IndexCross_i) Finding conflict */
		for (index = 0; index < IndexCross_i; index++)
		{
			if (Model_Conflict[i] == ConflictSolution[index])
			{
				flag = 1;
				break;
			}
		}
		/* The first paragraph is not found, find the remaining parts(except for the gene exchange segment) */
		if (!flag)
		{
			/* [IndexCross_i + 1, Chrom_length) Finding conflict */
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

	/* Encoding into a offspring */
	for (int i = 0; i < Chrom_length; i++)
	{
		ptr = pointer[ConflictSolution[i]];
		ERR_CODE(Add_tail(p,ptr->index,ptr->type,0,0));
		if (errcode) {fprintf(ErrFile, ERR423);}
	}
	/* Assign all instructions in the SerialSchedule list to each engineering team */
	Offspring->SerialSchedule = p;
	ERR_CODE(Task_Assignment(Offspring->SerialSchedule,Offspring->Schedule));

	return Offspring;
}

int GA_Cross(Solution* Father, Solution* Mother)
/*--------------------------------------------------------------
**  Input:   Father: A individual in the Parent, Mother: A individual in the mother
**  Output:  Error code
**  Purpose: Crossover operation of GA
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0; /* Error code & error count */
	int temp;	/* Temporary variable */
	int Length_Cross;    /* Cross length */
	int *Father_index, *Mother_index;
	PDecision_Variable* Father_pointer;
	int *Father_cross, *Mother_cross; /* Cross segments */
	int *Conflict_Father, *Conflict_Mother;  /* Save the position of conflict */   
	int Length_Conflict;	/* conflict length */ 

	/* Memory allocation */
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

/* Generate the cross position randomly, satisfy the condition of IndexCross_i < IndexCross_j */
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

	/* Cross segments */
	Length_Cross = IndexCross_j - IndexCross_i+1; /* Cross length */
	Father_cross = (int*)calloc(Length_Cross, sizeof(int)); /* Gene segment in parent */
	Mother_cross = (int*)calloc(Length_Cross, sizeof(int)); /* Gene segment in mother */
	ERR_CODE(MEM_CHECK(Father_cross));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(Father_cross));	if (errcode) err_count++;
	temp = 0;
	for (int i = IndexCross_i; i <= IndexCross_j; i++)
	{
		Father_cross[temp] = Father_index[i];
		Mother_cross[temp] = Mother_index[i];
		temp++;
	}

	// Start Crossing - find the conflict length in Father_Cross and Mother_cross
	Length_Conflict = Get_Conflict_Length(Father_cross, Mother_cross, Length_Cross);
	Conflict_Father = Get_Conflict(Father_cross, Mother_cross, Length_Cross, Length_Conflict);
	Conflict_Mother = Get_Conflict(Mother_cross, Father_cross, Length_Cross, Length_Conflict);

	/* Swap the cross segments of Father and Mother */
	for (int i = IndexCross_i; i <= IndexCross_j; i++)
	{
		temp = Father_index[i];
		Father_index[i] = Mother_index[i];
		Mother_index[i] = temp;
	}

	/* Dealing with the conflict individuals */
	Solution* Descendant_ONE = Handle_Conflict(Father_index, Father_pointer, Conflict_Father, Conflict_Mother, Length_Conflict);
	Solution* Descendant_TWO = Handle_Conflict(Mother_index, Father_pointer, Conflict_Mother, Conflict_Father, Length_Conflict);

	Offspring[Length_SonSoliton++] = Descendant_ONE;
	Offspring[Length_SonSoliton++] = Descendant_TWO;

	/* Free memory */
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
**  Input:   Index_Offspring: Offspring index
**  Output:  Error code
**  Purpose: Mutation operation of GA
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0; /* Error code & error count */
	int temp;	/* Temporary variable */
	int IndexVariation_i, IndexVariation_j;
	int *Index;
	PDecision_Variable* Index_pointer;
	LinkedList* p;	/* Temporary struct pointer */
	Solution* Offptr;
	/* Initializing related parameters */
	Offptr = (Solution*)calloc(1, sizeof(Solution));
	p = (LinkedList*)calloc(1, sizeof(LinkedList));
	ERR_CODE(MEM_CHECK(Offptr));	if (errcode) {fprintf(ErrFile, ERR402);}
	ERR_CODE(MEM_CHECK(p));	if (errcode) {fprintf(ErrFile, ERR402);}
	Offptr->C_01 = 0; Offptr->C_02 = 0; Offptr->C_03 = 0;
	Offptr->C_04 = 0; Offptr->C_05 = 0; Offptr->C_06 = 0;
	Offptr->P_Reproduction = 0; Offptr->objvalue = 0;
	p->head = NULL; p->tail = NULL; p->current = NULL;

	/* Memory allocation */
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

	/* Generate randomly two random numbers to represent the mutation positions and swap the two positions */
	IndexVariation_i = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	IndexVariation_j = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	while ((IndexVariation_i == IndexVariation_j))
	{
		IndexVariation_j = (int)floor(genrand_real1()*(Chrom_length - 0.0001));
	}

	/* swap the two positions */
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
	/* Encoding into a offspring */
	for (int i = 0; i < Chrom_length; i++)
	{
		ERR_CODE(Add_tail(p, Index_pointer[Index[i]]->index, Index_pointer[Index[i]]->type, 0, 0));
		if (errcode) {fprintf(ErrFile, ERR423);}
	}
	/* Assign all instructions in the SerialSchedule list to each engineering team */
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

void Swap_Group(Solution** group, Solution** son)
/*--------------------------------------------------------------
**  Input:  group: Individual in population, son: Individual in offspring
**  Output:  None
**  Purpose: Swapping two solutions
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
**  Purpose: Finding the best solution
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

	/* Print the Schedule result */
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
	/* Print BestSolution */
	fprintf(TemSolution, "C_01= %d, C_02= %d, C_03= %f, C_04= %f, C_05= %d, C_06= %f, Sum= %f\n",
		Groups[index]->C_01, Groups[index]->C_02, Groups[index]->C_03, Groups[index]->C_04, Groups[index]->C_05,
		Groups[index]->C_06, Groups[index]->objvalue);
	fprintf(TemSolution, "\n***********************************\n");

}

void GA_UpdateGroup()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  None
**  Purpose: Update the population
**--------------------------------------------------------------*/
{
	Solution* tempSolution;
	/* Sort the individuals based on fitness from small to large */  
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

	/* Update population */
	for (int i = 0; i < Num_offs; i++)
	{
		for (int j = 0; j < Num_group; j++)
		{
			if (Offspring[i]->objvalue < Groups[j]->objvalue)
			{
				Swap_Group(&Groups[j], &Offspring[i]);
				break;
			}
		}
	}

	/* Delete the offspring */
	for(int i=0;i<Num_offs;i++)
		Free_Solution(Offspring[i]);

	BestSolution();
}

int GA_Evolution()
/*--------------------------------------------------------------
**  Input:   None
**  Output:  Error code;
**  Purpose: Main cycle process of GA
**--------------------------------------------------------------*/
{
	int errcode = 0, err_sum = 0;	/* Error code and error count */
	int iter = 0;	/* counter */
	int M;	/* cross number */
	int Father_index, Mother_index;	/* Father index and Mother index */
	double Is_Crossover;	/* Mutation probability */
	double Is_mutation;		/* Cross probability */

	while (iter < Num_iteration)
	{
		fprintf(TemSolution, "Iteration: %d\n", iter + 1); /* Write in the report file */
		printf("********Iteration: %d / %d **********\n", iter + 1, Num_iteration); /* Output to console */
		
		/* 1.Selection */
		Father_index = Select_Individual();
		Mother_index = Select_Individual();

		/* Prevent father and mother from being the same individual */
		while (Mother_index == Father_index)
		{
			Mother_index = Select_Individual();
		}

		/* 2.Cross, Generate 2M new individuals through M crosses */
		M = Num_group - Num_group / 2;
		Length_SonSoliton = 0;	

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

		/* 3.Mutation */

		for (int IndexVariation = 0; IndexVariation < Length_SonSoliton; IndexVariation++)
		{
			Is_mutation = genrand_real1();
			if (Is_mutation <= P_mutation)
			{
				ERR_CODE(GA_Variation(IndexVariation));
				if (errcode) err_sum++;
			}

			/*  After mutation processing, the fitness value is recalculated.  */
			ERR_CODE(Calculate_Objective_Value(Offspring[IndexVariation]));
			if (errcode) err_sum++;

			//total_objvalue += Offspring[IndexVariation]->objvalue;
			printf("Complited: %d / %d\n", IndexVariation + 1, Length_SonSoliton);
		}
		
		/* 4. Update the population */
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
	int errcode = 0;	/* error code */ 
	inpfile = "BBM_Scenario1.inp";	/* .inp file */
	time_t T_begin = clock();

	/* Read data from the data.txt */
	ERR_CODE(readdata("data.txt", "err.txt"));
	if (errcode) fprintf(ErrFile, ERR406);

	/* Get each nodal actual demand at each integer moment */
	errcode = GetDemand("BBM_EPS.inp");
	if (errcode) fprintf(ErrFile, ERR412);

	/* Open the .inp file */
	Open_inp_file(inpfile, "report.rpt", "");

	/* Get nodal index, emitter coefficient, pipe index, nodal and pipe indexes for critical facilities */
	Get_FailPipe_keyfacility_Attribute();

	/* Close hydraulic model */
	ERR_CODE(ENclose());	if (errcode > 100) fprintf(ErrFile, ERR420);;

	/* Get the visible damages */
	ERR_CODE(Get_Select_Repository());
	if (errcode) fprintf(ErrFile, ERR416);

	/* Memory Allocation */
	ERR_CODE(Memory_Allocation());
	if (errcode) fprintf(ErrFile, ERR417);

	/* Open result file */
	if ((TemSolution = fopen("TemSolution.txt", "wt")) == NULL)
	{
		printf("Can not open the TemSolution.txt!\n");
		assert(0); //Terminate program, return a error message
	}
	key_solution = fopen("key_solution.txt", "wt");
	
	/* Initialize population */
	ERR_CODE(InitialGroups());
	if (errcode) fprintf(ErrFile, ERR418);

	/*  Main cycle process of GA */
	ERR_CODE(GA_Evolution());
	if (errcode) fprintf(ErrFile, ERR426);

	/*Free memory*/
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