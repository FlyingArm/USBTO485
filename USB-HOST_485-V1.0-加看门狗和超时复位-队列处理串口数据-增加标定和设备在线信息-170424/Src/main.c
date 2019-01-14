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
unsigned char  USB_R_BUF[20],USB_R_BUF_LAST[21];
unsigned char  USB_T_BUF[20],USB_0B_T_BUF[20],USB_1E_T_BUF[20],USB_1F_T_BUF[20];
unsigned char PM_25[2],CM_Calibrate_Value[2];
unsigned char Testing_flag=0xFF,Wifi_flag=0xFF,PM_flag=0xFF,CM_Calibrate_flag=0xFF;
unsigned char USB_REC_FLAG,UART_Rx_flag=0,USB_SEND_flag = 1;
unsigned char   DataCmd_0B[30],DataCmd_1E[30],DataCmd_1F[30],DataCmd_25[30];
unsigned char LengthCmd_0B[1],LengthCmd_1E[1],LengthCmd_1F[1],LengthCmd_25[1],LengthCmd_All[1];
const unsigned char Version[14] = {"*USB-485-V1.0"};

uint8_t aRxBuffer_2[RXBUFFERSIZE];


int main(void)
{
  System_Init();    //系统初始化
	IP_485 = *(__IO uint32_t *)FLASH_USER_START_ADDR;   //从Flash起始地址读取工装板模块IP
	IP_485_2 = (unsigned char)IP_485;
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(&huart1,UART_IT_TXE);
	HAL_UART_Receive_IT(&huart1,uart_rev.rear, 1);	 //上电初期指定串口1，接收数据，并将串口接收的数据放在aRxBuffer中，每次接收1个字节
		 
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
		LengthCmd_0B[0] = 0x01;
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
				if(restart_system_cnt++ >= 30)//30s接收不到上位机的数据，会重启系统
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
void RxExecute(void)
{
	unsigned char add_sum,i,length,checksum,version_cnt;
	add_sum=0;
	for(i=0;i<aRxBuffer_2[1]+3;i++)
	{	
		add_sum += aRxBuffer_2[i];
	}  
	if(add_sum != 0)		return;

	if(aRxBuffer_2[3] == IP_485_2 && aRxBuffer_2[2] != 0xDD)
	{
			switch(aRxBuffer_2[2])	
			{    
				case 0x0b:      //读取所有测量数据 11 02 0b IP CS  (11 02 0b 01 e1)
				{
						if(aRxBuffer_2[1] == 0x02)	 // 16 12 0B IP 00 1E 03 0C 01 74 00 00 00 00 00 00 00 00 00 00 CS 
						{   
									USB_R_BUF_LAST[0]=0x16;
									USB_R_BUF_LAST[1]=(LengthCmd_0B[0]+1);//0x12	
									USB_R_BUF_LAST[2]=aRxBuffer_2[2];
									USB_R_BUF_LAST[3]=IP_485_2;
									length = USB_R_BUF_LAST[1] +2;   //除cs外，其它返回数据的字节长度（此时为length=20）
									for(i=0;i<length-4;i++)
									{
											USB_R_BUF_LAST[i+4] = DataCmd_0B[i];		//把去除帧头和帧尾的数据传递给最终要输出的数据						
									}
						}
				}
				break;

				case 0x1f:      //查询设备编号 11 02 1f IP CS
				{
						if(aRxBuffer_2[1] == 0x02)	
						{        
									USB_R_BUF_LAST[0]=0x16;
//									USB_R_BUF_LAST[1]=(LengthCmd_1F[0]+1);	//除cs外，其它返回数据的字节长度（此时为length=14
									USB_R_BUF_LAST[1]=(LengthCmd_1F[0]+1);			//0x0C				
									USB_R_BUF_LAST[2]=aRxBuffer_2[2];
									USB_R_BUF_LAST[3]=IP_485_2;
									length = USB_R_BUF_LAST[1] +2;  //length=14
									for(i=0;i<length-4;i++)
									{
											USB_R_BUF_LAST[i+4] = DataCmd_1F[i];								
									}	
						} 
				}
				break;	
						
				case 0x1e:      //查询软件版本号 11 02 1e IP CS
				{														//
						if(aRxBuffer_2[1] == 0x02)	
						{        
									USB_R_BUF_LAST[0]=0x16;
//									USB_R_BUF_LAST[1]=(LengthCmd_1E[0]+1);		
									USB_R_BUF_LAST[1]=(LengthCmd_1E[0]+1);		//0x0F							
									USB_R_BUF_LAST[2]=aRxBuffer_2[2];
									USB_R_BUF_LAST[3]=IP_485_2;
									length = USB_R_BUF_LAST[1] +2;  ////除cs外，其它返回数据的字节长度(length=17)
									for(i=0;i<length-4;i++)
									{
											USB_R_BUF_LAST[i+4] = DataCmd_1E[i];								
									}	
						}	
				}		
				break;		
					
				case 0x25:      //wifi产测模式 11 03 25 IP 01 CS
				{
							if(aRxBuffer_2[1] == 0x03)	
							{  
										Wifi_flag=aRxBuffer_2[4];
										if(aRxBuffer_2[4] ==0x01)
										{
												USB_R_BUF_LAST[0]=0x16;
												USB_R_BUF_LAST[1]=0x03;				
												USB_R_BUF_LAST[2]=aRxBuffer_2[2];
												USB_R_BUF_LAST[3]=IP_485_2;
												length = USB_R_BUF_LAST[1] +2;//length=5
												USB_R_BUF_LAST[4] = DataCmd_25[0];	
										}
								}	
				}		
				break;

				case 0x24:      //显示测试中 11 03 24 IP 01 CS 应答16 03 24 IP 00/01/02/03 CS
				{
						if(aRxBuffer_2[1] == 0x03)	
						{  
							USB_R_BUF_LAST[0]=0x16;
							USB_R_BUF_LAST[1]=0x03;				
							USB_R_BUF_LAST[2]=aRxBuffer_2[2];
							USB_R_BUF_LAST[3]=IP_485_2;
							length = USB_R_BUF_LAST[1] +2;//length=	0x05		
							Testing_flag=aRxBuffer_2[4];//length=0
						}		
				break;							
				}
						
				case 0x0E:      // PM2.5标定 11 05 0E IP 10 PM_25[0] PM_25[1] CS
				{
						if(aRxBuffer_2[1] == 0x05)	
						{  
							PM_flag = aRxBuffer_2[4];
							PM_25[0] =aRxBuffer_2[5];
							PM_25[1] =aRxBuffer_2[6];
							USB_R_BUF_LAST[0] = 0x16;
							USB_R_BUF_LAST[1] = 0x05;
							USB_R_BUF_LAST[2] = 0x0E;
							USB_R_BUF_LAST[3] = IP_485_2;
							USB_R_BUF_LAST[4] = PM_flag;
							USB_R_BUF_LAST[5] = PM_25[0];
							USB_R_BUF_LAST[6] = PM_25[1];
							length = USB_R_BUF_LAST[1] +2;//length=7
						}		
				break;							
				}
				
				case 0x0F:      // CM1106标定 11 05 0F IP 10 CM[0] CM_25[1] CS
				{
						if(aRxBuffer_2[1] == 0x05)	
						{  
							CM_Calibrate_flag = aRxBuffer_2[4];
							CM_Calibrate_Value[0] =aRxBuffer_2[5];
							CM_Calibrate_Value[1] =aRxBuffer_2[6];
							USB_R_BUF_LAST[0] = 0x16;
							USB_R_BUF_LAST[1] = 0x05;
							USB_R_BUF_LAST[2] = 0x0F;
							USB_R_BUF_LAST[3] = IP_485_2;
							USB_R_BUF_LAST[4] = CM_Calibrate_flag;
							USB_R_BUF_LAST[5] = CM_Calibrate_Value[0];
							USB_R_BUF_LAST[6] = CM_Calibrate_Value[1];
							length = USB_R_BUF_LAST[1] +2;//length=7							
						}		
				break;							
				}
				
				case 0xFE:      // 查询工装板软件版本11 02 FE IP CS
				{
						if(aRxBuffer_2[1] == 0x02)	
						{  
									for(version_cnt=0;version_cnt<14;version_cnt++)
									{
										USB_R_BUF_LAST[version_cnt]=Version[version_cnt];
									}
									HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,14 ,0xffff);   //输出软件版本号
						}		
				break;							
				}
		}		
			
		if(USB_R_BUF_LAST[3]>0 && aRxBuffer_2[2]!=0xfe)   
			//当IP大于零，且命令不是查询工装软件版本时，计算CS校验位并将USB_R_BUF_LAST的值输出
		{
				checksum = 0;
				for(i=0;i<length;i++)					//计算checksum
				{
						checksum += USB_R_BUF_LAST[i];								
				}
				USB_R_BUF_LAST[length] =-checksum; 
				if(length != 0x14 && length != 0x0E && length != 0x11 && length != 0x05 && length != 0x07) 
				return;//返回数据长度与查询不符合，说明主机没有查询到从机数据，为异常情况，不进行输出
				HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //串口输出USB_R_BUF的数据
				//memset(USB_R_BUF_LAST,0,21);
		}
	}
	
	else if(aRxBuffer_2[2] == 0xFB) // 查询工装板IP号 11 01 FB F3 应答 16 01 IP CS
	{
			if(aRxBuffer_2[1] == 0x01)	
			{        
						USB_R_BUF_LAST[0]=0x16;
						USB_R_BUF_LAST[1]=0x01;				
						USB_R_BUF_LAST[2]= *(__IO uint32_t *)FLASH_USER_START_ADDR;
						length = USB_R_BUF_LAST[1] +2;  //length=3
						checksum = 0;
						for(i=0;i<length;i++)					//计算checksum
						{
								checksum += USB_R_BUF_LAST[i];								
						}
						USB_R_BUF_LAST[length] =-checksum; 
						HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //串口输出USB_R_BUF的数据
			} 	
	}
			
	else if(aRxBuffer_2[2] == 0xDD)	//当指令的IP与IP_485_2不等时，且第三个字节为0xdd，则写IP地址 11 02 DD IP CS
	{
				HAL_FLASH_Unlock();
				Erase_Flash();
				Address = FLASH_USER_START_ADDR;
				HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address, aRxBuffer_2[3]);
				HAL_FLASH_Lock(); 
		
				USB_R_BUF_LAST[0]=0x16;
				USB_R_BUF_LAST[1]=0x02;				
				USB_R_BUF_LAST[2]=aRxBuffer_2[2];
				USB_R_BUF_LAST[3]= *(__IO uint32_t *)FLASH_USER_START_ADDR;
				length = USB_R_BUF_LAST[1] +2;  //length=4
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
void USB_HOST_Task(USBH_HandleTypeDef *phost)
{
	  static unsigned char SendCnt,checksum_2,i_2;
		if(SystickFlag1S == 1)
		{
		    SystickFlag1S=0;
				if(Testing_flag<=0x03)
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x02;
						USB_T_BUF[2]=0x24;
						USB_T_BUF[3]=Testing_flag;
						checksum_2 = 0;
						for(i_2=0;i_2<4;i_2++)					//计算checksum
						{
									checksum_2 += USB_T_BUF[i_2];								
						}
						USB_T_BUF[4] =-checksum_2; 
						USBH_CDC_Transmit(phost,USB_T_BUF,5);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
						Testing_flag=0xFF;
						SendCnt =100;
				}
				if(Wifi_flag==0x10 || Wifi_flag==0x01)
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x02;
						USB_T_BUF[2]=0x25;
						USB_T_BUF[3]=Wifi_flag;
						checksum_2 = 0;
						for(i_2=0;i_2<4;i_2++)					//计算checksum
						{
								checksum_2 += USB_T_BUF[i_2];								
						}
						USB_T_BUF[4] =-checksum_2; 
						USBH_CDC_Transmit(phost,USB_T_BUF,5);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
						Wifi_flag=0xFF;
						SendCnt =100;
				}
				if(PM_flag==0x10)
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x04;
						USB_T_BUF[2]=0x0E;
						USB_T_BUF[3]=PM_flag;
						USB_T_BUF[4]=PM_25[0];
						USB_T_BUF[5]=PM_25[1];
						checksum_2 = 0;
						for(i_2=0;i_2<6;i_2++)					//计算checksum
						{
								checksum_2 += USB_T_BUF[i_2];								
						}
						USB_T_BUF[6] =-checksum_2; 
						USBH_CDC_Transmit(phost,USB_T_BUF,7);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
						PM_flag=0xFF;
						SendCnt =100;
				}
				if(CM_Calibrate_flag==0x10)
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x04;
						USB_T_BUF[2]=0x0F;
						USB_T_BUF[3]=CM_Calibrate_flag;
						USB_T_BUF[4]=CM_Calibrate_Value[0];
						USB_T_BUF[5]=CM_Calibrate_Value[1];
				
						checksum_2 = 0;
						for(i_2=0;i_2<6;i_2++)					//计算checksum
						{
								checksum_2 += USB_T_BUF[i_2];								
						}
						USB_T_BUF[6] =-checksum_2; 
						USBH_CDC_Transmit(phost,USB_T_BUF,7);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
						CM_Calibrate_flag=0xFF;
						SendCnt =100;
				}
				if(SendCnt==2)   // 读取编号（上电2s）  
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x01;
						USB_T_BUF[2]=0x1F;
						USB_T_BUF[3]=0xCF;
						USBH_CDC_Transmit(phost,USB_T_BUF,4);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
				}
				else if(SendCnt==5)		// 读取版本（上电5s）
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x01;
						USB_T_BUF[2]=0x1E;
						USB_T_BUF[3]=0xD0;
						USBH_CDC_Transmit(phost,USB_T_BUF,4);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
				}
				else if(SendCnt<=60)  //读取数据
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x01;
						USB_T_BUF[2]=0x0b;
						USB_T_BUF[3]=0xe3;
						USBH_CDC_Transmit(phost,USB_T_BUF,4);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
				}
				if(++SendCnt>=30)     SendCnt=0;
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
		
		if(checksum_3 == 0)//成功校验上
		{
			HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_12);//当收到校验上的一帧USB数据后，PB12会翻转
			if(USB_R_BUF[2]==0x1F)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)
				{
						DataCmd_1F[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_1F[0]=USB_R_BUF[1];   //将收到数据的第二个字节（表示长度）放在LengthCmd中
			}
			else if(USB_R_BUF[2]==0x1E)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)
				{
						DataCmd_1E[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_1E[0]=USB_R_BUF[1];  //将收到数据的第二个字节（表示长度）放在LengthCmd中
			}
			else if(USB_R_BUF[2]==0x0B)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)//USB_R_BUF[1]=0x11
				{
						DataCmd_0B[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_0B[0]=USB_R_BUF[1];		//将收到数据的第二个字节（表示长度）放在LengthCmd中
				DataCmd_0B[USB_R_BUF[1]-4] = 0x01;//当USB有完整帧应答时，将DataCmd_0B[13],即DF14置为1
			}
			else if(USB_R_BUF[2]==0x25)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)
				{
						DataCmd_25[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_25[0]=USB_R_BUF[1];		//将收到数据的第二个字节（表示长度）放在LengthCmd中
			}
		}
		
		else if(checksum_3!=0)//校验不上
		{
			if(USB_R_BUF[2]==0x1F)
			{
					LengthCmd_1F[0] = 0x01;
			}
			else if(USB_R_BUF[2]==0x1E)
			{
					LengthCmd_1E[0] = 0x01;
			}
			else if(USB_R_BUF[2]==0x0B)
			{
					LengthCmd_0B[0] = 0x01;
					DataCmd_0B[USB_R_BUF[1]-4] = 0x00;//校验不上时，将DataCmd_0B[13],即DF14清为0
			}
			else if(USB_R_BUF[2]==0x25)
			{
					LengthCmd_25[0] = 0x01;
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

