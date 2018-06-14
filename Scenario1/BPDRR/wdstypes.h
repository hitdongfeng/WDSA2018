#ifndef WDSTYPES_H
#define WDSTYPES_H
#include<stdlib.h>

/*-----------------------------
 Definition of global constants
-----------------------------*/

#define	  Ndemands	 4201			/* No. of nodes (basedemand >0) */
#define	  Start_pipeindex	6440	/* Start pipe index for pdd model(the nodal demand is zero for pdd model,  by the corresponding pipe flow instead) */
#define	  RestorStartTime 1800		/* Start time for repair (sec) */
#define   SimulationStartTime 0		/* Start time for hydraulic simulation */
#define	  SimulationEndTime 604800	/* End time for hydraulic simulation */

#define   MAX_CREWS	 3				/* No. of screws */
#define   MAX_LINE   500			/* Maximum number of characters per line in data.txt */
#define   MAX_TOKS   50				/* Maximum number of tokens per line in data.txt */
#define	  MAX_ID	 31				/* Maximum number of characters for ID */
#define	  Time_Step	 900			/* Hydraulic Simulation Time Step (SEC) */
#define	  Pattern_length 24			/* Pattern length */
#define	  Break_Weight_Leak	0.5		/* The selection weight of a pipe and leakage */
#define	  NUM_BreakOperation 2		/* Number of break operations(isolation + replacemant) */
#define	  NUM_LeakOperation 1		/* Number of leak operations(repair) */
#define	  MAX_Fire_Volume 756000	/* Total water supply per hydrant (L) */
#define	  NUM_Criteria	6			/* No. of criteria */
#define	  Time_of_Consecutive 8 	/* Nodes without service for more than 8 consecutive hours */
#define	  FLow_Tolerance	1e-3	/* Tolerance of flow */


#define   DATA_SEPSTR    " \t\n\r" /* Separator in data.txt */
#define   U_CHAR(x) (((x) >= 'a' && (x) <= 'z') ? ((x)&~32) : (x)) /* uppercase char of x */
#define   MEM_CHECK(x)  (((x) == NULL) ? 402 : 0 )   /* Macro to test for successful allocation of memory  */
#define   ERR_CODE(x)  (errcode = ((errcode>100) ? (errcode) : (x))) /*Macro to evaluate function x with error checking
																	   (Fatal errors are numbered higher than 100) */

static void SafeFree(void **pp)          /* Safely free memory */
{                                       
	if (pp != NULL && *pp != NULL)
	{
		free(*pp);
		*pp = NULL;
	}
}
#define SafeFree(p) SafeFree((void**)&(p))

/*----------------------------------
 Define global enumeration variable
------------------------------------*/

/* Fault status */
enum Pipe_Status 
{
	_Isolate=1,	//Isolation
	_Replace,	//Replacement
	_Repair,	//Repair
	_Reopen		//Reopen
};

/* Damage types */
enum Demage_type
{
	_Break=1,	//break
	_Leak		//leak
};


/*-----------------------------
	    Global structs
-----------------------------*/

/* Failure pipe */
struct FailurePipe
{
	char pipeID[MAX_ID + 1];	//Pipe ID
	int pipeindex;				//Pipe index(start from 1)
};
typedef struct FailurePipe SFailurePipe;

/* Hospital */
struct Hospital
{
	char nodeID[MAX_ID + 1];	//Node ID
	char pipeID[MAX_ID + 1];	//Pipe ID associated with hospital (Used to simulate demand)
	int	nodeindex;				//Node index
	int pipeindex;				//Pipe index associated with hospital
};
typedef struct Hospital SHospital;

/* Hydrant */
struct Firefight
{
	char ID[MAX_ID + 1];	//Pipe ID
	int index;				//Pipe index(start from 1)
	float fire_flow;		//Fire fight fow
	float cumu_flow;		//Cumulative fire flow
};
typedef struct Firefight SFirefight;

