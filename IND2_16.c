
/*����� ������� �� ������ � ��� 16*2 �������� (BC1602A) 4-bit 24.05.00
��������� ������ ������� 08.08.10*/
#include "cpu.h"
//#define DEBUG
#define RUS
#ifdef DEBUG
  #define NO_DISP		//������ ��������� �������� ���������� ������� ��� ������ � AVRStudio
#endif

//����� �����/������ ������������ � ����������
#define	DData		DDRA
#define DataW		PORTA
#define	DataR		PINA
#define	_RS0		PORTA_Bit1 = 0; _NOP()
#define	_RS1		PORTA_Bit1 = 1; _NOP()
#define	_RW0		PORTA_Bit2 = 0; _NOP()
#define	_RW1		PORTA_Bit2 = 1; _NOP()
#define	_EI0		_NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); PORTA_Bit3 = 0; _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP()				
#define	_EI1		_NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); PORTA_Bit3 = 1; _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); _NOP()


#define   mS		2000               //��� 3.6864 ��� ������

#define   CHECK		1                 //��������� ���������� �������
#define   NO_CHECK	0                 //�� ��������� ���������� �������

#define   FUN_SET	0x28              //������� ��������� ���������� �������
#define   DISP_ON	0x0C              //Display on (��� �������)
#define   DISP_CLR	0x01              //������� �������
#define   ENTRY_MOD	0x06              //Entry mode set

#define   TimeOutBusyInd  50

uchar SymbolPtr;	             //��������� �� ������� ���������������
extern uchar volatile	Delay1;

flash uchar SymbolTabRUS[64] = {	//������� ����������� �������� �� �������
  0x41,		//C0 "�"
  0xA0,		//C1 "�"
  0x42,		//C2 "�"
  0xA1,		//C3 "�"
  0xE0,		//C4 "�"
  0x45,		//C5 "�"
  0xA3,		//C6 "�"
  0xA4,		//C7 "�"
  0xA5,		//C8 "�"
  0xA6,		//C9 "�"
  0x4B,		//CA "�"
  0xA7,		//CB "�"
  0x4D,		//CC "�"
  0x48,		//CD "�"
  0x4F,		//CE "�"
  0xA8,		//CF "�"
  0x50,		//D0 "�"
  0x43,		//D1 "�"
  0x54,		//D2 "�"
  0xA9,		//D3 "�"
  0xAA,		//D4 "�"
  0x58,		//D5 "�"
  0xE1,		//D6 "�"
  0xAB,		//D7 "�"
  0xAC,		//D8 "�"
  0xE2,		//D9 "�"
  0xAD,		//DA "�"
  0xAE,		//DB "�"
  0x62,		//DC "�"
  0xAF,		//DD "�"
  0xB0,		//DE "�"
  0xB1,		//DF "�"
  0x61,		//E0 "�"
  0xB2,		//E1 "�"
  0xB3,		//E2 "�"
  0xB4,		//E3 "�"
  0xE3,		//E4 "�"
  0x65,		//E5 "�"
  0xB6,		//E6 "�"
  0xB7,		//E7 "�"
  0xB8,		//E8 "�"
  0xB9,		//E9 "�"
  0xBA,		//EA "�"
  0xBB,		//EB "�"
  0xBC,		//EC "�"
  0xBD,		//ED "�"
  0x6F,		//EE "�"
  0xBE,		//EF "�"
  0x70,		//F0 "�"
  0x63,		//F1 "�"
  0xBF,		//F2 "�"
  0x79,		//F3 "�"
  0xE4,		//F4 "�"
  0x78,		//F5 "�"
  0xE5,		//F6 "�"
  0xC0,		//F7 "�"
  0xC1,		//F8 "�"
  0xE6,		//F9 "�"
  0xC2,		//FA "�"
  0xC3,		//FB "�"
  0xC4,		//FC "�"
  0xC5,		//FD "�"
  0xC6,		//FE "�"
  0xC7		//FF "�"
};

flash uchar TabCG[3][8] = {0x0E,0x11,0x11,0x11,0x1F,0x11,0x11,0x00,  //C0 "�" 		//������ ���������������
                            0x1F,0x10,0x10,0x1E,0x11,0x11,0x1E,0x00,  //C1 "�"
                            0x1E,0x11,0x11,0x1E,0x31,0x31,0x1E,0x00  //C2 "�"
};

/************************************************************************/
//�������� � mkS (��������������)
void WaitmkS(uint wait) {while(wait--);}

/************************************************************************/
//�������� � mS (��������������)
void WaitmS(uchar wait){
 uint wt;
 while(wait--) for(wt=0; wt<=mS; wt++);
}

/************************************************************************/
//����� ���������� �������
void DispReady(void){

  DData &= 0x0F;                  //���� �� ����� (� �������. ������. ����������)
  DataW |= 0xF0;
  _NOP();
#ifdef NO_DISP
  return;
#endif
  _RS0;
  _RW1;

  Delay1 = TimeOutBusyInd;       //��� �������� �������� ���������� ���������� (����� ��� ������ ���� ������)
  while(Delay1){
    _EI1;
    if(!(DataR&0x80)) break;     //No Busy
    _EI0;
    _EI1;
    _EI0;
  }

}

/************************************************************************/
//������ ������� � ������� (� ������� ������)
int WrDispCommand(uchar Command, uchar Prov){

  if(Prov == CHECK) DispReady(); //����� ���������� ����������

  DData |= 0xF0;                  //���� �� ��������
  _NOP();
  _RS0;
  _RW0;

  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & Command);      //������� ������� ���� ������� �� ���� ������ ����������
  _EI0;                        //�������������� ������ (�� ������� ������)
  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & (Command << 4));              //������� ������� ���� ������� �� ���� ������ ����������
  _EI0;                        //�������������� ������ (�� ������� ������)
  _RS1;
  DataW |= 0xF0;
  return(1);
}

