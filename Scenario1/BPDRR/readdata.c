/*******************************************************************
Name: readdata.c
Purpose: 用于读取data.txt文件中的数据
Data: 5/3/2018
Author: Qingzhou Zhang
Email: wdswater@gmail.com
********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "wdstext.h"
#include "wdstypes.h"
#define EXTERN extern
#include "wdsvars.h"
#define MAXERRS  5   /* 错误信息累积最大次数 */

char *Tok[MAX_TOKS];	/* 定义字段数组，用于存储字段 */
int Ntokens;			/* data.txt中每行字段数量 */
int hospital_count;		/* 医院设施计数 */
int	firefight_count;	/* 消火栓计数 */
int	break_count;		/* 爆管计数 */
int	leak_count;			/* 漏损管道计数 */

/* 定义data.txt文件标题数组 */
char *Sect_Txt[] = {"[Hospital]", 
					"[Firefight]",
					"[BREAKS]",
					"[LEAKS]",
					"[Decision_Variable]",
					"[Schedule_Crew1]",
					"[Schedule_Crew2]",
					"[Schedule_Crew3]",
					NULL
				   };

/* 定义data.txt文件标题枚举 */
enum Sect_Type {
	_Hospital,
	_Firefight,
	_BREAKS,
	_LEAKS,
	_Decision_Variable,
	_Schedule_Crew1,
	_Schedule_Crew2,
	_Schedule_Crew3,
	_END
};


int  str_comp(char *s1, char *s2)
/*---------------------------------------------------------------
**  Input:   s1 = character string; s2 = character string                                
**  Output:  1 if s1 is same as s2, 0 otherwise                                                      
**  Purpose: case insensitive comparison of strings s1 & s2  
**---------------------------------------------------------------*/
{
	for (int i = 0; U_CHAR(s1[i]) == U_CHAR(s2[i]); i++)
		if (!s1[i + 1] && !s2[i + 1]) return(1);
	return(0);
}                                       

void Open_file(char *f1,char *f2)
/*----------------------------------------------------------------
**  Input: f1 = data.txt文件指针, f2 = 错误报告文件指针
**  Output: none
**  Purpose: 打开数据和错误报告文件
**----------------------------------------------------------------*/
{
	/* 初始化文件指针为 NULL */
	InFile = NULL;
	ErrFile = NULL;

	if (str_comp(f1, f2))
	{
		printf("Cannot use duplicate file names!\n");
		assert(0); //终止程序，返回错误信息
	}

	/* 只读方式打开数据文件 */
	if ((InFile = fopen(f1, "rt")) == NULL || (ErrFile = fopen(f2, "wt")) == NULL)
	{
		printf("Can not open the data.txt or error report file!\n");
		assert(0); //终止程序，返回错误信息
	}
}

void initializeList(LinkedList *list)
/*----------------------------------------------------------------
**  Input:   *list, pointer to list
**  Output:  none
**  Purpose: initializes LinkedList pointers to NULL
**----------------------------------------------------------------*/
{
	list->head = NULL;
	list->tail = NULL;
	list->current = NULL;
}

void Init_pointers()
/*----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: initializes global pointers to NULL
**----------------------------------------------------------------*/
{
	//Part_init_solution = NULL;	/* 初始解指针 */
	Hospitals = NULL;				/* 医院设施结构体指针 */
	Firefighting = NULL;			/* 消火栓结构体指针 */
	BreaksRepository = NULL;		/* 爆管仓库指针(用于存储所有爆管) */
	LeaksRepository = NULL;			/* 漏损管道仓库指针(用于存储所有漏损管道) */
	Schedule = NULL;				/* 工程队调度指针 */
	initializeList(&decisionlist);	/* 决策变量指针结构体 */
	initializeList(&IniVisDemages);	/* 模拟开始时刻(6:30)可见受损管道数组指针 */
	initializeList(&NewVisDemages);	/* 修复过程中新出现的可见受损管道数组指针 */
	ActuralBaseDemand = NULL;		/* 节点实际需水量数组指针 */
}

