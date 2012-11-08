/*
Quatrz - 11059200 Hz
Fuses
		OSCCAL  = AF, AE, A9, AB
		BLEV    = 0
		BODEN   = 1
		SUT     = 3
		CKSEL   = F
		BLB1    = 3
		BLB0    = 3
		OCDEN   = 1
		JTAGEN  = 1
		CKOPT   = 1
		EESV    = 0
		BSIZ    = 1
		BRST    = 1
*/

#include "cpu.h"
#include "ind2_16.h"
#include <stdio.h>
#include <string.h>
//#include "rf12.h"
#include "event.h"
#include "UART.h"

#define		MSK_ST_TOUR	0x80
#define		MSK_NEXT	0x20
#define		MSK_PREV	0x40
#define		MSK_CANCEL	0x10

#define		SND_SHORT		0x00
#define		SND_LONG		0x1F
#define		SND_SHORT_SHORT	0x02
#define		SND_SHORT_LONG	0x3E




#define		_mS10		~(432 - 1)  //Prescaller 256


#define		LEDPORT		PORTC
#define		_LedOn(p)	LEDPORT &= ~(1 << p)
#define		_LedOff(p)	LEDPORT |= (1 << p)
#define		_LedOffAll	LEDPORT |= 0x0F


//#define		_SndOn		PORTD &= ~(0x0C)
//#define		_SndOff		PORTD |= (0x0C)
#define		_SndOn		PORTC_Bit3 = 0
#define		_SndOff		PORTC_Bit3 = 1



typedef enum
{
	INIT_ST,
	IDLE_ST,
	READY_TIME_ST,
	TIME_OUT_START_ST,
	TOUR_ST,
	STOP_ST,
	VIEW_ST,
}DEV_STATE;


typedef enum
{
	UPDATE_DISP_TIME,
	UPDATE_DISP_LAP,
	TOUR_GO,
	OUT_OF_BASE
}FLAGS;

uchar volatile	Flags;
uchar			LapNum;			//number of passed lap
uint*			LapResult;		//pointer to current storage cell
uint			Results[10];	//table for pass results
uint			Result;			//current time of tour
uint			PressTime;
uint			ReadyTimer;
uchar			CurCode;		//for buttons handler
uchar volatile	ScanCode;		
uchar volatile	TmpCode;
//uchar volatile	Drebezg;		
uchar volatile	Delay1;			//variable for LCD & UART delay
uchar			StateDev;
uchar volatile	SndTime;
uchar volatile	Ring;
uchar volatile	Sound;
uchar			LastSecondSnd;	//var. Buzzer cnt to end of starting time
/************************************************************************/
/*	П Р Е Р Ы В А Н И Я						*/
/************************************************************************/

#pragma vector = TIMER1_OVF_vect
__interrupt  void TIMER1_OVF_interrupt(void)
{
	uchar i;
	uchar* p;

	TCNT1 += _mS10;
	Result++;			//result. always increment
	if (Flags & (1 << TOUR_GO))
	{
		if (!(Result %10)) Flags |= (1 << UPDATE_DISP_TIME);
	}
	if (ReadyTimer)
	{
		ReadyTimer--;			//ready timer always decrement
		if (!(ReadyTimer % 100)) Flags |= (1 << UPDATE_DISP_TIME);
	}

	if(Delay1) --Delay1;



	//buttons handle

	if (!(PINC & 0x20)) 		//press but 2 Cancel
	{
		if ((CurCode ^ TmpCode) & MSK_CANCEL)
		{
			//      _SndOnShort;
			CurCode |= MSK_CANCEL;
			ScanCode |= MSK_CANCEL;
		}
		else TmpCode |= MSK_CANCEL;
	}
	else {TmpCode &= ~MSK_CANCEL; CurCode &= ~MSK_CANCEL;}

	if (!(PINC & 0x80)) 		//press but  4 Next
	{
		if ((CurCode ^ TmpCode) & MSK_NEXT)
		{
			//      _SndOnShort;
			CurCode |= MSK_NEXT;
			ScanCode |= MSK_NEXT;
		}
		else TmpCode |= MSK_NEXT;
	}
	else {TmpCode &= ~MSK_NEXT; CurCode &= ~MSK_NEXT;}

	if (!(PINC & 0x10)) 		//press but 1 Prev
	{
		if ((CurCode ^ TmpCode) & MSK_PREV)
		{
			//      _SndOnShort;
			CurCode |= MSK_PREV;
			ScanCode |= MSK_PREV;
		}
		else TmpCode |= MSK_PREV;
	}
	else {TmpCode &= ~MSK_PREV; CurCode &= ~MSK_PREV;}

	if (!(PINC & 0x40)) 		//press but 3 StartTour
	{
		if ((CurCode ^ TmpCode) & MSK_ST_TOUR)
		{
			//      _SndOnShort;
			CurCode |= MSK_ST_TOUR;
			ScanCode |= MSK_ST_TOUR;
		}
		else TmpCode |= MSK_ST_TOUR;
	}
	else {TmpCode &= ~MSK_ST_TOUR; CurCode &= ~MSK_ST_TOUR;}

	//Sound controll
	if (SndTime) SndTime--;
	else if (Ring)
	{
		SndTime = 2;
		if (Ring & 0x01) {_SndOn;}
		else {_SndOff;}
		Ring--;
	}
	else if (Sound)
	{
		SndTime = 20;
		if (Sound & 0x01) {_SndOn;}
		else {_SndOff;}
		Sound >>= 1;
	}
	else {_SndOff;}
	// Morgun controll
	for (i = 0; i < 4; i++)
	{
		p = &LedTime[i];
		if (*p) (*p)--;
		else _LedOff(i);
	}

}


