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
uchar	TxID[4];
uchar	RxID[4];

T_EVENT	rx_event;


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
	TxID[0] = 0;
	TxID[1] = 0;
	TxID[2] = 0;
	TxID[3] = 0;
	RxID[0] = 0;
	RxID[1] = 0;
	RxID[2] = 0;
	RxID[3] = 0;
}

/************************************************************************/
/*Set active device*/
void SetCS(uchar addr)
{
	uchar a;

	a = 0x08 << addr;
	PORTD &= 0x0F;
	PORTD |= a;
}

/************************************************************************/
/*get active device*/
uchar GetCS(void)
{
	if (PIND & 0x10) return 1;
	if (PIND & 0x20) return 2;
	return 3;
}

/************************************************************************/
void Morgun(uchar a)
{
	_LedOn(a - 1);
	LedTime[a- 1] = 20;
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
void SendPacket(T_EVENT* event)
{
	uchar i, crc;
	uchar cnt;
	uchar addr = event->addr;
    uchar* ptr = (uchar*)event;

	Delay1 = 5;

	while ((UARTBusyFlag) || (Delay1));
	TxID[addr-1]++;

	tx_buffer[0] = 0x7E;
	crc = 0;

	for (i = 1; i < sizeof(T_EVENT) + 1; i++)
	{
		tx_buffer[cnt] = *ptr;
		crc += *ptr;
		ptr++;
	}
	tx_buffer[i] = TxID[addr-1];
	i++;
	tx_buffer[i] = crc;
	i++;
	SetCS(addr);
	Morgun(addr);
	TxBuffer(tx_buffer, i);
}

/************************************************************************/
T_EVENT* GetPacket(void)
{
	static uchar parse_state = 0;
	static uchar crc, cnt, cur_ID;
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
					buf = (uchar*)&rx_event;
				}
				break;

			case 1://rx event
				buf[cnt] = rx_byte;
				crc += rx_byte;
				cnt++;
				if (cnt >= sizeof(T_EVENT)) parse_state++;
				break;

			case 2://rx id
				cur_ID = rx_byte;
				crc += rx_byte;
				parse_state++;
				break;

			case 3://rx crc and check
				parse_state = 0;
				cnt = GetCS();
				if (crc != rx_byte) break;			//CRC mismatch
				if (RxID[cnt] == cur_ID) break;		//repeat packet
				RxID[cnt] = cur_ID;
				Morgun(cnt);
				return &rx_event;

			default:
				parse_state = 0;
				break;
		}
	}
	else
	{
		if (parse_state)	//if time expired reset parser
			if(Delay1 == 0) parse_state = 0;
	}
	return NULL;
}