int  Str_match(char *str, char *substr)
/*--------------------------------------------------------------
**  Input:   *str    = string being searched
**           *substr = substring being searched for
**  Output:  returns 1 if substr found in str, 0 if not
**  Purpose: sees if substr matches any part of str
**--------------------------------------------------------------*/
{
	int i, j;

	if (!substr[0]) return(0);	/* Fail if substring is empty */

	/* Skip leading blanks of str. */
	for (i = 0; str[i]; i++)
		if (str[i] != ' ') break;

	/* Check if substr matches remainder of str. */
	for (i = i, j = 0; substr[j]; i++, j++)
		if (!str[i] || U_CHAR(str[i]) != U_CHAR(substr[j]))
			return(0);
	return(1);
}

int  Find_match(char *line, char *keyword[])
/*--------------------------------------------------------------
**  Input:   *line      = line from input file
**           *keyword[] = list of NULL terminated keywords
**  Output:  returns index of matching keyword or
**           -1 if no match found
**  Purpose: determines which keyword appears on input line
**--------------------------------------------------------------*/
{
	int i = 0;
	while (keyword[i] != NULL)
	{
		if (Str_match(line, keyword[i])) return(i);
		i++;
	}
	return(-1);
}

void  Get_count()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: determines number of system elements
**--------------------------------------------------------------*/
{
	char  line[MAX_LINE + 1];	/* Line from data.txt file    */
	char  *tok;                 /* First token of line          */
	int   sect, newsect;        /* data.txt sections          */

	/* Initialize network component counts */
	Nhospital = 0;		/* 医院基础设施数量 */
	Nfirefight = 0;		/* 消火栓数量 */
	Nbreaks = 0;		/* 爆管管道数量 */
	Nleaks = 0;			/* 漏失管道数量 */
	Ndecisionvars = 0;	/* 初始决策变量数量 */
	NvarsCrew1 = 0;		/* crew1初始解决策变量 */
	NvarsCrew2 = 0;		/* crew2初始解决策变量 */
	NvarsCrew3 = 0;		/* crew3初始解决策变量 */
	sect = -1;			/* data.txt中数据片段索引 */

	/* Make pass through data.txt counting number of each parameter */
	while (fgets(line, MAX_LINE, InFile) != NULL)
	{
		/* Skip blank lines & those beginning with a comment */
		tok = strtok(line, DATA_SEPSTR);
		if (tok == NULL) continue;
		if (*tok == ';') continue;

		/* Check if line begins with a new section heading */
		if (*tok == '[')
		{
			newsect = Find_match(tok, Sect_Txt);
			if (newsect >= 0)
			{
				sect = newsect;
				if (sect == _END) break;
				continue;
			}
			else continue;
		}
		/* Add to count of current component */
		switch (sect)
		{
			case _Hospital:	Nhospital++;	break;
			case _Firefight: Nfirefight++;	break;
			case _BREAKS:	Nbreaks++;	break;
			case _LEAKS:	Nleaks++;	break;
			case _Decision_Variable:  Ndecisionvars++;    break;
			case _Schedule_Crew1:	NvarsCrew1++; break;
			case _Schedule_Crew2:	NvarsCrew2++; break;
			case _Schedule_Crew3:	NvarsCrew3++; break;
			default: break;
		}
	}
}

int  Alloc_Memory()
/*----------------------------------------------------------------
**  Input:   none
**  Output:  error code
**  Returns: error code
**  Purpose: allocates memory for breaks and leanks 
**----------------------------------------------------------------*/
{
	int errcode = 0, err_count = 0;
	
	Hospitals = (SHospital*)calloc(Nhospital, sizeof(SHospital));
	Firefighting = (SFirefight*)calloc(Nfirefight, sizeof(SFirefight));

	if (Nbreaks > 0)
		BreaksRepository = (SBreaks*)calloc(Nbreaks, sizeof(SBreaks));

	if (Nleaks > 0)
		LeaksRepository = (SLeaks*)calloc(Nleaks, sizeof(SLeaks));

	Schedule = (LinkedList*)calloc(MAX_CREWS, sizeof(LinkedList));

	ActuralBaseDemand = (float**)calloc(Ndemands, sizeof(float*));
	for (int i = 0; i < Ndemands; i++)
		ActuralBaseDemand[i] = (float*)calloc(Pattern_length, sizeof(float));

	ERR_CODE(MEM_CHECK(Hospitals));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(Firefighting));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(BreaksRepository));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(LeaksRepository));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(Schedule));	if (errcode) err_count++;
	ERR_CODE(MEM_CHECK(ActuralBaseDemand));	if (errcode) err_count++;

	if (err_count)
	{
		fprintf(ErrFile, ERR402);
		return (402);
	}

	return errcode;
}

