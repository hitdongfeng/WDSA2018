
//该文件定义了BPDRR程序中使用的函数

#ifndef WDSfUNS_H
#define WDSfUNS_H


int  readdata(char*, char*);		  /* Opens data.txt file & reads parameter data */
void Emptymemory();					  /* Free global variable dynamic memory */
int  Add_tail(LinkedList*, int, int, long, long); /* Add a Decision_Variable struct to the tail of the list */
void Add_SerCapcity_list(Sercaplist*, Sercapacity*); /* Add a Sercapacity struct to the tail of the list */
void Get_FailPipe_keyfacility_Attribute(); /* 获取受损管道和关键基础设置(医院、消火栓)相关属性值 */
void Open_inp_file(char*, char*, char*);	/* 打开.inp文件 */
LinkedList* Randperm();	/* 随机生成可见爆管/漏损操作顺序，供工程队从中选取 */
int Get_Select_Repository(); /* 生成可见爆管/漏损相关信息，供随机选择函数调用 */
int Task_Assignment(LinkedList*, STaskassigmentlist*); /* 将SerialSchedule链表中的所有指令分配至每个工程队 */



#endif
