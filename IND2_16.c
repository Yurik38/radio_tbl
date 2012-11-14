
/*Набор функций по работе с ЖКД 16*2 символов (BC1602A) 4-bit 24.05.00
Поддержка руской таблицы 08.08.10*/
#include "cpu.h"
//#define DEBUG
#define RUS
#ifdef DEBUG
  #define NO_DISP		//макрос отключает ожидание готовности дисплея для дебага в AVRStudio
#endif

//порты ввода/вывода подключенные к индикатору
#define	DData		DDRA
#define DataW		PORTA
#define	DataR		PINA
#define	_RS0		PORTA_Bit1 = 0; _NOP()
#define	_RS1		PORTA_Bit1 = 1; _NOP()
#define	_RW0		PORTA_Bit2 = 0; _NOP()
#define	_RW1		PORTA_Bit2 = 1; _NOP()
#define	_EI0		_NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); PORTA_Bit3 = 0; _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP()				
#define	_EI1		_NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); PORTA_Bit3 = 1; _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP()


#define   mS		2000               //Для 3.6864 МГц кварца

#define   CHECK		1                 //Проверять готовность дисплея
#define   NO_CHECK	0                 //Не проверять готовность дисплея

#define   FUN_SET	0x28              //Функция установки параметров дисплея
#define   DISP_ON	0x0C              //Display on (без курсора)
#define   DISP_CLR	0x01              //Очистка дисплея
#define   ENTRY_MOD	0x06              //Entry mode set

#define   TimeOutBusyInd  50

uchar SymbolPtr;	             //Указатель по таблице знакогенератора
extern uchar volatile	Delay1;

flash uchar SymbolTabRUS[64] = {	//Таблица подстановки символов из дисплея
  0x41,		//C0 "А"
  0xA0,		//C1 "Б"
  0x42,		//C2 "В"
  0xA1,		//C3 "Г"
  0xE0,		//C4 "Д"
  0x45,		//C5 "Е"
  0xA3,		//C6 "Ж"
  0xA4,		//C7 "З"
  0xA5,		//C8 "И"
  0xA6,		//C9 "Й"
  0x4B,		//CA "К"
  0xA7,		//CB "Л"
  0x4D,		//CC "М"
  0x48,		//CD "Н"
  0x4F,		//CE "О"
  0xA8,		//CF "П"
  0x50,		//D0 "Р"
  0x43,		//D1 "С"
  0x54,		//D2 "Т"
  0xA9,		//D3 "У"
  0xAA,		//D4 "Ф"
  0x58,		//D5 "Х"
  0xE1,		//D6 "Ц"
  0xAB,		//D7 "Ч"
  0xAC,		//D8 "Ш"
  0xE2,		//D9 "Щ"
  0xAD,		//DA "Ъ"
  0xAE,		//DB "Ы"
  0x62,		//DC "Ь"
  0xAF,		//DD "Э"
  0xB0,		//DE "Ю"
  0xB1,		//DF "Я"
  0x61,		//E0 "а"
  0xB2,		//E1 "б"
  0xB3,		//E2 "в"
  0xB4,		//E3 "г"
  0xE3,		//E4 "д"
  0x65,		//E5 "е"
  0xB6,		//E6 "ж"
  0xB7,		//E7 "з"
  0xB8,		//E8 "и"
  0xB9,		//E9 "й"
  0xBA,		//EA "к"
  0xBB,		//EB "л"
  0xBC,		//EC "м"
  0xBD,		//ED "н"
  0x6F,		//EE "о"
  0xBE,		//EF "п"
  0x70,		//F0 "р"
  0x63,		//F1 "с"
  0xBF,		//F2 "т"
  0x79,		//F3 "у"
  0xE4,		//F4 "ф"
  0x78,		//F5 "х"
  0xE5,		//F6 "ц"
  0xC0,		//F7 "ч"
  0xC1,		//F8 "ш"
  0xE6,		//F9 "щ"
  0xC2,		//FA "ъ"
  0xC3,		//FB "ы"
  0xC4,		//FC "ь"
  0xC5,		//FD "э"
  0xC6,		//FE "ю"
  0xC7		//FF "я"
};

flash uchar TabCG[3][8] = {0x0E,0x11,0x11,0x11,0x1F,0x11,0x11,0x00,  //C0 "А" 		//пример знакогенератора
                            0x1F,0x10,0x10,0x1E,0x11,0x11,0x1E,0x00,  //C1 "Б"
                            0x1E,0x11,0x11,0x1E,0x31,0x31,0x1E,0x00  //C2 "В"
};

