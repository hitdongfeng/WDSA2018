
//该文件定义了BPDRR程序中使用的函数

#ifndef WDSfUNS_H
#define WDSfUNS_H


int  readdata(char*, char*);		  /* Opens data.txt file & reads parameter data */
int GetDemand(char*);	/* 获取所有时间步长(小时)需水量节点实际需水量 */
void Emptymemory();					  /* Free global variable dynamic memory */
int  Add_tail(LinkedList*, int, int, long, long); /* Add a Decision_Variable struct to the tail of the list */
void Add_SerCapcity_list(Sercaplist*, Sercapacity*); /* Add a Sercapacity struct to the tail of the list */
void Get_FailPipe_keyfacility_Attribute(); /* 获取受损管道和关键基础设置(医院、消火栓)相关属性值 */
void Open_inp_file(char*, char*, char*);	/* 打开.inp文件 */
LinkedList* Randperm();	/* 随机生成可见爆管/漏损操作顺序，供工程队从中选取 */
int Get_Select_Repository(); /* 生成可见爆管/漏损相关信息，供随机选择函数调用 */
int Task_Assignment(LinkedList*, STaskassigmentlist*); /* 将SerialSchedule链表中的所有指令分配至每个工程队 */
int Breaks_Adjacent_operation(int, int,int, float, float); /* 关闭或开启爆管附近管道，对爆管进行隔离或复原 */
int Leaks_operation(int, int, float, float); /* 修复漏损管道，对漏损管道进行复原 */


#endif
