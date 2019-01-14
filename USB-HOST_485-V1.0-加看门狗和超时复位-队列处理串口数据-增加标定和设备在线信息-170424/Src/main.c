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



UART_HandleTypeDef huart1; //����
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
  System_Init();    //ϵͳ��ʼ��
	IP_485 = *(__IO uint32_t *)FLASH_USER_START_ADDR;   //��Flash��ʼ��ַ��ȡ��װ��ģ��IP
	IP_485_2 = (unsigned char)IP_485;
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(&huart1,UART_IT_TXE);
	HAL_UART_Receive_IT(&huart1,uart_rev.rear, 1);	 //�ϵ����ָ������1���������ݣ��������ڽ��յ����ݷ���aRxBuffer�У�ÿ�ν���1���ֽ�
		 
  while (1)
  {
		WatchDog_Feed();//ι��	
		if(SystickFlag20ms_2==1)   //20ms�ı�־��ÿ20msö��һ��USB�ӻ�
			{
				SystickFlag20ms_2=0;
				if(SysBootCnt<=10000)	SysBootCnt++;
				else SysBootCnt = 10;
				MX_USB_HOST_Process();   //USB���豸ö�١���ȡ�豸��ʶ��
			}
			USB_RT_Process(&hUsbHostFS);		//��USB�ӻ���ѯ����
			UartPCTask();		//PC����������
	}
}

/*----------------------------------------------------------------------------------------
* ��������: USB�ӻ����ݲ�ѯ
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* ��    ����1.0
-----------------------------------------------------------------------------------------*/
void USB_RT_Process(USBH_HandleTypeDef *phost)
{
//	CDC_HandleTypeDef *CDC_Handle =  (CDC_HandleTypeDef*) phost->pActiveClass->pData; 
  if(phost->gState != HOST_CLASS)	  //״̬�жϣ�ö�ٲ��ɹ��򷵻�
	{
		memset(DataCmd_0B,0,sizeof(DataCmd_0B));//��USBö�ٲ��ϣ�ʹ��ʵʱ����ָ����Ӧ��
		LengthCmd_0B[0] = 0x01;
		return;
	}

	if(USB_SEND_flag == 1)
	{
		USB_SEND_flag = 0;
		pUartTest.b.dwDTERate = 9600;//��������: ���⴮�ڲ���������
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
		if(SysBootCnt>=10)   //�����ϵ�ʱ����һ����ʱ
		{
			USB_SEND_flag = 2;
			USB_HOST_Task(&hUsbHostFS);   //USB������
		}
	}
}

/*-----------------------------------------------------------------------------
* �� �� ��: UartPCTask()
* ��������: PC������������
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* ��    ����
------------------------------------------------------------------------------*/
void  UartPCTask(void)  
{
		static unsigned int restart_system_cnt;
//		if(SystickFlag10ms == 1)//10ms�����Ӷ���ȡ���ݺͽ�״̬������У��
//		{
//			SystickFlag10ms = 0;
			usart_state_machine();//��ѭ���в���ȡ��������
//		}
		if(UART_Rx_flag ==1)		    //�ɹ��յ�ָ��
		{
			RxExecute();									//����ָ����Ӧ
			restart_system_cnt = 0;//���յ�һ֡���ݣ���ϵͳ������������
			memset(aRxBuffer_2,0,40);   //��aRxBuffer_2ǰ40���ֽ�����
			UART_Rx_flag =0; //ȡ��������һ֡����Ҫ��������֡���ݱ�־����
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);//���յ�У���ϵ�һ֡���ݺ�PA5�ᷭת
		}
		else if(UART_Rx_flag ==2)
		{
			UART_Rx_flag =0; //ȡ��������һ֡����Ҫ��������֡���ݱ�־����
		}
		else
		{
			if(SystickFlag1S_restart ==1)
			{	
				SystickFlag1S_restart = 0;
				if(restart_system_cnt++ >= 30)//30s���ղ�����λ�������ݣ�������ϵͳ
				{
					restart_system_cnt = 0;
					HAL_NVIC_SystemReset();//����ϵͳ
				}
			}
		}
}

