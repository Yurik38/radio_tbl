#ifndef _EVENT_H_
#define _EVENT_H_

/* Код команды в кнопку */
#define		START_ROUND	0x01
#define		CANCEL		0x02
#define		NUM_PACK_Q	0x03
#define		BAT_VOLT_Q	0x04
#define		SOUND		0x05
#define		PARAM		0x06
/*кнопки и внутренние события пульта*/
#define		NEXT		0x07
#define		PREV		0x08
#define 	READY_TIME_OUT	0x09
#define		STOP		0x0A
/* Код ответа из кнопки */
#define		TIME_STAMP	0x10
#define		NUM_PACKET	0x11
#define		VOLTAGE		0x12

typedef struct
{
	uint		param0;
	uchar		cmd;
	uchar		addr;
}T_EVENT;

void InitEventList(void);
void PostEvent(uchar cmd, uint param, uchar addr);
T_EVENT* GetEvent(void);
T_EVENT* GetCurEventAddr(void);

#endif //_EVENT_H_
