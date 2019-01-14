/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
//#include "stm32f4xx_hal.h"
#include "main.h"

UART_HandleTypeDef huart1; //串口
CDC_LineCodingTypeDef pUartTest;

int cnt1,cnt2,IP_485,IP_485_2;	
unsigned int SysBootCnt,t_buf_cnt,t_buf_0b_cnt,t_buf_1e_cnt,t_buf_1f_cnt,r_buf_cnt;
unsigned char  USB_R_BUF[21],USB_R_BUF_LAST[21];
unsigned char  USB_T_BUF[21],USB_0B_T_BUF[20],USB_1E_T_BUF[20],USB_1F_T_BUF[20];
unsigned char PM_25[2],CM_Calibrate_Value[2];
unsigned char Testing_flag=0xFF,Wifi_flag=0xFF,PM_flag=0xFF,CM_Calibrate_flag=0xFF;
unsigned char USB_REC_FLAG,UART_Rx_flag=0,USB_SEND_flag = 1;

unsigned char   DataCmd_0B[30],DataCmd_0E[30],DataCmd_1F[30],DataCmd_FC[30],DataCmd_0C[10];
//unsigned char LengthCmd_0B[1],LengthCmd_1E[1],LengthCmd_1F[1],LengthCmd_0C[1],LengthCmd_25[1],LengthCmd_All[1];

const unsigned char Version[16] = {"USB-485-CGK-V1.3"};

uint8_t aRxBuffer_2[RXBUFFERSIZE];


int main(void)
{
  System_Init();    //系统初始化
	IP_485 = *(__IO uint32_t *)FLASH_USER_START_ADDR;   //从Flash起始地址读取工装板模块IP
	IP_485_2 = (unsigned char)IP_485;
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(&huart1,UART_IT_TXE);
	HAL_UART_Receive_IT(&huart1,uart_rev.rear, 1);	 //上电初期指定串口1，接收数据，并将串口接收的数据放在aRxBuffer中，每次接收1个字节

	
	// xf add		 
	//memcpy(DataCmd_1F,"201901090001",16);
	//memcpy(DataCmd_FC,"YDHJY-V0.8",16);
	//memset(DataCmd_0C,0,16);
	
	
  while (1)
  {
		WatchDog_Feed();//喂狗	
		if(SystickFlag20ms_2==1)   //20ms的标志，每20ms枚举一次USB从机
		{
			SystickFlag20ms_2=0;
			if(SysBootCnt<=10000)	SysBootCnt++;
			else SysBootCnt = 10;
			MX_USB_HOST_Process();   //USB从设备枚举、获取设备标识符
		}
		USB_RT_Process(&hUsbHostFS);		//向USB从机查询数据
		UartPCTask();		//PC串口任务处理
	}
}

/*----------------------------------------------------------------------------------------
* 功能描述: USB从机数据查询
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2016-09-20
* 版    本：1.0
-----------------------------------------------------------------------------------------*/
void USB_RT_Process(USBH_HandleTypeDef *phost)
{
//	CDC_HandleTypeDef *CDC_Handle =  (CDC_HandleTypeDef*) phost->pActiveClass->pData; 
  if(phost->gState != HOST_CLASS)	  //状态判断，枚举不成功则返回
	{
		memset(DataCmd_0B,0,sizeof(DataCmd_0B));//若USB枚举不上，使查实时数据指令无应答		
		return;
	}

	if(USB_SEND_flag == 1)
	{
		USB_SEND_flag = 0;
		pUartTest.b.dwDTERate = 9600;//功能描述: 虚拟串口参数的配置
		pUartTest.b.bCharFormat = 0;
		pUartTest.b.bParityType = 0;
		pUartTest.b.bDataBits = 8;
		USBH_CDC_SetLineCoding(phost,&pUartTest);
		//memset(pUartTest,0,sizeof(CDC_LineCodingTypeDef));
		//memcpy(&pUartTest,pUart,sizeof(CDC_LineCodingTypeDef));
	}

	if(SystickFlag20ms ==1)	
	{	
		SystickFlag20ms =0;
		if(SysBootCnt>=10)   //主机上电时，给一个延时
		{
			USB_SEND_flag = 2;
			USB_HOST_Task(&hUsbHostFS);   //USB任务处理
		}
	}
}