/************************************************************************/
/*	Init board */
/************************************************************************/
void InitCPU(void)
{
	PORTA = 0xFE;
	DDRA = 0xFE; //0xFE;
	PORTB = 0xFF;
	DDRB = 0xFB;
	PORTC = 0xFF;
	DDRC = 0x0F;
	PORTD = 0xFF;
	DDRD = 0xFF;
}

/*************************************************************************/
void InitTimers(void)
{
	TIMSK = (1<<TOIE1);
	TCCR1B = 0x04;    //Prescaller 4
	TCNT1 = _mS10;

	//  TCCR1A = 0x00;
	//  TCCR1B = 0x02;
}

/************************************************************************/
/* F U N C T I O N */
/************************************************************************/
inline void SndOn(uchar mask)
{
	_CLI();
	Sound = mask;
	_SndOn;
	SndTime = 20;
	_SEI();
}

/************************************************************************/
inline void SndOnRing(uchar len)
{
	_CLI();
	_SndOn;
	SndTime = 2;
	Ring = len;
	_SEI();
}

/************************************************************************/
void PrintTime(uchar ten_min, uint timer)
{
	putchar(ten_min + 0x30);
	putchar(timer / 6000 + 0x30);
	timer %= 6000;
	putchar(':');
	putchar(timer / 1000 + 0x30);
	timer %= 1000;
	putchar(timer / 100 + 0x30);
	putchar('.');
	timer %= 100;
	putchar(timer / 10 + 0x30);
	putchar(timer % 10 + 0x30);
}

/************************************************************************/
void PrintTimeShort(uchar ten_min, uint timer)
{
	putchar(ten_min + 0x30);
	putchar(timer / 6000 + 0x30);
	timer %= 6000;
	putchar(':');
	putchar(timer / 1000 + 0x30);
	timer %= 1000;
	putchar(timer / 100 + 0x30);
}


/************************************************************************/
void KeyHandler(void)
{

	if (!(ScanCode)) return;

	if (ScanCode & MSK_ST_TOUR)				//tour's begin
	{
		PostEvent(START_ROUND, 0, MAIN_DEV);
		PostEvent(START_ROUND, 0, TURN_BTN);
		PostEvent(START_ROUND, 0, START_BTN);
		ScanCode &= ~MSK_ST_TOUR;
	}

	if (ScanCode & MSK_NEXT)				//but +
	{
		PostEvent(NEXT, 0, MAIN_DEV);
		ScanCode &= ~MSK_NEXT;
	}

	if (ScanCode & MSK_PREV)				//but -
	{
		PostEvent(PREV, 0, MAIN_DEV);
		ScanCode &= ~MSK_PREV;
	}

	if (ScanCode & MSK_CANCEL)				//but reset
	{
		PostEvent(CANCEL, 0, MAIN_DEV);
		PostEvent(CANCEL, 0, TURN_BTN);
		PostEvent(CANCEL, 0, START_BTN);
		ScanCode &= ~MSK_CANCEL;
	}
}

