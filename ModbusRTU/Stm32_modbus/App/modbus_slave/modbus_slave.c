#include "modbus_slave.h"
#include "modbus_crc.h"
#include "bsp_rs485.h"
#include "register_map.h"

#define MODBUS_FUNC_READ_HOLDING       0x03U //读保持寄存器
#define MODBUS_FUNC_WRITE_SINGLE       0x06U //写单个寄存器
#define MODBUS_FUNC_WRITE_MULTIPLE     0x10U //写多个寄存器

#define MODBUS_EX_ILLEGAL_FUNCTION     0x01U //非法功能码
#define MODBUS_EX_ILLEGAL_ADDRESS      0x02U //非法数据地址
#define MODBUS_EX_ILLEGAL_VALUE        0x03U //非法数据值

static volatile uint8_t s_rx_buf[MODBUS_RX_BUFFER_SIZE];
static volatile uint16_t s_rx_len = 0U;
static uint8_t s_slave_id = 1U;

volatile uint8_t g_modbus_frame_ready = 0U;

static void Modbus_Send(uint8_t *buf, uint16_t len);
static void Modbus_SendException(uint8_t func, uint8_t code);
static void Modbus_HandleFrame(uint8_t *frame, uint16_t len);
static void Modbus_HandleReadHolding(uint8_t *frame, uint16_t len);
static void Modbus_HandleWriteSingle(uint8_t *frame, uint16_t len);
static void Modbus_HandleWriteMultiple(uint8_t *frame, uint16_t len);
static uint16_t ReadU16BE(const uint8_t *p);
static void WriteU16BE(uint8_t *p, uint16_t value);

void Modbus_Init(uint8_t slave_id)
{
    s_slave_id = slave_id;
    s_rx_len = 0U;
    g_modbus_frame_ready = 0U;
}

void Modbus_ReceiveByte(uint8_t byte)
{
	if(g_modbus_frame_ready!=0)
	{
		return;
	}
	if(s_rx_len<MODBUS_RX_BUFFER_SIZE)
	{
		s_rx_buf[s_rx_len]=byte;
		s_rx_len++;
	}
	else
	{
		s_rx_len=0;
	}
}

void Modbus_Poll(void)
{
	uint8_t frame[MODBUS_RX_BUFFER_SIZE];
	uint16_t len,i;
	if(g_modbus_frame_ready==0)
	{
		return;
	}
	__disable_irq();
	len=s_rx_len;
	for(i=0;i<len;i++)
	{
		frame[i]=s_rx_buf[i];
	}
	g_modbus_frame_ready=0U;
	s_rx_len=0U;
	__enable_irq();
	Modbus_HandleFrame(frame,len);
}

static void Modbus_HandleFrame(uint8_t *frame,uint16_t len)
{
	uint16_t crc_calc;
	uint16_t crc_recv;
	if(len<4U)
	{
		return; 
	}
	if(frame[0]!=s_slave_id && frame[0]!=0U)
	{
		return;
	}
	crc_calc=Modbus_CRC16(frame,(uint16_t)(len-2U));
	crc_recv=frame[len-1U]<<8|frame[len-2U];
	if(crc_calc!=crc_recv)
	{
		return;
	}
	switch(frame[1])
	{
		case MODBUS_FUNC_READ_HOLDING:
			Modbus_HandleReadHolding(frame,len);
			break;
		case MODBUS_FUNC_WRITE_SINGLE:
			Modbus_HandleWriteSingle(frame,len);
			break;
		case MODBUS_FUNC_WRITE_MULTIPLE:
			Modbus_HandleWriteMultiple(frame,len);
			break;
		default:
			if(frame[0]!=0)
			{
				Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_FUNCTION);
			}
			break;
	}
}