/*-----------------------------------------------------------------------------
* 函 数 名: UartPCTask()
* 功能描述: PC串口任务处理函数
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2016-09-20
* 版    本：
------------------------------------------------------------------------------*/
void  UartPCTask(void)  
{
		static unsigned int restart_system_cnt;
//		if(SystickFlag10ms == 1)//10ms用来从队列取数据和进状态机进行校验
//		{
//			SystickFlag10ms = 0;
			usart_state_machine();//主循环中不断取队列数据
//		}
		if(UART_Rx_flag ==1)		    //成功收到指令
		{
			RxExecute();									//串口指令响应
			restart_system_cnt = 0;//接收到一帧数据，将系统重启计数清零
			memset(aRxBuffer_2,0,40);   //将aRxBuffer_2前40个字节清零
			UART_Rx_flag =0; //取了完整的一帧，就要将接收整帧数据标志清零
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);//当收到校验上的一帧数据后，PA5会翻转
		}
		else if(UART_Rx_flag ==2)
		{
			UART_Rx_flag =0; //取了完整的一帧，就要将接收整帧数据标志清零
		}
		else
		{
			if(SystickFlag1S_restart ==1)
			{	
				SystickFlag1S_restart = 0;
				if(restart_system_cnt++ >= 30)  //30s接收不到上位机的数据，会重启系统
				{
					restart_system_cnt = 0;
					HAL_NVIC_SystemReset();//重启系统
				}
			}
		}
}

/*-----------------------------------------------------------------------------
* 函 数 名: RxExecute()
* 功能描述: PC串口响应函数，处理收到的串口指令，并将对应数据实时返回给PC端
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2016-09-20
* 版    本：V1.0
------------------------------------------------------------------------------*/

uint8_t device_ison = 0x01;
void RxExecute(void)
{
	unsigned char add_sum,i,length,checksum,version_cnt;
	add_sum=0;
	for(i=0;i<aRxBuffer_2[1]+3;i++)
	{	
		add_sum += aRxBuffer_2[i];
	}  
	if(add_sum != 0)		
		return;

	if(aRxBuffer_2[3] == IP_485_2 && (aRxBuffer_2[2] != 0xFA  || aRxBuffer_2[2] != 0xFB ))
	{
			switch(aRxBuffer_2[2])	
			{    
				case 0x0b:      //读取所有测量数据 11 02 0b IP CS  (11 02 0b 01 e1)
				{
						if(aRxBuffer_2[1] == 0x02)	 // 16 12 0B IP 00 1E 03 0C 01 74 00 00 00 00 00 00 00 00 00 00 CS 
						{   
								memcpy(USB_R_BUF_LAST,DataCmd_0B,DataCmd_0B[1] + 3);
								USB_R_BUF_LAST[3]=IP_485_2;
								USB_R_BUF_LAST[17]=device_ison;
								//USB_R_BUF_LAST[17]=0x01;
						}
				}
				break;

				case 0x1f:      //查询设备编号 11 02 1f IP CS
				{
						if(aRxBuffer_2[1] == 0x02)	
						{      
							memcpy(USB_R_BUF_LAST,DataCmd_1F,DataCmd_1F[1] + 3);
							USB_R_BUF_LAST[3]=IP_485_2;	
						} 
				}
				break;	
						
				case 0xFC:      //查询软件版本号 11 02 FC IP CS
				{														//
						if(aRxBuffer_2[1] == 0x02)	
						{        
							memcpy(USB_R_BUF_LAST,DataCmd_FC,DataCmd_FC[1] + 3);
							USB_R_BUF_LAST[3]=IP_485_2;	
						}	
				}		
				break;		
				
				case 0x0C:      //查询校准系数 11 02 0C IP CS
				{								//应答11 12 0C IP DF1-DF16 CS
						if(aRxBuffer_2[1] == 0x02)	
						{        
							memcpy(USB_R_BUF_LAST,DataCmd_0C,DataCmd_0C[1] + 3);
							USB_R_BUF_LAST[3]=IP_485_2;	
						}	
				}		
				break;	
						
				case 0x0E:      // PM2.5标定 11 0E 0E IP DF1-DF12 CS
				{								//           16 06 0E IP DF1-DF4  CS
//						if(aRxBuffer_2[1] == 0x0E)	
//						{  
//							memcpy(USB_R_BUF_LAST,DataCmd_0E,DataCmd_0E[1] + 3);
//							USB_R_BUF_LAST[3]=IP_485_2;	
//						}				
						USB_R_BUF_LAST[0] = 0x16;
						USB_R_BUF_LAST[1] = 0x06;
						USB_R_BUF_LAST[2] = 0x0E;
						USB_R_BUF_LAST[3] = IP_485_2;					
					
						USB_R_BUF_LAST[4] = 0x00;
						USB_R_BUF_LAST[5] = 0x00;
						USB_R_BUF_LAST[6] = 0x00;
						USB_R_BUF_LAST[7] = 0x00;
				}
				break;		
				case 0xFE:      // 查询工装板软件版本11 02 FE IP CS
				{
					USB_R_BUF_LAST[0] = 0x16;
					USB_R_BUF_LAST[1] = 0x12;
					USB_R_BUF_LAST[2] = 0xFE;
					USB_R_BUF_LAST[3] = IP_485_2;
					
					memcpy(&USB_R_BUF_LAST[4],Version,16);
				}
				break;	
				
				default:
					USB_R_BUF_LAST[1] = 0x00;
					break;
		}		
			
		if( (USB_R_BUF_LAST[1] > 0) && (USB_R_BUF_LAST[3] > 0) )   
		//当IP大于零，且命令不是查询工装软件版本时，计算CS校验位并将USB_R_BUF_LAST的值输出
		{
				length = USB_R_BUF_LAST[1] + 2;
				checksum = 0;
				for(i=0;i<length;i++)					//计算checksum
				{
						checksum += USB_R_BUF_LAST[i];								
				}
				USB_R_BUF_LAST[length] =-checksum; 
				//if(length != 0x14 && length != 0x0E && length != 0x11 && length != 0x05 && length != 0x07 && length != 0x0A && length != 0x12) 
				//	return;//返回数据长度与查询不符合，说明主机没有查询到从机数据，为异常情况，不进行输出
				HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //串口输出USB_R_BUF的数据
				//memset(USB_R_BUF_LAST,0,21);
		}
	}
	
	else if(aRxBuffer_2[2] == 0xFB) // 查询工装板IP号 11 02 FB 00 F2 应答 16 04 FB 00 IP DF2 CS
	{
			if(aRxBuffer_2[1] == 0x02)	
			{        
						USB_R_BUF_LAST[0]=0x16;
						USB_R_BUF_LAST[1]=0x04;				
						USB_R_BUF_LAST[2]=0xFB;	
						USB_R_BUF_LAST[3]=0x00;	
						USB_R_BUF_LAST[4]= *(__IO uint32_t *)FLASH_USER_START_ADDR;
						USB_R_BUF_LAST[5]=0x00;	
						length = USB_R_BUF_LAST[1] +2;  //length=6
						checksum = 0;
						for(i=0;i<length;i++)					//计算checksum
						{
								checksum += USB_R_BUF_LAST[i];								
						}
						USB_R_BUF_LAST[length] =-checksum; 
						HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //串口输出USB_R_BUF的数据
			} 	
	}
			
	else if(aRxBuffer_2[2] == 0xFA)	
	{
		//当指令的IP与IP_485_2不等时，且第三个字节为0xFA，则写IP地址 11 02 FA IP DF2 CS
		//																								      应答 16 04 FA 00 IP  DF2 CS
				HAL_FLASH_Unlock();
				Erase_Flash();
				Address = FLASH_USER_START_ADDR;
				HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, aRxBuffer_2[3]);
				HAL_FLASH_Lock(); 
		
				USB_R_BUF_LAST[0]=0x16;
				USB_R_BUF_LAST[1]=0x04;				
				USB_R_BUF_LAST[2]=0xFA;	
				USB_R_BUF_LAST[3]=0x00;	
				USB_R_BUF_LAST[4]= *(__IO uint32_t *)FLASH_USER_START_ADDR;
				USB_R_BUF_LAST[5]=0x00;	
				length = USB_R_BUF_LAST[1] +2;  //length=6
				checksum = 0;
				for(i=0;i<length;i++)					//计算checksum
				{
						checksum += USB_R_BUF_LAST[i];								
				}
				USB_R_BUF_LAST[length] =-checksum; 
				HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //串口输出USB_R_BUF的数据
	}
}
	
