#include "protocol.h"

uint8_t Calc_Checksum(uint8_t *data, uint16_t length)
{
		uint8_t sum=0;
		for(uint8_t i=0;i<length;i++)
		{
			sum+=data[i];
		}
		return sum;
}

void Send_Sensor_Data(uint8_t t_i, uint8_t t_d, uint8_t h_i, uint8_t h_d, uint8_t fan_st)
{
		static DataFrame_t frame;
		
		frame.header1=0xA5;
		frame.header2=0x5A;
		frame.cmd=0x01;
		frame.len=sizeof(SensorPayload_t);
		
		frame.payload.temp_int=t_i;
		frame.payload.temp_dec=t_d;
		frame.payload.humi_int=h_i;
		frame.payload.humi_dec=h_d;
		frame.payload.fan_state=fan_st;
		
		//计算校验和
		uint8_t *calc_start_ptr=(uint8_t*)&frame.cmd;
		uint16_t len=1+1+frame.len;
		frame.checksum=Calc_Checksum(calc_start_ptr,len);
	
		print_all((uint8_t*)&frame,sizeof(DataFrame_t));
}