/************************************************************************/
//Задержка в mkS (приблизитлеьно)
void WaitmkS(uint wait) {while(wait--);}

/************************************************************************/
//Задержка в mS (приблизительно)
void WaitmS(uchar wait){
 uint wt;
 while(wait--) for(wt=0; wt<=mS; wt++);
}

/************************************************************************/
//Ждать готовности дисплея
void DispReady(void){

  DData &= 0x0F;                  //Порт на прием (с подключ. подтяг. резисторов)
  DataW |= 0xF0;
  _NOP();
#ifdef NO_DISP
  return;
#endif
  _RS0;
  _RW1;

  Delay1 = TimeOutBusyInd;       //Для контроля таймаута готовности индикатора (ждать нет ответа идти дальше)
  while(Delay1){
    _EI1;
    if(!(DataR&0x80)) break;     //No Busy
    _EI0;
    _EI1;
    _EI0;
  }

}

/************************************************************************/
//Запись команды в дисплей (в регистр команд)
int WrDispCommand(uchar Command, uchar Prov){

  if(Prov == CHECK) DispReady(); //Ждать готовности индикатора

  DData |= 0xF0;                  //Порт на передачу
  _NOP();
  _RS0;
  _RW0;

  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & Command);      //Вынести старший нибл команды на шину данных индикатора
  _EI0;                        //Застробировать данные (по заднему фронту)
  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & (Command << 4));              //Вынести младший нибл команды на шину данных индикатора
  _EI0;                        //Застробировать данные (по заднему фронту)
  _RS1;
  DataW |= 0xF0;
  return(1);
}

/************************************************************************/
//Запись данных в дисплей (в регистр данных)
void WrDispData(uchar Data){

  DispReady();                //Ждать готовность дисплея

  DData |= 0xF0;                 //Порт на передачу
  _NOP();
  _RS1;
  _RW0;

  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & Data);      //Вынести старший нибл команды на шину данных индикатора
  _EI0;                        //Застробировать данные (по заднему фронту)
  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & (Data << 4));              //Вынести младший нибл команды на шину данных индикатора
  _EI0;                        //Застробировать данные (по заднему фронту)
  _RS1;
  DataW |= 0xF0;
}

/************************************************************************/
//Прочесть данные DD RAM или CG RAM дисплея по ранее установленному адресу
uchar RdDispData(void){

 uchar Data;

  DispReady();

  _RS1;
  _RW1;

  _EI1;
  Data = DataR & 0xF0;
  _EI0;                        //Застробировать данные (по заднему фронту)
  _EI1;
  Data |= ((DataR >> 4) & 0x0F);
  _EI0;

  return(Data);
}

/************************************************************************/
//Чтение счетчика адресса (АС)
uchar ReadAdr(void){

 uchar Data;

  DispReady();                  //Ждать готовности дисплея

  _RS0;
  _RW1;

  _EI1;
  Data = DataR & 0xF0;
  _EI0;
  _EI1;
  Data |= ((DataR >> 4) & 0x0F);
  _EI0;

  return(Data & 0x7F);
}

/************************************************************************/
//Инициализация ЖКД на 4-битный режим
uchar InitDisp(void){

  DData |= 0xFE;                  //Порт на передачу
  DataW |= 0xFE;
  WaitmS(100);                     //Задержка по включению питания на дисплей
  _RW0;
  _RS0;
  _EI1;
  DataW &= 0x0E;
  DataW |= 0x30;
  _EI0;                        //Застробировать данные (по заднему фронту)
//  _RS1;

  WaitmS(15);
//  _RS0;
  _EI1;                        //Застробировать данные (по заднему фронту)
  _EI0;
//  _RS1;

  WaitmS(5);
  _EI1;                        //Застробировать данные (по заднему фронту)
  _EI0;

//  _RS0;
  _EI1;                        //Застробировать данные (по заднему фронту)
  DataW &= 0x0E;
  DataW |= 0x20;
  _EI0;
//  _RS1;

  WrDispCommand(FUN_SET, CHECK);
  if (Delay1 == 0) return(0);   //Индикатор не обнаружен

  WrDispCommand(DISP_ON, CHECK);   //Включить дисплей (курсора нет)
  WrDispCommand(DISP_CLR, CHECK);  //Сборос дисплея (полная очистка)
  WrDispCommand(ENTRY_MOD, CHECK); //Entry mode set
  return(1);
}