static void Modbus_HandleReadHolding(uint8_t *frame, uint16_t len)
{
	uint16_t addr;
	uint16_t count;
	uint16_t i;
	uint16_t value;
	uint8_t tx[MODBUS_RX_BUFFER_SIZE];
	if(len!=8U)  //如果数据长度不够，并且非广播，则抛出数据错误
	{
		if(frame[0]!=0)
		{
			Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_VALUE);
			return;
		}
	}
	if(frame[0]==0) //广播地址还读个锤子的保持寄存器，返回
	{
		return;
	}
	addr=ReadU16BE(&frame[2]);
	count=ReadU16BE(&frame[4]);
	if(count==0 || count>125)
	{
		Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_VALUE);
		return;
	}
	if(RegisterMap_CheckRange(addr,count)==0U) //检查从addr开始读count个寄存器是否越界
	{
		Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_ADDRESS);
		return;
	}
	tx[0]=s_slave_id;
	tx[1]=MODBUS_FUNC_READ_HOLDING;
	tx[2]=count*2;
	for(i=0;i<count;i++)
	{
		value=RegisterMap_ReadHolding((uint16_t)(addr+i));
		WriteU16BE(&tx[3U+(i*2U)],value);
	}
	Modbus_Send(tx,(uint16_t)(3U+count*2U));
}

static void Modbus_HandleWriteSingle(uint8_t *frame,uint16_t len)
{
	uint16_t addr;
	uint16_t value;
	if(len!=8U)
	{
		if(frame[0]!=0U)
		{
			Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_VALUE);
		}
		return;
	}
	addr=ReadU16BE(&frame[2]);
	value=ReadU16BE(&frame[4]);
	if(RegisterMap_WriteHolding(addr,value)==0U)
	{
		if(frame[0]!=0U)
		{
			Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_ADDRESS);
		}
		return;
	}
	if(frame[0]!=0U)
	{
		Modbus_Send(frame,6U);
	}
}

static void Modbus_HandleWriteMultiple(uint8_t *frame, uint16_t len)
{
	uint16_t addr;
	uint16_t count;
	uint8_t byte_count;
	uint16_t i;
	if(len<9U)
	{
		if(frame[0]!=0)
		{
			Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_VALUE);
		}
		return;
	}
	addr=ReadU16BE(&frame[2]);
	count=ReadU16BE(&frame[4]);
	byte_count=frame[6];
	if((count==0U) || (count>123U) || ((uint8_t)2*count!=byte_count) || len!=(uint16_t)(9U+byte_count) || (RegisterMap_CheckRange(addr,count)==0U))
	{
		if(frame[0]!=0)
		{
			Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_ADDRESS);
		}
		return;
	}
	for(i=0U;i<count;i++)
	{
		if(RegisterMap_WriteHolding((uint16_t)(addr+i),ReadU16BE(&frame[7U+(i*2U)]))==0U)
		{
			if(frame[0]!=0)
			{
				Modbus_SendException(frame[1],MODBUS_EX_ILLEGAL_ADDRESS);
			}
			return;
		}
	}	
	if(frame[0]!=0)
	{
		Modbus_Send(frame,6U);
	}
}

static void Modbus_SendException(uint8_t func,uint8_t code)
{
	uint8_t tx[5];
	tx[0]=s_slave_id;
	tx[1]=(uint8_t)(func|0x80U);
	tx[2]=code;
	Modbus_Send(tx,3U);
}

static void Modbus_Send(uint8_t *buf,uint16_t len)
{
	uint16_t crc;
	crc=Modbus_CRC16(buf,len);
	buf[len]=(uint8_t)(crc&0x00FFU);
	buf[len+1]=(uint8_t)(crc>>8U);
	RS485_Send_Data(buf,(uint8_t)(len+2));
}

static uint16_t ReadU16BE(const uint8_t *p)
{
	return (uint16_t)(((uint16_t)p[0]<<8U)|p[1]);
}

static void WriteU16BE(uint8_t *p,uint16_t value)
{
	p[0]=(uint8_t)(value>>8U);
	p[1]=(uint8_t)(value&0x00FFU);
}
