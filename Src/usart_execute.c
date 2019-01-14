#include "usart_execute.h"

struct uart uart_rev;
recv_state_t recv_state;
fream_direction usart_frame_direction;
unsigned char aRxBuffer[BUFFSIZE];//队列缓冲区

/*----------------------------------------------------------------------------------------
* 功能描述: 串口回调函数，每收到1个字节数据，此函数进入一次
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2016-09-20
* 版    本：1.0
-----------------------------------------------------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)  
{  
    uint8_t ret = HAL_OK;  
    /* Set transmission flag: trasfer complete*/  
    uart_rev.rear++;    //更新rear指针  
    if(uart_rev.rear >= (aRxBuffer + BUFFSIZE))  
        uart_rev.rear = aRxBuffer;  
    do  
    {  
        ret = HAL_UART_Receive_IT(&huart1,uart_rev.rear,1);  
    }while(ret != HAL_OK);  
}  		

/*----------------------------------------------------------------------------------------
* 功能描述:从接收队列缓冲区中读取数据 
* 参		数: uint8_t *fmt--------------------------------接收到的数据 
						uint8_t read_finished_flag--------------是否取出数据标志 
* 返 回 值:0:没有读取到数据		1:有读取到数据 
* 设 计 者：HuShengS 日期：2017-04-20
* 修 改 者：HuShengS 日期：2017-04-20
* 版    本：1.0
-----------------------------------------------------------------------------------------*/ 
//int8_t uart_queen_read(uint8_t *fmt, uint16_t time_out)  
int8_t uart_queen_read(uint8_t *fmt,uint8_t read_finished_flag)  
{  
    if(uart_rev.front != uart_rev.rear)  //如果队首指针和队尾指针不同表明缓冲区中有数据还未收取
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
* 功能描述: 串口状态机处理函数
* 设 计 者：HuShengS 日期：2017-04-20
* 修 改 者：HuShengS 日期：2017-04-20
* 版    本：1.0
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
	
	if(uart_queen_read(temp,read_finished_flag1) == 0) return;//调用读队列数据函数,若没有取到数据，返回则进行赋值
	aRxBuffer_2[Uart_Cnt]= *temp;//将暂存在*temp中的数据取出来
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
			else//异常情况
			{
				Uart_Cnt = -1;
				recv_state = RECV_STATE_START;
				recv_cs = 0;
			}
			break;
		
		case RECV_STATE_LEN:
			length = aRxBuffer_2[Uart_Cnt];
			if(length > 0x12)////长度大于18时，判断为异常帧
			{
				Uart_Cnt = -1;
				recv_state = RECV_STATE_START;
				recv_cs = 0;
			}
			else //正常情况
			{
				recv_cs += aRxBuffer_2[Uart_Cnt];
				recv_state = RECV_STATE_CMD;
			}
			break;
		
		case RECV_STATE_CMD:
			recv_cs += aRxBuffer_2[Uart_Cnt];
			if(length == 0x00)//长度为0时，判断为异常帧
			{
				Uart_Cnt = -1;
				recv_state = RECV_STATE_START;
				recv_cs = 0;
			}
			else if(length == 0x01) recv_state =RECV_STATE_CS;
			else recv_state = RECV_STATE_DATA;
			recv_data_cnt = 0;//将数据字节计数清零，为数据位的接收做准备			
			break;
		
		case RECV_STATE_DATA:
			recv_buffer[recv_data_cnt++] = aRxBuffer_2[Uart_Cnt];
			recv_cs += recv_buffer[recv_data_cnt-1];
			if(recv_data_cnt == length-1)//当长度数据位接收完成时
			{
				recv_state = RECV_STATE_CS;
			}
			break;
			
		case RECV_STATE_CS:
			recv_cs += aRxBuffer_2[Uart_Cnt];
			if(recv_cs == 0)
			{
				if(usart_frame_direction == tx_direction)//数据校验上，且为来自上位机的一帧数据0x11
				{
					UART_Rx_flag=1;
				}
				else if(usart_frame_direction == rx_direction)//数据校验上，为485总线上从机应答的一帧数据0x16
				{
					UART_Rx_flag=2;
				}
			}
			else Uart_Cnt = -1;//当一帧接收完，但没校验上，将Uart_Cnt赋值-1，后面++后即为0
			recv_state = RECV_STATE_START;
			break;
			
		default:
			break;
	}
	if(Uart_Cnt++ == 40) Uart_Cnt = 0;//计数到达缓冲终点，将计数清零
	if(UART_Rx_flag == 1 || UART_Rx_flag == 2)//有校验上的一帧时，将计数清零
	{
		Uart_Cnt = 0;//校验上一帧后将计数清零
	}
}

/*-----------------------------------------------------------------------------
* 函 数 名: Add_Check()
* 功能描述: 和校验函数
* 全局变量: 
* 输    入:需要校验的帧数据array[]，数据字节长度length
* 返    回:校验成功return 1；校验不成功return 0
* 设 计 者：HuShengS 日期：2016-09-20
* 修 改 者：HuShengS 日期：2016-09-20
* 版    本：1.0
------------------------------------------------------------------------------*/
int Add_Check(unsigned char array[],int length)	//length为和校验数据帧的总长度
{
		int i;
		unsigned char checksum=0;
		for(i=0;i<length;i++)					//计算checksum
		{
				checksum += array[i];								
		}
		if(array[length] ==(unsigned char)(-checksum))		return 1;
		else return 0;
}
