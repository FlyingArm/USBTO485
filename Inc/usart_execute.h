#ifndef _USART_EXECUTE_H
#define _USART_EXECUTE_H

#include "main.h"

#define BUFFSIZE 200

typedef enum{
  RECV_STATE_START,
  RECV_STATE_LEN,
  RECV_STATE_CMD,
  RECV_STATE_DATA,
  RECV_STATE_CS
}recv_state_t;

struct uart  
{  
    uint8_t *rear;          //在中断函数中更改  
    uint8_t *front;         //在主循环中更改  
}; 

typedef enum{
	tx_direction = 1,
	rx_direction = 2
} fream_direction;

extern recv_state_t recv_state;
extern fream_direction usart_frame_direction;
extern struct uart uart_rev;
extern unsigned char aRxBuffer[BUFFSIZE];

extern int8_t uart_queen_read(uint8_t *fmt,uint8_t read_finished_flag);
extern void usart_state_machine(void);
//extern uint8_t Uart_Cnt;

#endif