/* Break */
struct Breaks
{
	int isolate_time;		//Isolate time (minutes)
	int replace_time;		//Replace time (hours)
	int num_isovalve;		//No. of pipes closed to isolate the broken pipe
	int nodeindex;			//Virtual Node Index（start from 1）
	int pipeindex;          //Broken pipe index（start from 1）
	int flowindex;			//Flow pipe index (start from 1)
	char nodeID[MAX_ID + 1];//Virtual node ID added to simulate a broken pipe(Set the emitter coefficient to 0 to close the broken pipe)
	char pipeID[MAX_ID + 1];//Broken pipe ID (Set the status of the broken pipe to open to restore water supply)
	char flowID[MAX_ID + 1];//Flow pipe ID
	float pipediameter;		//Broken pipe diameter(mm)
	float emittervalue;     //Emitter coefficient of Virtual node
	SFailurePipe *pipes;	//Pipes closed for isolation
};
typedef struct Breaks SBreaks;

/* 定义漏损管道结构体 */
struct Leaks
{
	int repair_time;		//Repair time(hours)
	int nodeindex;			//Virtual Node Index（start from 1）
	int pipeindex;          //Leak pipe index（start from 1）
	int flowindex;			//Flow pipe index (start from 1)
	char nodeID[MAX_ID + 1];//Virtual node ID added to simulate a broken pipe(Set the emitter coefficient to 0 to close the broken pipe)
	char pipeID[MAX_ID + 1];//Leak pipe ID (Set the status of the broken pipe to open to restore water supply)
	char flowID[MAX_ID + 1];//Flow pipe ID
	float pipediameter;		//Leak pipe diameter(mm)
	float emittervalue;     //Emitter coefficient of Virtual node
};
typedef struct Leaks SLeaks;

/* Decision variable */
struct Decision_Variable
{
	int index;		//Pipe array index (start from 0)
	int type;		//Operation types, 1:Isolation; 2:Replacement; 3:Repair; 4:Reopen
	long starttime;	//Start time 
	long endtime;	//End time
	struct Decision_Variable *next;	
};
typedef  struct Decision_Variable* PDecision_Variable;

/* Define LinkedList list */
typedef struct _linkedlist
{
	PDecision_Variable head;	/* point to head */
	PDecision_Variable tail;	/* point to tail */
	PDecision_Variable current;	/* Current pointer for traversing list */
}LinkedList;

/* Water supply capacity of WDS */
typedef struct _sercapacity
{
	double Functionality;	/* Overall water supply capacity  */
	int Numkeyfac;			/* No. of critical facilities with service */
	double MeankeyFunc;		/* Average water supply capacity */
	long time;				/* Simulation time */
	struct _sercapacity *next;
}Sercapacity;

/* Define Sercaplist list */
typedef struct _sercaplist
{
	Sercapacity* head;		/* point to head */
	Sercapacity* tail;		/* point to tail */
	Sercapacity* current;	/* Current pointer for traversing list */
}Sercaplist;

/* Define damagebranch struct, called in randperm function */
typedef struct _damagebranch
{
	int index;
	int count;
	struct _damagebranch *next;
}Sdamagebranch;

/* Define damagebranchlist list */
typedef struct _damagebranchlist
{
	Sdamagebranch* head;	/* point to head */
	Sdamagebranch* tail;	/* point to tail */
	Sdamagebranch* current;	/* Current pointer for traversing list */
}Sdamagebranchlist;

/* Schedule index */
typedef struct _Scheduleindex
{
	PDecision_Variable pointer; /* Corresponding position pointer in SerialSchedule list */
	struct _Scheduleindex *next;
}Scheduleindex;

/* Taskassigment list */
typedef struct _Taskassigmentlist
{
	Scheduleindex* head;	/* point to head */
	Scheduleindex* tail;	/* point to tail */
	Scheduleindex* current;	/* Current pointer for traversing list */
}STaskassigmentlist;

/* Criteria struct (for C_05 counting) */
typedef struct _Criteria
{
	int flag;  /* flag */
	int count; /* counter */
}SCriteria;

#endif