/************************************************************************/
//Очистка дисплея с возвратом курсора в 0-ю позицию
void ClrAllDisp(void){

  WrDispCommand(DISP_CLR, CHECK);
  WrDispCommand(0x80, CHECK);
  SymbolPtr = 0;
}

/************************************************************************/
//Очистка строки дисплея, с возвращение курсора в начало строки
void ClrStrDisp(uchar Str){

 uchar Add = 0x80, n;

  if(Str) Add = Add+0x40;
  WrDispCommand(Add, CHECK);

  for(n=0; n<16; n++){
    WrDispData(0x20);
  }
  WrDispCommand(Add, CHECK);  //Курсор вернуть в начало строки
}

/************************************************************************/
//Позиционирование курсора на дисплее
void SetCursDisp(uchar Row, uchar Column){

 uchar Add = 0x80;

  if(Row) Add = Add+0x40;
  Add = Add+Column;
  WrDispCommand(Add, CHECK);
}

/************************************************************************/
//Включить индикацию курсора
void OnCursor(void){
  WrDispCommand(DISP_ON|0x02, CHECK);
}

/************************************************************************/
//Включить индикацию курсора с подсветкой
void OnCursorBlink(void){
  WrDispCommand(DISP_ON|0x03, CHECK);
}

/************************************************************************/
//Выключить индикацию курсора
void OffCursor(void){
  WrDispCommand(DISP_ON&0xfc, CHECK);
}

/************************************************************************/
//Поиск символа в таблице, может он уже загружен
uchar ScanTab8Char(uchar Sim){

 uchar i;

  WrDispCommand(0x80+20, CHECK);

  for(i=0; i<9; i++){
    if(Sim == RdDispData()) return(1);
    RdDispData();
  }
  return(0);
}

/************************************************************************/
//Загрузка символа в занкогенератор по SymbolPtr
void LoadCG(uchar Sim){

 uchar IndSim, i;

  if(Sim >= 'А')
    {
      IndSim = Sim - 'А';
    }
      else
    {
      switch(Sim){
        case 'Ё': {IndSim = 64;} break;
        case 'ё': {IndSim = 65;} break;
        case 'Є': {IndSim = 66;} break;
        case 'є': {IndSim = 67;} break;
        case 'Ї': {IndSim = 68;} break;
        case 'ї': {IndSim = 69;} break;
        case 0xa9: {IndSim = 70;} break;
        default:   IndSim = 0;
      }
    }

  WrDispCommand(0x40+(SymbolPtr*8), CHECK);        //Установить начальный адресс в CG RAM
  for(i=0; i<8; i++) { WrDispData(TabCG[IndSim][i]); }
}

/************************************************************************/
#ifndef RUS
//Подставная функция вывода символа на ЖКД индикатор для анлийского языка
int putchar(int c)
{
  uchar a = (uchar) c;
  if(a == '\n') { ClrStrDisp(1); return((int)'\n'); }
  WrDispData(a);
  return((int)a);
}
#else
/************************************************************************/
//Подставная функция вывода символа на ЖКД индикатор для русского языка
int putchar(int c)
{
  uchar a = (uchar) c;
  uchar i;
  if (a == '\n'){ClrStrDisp(1); return((int)'\n'); }
  if (a == 'Ё') WrDispData(0xA2);
  else if (a == 'ё') WrDispData(0xB5);
  else if (a > 0xBF) {
    i = a - 0xC0;
    WrDispData(SymbolTabRUS[i]);
  }
  else WrDispData(a);
  return((int)a);
}
#endif

/************************************************************************/
void WriteHex(uchar Byte)
{
uchar Tmp;
  Tmp = Byte >> 4;
  if ((Tmp & 0xF) < 0x0A) putchar(0x30 + (Tmp & 0x0F));
  else putchar(0x37 + (Tmp & 0x0F));
  if ((Byte & 0xF) < 0x0A) putchar(0x30 + (Byte & 0x0F));
  else putchar(0x37 + (Byte & 0x0F));
}

/************************************************************************/
void WriteDec(uint num)
{
  uchar tmp[5];
  uchar *ptmp = tmp;
  uint	n = num;
  uchar ret = 0;
  uchar i;

  while (n > 0)
  {
    *ptmp = n % 10;
    ptmp++;
    n /= 10;
    ret++;
  }
  if (ret == 0)
  {
    putchar('0');
    return;
  }
  for (i = ret; i > 0; i--)
  {
    putchar(tmp[i-1] + '0');
  }
  return;
}

/************************************************************************/
void WriteStr(uchar *str)
{
  while (*str) {
    putchar(*str);
    str++;
  }
}
