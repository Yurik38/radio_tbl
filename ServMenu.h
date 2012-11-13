#ifndef _SERVMENU_H_
#define _SERVMENU_H_
typedef struct
{
  char  flash *menustr;     		// Строка
  void   (*func)(void);	  		// Указатель на функцию
} TSERVMENU;

typedef struct
{
  uint    	Max;
  uint    	Min;
  uint    	Avg;
} TCALVALUE;

extern TCALVALUE       CalValue[5];


void Service_Menu(void);
#endif //_SERVMENU_H_