int  Get_tokens(char *s)
/*--------------------------------------------------------------
**  输入: *s = string to be tokenized
**  输出: returns number of tokens in s
**  功能: scans string for tokens, saving pointers to them
**       in module global variable Tok[]
**--------------------------------------------------------------*/
{
	int  len, m, n;
	char *c;

	/* Begin with no tokens */
	for (n = 0; n < MAX_TOKS; n++) Tok[n] = NULL;
	n = 0;

	/* Truncate s at start of comment */
	c = strchr(s, ';');
	if (c) *c = '\0';
	len = strlen(s);

	/* Scan s for tokens until nothing left */
	while (len > 0 && n < MAX_TOKS)
	{
		m = strcspn(s, DATA_SEPSTR);      /* Find token length */
		len -= m + 1;                     /* Update length of s */
		if (m == 0) s++;				  /* No token found */
		else
		{
			if (*s == '"')                /* Token begins with quote */
			{
				s++;                      /* Start token after quote */
				m = strcspn(s, "\"\n\r"); /* Find end quote (or EOL) */
			}
			s[m] = '\0';                  /* Null-terminate the token */
			Tok[n] = s;                   /* Save pointer to token */
			n++;                          /* Update token count */
			s += m + 1;                   /* Begin next token */
		}
	}
	return(n);
}

int  Get_int(char *s, int *y)
/*-----------------------------------------------------------
**  输入: *s = character string
**  输出: *y = int point number
**             returns 1 if conversion successful, 0 if not
**  功能: converts string to int point number
**-----------------------------------------------------------*/
{
	char *endptr;
	*y = (int)strtod(s, &endptr);
	if (*endptr > 0) return(0);
	return(1);
}

int  Get_long(char *s, long *y)
/*-----------------------------------------------------------
**  输入: *s = character string
**  输出: *y = int point number
**             returns 1 if conversion successful, 0 if not
**  功能: converts string to int point number
**-----------------------------------------------------------*/
{
	char *endptr;
	*y = (long)strtod(s, &endptr);
	if (*endptr > 0) return(0);
	return(1);
}

int  Get_float(char *s, float *y)
/*-----------------------------------------------------------
**  输入: *s = character string
**  输出: *y = float point number
**             returns 1 if conversion successful, 0 if not
**  功能: converts string to floating point number
**-----------------------------------------------------------*/
{
	char *endptr;
	*y = (float)strtod(s, &endptr);
	if (*endptr > 0) return(0);
	return(1);
}

