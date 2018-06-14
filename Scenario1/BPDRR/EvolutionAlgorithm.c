/*******************************************************************
Name: EvolutionAlgorithm.c
Purpose: Functions associated with the gene algorithm
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
#define EXTERN extern
//#define EXTERN 
#include "wdsvars.h"


Sdamagebranchlist Break, Leak;		/* Visible breals/leaks list */
int Num_breaks = 0, Num_leaks = 0;	/* No. of visible breals/leaks */
int Break_count, Leak_count;		/* visible breals/leaks counter */


void Add_damagebranch_list(Sdamagebranchlist* list, Sdamagebranch *ptr)
/*--------------------------------------------------------------
**  Input:   list: pointer to Sdamagebranchlist chain table
**			 ptr: target structure
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
**  Input:   None
**  Output:  Error Code
**  Purpose: Generate visible breaks/leaks related information for random selection function call 
**--------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;	/* Error code & error count */
	Sdamagebranch *ptr;	/* temporary structure pointer */

	/* initialize Break, Leak, p structure */
	Break.head = NULL; Break.tail = NULL; Break.current = NULL;
	Leak.head = NULL; Leak.tail = NULL; Leak.current = NULL;

	/* Get No. of breaks and leaks */
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
			assert(0); //Terminate the program and return an error message
		}
		decisionlist.current = decisionlist.current->next;
	}
	if (err_count) errcode = 416;

	return errcode;
}

Sdamagebranch* find_visibledamage_index(Sdamagebranchlist* list, int index,int type, int* OperationType)
/*--------------------------------------------------------------
**  Input:   list: pointer to Sdamagebranchlist chain table
**			 index: index
**			 number: No. of breaks/leaks, break£º1; leak: 2.
**  Output:  Returns struct pointer of the specified index, and returns NULL if the specified index cannot be found
**  Purpose: Get the specified operation flowchart based on the random index value
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
					if (NvarsCrew1 > 0 || NvarsCrew2 > 0 || NvarsCrew3 > 0)
					{
						for (int i = 0; i < MAX_CREWS; i++)
						{
							ExistSchedule[i].current = ExistSchedule[i].head;
							while (ExistSchedule[i].current != NULL)
							{
								if ((ExistSchedule[i].current->index == list->current->index) && (ExistSchedule[i].current->type== _Break))
								{
									list->current->count = 1;
									break;
								}
								ExistSchedule[i].current = ExistSchedule[i].current->next;
							}
							if (list->current->count == 1)
								break;
						}
					}

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
**  Input:  None
**  Output:  Return LinkedList pointer
**  Purpose:  Randomly generate visible breaks / leaks operation sequence for engineering team to choose from
**----------------------------------------------------------------*/
{
	int errcode = 0;					/* Error code */
	int temptype;						/* Temporary variable */
	int Random_breakindex, Random_leakindex; /* Random index of break/leak */
	double randomvalue;					/* Random float value [0,1] */
	Sdamagebranch *ptr;					/* Temporary struct pointer */
	LinkedList* p;						/* Temporary struct pointer */
	
	/* Initializing related parameters */
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

	/* Initialize the count value of Break, Leak */
	Break.current = Break.head;
	while (Break.current != NULL)
	{
		Break.current->count = 0;
		Break.current = Break.current->next;
	}

	Leak.current = Leak.head;
	while (Leak.current != NULL)
	{
		Leak.current->count = 0;
		Leak.current = Leak.current->next;
	}

	return p;
}

void Add_Taskassigmentlist(STaskassigmentlist* list, Scheduleindex *ptr)
/*--------------------------------------------------------------
**  Input:   list: pointer to STaskassigmentlist chain table
**			 ptr: Target structural pointer
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

int Find_Replace_Crow(PDecision_Variable ptr, STaskassigmentlist* schedule)
/*--------------------------------------------------------------
**  Input:   ptr: pointer to Decision_Variable
**  Output:  The engineering team index that performs the Isolate operation, otherwise return -1.
**  Purpose: Find the engineering team index that performs the isolate operation for replace operation
**--------------------------------------------------------------*/
{
	for (int i = 0; i < MAX_CREWS; i++)
	{
		schedule[i].current = schedule[i].head;
		while (schedule[i].current != NULL)
		{
			if (schedule[i].current->pointer->type == _Break)
			{
				if (schedule[i].current->pointer->index == ptr->index)
					return i;
			}
			schedule[i].current = schedule[i].current->next;
		}
	}
	return -1;
}

