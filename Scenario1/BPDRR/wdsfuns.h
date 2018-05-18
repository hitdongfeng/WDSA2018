
//该文件定义了BPDRR程序中使用的函数

#ifndef WDSfUNS_H
#define WDSfUNS_H


int  readdata(char*, char*);		  /* Opens data.txt file & reads parameter data */
void Emptymemory();					  /* Free global variable dynamic memory */
int  Add_tail(LinkedList*, int, int, long, long); /* Add a Decision_Variable struct to the tail of the list */
void Add_SerCapcity_list(Sercaplist*, Sercapacity*); /* Add a Sercapacity struct to the tail of the list */
void Open_file(char*, char*);			/* 打开数据和错误报告文件 */
void Get_FailPipe_keyfacility_Attribute(); /* 获取受损管道和关键基础设置(医院、消火栓)相关属性值 */
void Open_inp_file(char*, char*, char*);	/* 打开.inp文件 */




#endif