/*-----------------------------------------------------------------------------
* �� �� ��: RxExecute()
* ��������: PC������Ӧ�����������յ��Ĵ���ָ�������Ӧ����ʵʱ���ظ�PC��
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* ��    ����V1.0
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
				case 0x0b:      //��ȡ���в������� 11 02 0b IP CS  (11 02 0b 01 e1)
				{
						if(aRxBuffer_2[1] == 0x02)	 // 16 12 0B IP 00 1E 03 0C 01 74 00 00 00 00 00 00 00 00 00 00 CS 
						{   
									USB_R_BUF_LAST[0]=0x16;
									USB_R_BUF_LAST[1]=(LengthCmd_0B[0]+1);//0x12	
									USB_R_BUF_LAST[2]=aRxBuffer_2[2];
									USB_R_BUF_LAST[3]=IP_485_2;
									length = USB_R_BUF_LAST[1] +2;   //��cs�⣬�����������ݵ��ֽڳ��ȣ���ʱΪlength=20��
									for(i=0;i<length-4;i++)
									{
											USB_R_BUF_LAST[i+4] = DataCmd_0B[i];		//��ȥ��֡ͷ��֡β�����ݴ��ݸ�����Ҫ���������						
									}
						}
				}
				break;

				case 0x1f:      //��ѯ�豸��� 11 02 1f IP CS
				{
						if(aRxBuffer_2[1] == 0x02)	
						{        
									USB_R_BUF_LAST[0]=0x16;
//									USB_R_BUF_LAST[1]=(LengthCmd_1F[0]+1);	//��cs�⣬�����������ݵ��ֽڳ��ȣ���ʱΪlength=14
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
						
				case 0x1e:      //��ѯ����汾�� 11 02 1e IP CS
				{														//
						if(aRxBuffer_2[1] == 0x02)	
						{        
									USB_R_BUF_LAST[0]=0x16;
//									USB_R_BUF_LAST[1]=(LengthCmd_1E[0]+1);		
									USB_R_BUF_LAST[1]=(LengthCmd_1E[0]+1);		//0x0F							
									USB_R_BUF_LAST[2]=aRxBuffer_2[2];
									USB_R_BUF_LAST[3]=IP_485_2;
									length = USB_R_BUF_LAST[1] +2;  ////��cs�⣬�����������ݵ��ֽڳ���(length=17)
									for(i=0;i<length-4;i++)
									{
											USB_R_BUF_LAST[i+4] = DataCmd_1E[i];								
									}	
						}	
				}		
				break;		
					
				case 0x25:      //wifi����ģʽ 11 03 25 IP 01 CS
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

				case 0x24:      //��ʾ������ 11 03 24 IP 01 CS Ӧ��16 03 24 IP 00/01/02/03 CS
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
						
				case 0x0E:      // PM2.5�궨 11 05 0E IP 10 PM_25[0] PM_25[1] CS
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
				
				case 0x0F:      // CM1106�궨 11 05 0F IP 10 CM[0] CM_25[1] CS
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
				
				case 0xFE:      // ��ѯ��װ������汾11 02 FE IP CS
				{
						if(aRxBuffer_2[1] == 0x02)	
						{  
									for(version_cnt=0;version_cnt<14;version_cnt++)
									{
										USB_R_BUF_LAST[version_cnt]=Version[version_cnt];
									}
									HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,14 ,0xffff);   //�������汾��
						}		
				break;							
				}
		}		
			
		if(USB_R_BUF_LAST[3]>0 && aRxBuffer_2[2]!=0xfe)   
			//��IP�����㣬������ǲ�ѯ��װ����汾ʱ������CSУ��λ����USB_R_BUF_LAST��ֵ���
		{
				checksum = 0;
				for(i=0;i<length;i++)					//����checksum
				{
						checksum += USB_R_BUF_LAST[i];								
				}
				USB_R_BUF_LAST[length] =-checksum; 
				if(length != 0x14 && length != 0x0E && length != 0x11 && length != 0x05 && length != 0x07) 
				return;//�������ݳ������ѯ�����ϣ�˵������û�в�ѯ���ӻ����ݣ�Ϊ�쳣��������������
				HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //�������USB_R_BUF������
				//memset(USB_R_BUF_LAST,0,21);
		}
	}
	
	else if(aRxBuffer_2[2] == 0xFB) // ��ѯ��װ��IP�� 11 01 FB F3 Ӧ�� 16 01 IP CS
	{
			if(aRxBuffer_2[1] == 0x01)	
			{        
						USB_R_BUF_LAST[0]=0x16;
						USB_R_BUF_LAST[1]=0x01;				
						USB_R_BUF_LAST[2]= *(__IO uint32_t *)FLASH_USER_START_ADDR;
						length = USB_R_BUF_LAST[1] +2;  //length=3
						checksum = 0;
						for(i=0;i<length;i++)					//����checksum
						{
								checksum += USB_R_BUF_LAST[i];								
						}
						USB_R_BUF_LAST[length] =-checksum; 
						HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //�������USB_R_BUF������
			} 	
	}
			
	else if(aRxBuffer_2[2] == 0xDD)	//��ָ���IP��IP_485_2����ʱ���ҵ������ֽ�Ϊ0xdd����дIP��ַ 11 02 DD IP CS
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
				for(i=0;i<length;i++)					//����checksum
				{
						checksum += USB_R_BUF_LAST[i];								
				}
				USB_R_BUF_LAST[length] =-checksum; 
				HAL_UART_Transmit(&huart1,(uint8_t *)USB_R_BUF_LAST,length+1 ,0xffff);   //�������USB_R_BUF������
	}
}
	
/*-----------------------------------------------------------------------------
* �� �� ��: USB_HOST_Task()
* ��������: USB��������USB�ӻ�����ָ���USB�ص����������������
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* ��    ����V1.0
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
						for(i_2=0;i_2<4;i_2++)					//����checksum
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
						for(i_2=0;i_2<4;i_2++)					//����checksum
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
						for(i_2=0;i_2<6;i_2++)					//����checksum
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
						for(i_2=0;i_2<6;i_2++)					//����checksum
						{
								checksum_2 += USB_T_BUF[i_2];								
						}
						USB_T_BUF[6] =-checksum_2; 
						USBH_CDC_Transmit(phost,USB_T_BUF,7);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
						CM_Calibrate_flag=0xFF;
						SendCnt =100;
				}
				if(SendCnt==2)   // ��ȡ��ţ��ϵ�2s��  
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x01;
						USB_T_BUF[2]=0x1F;
						USB_T_BUF[3]=0xCF;
						USBH_CDC_Transmit(phost,USB_T_BUF,4);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
				}
				else if(SendCnt==5)		// ��ȡ�汾���ϵ�5s��
				{
						USB_T_BUF[0]=0x11;
						USB_T_BUF[1]=0x01;
						USB_T_BUF[2]=0x1E;
						USB_T_BUF[3]=0xD0;
						USBH_CDC_Transmit(phost,USB_T_BUF,4);
						USBH_CDC_Receive(phost,USB_R_BUF,8);
				}
				else if(SendCnt<=60)  //��ȡ����
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
* ��������: USB�ص�����
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2017-03-30
* ��    ����1.1
-----------------------------------------------------------------------------------------*/	
void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost)
{
		unsigned char checksum_3=0,i_3;	
		for(i_3=0;i_3<(USB_R_BUF[1]+3);i_3++)					//���յ������ݽ��к�У�飬��Żظ��ֽ�Ϊ14���ֽ�
		{
				checksum_3 += USB_R_BUF[i_3];								
		}
		
		if(checksum_3 == 0)//�ɹ�У����
		{
			HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_12);//���յ�У���ϵ�һ֡USB���ݺ�PB12�ᷭת
			if(USB_R_BUF[2]==0x1F)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)
				{
						DataCmd_1F[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_1F[0]=USB_R_BUF[1];   //���յ����ݵĵڶ����ֽڣ���ʾ���ȣ�����LengthCmd��
			}
			else if(USB_R_BUF[2]==0x1E)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)
				{
						DataCmd_1E[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_1E[0]=USB_R_BUF[1];  //���յ����ݵĵڶ����ֽڣ���ʾ���ȣ�����LengthCmd��
			}
			else if(USB_R_BUF[2]==0x0B)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)//USB_R_BUF[1]=0x11
				{
						DataCmd_0B[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_0B[0]=USB_R_BUF[1];		//���յ����ݵĵڶ����ֽڣ���ʾ���ȣ�����LengthCmd��
				DataCmd_0B[USB_R_BUF[1]-4] = 0x01;//��USB������֡Ӧ��ʱ����DataCmd_0B[13],��DF14��Ϊ1
			}
			else if(USB_R_BUF[2]==0x25)
			{
				for(r_buf_cnt=3;r_buf_cnt<=(USB_R_BUF[1]+1);r_buf_cnt++)
				{
						DataCmd_25[r_buf_cnt-3]=USB_R_BUF[r_buf_cnt];
				}
				LengthCmd_25[0]=USB_R_BUF[1];		//���յ����ݵĵڶ����ֽڣ���ʾ���ȣ�����LengthCmd��
			}
		}
		
		else if(checksum_3!=0)//У�鲻��
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
					DataCmd_0B[USB_R_BUF[1]-4] = 0x00;//У�鲻��ʱ����DataCmd_0B[13],��DF14��Ϊ0
			}
			else if(USB_R_BUF[2]==0x25)
			{
					LengthCmd_25[0] = 0x01;
			}
			return;//У�鲻��ʱ���˳�
		}
}

/* USER CODE BEGIN 4 */
/* ���õδ�ʱ��������ʱ*/
void Delay_ms(volatile unsigned int nTime)
{
	TimingDelay = nTime;
	while(TimingDelay !=0);
}

///* Private function prototypes �ض��������ӡ����----------------------------------*/
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

