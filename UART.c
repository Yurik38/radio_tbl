#include "cpu.h"
#include "event.h"
#include "UART.h"

#define	RX_BUF_SIZE		4
#define TX_BUF_SIZE		8


extern uchar volatile	Delay1;

uchar 	tx_cnt;
uchar	tx_buffer[TX_BUF_SIZE];
uchar*	tx_ptr;

uchar	rx_buffer[RX_BUF_SIZE];
uchar	rx_rd_ptr;
uchar	rx_wr_ptr;
uchar	UARTBusyFlag;
uchar 	LedTime[4];
uchar	ID[4];

TPACKET	rx_packet;


/************************************************************************/
#pragma vector = USART_RXC_vect
__interrupt void USART_RXC_vector(void)
{
  uchar a = rx_wr_ptr;
  a++;
  if (a >= RX_BUF_SIZE) a = 0;
  if (a == rx_rd_ptr)
  {
    a = UDR;
    return;
  }
  rx_buffer[rx_wr_ptr] = UDR;
  rx_wr_ptr = a;
}

/************************************************************************/
#pragma vector = USART_UDRE_vect
__interrupt void USART_UDRE_vector(void)
{
  if (tx_cnt--)
  {
    UDR = *tx_ptr;
    tx_ptr++;
  }
  else 				//Запрещаем прерывание по пустому UDR
  {
    UCSRB &= ~(1 << UDRIE);
    UCSRB |= 1 << TXCIE;	//разреш прерыв по окончании передачи
  }
}

/************************************************************************/
#pragma vector = USART_TXC_vect
__interrupt void USART_TXC_vector(void)
{
  UCSRB &= ~(1 << TXCIE);		
  UARTBusyFlag = 0;
}

/************************************************************************/
/*Инициализация СОМ-порта. BaudRate передается в сотлях. Типа 96 - 9600, 1152 - 115200 и т.д.*/
void InitUART(uint baud_rate)
{
  uchar u2x_bit = 0;
  uint  ubrr_reg;

  ubrr_reg = (CPU_FREQ/100 * (1 + u2x_bit))/(16 * baud_rate) - 1;
  UBRRH = _HiByte(ubrr_reg);
  UBRRL = ubrr_reg;
  UCSRA |= (U2X_BIT << U2X);
  UCSRB = (1 << TXCIE) + (1 << RXCIE) + (1 << TXEN) + (1 << RXEN);
  UARTBusyFlag = 0;
  ID[0] = 0;
  ID[1] = 0;
  ID[2] = 0;
  ID[3] = 0;
}

/************************************************************************/
/*выбрать необходимое устройство*/
void SetCS(uchar addr)
{
  uchar a;
  
  a = 0x08 << addr;
  PORTD &= 0x0F;
  PORTD |= a;
}

/************************************************************************/
/*выбрать необходимое устройство*/
uchar GetCS(void)
{
  if (PIND & 0x10) return 1;
  if (PIND & 0x20) return 2;
  return 3;
}

/************************************************************************/
void Morgun(uchar a)
{
  uchar i;
  
  if (a > 0) i = a - 1;
  _LedOn(i);
  LedTime[i] = 20;
}
  

/************************************************************************/
void TxBuffer(uchar* FirstByte, uchar Cnt)
{
  UARTBusyFlag = 1;
  UDR = *FirstByte;
  tx_cnt = Cnt - 1;
  if (tx_cnt)
  {
    tx_ptr = FirstByte + 1;
    UCSRB |= (1 << UDRIE); 	//Если есть еще что передавать, разрешаем прерывание по пустому UDR
  }
}

/************************************************************************/
uchar GetChar(void)
{
  uchar a;

  if (rx_rd_ptr == rx_wr_ptr) return 0;
  a = rx_buffer[rx_rd_ptr++];
  if (rx_rd_ptr >= RX_BUF_SIZE) rx_rd_ptr = 0;
  return a;
}

/************************************************************************/
/*прочитать текущий байт в указатель а. Вернуть 1 если есть новые данные, иначе, 0*/
uchar GetByte(uchar *a)
{
  if (rx_rd_ptr == rx_wr_ptr) return 0;
  *a = rx_buffer[rx_rd_ptr++];
  if (rx_rd_ptr >= RX_BUF_SIZE) rx_rd_ptr = 0;
  return 1;
}

/************************************************************************/
void SendPacket(TPACKET* packet, uchar addr)
{
  uchar i, crc;
  uchar ret;
  uchar* p_tmp;
  Delay1 = 5;
  while ((UARTBusyFlag) || (Delay1));
  packet->id = ID[addr-1];
  ID[addr-1]++;
  p_tmp = (uchar*)packet;
  
  tx_buffer[0] = 0x7E;
  crc = 0;
  ret = 1;
  
  for (i = 0; i < 4; i++)
  {
    if (p_tmp[i] == 0x7E) 
    {
      tx_buffer[ret] = 0x7D;
      ret++;
      tx_buffer[ret] = 0x5E;
      ret++;
    }
    else if (p_tmp[i] == 0x7D) 
    {
      tx_buffer[ret] = 0x7D;
      ret++;
      tx_buffer[ret] = 0x5D;
      ret++;
    }
    else 
    {
      tx_buffer[ret] = p_tmp[i];
      ret++;
    }
    crc += p_tmp[i];
  }
  tx_buffer[ret] = crc;
  ret++;
  SetCS(addr);
  Morgun(addr);
  TxBuffer(tx_buffer, ret);
}

/************************************************************************/
TPACKET* GetPacket(void)
{
  static uchar parse_state = 0;
  static uchar crc, cnt, CurID;
  static uchar *buf;
  uchar rx_byte;
  

  if (GetByte(&rx_byte))
  {
    Delay1 = 5;
    switch (parse_state)
    {
    case 0:
      if (rx_byte == 0x7E)		  //начало пакета
      {
        parse_state = 1;
        crc = 0;
        cnt = 0;
        buf = (uchar*)&rx_packet;
      }
      break;
        
    case 1:  
      if (rx_byte == 0x7D) parse_state = 2;
      else
      {
        buf[cnt] = rx_byte;
        crc += rx_byte;
        if (cnt >= 3) parse_state = 3;
        else cnt++;
      }
      break;
      
    case 2:
      rx_byte ^= 0x20;
      buf[cnt] = rx_byte;
      crc += rx_byte;
      if (cnt >= 3) parse_state = 3;
      else {cnt++; parse_state = 1;}
      break;
        
    case 3:
      parse_state = 0;
      if (crc != rx_byte) break;		
      if (CurID == rx_packet.id) break; 		
      CurID = rx_packet.id;
      //CntRxPacket++;
      crc = GetCS();
      Morgun(crc);
      return &rx_packet; 
      
      default: 
        parse_state = 0;
        break;
      }
    }
  return NULL;
}
