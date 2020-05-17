/*
 * Copyright (C) 2020 Thomas Steger
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */
#define TRACERDEV "/dev/ttyXRUSB0"

#include <string.h>
#include <unistd.h>
#include "modbus.h"

int main()
{
	uint8_t req1[] = {0x01, 0x2B, 0x0E, 0x01, 0x00};
	uint8_t req2[] = {0x01, 0x2B, 0x0E, 0x02, 0x00};
	uint8_t req3[] = {0x01, 0x43, 0x30, 0x00, 0x00, 0x0F};
	uint8_t req4[] = {0x01, 0x43, 0x33, 0x02, 0x00, 0x1B};
	uint8_t req5[] = {0x01, 0x43, 0x31, 0x00, 0x00, 0x76};
	uint8_t req6[] = {0x01, 0x43, 0x32, 0x00, 0x00, 0x04};
	uint8_t req7[] = {0x01, 0x43, 0x90, 0x00, 0x00, 0x76};
	uint8_t req8[] = {0x01, 0x43, 0x90, 0x13, 0x00, 0x03};
	uint8_t req9[] = {0x01, 0x02, 0x20, 0x00, 0x00, 0x01};
	uint8_t req10[] = {0x01, 0x41, 0x00, 0x04, 0x75, 0x73, 0x65, 0x72, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
	std::string device(TRACERDEV);
	cModbus ctx(device, 115200, 0, 8, 1);
	ctx.SetDebug(true);
	ctx.SetSlave(1);
	ctx.Connect();
	uint8_t resp[MODBUS_RTU_MAX_ADU_LENGTH];
	int resp_lenght = 0;
	
	for (;;)
	{
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req1, 5, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;
		}
			
			
			
			
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req2, 5, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req3, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req4, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req5, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req6, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req7, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req8, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req9, 6, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req10, 14, resp);
		if (resp_lenght>0)
		{
			printf("Main: Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;	
		}
		
		sleep(5);
	}
	
	return 0;
}