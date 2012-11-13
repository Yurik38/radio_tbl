#ifndef _IND2_16_
#define _IND2_16_

#define   Str1    0
#define   Str2    1

uchar InitDisp(void);
void ClrAllDisp(void);
void ClrStrDisp(uchar);
void SetCursDisp(uchar Row, uchar Column);
uchar RdDispData(void);
void OnCursor(void);
void OnCursorBlink(void);
void OffCursor(void);
void WrDispData(uchar);

int putchar(int c);
void WriteHex(uchar Byte);
void WriteDec(uint num);

void WriteStr(uchar *str);

#endif	//_IND2_16_

