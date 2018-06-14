
//This file defines the global variables in the BPDRR program.

#ifndef WDSVARS_H
#define WDSVARS_H
#include "wdstypes.h"

EXTERN int	Nhospital,		/* No. of hospitals */
			Nfirefight,		/* No. of fire hydrants */
			Nbreaks,		/* No. of Breaks */
			Nleaks,			/* No. of Leaks */
			Ndecisionvars,	/* New-Added decision variables */
			NvarsCrew1,		/* No. of decision variables for crew1 */
			NvarsCrew2,		/* No. of decision variables for crew2 */
			NvarsCrew3;		/* No. of decision variables for crew3 */

EXTERN char *inpfile;		/* .inp file */

EXTERN time_t	rawtime;	/* System Current Time */

EXTERN FILE *ErrFile,		/* Point to error file */
            *InFile,		/* point to data.txt file */
            *SenAnalys,		/* point to SenAnalys file */
			*key_solution;	/* Output each objective value */

EXTERN	SHospital* Hospitals;			/* Point to hospital struct */
EXTERN	SFirefight* Firefighting;		/* Point to hydrant struct */
EXTERN	SBreaks*	BreaksRepository;	/* Point to breaks repository */
EXTERN	SLeaks*		LeaksRepository;	/* Point to leaks repository */
EXTERN	LinkedList*	ExistSchedule;		/* Existing engineering team scheduling (initial Solution) */
EXTERN	LinkedList	decisionlist;		/* Point to decision list */
EXTERN	LinkedList	IniVisDemages;		/* Inilize visible damages at the begin of restoration (6:30) */
EXTERN	float**	ActuralBaseDemand;		/* Point to actual nodal demand array */
EXTERN	Sercaplist  SerCapcPeriod;		/* Water supply capacity of WDS at each time step*/
EXTERN	SCriteria* Criteria;			/* Count the No. of nodes without service for more than 8 consecutive hours (C_05) */

#endif
