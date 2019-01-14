#ifndef _MAIN_H
#define _MAIN_H

#include "usb_host.h"
#include "system_init.h"
#include "stm32f4xx_hal_flash.h"
#include "flash_execute.h"
//add by felix
//#include "stm32f4xx_it.h"
#include "usbh_cdc.h"
#include "usart_execute.h"

#define RXBUFFERSIZE  40

void Error_Handler(void);
void System_Init(void);
void MX_USB_HOST_Process(void);
void USB_RT_Process(USBH_HandleTypeDef *phost);

void RxExecute(void);
void UartPCTask(void);
void USB_HOST_Task(USBH_HandleTypeDef *phost);
int Add_Check(unsigned char array[],int length_2);
void Delay_ms(volatile unsigned int nTime);

extern const unsigned char Version[14];
extern int cnt1,cnt2,IP_485,IP_485_2;	
extern unsigned int SysBootCnt,t_buf_cnt,t_buf_0b_cnt,t_buf_1e_cnt,t_buf_1f_cnt,r_buf_cnt;
extern unsigned char  USB_R_BUF[20],USB_R_BUF_LAST[21];
extern unsigned char  USB_T_BUF[20],USB_0B_T_BUF[20],USB_1E_T_BUF[20],USB_1F_T_BUF[20];
extern unsigned char PM_25[2],CM_Calibrate_Value[2];
extern unsigned char Testing_flag,Wifi_flag,PM_flag,CM_Calibrate_flag;
extern unsigned char USB_REC_FLAG,UART_Rx_flag,USB_SEND_flag;
extern uint8_t aRxBuffer_2[RXBUFFERSIZE];
extern UART_HandleTypeDef huart1;

#endif
