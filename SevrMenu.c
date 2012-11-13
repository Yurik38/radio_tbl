
#include "ind2_16_1.h"
#include "int_EEPROM.h"
#include "const.h"
#include "ServMenu.h"

extern uchar volatile	ScanCode;
extern uchar volatile	Flags;
extern uchar volatile	Sec;
extern uint  		Voltage;


#define maxMenuItem	(sizeof(servmenu)/sizeof(TSERVMENU))

//текст для меню
static char flash _servm0[] = "Calibration@";
static char flash _servm1[] = "Check stick@";
static char flash _servm2[] = "Voltage@";
static char flash _servm3[] = "Version@";
static char flash Str_Digit[] =  {"12345@"};
static char flash Str_Ver[] =  {"    4.2.2@"};

void Calibrate_Menu(void);
void Check_Menu(void);
void Volt_Menu(void);
void Ver_Menu(void);

TSERVMENU flash servmenu[] = {
  {(char flash *) _servm0, Calibrate_Menu},
  {(char flash *) _servm1, Check_Menu},
  {(char flash *) _servm2, Volt_Menu},
  {(char flash *) _servm3, Ver_Menu}
  };

TCALVALUE       CalValue[5];


/************************************************************************/
/*	О Р Г А Н И З А Ц И Я   С Л У Ж Е Б Н О Г О    М Е Н Ю		*/
/************************************************************************/
void Service_Menu(void)
{

uchar  MenuItem;
  Flags |= (1 << calibrate);
  for (MenuItem = 0; MenuItem < 5; MenuItem++)
  {
    CalValue[MenuItem].Max = 0x0000;
    CalValue[MenuItem].Min = 0xFFFF;
  }
  MenuItem = 0;
  ClrAllDisp();
  WriteStr(servmenu[0].menustr);
  for (;;)
  {
    switch (ScanCode)
    {
      case 0x7E: 	// Enter
        ScanCode = 0;
        SetCursDisp(0,15);
        put_char('*');
        servmenu[MenuItem].func();
        WriteStr(servmenu[MenuItem].menustr);
        break;

      case 0xBE: 	// +
        ScanCode = 0;
        if (MenuItem == (maxMenuItem - 1)) MenuItem = 0;
        else MenuItem++;
        ClrStrDisp(0);
        WriteStr(servmenu[MenuItem].menustr);
        break;

      case 0xDE: 	// -
        ScanCode = 0;
        if (MenuItem == 0) MenuItem = maxMenuItem - 1;
        else MenuItem --;
        ClrStrDisp(0);
        WriteStr(servmenu[MenuItem].menustr);
        break;

      case 0xEE: 	// Esc
        ScanCode = 0;
        ClrAllDisp();
        Flags &= ~(1 << calibrate);
        return;
    }
  }
}

/************************************************************************/
void Calibrate_Menu(void)
{
uchar i;
uint Addr;

  for(;;)
  {
    switch (ScanCode)
    {
      case 0x7E:                        //Enter
        ScanCode = 0;
        Addr = calAddr;
        //Запоминаем калибровачные значения
        for (i = 0; i < 5; i++)
        {
          EEPutInt(Addr, CalValue[i].Max); Addr += 2;
          EEPutInt(Addr, CalValue[i].Min); Addr += 2;
          EEPutInt(Addr, CalValue[i].Avg); Addr += 2;
        }
        ClrAllDisp();
//        Flags &= ~(1 << calibrate);
        return;

      case 0xEE:			//Среднее значение (Esc)
        ScanCode = 0;
        ClrAllDisp();
        return;
    }
    if (Sec == 0)
    {
      SetCursDisp(1, 10);
      WriteHex((uchar)(CalValue[1].Min>>8));
      WriteHex((uchar)(CalValue[1].Min));
      Sec = 40;
    }
  }
}

/************************************************************************/
void Check_Menu(void)
{
uchar Ch = 0;
uchar b = 1;

  SetCursDisp(1, 0);
  WriteStr(Str_Digit);
  SetCursDisp(1, 0);
  OnCursorBlink();
  for(;;)
  {
    switch (ScanCode)
    {
      case 0x7D:					//Секундомер старт/стоп Канал "+"
        ScanCode = 0;
        if (Ch >= 4) Ch = 0;
        else Ch++;
        b |= 1;
        break;

      case 0xBD:					//секундомер "0" Канал "-"
        ScanCode = 0;
        if (Ch == 0) Ch = 4;
        else Ch--;
        b |= 1;
        break;

      case 0xEE:
        ScanCode = 0;
        OffCursor();
        ClrAllDisp();
        return;
    }

    if ((Sec == 0) || (b & 1))
    {
      if (b & 1) b &= ~1;
      else
      {
        OffCursor();
        SetCursDisp(1, 10);
        WriteHex((uchar)(CalValue[Ch].Avg>>8));
        WriteHex((uchar)(CalValue[Ch].Avg));
        Sec = 40;
      }
      SetCursDisp(1, Ch);
      OnCursorBlink();
    }
  }
}

/************************************************************************/
void Volt_Menu(void)
{
uchar b;
uint Tmp;
  SetCursDisp(1, 2);
  for(;;)
  {
    if (ScanCode == 0xEE)
    {
      ScanCode = 0;
      ClrAllDisp();
      return;
    }

    if (Sec == 0)
    {
      SetCursDisp(1, 2);
      put_char(' ');
      Tmp = Voltage * 33;			
      Tmp >>= 8;
      b = Tmp / 100;
      if (b) put_char(b + 0x30);			//рисуем целую часть
      else put_char(' ');
      b = Tmp / 10;
      put_char((b % 10) + 0x30);
      put_char('.');
      put_char((Tmp % 10) + 0x30);
      put_char('V');
      Sec = 40;
    }
  }
}

/************************************************************************/
void Ver_Menu(void)
{

  SetCursDisp(1, 0);
  WriteStr(Str_Ver);
  for(;;)
  {
    if (ScanCode == 0xEE)
    {
      ScanCode = 0;
      ClrAllDisp();
      return;
    }
  }
}
