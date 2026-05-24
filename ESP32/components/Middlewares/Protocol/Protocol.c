#include "Protocol.h"

uint8_t Calc_Checksum(uint8_t *data, uint16_t length)
{
	uint8_t sum=0;
	for(uint8_t i=0;i<length;i++)
	{
		sum+=data[i];
	}
	return sum;
}