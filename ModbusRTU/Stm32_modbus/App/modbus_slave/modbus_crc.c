#include "modbus_crc.h"

uint16_t Modbus_CRC16(const uint8_t *data,uint16_t length)
{
	uint16_t crc=0xFFFFU;
	uint16_t i;
	while(length--)
	{
		crc^=*data++;
		for(i=0;i<8;i++)
		{
			if(crc&0x0001U)
			{
				crc=(uint16_t)((crc>>1U)^0xA001U);
			}
			else
			{
				crc>>=1U;
			}
		}
	}
	return crc;
}