int Find_Finished_Crow(STaskassigmentlist* Schedule)
/*--------------------------------------------------------------
**  Input:   None
**  Output:  Returns the engineering team index of the completed task, otherwise returns an error value of -1.
**  Purpose: Returns the team index of the completed task to continue assigning the next instruction
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

int Task_Assignment(LinkedList *list, STaskassigmentlist* schedule)
/**----------------------------------------------------------------
**  Input:   None
**  Output:  Error Code
**  Purpose: Assign all instructions in the SerialSchedule list to each engineering team
**----------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;
	int crowindex;	/* Engineering team index of replace operation */
	
	Scheduleindex* ptr;	/* Temporary pointer */

	/* If there is an initial solution, add the initial solution first */
	if (NvarsCrew1 > 0 || NvarsCrew2 > 0 || NvarsCrew3 > 0)
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
				Add_Taskassigmentlist(&schedule[i], ptr);
				ExistSchedule[i].current = ExistSchedule[i].current->next;
			}
		}
	}
	
	/* Assign all instructions in the SerialSchedule list to each engineering team */
	list->current = list->head;
	while (list->current != NULL)
	{
		/* Assign to the appropriate engineering team for the isolate operation */
		if (list->current->type == _Isolate)
		{
			crowindex = Find_Finished_Crow(schedule);
			if (crowindex < 0) err_count++;
			
			if (schedule[crowindex].head == NULL)
			{
				list->current->starttime = RestorStartTime;
				list->current->endtime = list->current->starttime + 60 * BreaksRepository[list->current->index].isolate_time;
			}
			else
			{
				list->current->starttime= schedule[crowindex].tail->pointer->endtime;
				list->current->endtime = list->current->starttime + 60 * BreaksRepository[list->current->index].isolate_time;
			}
				ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
				ptr->pointer = list->current;
				ptr->next = NULL;
				Add_Taskassigmentlist(&schedule[crowindex], ptr);
		}
		/* Assign to the appropriate engineering team for the replace operation */
		else if (list->current->type == _Replace)
		{
			crowindex = Find_Replace_Crow(list->current, schedule);
			if (crowindex < 0) err_count++;

			list->current->starttime = schedule[crowindex].tail->pointer->endtime;
			list->current->endtime = list->current->starttime + 3600 * BreaksRepository[list->current->index].replace_time;
			
			ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
			ptr->pointer = list->current;
			ptr->next = NULL;
			Add_Taskassigmentlist(&schedule[crowindex], ptr);
		}

		/* Assign to the appropriate engineering team for the repair operation */
		else if (list->current->type == _Repair)
		{
			crowindex = Find_Finished_Crow(schedule);
			if (crowindex < 0) err_count++;

			if (schedule[crowindex].head == NULL)
			{
				list->current->starttime = RestorStartTime;
				list->current->endtime = list->current->starttime + 3600 * LeaksRepository[list->current->index].repair_time;
			}
			else
			{
				list->current->starttime = schedule[crowindex].tail->pointer->endtime;
				list->current->endtime = list->current->starttime + 3600 * LeaksRepository[list->current->index].repair_time;
			}
			ptr = (Scheduleindex*)calloc(1, sizeof(Scheduleindex));
			ptr->pointer = list->current;
			ptr->next = NULL;
			Add_Taskassigmentlist(&schedule[crowindex], ptr);
		}

		list->current = list->current->next;
	}

	if (err_count)
		errcode = 417;
	return errcode;
}


void FreeMemory(LinkedList*	SerialSchedule,STaskassigmentlist* Schedule)
/*--------------------------------------------------------------
**  Input:   SerialSchedule: Point to SerialSchedule
**			 Schedule: Engineering scheduling pointer (includes initial and new solutions)
**  Output:  none
**  Purpose: free memory
**--------------------------------------------------------------*/
{
	/* Free the memory of schedule array  */
	for (int i = 0; i < MAX_CREWS; i++)
	{
		Schedule[i].current = Schedule[i].head;
		while (Schedule[i].current != NULL)
		{
			Schedule[i].head = Schedule[i].head->next;
			SafeFree(Schedule[i].current);
			Schedule[i].current = Schedule[i].head;
		}
	}

	/* Free SerialSchedule */
	SerialSchedule->current = SerialSchedule->head;
	while (SerialSchedule->current != NULL)
	{
		SerialSchedule->head = SerialSchedule->head->next;
		SafeFree(SerialSchedule->current);
		SerialSchedule->current = SerialSchedule->head;
	}
}

//#define EVO
#ifdef EVO

int main(void)
{
	int errcode = 0;	//Error code
	inpfile = "BBM_Scenario1.inp";

	STaskassigmentlist* Schedule; 
	Schedule = (STaskassigmentlist*)calloc(MAX_CREWS, sizeof(STaskassigmentlist));
	LinkedList*	SerialSchedule;



	/* read the data in data.txt */
	errcode = readdata("data.txt", "err.txt");
	if (errcode) { fprintf(ErrFile, ERR406); return (406); }

	/*Open the .inp file */
	Open_inp_file(inpfile, "report.rpt", "");

	/*Get property for damaged pipes and critical facilities (hospitals and hydrants) settings */
	Get_FailPipe_keyfacility_Attribute();

	/* Generate visible breaks/leaks related information for random selection function call  */
	errcode = Get_Select_Repository();
	if (errcode) { fprintf(ErrFile, ERR416); return (416); }

	SerialSchedule = Randperm();

	
	
	/* Print SerialSchedule */
	SerialSchedule->current = SerialSchedule->head;
	while (SerialSchedule->current != NULL)
	{
		printf("index: %d	type: %d\n", SerialSchedule->current->index, SerialSchedule->current->type);

		SerialSchedule->current = SerialSchedule->current->next;
	}

	errcode = Task_Assignment(SerialSchedule, Schedule);

	/* Print Schedule */
	for (int i = 0; i < MAX_CREWS; i++)
	{
		printf("\nSchedule[%d]:\n", i);
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
	
	
	FreeMemory(SerialSchedule,Schedule);
	getchar();
	return 0;
}

#endif