/*-----------------------------------------------------------------------------
* 函 数 名: USB_HOST_Task()
* 功能描述: USB任务处理，向USB从机发送指令，在USB回调函数里面接收数据
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2016-09-20
* 版    本：V1.0
------------------------------------------------------------------------------*/
uint8_t device_ison_count = 0;
void USB_HOST_Task(USBH_HandleTypeDef *phost)
{
	  static unsigned char SendCnt,checksum_2,i_2;
		if(SystickFlag1S == 1)
		{
				device_ison_count++;
				if(device_ison_count > 4) {
					device_ison_count = 0;
					device_ison = 0x00;
				}
		    SystickFlag1S=0;
				// CGK_cal 
				// 11 0E 0E IP DF1 DF2 DF3 DF4 DF5 DF6 DF7 DF8 DF9 DF10 DF11 DF12 CS
				if(CM_Calibrate_flag==0x10)
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x0E;
						USB_T_BUF[2]=0x0E;
						USB_T_BUF[3]=0x00;		
					
						for(i_2=4;i_2<16;i_2++)					//计算checksum
						{
							USB_T_BUF[i_2] = 0x00;
						}
				
						checksum_2 = 0;
						for(i_2=0;i_2<(USB_T_BUF[1] + 2 );i_2++)					//计算checksum
						{
								checksum_2 += USB_T_BUF[i_2];								
						}
						USB_T_BUF[USB_T_BUF[1] + 2] =-checksum_2; 
						USBH_CDC_Transmit(phost,USB_T_BUF,17);
						USBH_CDC_Receive(phost,USB_R_BUF,9);
						CM_Calibrate_flag=0xFF;
						return ;
						//SendCnt =100;
				}
				// read slave device number
				// 11 02 1F 00 CS
				if(SendCnt==2)        // 读取编号（上电2s）  
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x02;
						USB_T_BUF[2]=0x1F;
						USB_T_BUF[3]=0x00;
						USB_T_BUF[4]=0xCE;
						USBH_CDC_Transmit(phost,USB_T_BUF,5);
						USBH_CDC_Receive(phost,USB_R_BUF,21);
				}
				// read the slave software version number
				// 11 02 FC 00 CS
				else if(SendCnt==5)		// 读取版本（上电5s）
				{					
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x02;
						USB_T_BUF[2]=0xFC;
						USB_T_BUF[3]=0x00;
						USB_T_BUF[4]=0xF1;
						USBH_CDC_Transmit(phost,USB_T_BUF,5);
						USBH_CDC_Receive(phost,USB_R_BUF,21);
				}
				// read the whole machine CGK calibration coefficient
				// 11 02 0C 00 CS
				else if(SendCnt==10)	//读取CGK校准系数（上电10s）
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x02;
						USB_T_BUF[2]=0x0C;
						USB_T_BUF[3]=0x00;
						USB_T_BUF[4]=0xE1;
						USBH_CDC_Transmit(phost,USB_T_BUF,5);
						USBH_CDC_Receive(phost,USB_R_BUF,21);
				}	
				// read cgk sensor's data
				// 11 02 0B 00 E2
				else if(SendCnt<=60)  //读取数据
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x02;
						USB_T_BUF[2]=0x0B;
						USB_T_BUF[3]=0x00;
						USB_T_BUF[4]=0xE2;
						USBH_CDC_Transmit(phost,USB_T_BUF,5);
						USBH_CDC_Receive(phost,USB_R_BUF,21);
				}
				if(++SendCnt>=30) {
					SendCnt=0;
				}
		}
}

