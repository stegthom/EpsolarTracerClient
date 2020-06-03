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

#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "modbus.h"


std::vector<std::string> split(const std::string& input, const std::string& separator = " ")
{
  std::vector<std::string> result;
  std::string::size_type position, start = 0;
 
  while (std::string::npos != (position = input.find(separator, start)))
  {
    result.push_back(input.substr(start, position-start));
    start = position + separator.size();
  }
 
  result.push_back(input.substr(start));
  return result;
}


int main( int argc, char* argv[] )
{
	if (argc < 2)
	{
		printf("Error: No Device\n");
		printf("Usage: modbussend [device]\n");
		exit(0);
	}
	std::string device(argv[1]);
	cModbus ctx(device, 115200, 0, 8, 1);
	ctx.SetDebug(true);
	ctx.SetSlave(1);
	if (ctx.Connect() == -1)
		exit(0);
	
	printf("modbussend: send command or receive commands over modbus\n");
	printf("Usage: send [HEX] - Send [HEX] Command and receive response\n");
	printf("       receive    - Receive Request\n");
	
	std::string msg;
	std::vector<std::string> command;
	uint command_num;
	for (;;)
	{
		msg.clear();
		command.clear();
		command_num = 0;
		printf ("Modbus:> ");
		std::getline (std::cin,msg);
		if (msg.back() == '\n' || msg.back() == '\r')
		msg.pop_back();
	
	command = split(msg);
	command_num = command.size(); //number of strings in command
	
	
	if (command.at(0).compare("send") == 0)
	{
		if (command_num < 2)
		{
			
			printf("Error wrong Parameterlist\n");
		}
		else
		{
			uint8_t resp[MODBUS_RTU_MAX_ADU_LENGTH];
			int resp_lenght = 0;
			int i=0;
			std::vector<uint8_t> req;
			for (i = 0; i < command.at(1).length(); i += 2) 
			{
				uint8_t byte;
				std::string byteString = command.at(1).substr(i, 2);
				try
				{
					byte = (uint8_t) stoi(byteString, NULL, 16);  //covert string to HEX Value
				}
				catch (...)
				{
					printf("Error can not convert Input to HEX\n");
					continue;
				}
					
				req.push_back(byte);
			}
			
			
		printf("sending Request\n");
		resp_lenght = ctx.SendRequest(req.data(), req.size(), resp);
		if (resp_lenght>0)
		{
			printf("Response received: %d Bytes", resp_lenght);
			int n = 0;
			for (n=0; n<resp_lenght; n++)
			{
				printf("0x%02X, ", resp[n]);
			}
			printf("\n");
			resp_lenght = 0;
		}
			
			}
			
		}
	else if	(command.at(0).compare("receive") == 0)
	{
		uint8_t req[MODBUS_RTU_MAX_ADU_LENGTH];
		int req_lenght = 0;
		printf("Waiting for Request:\n");
		req_lenght = ctx.ReceiveRequest(req);
		if (req_lenght>0)
		{
			printf("Request received: %d Bytes", req_lenght);
			int n = 0;
			for (n=0; n<req_lenght; n++)
			{
				printf("0x%02X, ", req[n]);
			}
			printf("\n");
			req_lenght = 0;
		}
	}
	}
	
	return 0;
}

