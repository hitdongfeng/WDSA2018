
//���ļ�������BPDRR�����е�ȫ�ֱ������ļ����ݱ��������ͽ�����֯��ÿ����������һ���򵥵�˵����

#ifndef WDSVARS_H
#define WDSVARS_H
#include "wdstypes.h"

EXTERN int	Nbreaks,		/* ���ܹܵ����� */
			Nleaks,			/* ©ʧ�ܵ����� */
			Ninivariables;	/* ��ʼ��������� */

EXTERN FILE *ErrFile;		/* ���󱨸��ļ�ָ�� */



//EXTERN	PDecision_Variable	Part_init_solution; /* ��ʼ��ָ�� */
EXTERN	SBreaks*	BreaksRepository;	/* ���ֿܲ�ָ��(���ڴ洢���б���) */
EXTERN	SLeaks*		LeaksRepository;	/* ©��ܵ��ֿ�ָ��(���ڴ洢����©��ܵ�) */
EXTERN	SCrew*		Schedule;			/* ���̶ӵ���ָ�� */
EXTERN	LinkedList	linkedlist;			/* ����ܵ��ܵ������޸�ָ��ṹ��(���ڴ洢����ָ��) */







#endif