/*----------------------------------------------------------------------------------------
* 功能描述: USB回调函数
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2017-03-30
* 版    本：1.1
-----------------------------------------------------------------------------------------*/	
void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost)
{
		unsigned char checksum_3=0,i_3;	
		for(i_3=0;i_3<(USB_R_BUF[1]+3);i_3++)					//对收到的数据进行和校验，编号回复字节为14个字节
		{
				checksum_3 += USB_R_BUF[i_3];								
		}
	
		if(checksum_3 == 0)  //成功校验上
		{
			HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_12);  //当收到校验上的一帧USB数据后，PB12会翻转			
			
			if(USB_R_BUF[2]==0x0B) {
				// clear count 
				device_ison = 0x01;
				device_ison_count = 0;
				memcpy(DataCmd_0B,USB_R_BUF,USB_R_BUF[1] + 3);
			} else if(USB_R_BUF[2]==0x0E) {
				memcpy(DataCmd_0E,USB_R_BUF,USB_R_BUF[1] + 3);
			} else if(USB_R_BUF[2]==0x1F) {
				memcpy(DataCmd_1F,USB_R_BUF,USB_R_BUF[1] + 3);
			} else if(USB_R_BUF[2]==0xFC) {
				memcpy(DataCmd_FC,USB_R_BUF,USB_R_BUF[1] + 3);
			} else if(USB_R_BUF[2]==0x0C) {
				memcpy(DataCmd_0C,USB_R_BUF,USB_R_BUF[1] + 3);
			}
		}
		
		else if(checksum_3!=0)  //校验不上
		{
			if(USB_R_BUF[2]==0x0B) {
					DataCmd_0B[1] = 0x00;
			} else if(USB_R_BUF[2]==0x0E) {
					//DataCmd_0E[1] = 0x00;
			} else if(USB_R_BUF[2]==0x1F) {
					DataCmd_1F[1] = 0x00;
			} else if(USB_R_BUF[2]==0xFC) {
					DataCmd_FC[1] = 0x00;
			} else if(USB_R_BUF[2]==0x0C) {
					DataCmd_0C[1] = 0x00;
			}
			return;//校验不上时，退出
		}
}

/* USER CODE BEGIN 4 */
/* 利用滴答定时器进行延时*/
void Delay_ms(volatile unsigned int nTime)
{
	TimingDelay = nTime;
	while(TimingDelay !=0);
}

///* Private function prototypes 重定向输出打印函数----------------------------------*/
//#ifdef __GNUC__
///* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
//   set to 'Yes') calls __io_putchar() */
//#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//#endif /* __GNUC__ */

//PUTCHAR_PROTOTYPE
//{
//  /* Place your implementation of fputc here */
//  /* e.g. write a character to the USART1 and Loop until the end of transmission */
//  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
//  return ch;
//}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