/************************************************************************/
//������ ������ � ������� (� ������� ������)
void WrDispData(uchar Data){

  DispReady();                //����� ���������� �������

  DData |= 0xF0;                 //���� �� ��������
  _NOP();
  _RS1;
  _RW0;

  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & Data);      //������� ������� ���� ������� �� ���� ������ ����������
  _EI0;                        //�������������� ������ (�� ������� ������)
  _EI1;
  DataW &= 0x0F;
  DataW |= (0xF0 & (Data << 4));              //������� ������� ���� ������� �� ���� ������ ����������
  _EI0;                        //�������������� ������ (�� ������� ������)
  _RS1;
  DataW |= 0xF0;
}

/************************************************************************/
//�������� ������ DD RAM ��� CG RAM ������� �� ����� �������������� ������
uchar RdDispData(void){

 uchar Data;

  DispReady();

  _RS1;
  _RW1;

  _EI1;
  Data = DataR & 0xF0;
  _EI0;                        //�������������� ������ (�� ������� ������)
  _EI1;
  Data |= ((DataR >> 4) & 0x0F);
  _EI0;

  return(Data);
}

/************************************************************************/
//������ �������� ������� (��)
uchar ReadAdr(void){

 uchar Data;

  DispReady();                  //����� ���������� �������

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
//������������� ��� �� 4-������ �����
uchar InitDisp(void){

  DData |= 0xFE;                  //���� �� ��������
  DataW |= 0xFE;
  WaitmS(100);                     //�������� �� ��������� ������� �� �������
  _RW0;
  _RS0;
  _EI1;
  DataW &= 0x0E;
  DataW |= 0x30;
  _EI0;                        //�������������� ������ (�� ������� ������)
//  _RS1;

  WaitmS(15);
//  _RS0;
  _EI1;                        //�������������� ������ (�� ������� ������)
  _EI0;
//  _RS1;

  WaitmS(5);
  _EI1;                        //�������������� ������ (�� ������� ������)
  _EI0;

//  _RS0;
  _EI1;                        //�������������� ������ (�� ������� ������)
  DataW &= 0x0E;
  DataW |= 0x20;
  _EI0;
//  _RS1;

  WrDispCommand(FUN_SET, CHECK);
  if (Delay1 == 0) return(0);   //��������� �� ���������

  WrDispCommand(DISP_ON, CHECK);   //�������� ������� (������� ���)
  WrDispCommand(DISP_CLR, CHECK);  //������ ������� (������ �������)
  WrDispCommand(ENTRY_MOD, CHECK); //Entry mode set
  return(1);
}

/************************************************************************/
//������� ������� � ��������� ������� � 0-� �������
void ClrAllDisp(void){

  WrDispCommand(DISP_CLR, CHECK);
  WrDispCommand(0x80, CHECK);
  SymbolPtr = 0;
}

/************************************************************************/
//������� ������ �������, � ����������� ������� � ������ ������
void ClrStrDisp(uchar Str){

 uchar Add = 0x80, n;

  if(Str) Add = Add+0x40;
  WrDispCommand(Add, CHECK);

  for(n=0; n<16; n++){
    WrDispData(0x20);
  }
  WrDispCommand(Add, CHECK);  //������ ������� � ������ ������
}

/************************************************************************/
//���������������� ������� �� �������
void SetCursDisp(uchar Row, uchar Column){

 uchar Add = 0x80;

  if(Row) Add = Add+0x40;
  Add = Add+Column;
  WrDispCommand(Add, CHECK);
}

/************************************************************************/
//�������� ��������� �������
void OnCursor(void){
  WrDispCommand(DISP_ON|0x02, CHECK);
}

/************************************************************************/
//�������� ��������� ������� � ����������
void OnCursorBlink(void){
  WrDispCommand(DISP_ON|0x03, CHECK);
}

/************************************************************************/
//��������� ��������� �������
void OffCursor(void){
  WrDispCommand(DISP_ON&0xfc, CHECK);
}

/************************************************************************/
//����� ������� � �������, ����� �� ��� ��������
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
//�������� ������� � �������������� �� SymbolPtr
void LoadCG(uchar Sim){

 uchar IndSim, i;

  if(Sim >= '�')
    {
      IndSim = Sim - '�';
    }
      else
    {
      switch(Sim){
        case '�': {IndSim = 64;} break;
        case '�': {IndSim = 65;} break;
        case '�': {IndSim = 66;} break;
        case '�': {IndSim = 67;} break;
        case '�': {IndSim = 68;} break;
        case '�': {IndSim = 69;} break;
        case 0xa9: {IndSim = 70;} break;
        default:   IndSim = 0;
      }
    }

  WrDispCommand(0x40+(SymbolPtr*8), CHECK);        //���������� ��������� ������ � CG RAM
  for(i=0; i<8; i++) { WrDispData(TabCG[IndSim][i]); }
}

/************************************************************************/
#ifndef RUS
//���������� ������� ������ ������� �� ��� ��������� ��� ���������� �����
int putchar(int c)
{
  uchar a = (uchar) c;
  if(a == '\n') { ClrStrDisp(1); return((int)'\n'); }
  WrDispData(a);
  return((int)a);
}
#else
/************************************************************************/
//���������� ������� ������ ������� �� ��� ��������� ��� �������� �����
int putchar(int c)
{
  uchar a = (uchar) c;
  uchar i;
  if (a == '\n'){ClrStrDisp(1); return((int)'\n'); }
  if (a == '�') WrDispData(0xA2);
  else if (a == '�') WrDispData(0xB5);
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