/************************************************************************/
void UpdateDispLap(uchar num)
{
	Flags &= ~(1 << UPDATE_DISP_LAP);
	if (num > 9) return;
	SetCursDisp(0,1);
	putchar((num + 1) / 10 + 0x30);
	putchar((num + 1) % 10 + 0x30);
	SetCursDisp(0,8);
	if (num == 0) PrintTime(0, Results[0]);
	else PrintTime(0, (Results[num] - Results[num - 1]));
}

/************************************************************************/
inline void UpdateDispTime(uint time)
{
	Flags &= ~(1 << UPDATE_DISP_TIME);
	SetCursDisp(1, 8);
	PrintTime(0, time);		// print current time
}



/************************************************************************/
/*		M A I N */
/************************************************************************/

void main(void)
{

	//  uint tmp_param;
	T_EVENT* p_event;
	TPACKET* p_packet;

	uint speed, tmp;

	InitCPU();
	InitTimers();
	InitEventList();
	InitUART(1152);
	//  _UART_RX_EN;
	_SEI();
	LastSecondSnd = 5;
	if (InitDisp() == 0) _LedOn(0);

	SndOn(SND_SHORT_SHORT);
	_LedOffAll;

	StateDev = INIT_ST;
	LapNum = 0;
	ScanCode = 0;

	for(;;)
	{
		KeyHandler();
		p_packet = GetPacket();
		if (p_packet != NULL)
		{
			PostEvent(p_packet->cmd, p_packet->param0, MAIN_DEV);
		}
		p_event = GetEvent();
		if (p_event != NULL)
		{
			if ((p_event->addr > 0) && (p_event->addr < 5)) SendPacket(&p_event->pack, p_event->addr);
			if (p_event->addr != 5) p_event = NULL;
		}
		switch (StateDev)
		{
			case INIT_ST:				//init
				ClrAllDisp();
				WriteStr(" Пульт 1 *F3F*");
				SetCursDisp(1, 0);
				WriteStr(" Всего пультов 1");
				StateDev = IDLE_ST;
				Flags = 0;
				memset(Results, 0, sizeof(Results));
				break;

			case IDLE_ST:				//ready to begin
				if (p_event == NULL) break;
				if (p_event->pack.cmd == START_ROUND)		//event arrived
				{
					_CLI();
					ReadyTimer = 3000;			//30 second
					_SEI();
					ClrAllDisp();
					WriteStr(" Готовность\n");
					SetCursDisp(1, 1);
					PrintTimeShort(0, ReadyTimer);
					StateDev = READY_TIME_ST;
					SndOn(SND_LONG);
				}
				break;

			case READY_TIME_ST:			//time to rase altitude (F3F - 30 sec)
				if (Flags & (1 << UPDATE_DISP_TIME))	//update ready timer
				{
					Flags &= ~(1 << UPDATE_DISP_TIME);
					SetCursDisp(1,1);
					PrintTimeShort(0, ReadyTimer);
					if (ReadyTimer <= (LastSecondSnd*100)) SndOn(SND_SHORT);	//last second (buzzer)
					if (ReadyTimer == 0)
					{
						PostEvent(READY_TIME_OUT, 0, START_BTN);				//time expired. autostart tour. send event to start point
						ClrAllDisp();
						LapNum = 0;
						LapResult = Results;
						Flags |= ((1 << UPDATE_DISP_LAP) + (1 << UPDATE_DISP_TIME) + (1 << TOUR_GO));
						StateDev = TIME_OUT_START_ST;							//go to new state
					}
				}
				if (p_event == NULL) break;										//no event
				if (p_event->pack.cmd == TIME_STAMP)		//event arrived from strart point
				{
					if (Flags & ( 1 << OUT_OF_BASE))		//was out of base - start the rase
					{
						ClrAllDisp();
						LapNum = 0;
						LapResult = Results;
						Flags |= ((1 << UPDATE_DISP_LAP) + (1 << UPDATE_DISP_TIME) + (1 << TOUR_GO));
						SndOn(SND_LONG);
						ReadyTimer = 150;					//1.5 sec no reaction on event
						StateDev = TOUR_ST;					//tour running
						speed = p_event->pack.param0;
						PostEvent(SOUND, 2, TURN_BTN);
					}
					else 		//out of base wait event from start point again
					{
						Flags |= (1 << OUT_OF_BASE);
						SndOn(SND_SHORT_SHORT);
					}
					break;
				}
				if (p_event->pack.cmd == CANCEL) StateDev = INIT_ST;
				break;

			case TIME_OUT_START_ST:
				if (Flags & (1 << UPDATE_DISP_LAP)) UpdateDispLap(LapNum);
				if (Flags & (1 << UPDATE_DISP_TIME)) UpdateDispTime(Result);
				if (p_event == NULL) break;		//no event
				if (p_event->pack.cmd == TIME_STAMP)		// event arrived from start point
				{
					if (Flags & ( 1 << OUT_OF_BASE))		//was out of base - start the rase
					{
						SndOn(SND_LONG);
						ReadyTimer = 150;			//1.5 сек не будем реагировать на кнопки флагов
						StateDev = TOUR_ST;			//переходим в отсчет тура
						speed = p_event->pack.param0;
						PostEvent(SOUND, 2, TURN_BTN);
					}
					else
					{
						Flags |= (1 << OUT_OF_BASE);		//out of base wait event from start point again
						SndOn(SND_SHORT_SHORT);
					}
					break;
				}
				if (p_event->pack.cmd == CANCEL) StateDev = INIT_ST;	//приехало событие отмены. переходим через инит в готовность
				break;

			case TOUR_ST:				//отсчет тура
				if (Flags & (1 << UPDATE_DISP_LAP))
				{
					if (LapNum == 1)		// first pass - print the speed
					{
						SetCursDisp(0,0);
						WriteStr("Скорость ");
						tmp = *(LapResult - 1) - speed;
						speed = 36000 / tmp;
						putchar(speed / 100 + 0x30);
						speed %= 100;
						putchar(speed / 10 + 0x30);
						putchar(speed % 10 + 0x30);
						WriteStr("км/ч");
						Flags &= ~(1 << UPDATE_DISP_LAP);
					}
					else UpdateDispLap(LapNum);
				}
				if (Flags & (1 << UPDATE_DISP_TIME)) UpdateDispTime(Result);
				if (p_event == NULL) break;
/*				if (p_event->pack.cmd == STOP)		//приехало событие финиша
				{
					StateDev = STOP_ST;				//переходим в состояние финиша
					break;
				}*/
				if (p_event->pack.cmd == TIME_STAMP)		//приехало событие отмашки стартовой вешки
				{
					*LapResult = p_event->pack.param0;		//сохраняем время "круга"
					Flags |= 1 << UPDATE_DISP_LAP;
					if (LapNum >= 9)			//если был последний круг
					{
						Flags &= ~(1 << TOUR_GO);		//снимаем флаг тура
//						PostEvent(STOP, 0, MAIN_DEV);		//генерим событие финиша
						PostEvent(CANCEL, 0, START_BTN);
						PostEvent(SOUND, 3, START_BTN);
						PostEvent(CANCEL, 0, TURN_BTN);
						PostEvent(SOUND, 3, TURN_BTN);
						SndOnRing(40);
						StateDev = STOP_ST;				//переходим в состояние финиша
					}
					else					//если не последний круг
					{
						if (LapNum & 0x01) 			//круг четный пикаем поворотной вешкой (порт переключиться на нее)
						{
							PostEvent(SOUND, 1, TURN_BTN);
						}
						else					//круг не четный, пикаем старотовой кнопкой
						{
							PostEvent(SOUND, 1, START_BTN);
						}
						LapNum++;
						LapResult++;
						if (LapNum == 8) SndOn(SND_SHORT_LONG);				//предпоследний ходка
						else SndOn(SND_SHORT_LONG);
					}
					break;
				}
				if (p_event->pack.cmd == CANCEL) StateDev = INIT_ST;	//приехало событие отмены. переходим через инит в готовность
				break;

			case STOP_ST:				//финиш
				if (Flags & (1 << UPDATE_DISP_LAP)) UpdateDispLap(LapNum);
				if (p_event == NULL) break;
				if (p_event->pack.cmd == PREV)			//нажата кнопка "-"
				{
					if (LapNum) LapNum--;
					else LapNum = 9;
					Flags |= (1 << UPDATE_DISP_LAP);
				}
				if (p_event->pack.cmd == NEXT)		//нажата кнопка "+"
				{
					if (LapNum != 9) LapNum++;
					else LapNum = 0;
					Flags |= (1 << UPDATE_DISP_LAP);
				}
				if (p_event->pack.cmd == CANCEL) StateDev = INIT_ST;	//приехало событие отмены. переходим через инит в готовность
				break;
			default:
				break;
		}
	}
}

