
//This file defines the functions used in the BPDRR program

#ifndef WDSfUNS_H
#define WDSfUNS_H


int  readdata(char*, char*);		  /* Opens data.txt file & reads parameter data */
int GetDemand(char*);				  /* Get the nodal actual demand for each integer moment */
void Emptymemory();					  /* Free global variable dynamic memory */
int  Add_tail(LinkedList*, int, int, long, long);	 /* Add a Decision_Variable struct to the tail of the list */
void Add_SerCapcity_list(Sercaplist*, Sercapacity*); /* Add a Sercapacity struct to the tail of the list */
void Get_FailPipe_keyfacility_Attribute();	/* Get property for damaged pipes and critical facilities (hospitals and hydrants) settings */
void Open_inp_file(char*, char*, char*);	/* Open the .inp fire */
LinkedList* Randperm();		 /* Randomly generate visible breaks / leaks operation sequence for engineering team to choose from */
int Get_Select_Repository(); /* Generate visible breaks/leaks related information for random selection function call  */
int Task_Assignment(LinkedList*, STaskassigmentlist*); /* Assign all instructions in the SerialSchedule list to each engineering team */
int Breaks_Adjacent_operation(int, int,int, float, float); /* Iisolate or restore the damage by closing or reopening the pipes related the damage */
int Leaks_operation(int, int, float, float); /* Repair the leaks and recover the leaking pipeline */


#endif
