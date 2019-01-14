#ifndef _SYSTEM_INIT_H
#define _SYSTEM_INIT_H

//extern UART_HandleTypeDef huart1; //串口
extern TIM_HandleTypeDef htim1; //定时器
extern IWDG_HandleTypeDef hiwdg;//看门狗
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
void SystemClock_Config(void);
void Error_Handler(void);
void WatchDog_Init(unsigned char prer, unsigned int reld);
void WatchDog_Feed(void);

#endif
