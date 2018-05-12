
//该文件定义了BPDRR程序中使用的函数

#ifndef WDSfUNS_H
#define WDSfUNS_H


int  readdata(char*, char*);		  /* Opens data.txt file & reads parameter data */
void Emptymemory();					  /* Free global variable dynamic memory */
int  Add_tail(LinkedList*, int, int, long, long); /* Add a Decision_Variable struct to the tail of the list */








#endif
