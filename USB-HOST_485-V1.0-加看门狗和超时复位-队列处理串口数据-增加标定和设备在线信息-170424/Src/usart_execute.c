#include "usart_execute.h"

struct uart uart_rev;
recv_state_t recv_state;
fream_direction usart_frame_direction;
unsigned char aRxBuffer[BUFFSIZE];//���л�����

/*----------------------------------------------------------------------------------------
* ��������: ���ڻص�������ÿ�յ�1���ֽ����ݣ��˺�������һ��
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* ��    ����1.0
-----------------------------------------------------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)  
{  
    uint8_t ret = HAL_OK;  
    /* Set transmission flag: trasfer complete*/  
    uart_rev.rear++;    //����rearָ��  
    if(uart_rev.rear >= (aRxBuffer + BUFFSIZE))  
        uart_rev.rear = aRxBuffer;  
    do  
    {  
        ret = HAL_UART_Receive_IT(&huart1,uart_rev.rear,1);  
    }while(ret != HAL_OK);  
}  		

/*----------------------------------------------------------------------------------------
* ��������:�ӽ��ն��л������ж�ȡ���� 
* ��		��: uint8_t *fmt--------------------------------���յ������� 
						uint8_t read_finished_flag--------------�Ƿ�ȡ�����ݱ�־ 
* �� �� ֵ:0:û�ж�ȡ������		1:�ж�ȡ������ 
* �� �� �ߣ�HuShengS ���ڣ�2017-04-20
* �� �� �ߣ�HuShengS ���ڣ�2017-04-20
* ��    ����1.0
-----------------------------------------------------------------------------------------*/ 
//int8_t uart_queen_read(uint8_t *fmt, uint16_t time_out)  
int8_t uart_queen_read(uint8_t *fmt,uint8_t read_finished_flag)  
{  
    if(uart_rev.front != uart_rev.rear)  //�������ָ��Ͷ�βָ�벻ͬ�����������������ݻ�δ��ȡ
    {
				read_finished_flag = 1;
        *fmt=*uart_rev.front;  
        uart_rev.front++;  
        if (uart_rev.front >= (aRxBuffer+BUFFSIZE))  
        uart_rev.front = aRxBuffer;  
    }  
		else read_finished_flag = 0;
		return read_finished_flag;  
}

/*----------------------------------------------------------------------------------------
* ��������: ����״̬��������
* �� �� �ߣ�HuShengS ���ڣ�2017-04-20
* �� �� �ߣ�HuShengS ���ڣ�2017-04-20
* ��    ����1.0
-----------------------------------------------------------------------------------------*/
void usart_state_machine(void)
{
	static uint8_t length;
	static uint8_t recv_buffer[30];
	static uint8_t recv_cs;
	static uint8_t recv_data_cnt;
	static uint8_t a[10];
	static uint8_t *temp = a;
	static int8_t Uart_Cnt = 0;
	uint8_t read_finished_flag1;
	
	if(uart_queen_read(temp,read_finished_flag1) == 0) return;//���ö��������ݺ���,��û��ȡ�����ݣ���������и�ֵ
	aRxBuffer_2[Uart_Cnt]= *temp;//���ݴ���*temp�е�����ȡ����
	switch(recv_state)
	{
		case RECV_STATE_START:
			if(aRxBuffer_2[Uart_Cnt] == 0x11)
			{
				usart_frame_direction = tx_direction;
				recv_state = RECV_STATE_LEN;
				recv_cs += aRxBuffer_2[Uart_Cnt];
			}
			else if(aRxBuffer_2[Uart_Cnt] == 0x16)
			{
				usart_frame_direction = rx_direction;
				recv_state = RECV_STATE_LEN;
				recv_cs += aRxBuffer_2[Uart_Cnt];
			}
			else//�쳣���
			{
				Uart_Cnt = -1;
				recv_state = RECV_STATE_START;
				recv_cs = 0;
			}
			break;
		
		case RECV_STATE_LEN:
			length = aRxBuffer_2[Uart_Cnt];
			if(length > 0x12)////���ȴ���18ʱ���ж�Ϊ�쳣֡
			{
				Uart_Cnt = -1;
				recv_state = RECV_STATE_START;
				recv_cs = 0;
			}
			else //�������
			{
				recv_cs += aRxBuffer_2[Uart_Cnt];
				recv_state = RECV_STATE_CMD;
			}
			break;
		
		case RECV_STATE_CMD:
			recv_cs += aRxBuffer_2[Uart_Cnt];
			if(length == 0x00)//����Ϊ0ʱ���ж�Ϊ�쳣֡
			{
				Uart_Cnt = -1;
				recv_state = RECV_STATE_START;
				recv_cs = 0;
			}
			else if(length == 0x01) recv_state =RECV_STATE_CS;
			else recv_state = RECV_STATE_DATA;
			recv_data_cnt = 0;//�������ֽڼ������㣬Ϊ����λ�Ľ�����׼��			
			break;
		
		case RECV_STATE_DATA:
			recv_buffer[recv_data_cnt++] = aRxBuffer_2[Uart_Cnt];
			recv_cs += recv_buffer[recv_data_cnt-1];
			if(recv_data_cnt == length-1)//����������λ�������ʱ
			{
				recv_state = RECV_STATE_CS;
			}
			break;
			
		case RECV_STATE_CS:
			recv_cs += aRxBuffer_2[Uart_Cnt];
			if(recv_cs == 0)
			{
				if(usart_frame_direction == tx_direction)//����У���ϣ���Ϊ������λ����һ֡����0x11
				{
					UART_Rx_flag=1;
				}
				else if(usart_frame_direction == rx_direction)//����У���ϣ�Ϊ485�����ϴӻ�Ӧ���һ֡����0x16
				{
					UART_Rx_flag=2;
				}
			}
			else Uart_Cnt = -1;//��һ֡�����꣬��ûУ���ϣ���Uart_Cnt��ֵ-1������++��Ϊ0
			recv_state = RECV_STATE_START;
			break;
			
		default:
			break;
	}
	if(Uart_Cnt++ == 40) Uart_Cnt = 0;//�������ﻺ���յ㣬����������
	if(UART_Rx_flag == 1 || UART_Rx_flag == 2)//��У���ϵ�һ֡ʱ������������
	{
		Uart_Cnt = 0;//У����һ֡�󽫼�������
	}
}

/*-----------------------------------------------------------------------------
* �� �� ��: Add_Check()
* ��������: ��У�麯��
* ȫ�ֱ���: 
* ��    ��:��ҪУ���֡����array[]�������ֽڳ���length
* ��    ��:У��ɹ�return 1��У�鲻�ɹ�return 0
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* �� �� �ߣ�HuShengS ���ڣ�2016-09-20
* ��    ����1.0
------------------------------------------------------------------------------*/
int Add_Check(unsigned char array[],int length)	//lengthΪ��У������֡���ܳ���
{
		int i;
		unsigned char checksum=0;
		for(i=0;i<length;i++)					//����checksum
		{
				checksum += array[i];								
		}
		if(array[length] ==(unsigned char)(-checksum))		return 1;
		else return 0;
}
