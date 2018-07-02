#ifndef _GA_H_
#define _GA_H_
#include <stdio.h>
#include "wdstypes.h"


/* Defining parameters related to genetic algorithm */
int Num_group = 50;		/* Population size */
int Num_offs = 50;			/* offspring size */
int Num_iteration = 20000;	/* No. of iteration */
double P_mutation = 0.1;	/* Mutation probability */
double P_crossover = 0.8;	/* Cross probability */

/* Solution Structure */
typedef struct {
	int C_01;		/* Criteria 1*/
	int C_02;		/* Criteria 2*/
	double C_03;	/* Criteria 3*/
	double C_04;	/* Criteria 4*/
	int C_05;		/* Criteria 5*/
	double C_06;	/* Criteria 6*/
	double objvalue;	/* Objective */
	double P_Reproduction;	/* Selection probability of individual */
	LinkedList* SerialSchedule;  /* Serial Scheduling list (contains all scheduling instructions) */
	//LinkedList*	NewVisDemages;	 /* Visible damages emerging during the restoration */
	STaskassigmentlist Schedule[MAX_CREWS]; /* Restoration schedule for the 3 crews(contains initial and added schedules) */
}Solution;

Solution** Groups;			/* Population array */  
Solution** Offspring;		/* Offspring array */
int IndexCross_i;			/* Start position of crossover */
int IndexCross_j;			/* End position of crossover */
int Chrom_length;			/* Chromosome length */
int Length_SonSoliton;		/* No. of offspring */  
FILE *TemSolution;			/* Store the best solution for each generation */

/* 定义相关函数 */
int Memory_Allocation();			/* Memory allocation */
void Free_Solution(Solution*);		/* Free the memory of a solution */
void Free_GAmemory();				/* Free all memory allocated on the heap */
int InitialGroups();				/* Initialize the population */
int Calculate_Objective_Value(Solution*);/* Objective evalutation */
void Calc_Probablity();				/* Calcuiate the selection probability of offspring */
int Select_Individual();			/* Select a individual through roulette options */
int* Find_Mother_Index(Solution*, Solution*);/* Finding the corresponding index of a individual in a parent */
int Get_Conflict_Length(int*, int*, int);/* Get the number of conflicting variables in Father_cross and Mother_cross */
int *Get_Conflict(int*, int*, int, int);/* Get the conflicting variables in Father_cross and Mother_cross */
Solution* Handle_Conflict(int*, PDecision_Variable*, int*, int*, int);/*  Dealing with the conflict individuals */
int GA_Cross(Solution*, Solution*);	/* Crossover operation of GA */
int GA_Variation(int);				/* Mutation operation of GA */
void Swap_Group(Solution**, Solution**);/* Swapping two solutions */
void BestSolution();				/* Finding the best solution */
void GA_UpdateGroup();				/* Update the population */
int GA_Evolution();					/* Main cycle process of GA */


#endif // _GA_H_