int Add_tail(LinkedList *list, int index, int type,long starttime,long endtime)
/*--------------------------------------------------------------
**  Input:   list: pointer to LinkedList array
**			 index: 管道数组索引,从0开始
**			 type: 管道类型, 1:爆管隔离; 2:爆管替换; 3:漏损修复; 4:开阀
**		     type: 受损管道类型, 1:爆管; 2:漏损	
**			 starttime: 修复开始时刻
**			 endtime: 修复结束时刻
**  Output:  error code
**  Purpose: Add a Decision_Variable struct to the tail of the list
**--------------------------------------------------------------*/
{
	int errcode = 0; /* 初始化错误代码 */
	PDecision_Variable p;	/* 临时变量，用于存储可见爆管或漏损管道信息 */
	p = (PDecision_Variable)calloc(1, sizeof(struct Decision_Variable));
	ERR_CODE(MEM_CHECK(p));	if (errcode) return 402;
	p->type = type;
	p->index = index;
	p->starttime = starttime;
	p->endtime = endtime;
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

void Add_SerCapcity_list(Sercaplist* list, Sercapacity *ptr)
/*--------------------------------------------------------------
**  Input:   list: pointer to Sercaplist chain table
**			 ptr: 需要插入的Sercapacity结构体指针
**  Output:  none
**  Purpose: Add a Sercapacity struct to the tail of the list
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

int Hospital_data()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  errcode code
**  Purpose: processes  hospital data
**  Format:
**	[Hospital]
**  ;ID
**--------------------------------------------------------------*/
{
	if (Nhospital > 0)
	{
		strncpy(Hospitals[hospital_count].nodeID, Tok[0], MAX_ID);
		strncpy(Hospitals[hospital_count].pipeID, Tok[1], MAX_ID);
	}
	hospital_count++;

	return 0;
}

int Firefight_data()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  errcode code
**  Purpose: processes  firefighting data
**  Format:
**	[Hospital]
**  ;ID		Design_flow(L/s)
**--------------------------------------------------------------*/
{
	float x;
	if (Nfirefight > 0)
	{
		strncpy(Firefighting[firefight_count].ID, Tok[0], MAX_ID);
		if (!Get_float(Tok[1], &x))	return (403); /* 数值类型错误，含有非法字符 */
		Firefighting[firefight_count].fire_flow = x;
	}
	firefight_count++;

	return 0;
}

int Breaks_Value()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  error code
**  Purpose: processes  initialsolution data
**  Format:
**  [Initial_Solution]
**  PipeID BreakID Diameter(mm)	pipes_closed_for_isolation Isolation_time(min) replacement(h)
**--------------------------------------------------------------*/
{
	int errcode = 0;
	int time1, time2;
	float dia;
	SFailurePipe* ptr= (SFailurePipe*)calloc(Ntokens-5, sizeof(SFailurePipe));
	ERR_CODE(MEM_CHECK(ptr)); if (errcode) return(402);

	strncpy(BreaksRepository[break_count].pipeID, Tok[0], MAX_ID);
	strncpy(BreaksRepository[break_count].nodeID, Tok[1], MAX_ID);

	if (!Get_float(Tok[2], &dia)) return (403); /* 数值类型错误，含有非法字符 */
	BreaksRepository[break_count].pipediameter = dia;

	for (int i = 3; i < Ntokens - 2; i++)
		strncpy(ptr[i-3].pipeID, Tok[i], MAX_ID);
	BreaksRepository[break_count].pipes = ptr;
	BreaksRepository[break_count].num_isovalve = Ntokens - 5;

	if (!Get_int(Tok[Ntokens - 2], &time1)) return (403); /* 数值类型错误，含有非法字符 */
	if (!Get_int(Tok[Ntokens - 1], &time2)) return (403); /* 数值类型错误，含有非法字符 */ 
	BreaksRepository[break_count].isolate_time = time1;
	BreaksRepository[break_count].replace_time = time2;

	break_count++;

	return (errcode);
}

int Leaks_Value()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  error code
**  Purpose: processes  initialsolution data
**  Format:
**  [Initial_Solution]
**  PipeID LeakID Diameter(mm) reparation(h)
**--------------------------------------------------------------*/
{
	int time1;
	float dia;

	strncpy(LeaksRepository[leak_count].pipeID, Tok[0], MAX_ID);
	strncpy(LeaksRepository[leak_count].nodeID, Tok[1], MAX_ID);

	if (!Get_float(Tok[2], &dia))	return (403);
	LeaksRepository[leak_count].pipediameter = dia;

	if (!Get_int(Tok[Ntokens - 1], &time1))	return (403);
	LeaksRepository[leak_count].repair_time = time1;

	leak_count++;

	return 0;
}

int DecisionVars_data()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  errcode code
**  Purpose: processes  initialsolution data
**  Format:
**  [Initial_Solution]
**  Type	index
**--------------------------------------------------------------*/
{
	int x, y;
	int errcode;

	if (Ndecisionvars > 0)
	{

		if (!Get_int(Tok[0], &x))	return (403); /* 数值类型错误，含有非法字符 */
		if (!Get_int(Tok[1], &y))	return (403); /* 数值类型错误，含有非法字符 */
		errcode = Add_tail(&decisionlist, x, y,0,0);
	}
	return errcode;
}

int Crew_data(LinkedList *ptr,int NvarsCrew)
/*--------------------------------------------------------------
**  Input:   ptr: LinkedList结构体指针; NvarsCrew: Crew决策变量
**  Output:  errcode code
**  Purpose: processes  Crew data
**  Format:
**  [Initial_Solution]
**  index	type	starttime	endtime
**--------------------------------------------------------------*/
{
	int index, type;
	int errcode;
	long starttime, endtime;
	if (NvarsCrew > 0)
	{

		if (!Get_int(Tok[0], &index))	return (403); /* 数值类型错误，含有非法字符 */
		if (!Get_int(Tok[1], &type))	return (403); /* 数值类型错误，含有非法字符 */
		if (!Get_long(Tok[2], &starttime))	return (403); /* 数值类型错误，含有非法字符 */
		if (!Get_long(Tok[3], &endtime))	return (403); /* 数值类型错误，含有非法字符 */
		errcode = Add_tail(ptr, index, type,starttime,endtime);
	}
	return errcode;
}

int  newline(int sect, char *line)
/*--------------------------------------------------------------
**  Input:   sect  = current section of input file
**           *line = line read from input file
**  Output:  returns error code or 0 if no error found
**  Purpose: processes a new line of data from input file
**--------------------------------------------------------------*/
{
	switch (sect)
	{
	case _Hospital: return(Hospital_data()); break;
	case _Firefight: return(Firefight_data()); break;
	case _BREAKS:	return(Breaks_Value()); break;
	case _LEAKS:	return(Leaks_Value()); break;
	case _Decision_Variable:	return(DecisionVars_data()); break;
	case _Schedule_Crew1: return(Crew_data(&Schedule[0],NvarsCrew1)); break;
	case _Schedule_Crew2: return(Crew_data(&Schedule[1], NvarsCrew2)); break;
	case _Schedule_Crew3: return(Crew_data(&Schedule[2], NvarsCrew3)); break;
	}
	return(403);
}

int  readdata(char *f1, char *f2)
/*----------------------------------------------------------------
**  Input:   f1 = pointer to name of data.txt file; 
**			 f2 = pointer to name of errcode report file;
**  Output:  error code
**  Purpose: opens data file & reads parameter data
**----------------------------------------------------------------*/
{
	int		errcode = 0,
			inperr, errsum = 0;	  /* 错误代码与错误总数 */
	char	line[MAX_LINE + 1],   /* Line from input data file       */
			wline[MAX_LINE + 1];  /* Working copy of input line      */
	int		sect, newsect;        /* Data sections                   */
	
	Ntokens = 0;			/* 每行字段数量 */
	hospital_count=0;		/* 医院设施计数 */
	firefight_count = 0;	/* 消火栓计数 */
	break_count = 0;	    /* 爆管计数 */
	leak_count = 0;			/* 漏损管道计数 */

	Init_pointers();			/* Initialize global pointers to NULL. */
	Open_file(f1,f2);			/* Open input & report files */
	Get_count();				/* 获取相关类型参数的数量 */

	errcode = Alloc_Memory();	/* 为相关类型参数结构体分配内存 */
	if (errcode)
	{
		fprintf(ErrFile, ERR402);
		return (402);
	}

	rewind(InFile);				/* 将指针指向文件开头 */

	while (fgets(line, MAX_LINE, InFile) != NULL)
	{

		/* Make copy of line and scan for tokens */
		strcpy(wline, line);
		Ntokens = Get_tokens(wline);

		/* Skip blank lines and comments */
		if (Ntokens == 0) continue;
		if (*Tok[0] == ';') continue;

		/* 检查字符串是否超过了每行最大长度*/
		if (strlen(line) >= MAX_LINE)
		{
			fprintf(ErrFile, ERR404);
			fprintf(ErrFile, "%s\n", line);
			errsum++;
		}

		/* Check if at start of a new input section */
		if (*Tok[0] == '[')
		{
			newsect = Find_match(Tok[0], Sect_Txt);
			if (newsect >= 0)
			{
				sect = newsect;
				if (sect == _END) break;
				continue;
			}
			else
			{
				fprintf(ErrFile, ERR405,line);
				errsum++;
				break;
			}
		}
		/* Otherwise process next line of input in current section */
		else
		{
			inperr = newline(sect, line);
			if (inperr > 0)
			{
				fprintf(ErrFile, ERR403);
				errsum++;
			}
		}
		/* 搜索到文件末尾或达到错误信息最大数量时，结束while循环 */
		if (errsum == MAXERRS) break;
	}   /* End of while */

	if (errsum > 0)  errcode = 406; //输入文件中有一处或多处错误

	return (errcode);
}

void Emptymemory()
/*--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: free memory
**--------------------------------------------------------------*/
{
	/* 释放Hospitals数组内存 */
	SafeFree(Hospitals);

	/* 释放Firefighting数组内存 */
	SafeFree(Firefighting);
	
	/* 释放BreaksRepository数组内存 */
	for (int i = 0; i < Nbreaks; i++)
	{
		SafeFree(BreaksRepository[i].pipes);
		SafeFree(BreaksRepository[i]);
	}
	/* 释放LeaksRepository数组内存 */
	for (int i = 0; i < Nleaks; i++)
		SafeFree(LeaksRepository[i]);

	/* 释放decisionlist链表指针 */
	decisionlist.current = decisionlist.head;
	while (decisionlist.current != NULL)
	{
		decisionlist.head = decisionlist.head->next;
		SafeFree(decisionlist.current);
		decisionlist.current = decisionlist.head;
	}

	/* 释放Schedule数组内存 */
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

	/* 释放IniVisDemages链表指针 */
	IniVisDemages.current = IniVisDemages.head;
	while (IniVisDemages.current != NULL)
	{
		IniVisDemages.head = IniVisDemages.head->next;
		SafeFree(IniVisDemages.current);
		IniVisDemages.current = IniVisDemages.head;
	}

	/* 释放IniVisDemages链表指针 */
	NewVisDemages.current = NewVisDemages.head;
	while (NewVisDemages.current != NULL)
	{
		NewVisDemages.head = NewVisDemages.head->next;
		SafeFree(NewVisDemages.current);
		NewVisDemages.current = NewVisDemages.head;
	}

	/* 释放ActuralBaseDemand数组指针 */
	for (int i = 0; i < Ndemands; i++)
	{
		SafeFree(ActuralBaseDemand[i]);
	}
	SafeFree(ActuralBaseDemand);

	/* 释放SerCapcPeriod链表指针 */
	SerCapcPeriod.current = SerCapcPeriod.head;
	while (SerCapcPeriod.current != NULL)
	{
		SerCapcPeriod.head = SerCapcPeriod.head->next;
		SafeFree(SerCapcPeriod.current);
		SerCapcPeriod.current = SerCapcPeriod.head;
	}
}

//void File_close()
///*----------------------------------------------------------------
//**  Input:   none
//**  Output:  none
//**  Returns: none
//**  Purpose: frees all memory & files used by BPDRR
//**----------------------------------------------------------------*/
//{
//	if (InFile != NULL) fclose(InFile);
//	if (ErrFile != NULL) fclose(ErrFile);
//}


//int main(void)
//{
//	int errcode;
//
//	errcode = readdata("data.txt", "err.txt");
//	fclose(ErrFile);
//
//	for (int i = 0; i < Nhospital; i++)
//		printf("%s	%s\n", Hospitals[i].nodeID,Hospitals[i].pipeID);
//	printf("\n--------------------\n");
//
//	for (int i = 0; i < Nfirefight; i++)
//		printf("%s	%f\n", Firefighting[i].ID, Firefighting[i].fire_flow);
//	printf("\n--------------------\n");
//
//	for (int i = 0; i < Nbreaks; i++)
//	{
//		printf("%s	%s	%f	", BreaksRepository[i].pipeID, BreaksRepository[i].nodeID, BreaksRepository[i].pipediameter);
//		for (int j = 0; j < BreaksRepository[i].num_isovalve; j++)
//			printf("%s ", BreaksRepository[i].pipes[j].pipeID);
//		printf("%d	%d,	%d	%d	%d\n",
//			BreaksRepository[i].isolate_time, BreaksRepository[i].replace_time, BreaksRepository[i].isolate_flag, BreaksRepository[i].replace_flag, BreaksRepository[i].reopen_flag);
//	}
//	printf("--------------------\n");
//	for (int i = 0; i < Nleaks; i++)
//	{
//		printf("%s	%s	%f	%d, %d	%d\n", LeaksRepository[i].pipeID, LeaksRepository[i].nodeID, LeaksRepository[i].pipediameter, LeaksRepository[i].repair_time, LeaksRepository[i].repair_flag, LeaksRepository[i].reopen_flag);
//	}
//
//	printf("--------------------\n");
//	decisionlist.current = decisionlist.head;
//	while (decisionlist.current != NULL)
//	{
//		printf("%d	%d\n", decisionlist.current->index, decisionlist.current->type);
//		decisionlist.current = decisionlist.current->next;
//	}
//
//	printf("--------------------\n");
//	for (int i = 0; i < MAX_CREWS; i++)
//	{
//		Schedule[i].current = Schedule[i].head;
//		while (Schedule[i].current != NULL)
//		{
//			printf("%d	%d	%d	%d\n", Schedule[i].current->index, Schedule[i].current->type, Schedule[i].current->starttime,Schedule[i].current->endtime);
//			Schedule[i].current = Schedule[i].current->next;
//		}
//		printf("\n\n");
//	}
//
//	Emptymemory();
//	getchar();
//
//	return 0;
//
//}
