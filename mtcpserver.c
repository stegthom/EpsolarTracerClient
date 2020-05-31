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
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "mtcpserver.h"
#include "common.h"
#include "tracerctr.h"


cMtcpServer::cMtcpServer(int port, std::string forwardserv, int forwardp)
{
	forwardserver = forwardserv;
	forwardport = forwardp;
	
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
	{
		client[i] = NULL;
	}
   listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0)
     {
        LOG("Error Creating Server Socket: %s\n", strerror(errno));
        exit (0);
     }
   const int y = 1;
   setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

   memset (&server_addr, 0, sizeof (server_addr));

   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons(port);
   if(bind(listen_sock,(struct sockaddr*)&server_addr, sizeof( server_addr)) < 0)
     {
        LOG("Error Bind Server Socket to port %s\n", strerror(errno));
        exit (0);
     }
   if(listen(listen_sock, 5) == -1 )
     {
        LOG("Error Listen on Server Socket: %s\n", strerror(errno));
        exit (0);
     }
}
cMtcpServer::~cMtcpServer()
{
	
   close (listen_sock);
   int i;
   for (i=0;i<MAX_CLIENTS;i++)
   {
	   if (client[i])
	   {
		   client[i]->Abort();
		   client[i]->Join();
		   delete client[i];
		   client[i] = NULL;
	   }
   }
}


void cMtcpServer::Body()
{
	int i;
	
	
	for(;;)
	{
		this->SyncPoint();
		for(i=0;i<MAX_CLIENTS;i++)
		{
			if (client[i])
			{
				if (!client[i]->connected())
				{
					client[i]->Abort();
					client[i]->Join();
					delete client[i];
					client[i] = NULL;
				}
			}
		}
		
		int rv;
		fd_set set;
		struct timeval timeout;
		FD_ZERO(&set);
		FD_SET(listen_sock, &set);

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		rv = select(listen_sock + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
		{
			LOG("cMtcpServer::action: Error on Select\n"); /* an error accured */
			continue;
		}
		else if(rv == 0)
		{
		//   DEBUGLOG("cSERVER::action: Select() timeout occurred\n"); /* a timeout occured */
			//LOG("cMtcpClientHandle: Client Timeout Disconnecting\n");
			continue;
		}
		else
		{
		len = sizeof(client_addr);
        sock2 = accept(listen_sock, (struct sockaddr*)&client_addr, &len);
        if (sock2 < 0)
          {
             LOG("cMtcpServer: Error Socket accept: %s\n", strerror(errno));
          }
		else
		{
			//DEBUGLOG("cMtcpServer: New Client acceptet\n");
			for(i=0;i<MAX_CLIENTS;i++)
			{
				if (!client[i])
				{
					DEBUGLOG("cMtcpServer: New Client Connected to Server Slot %d\n", i);
					char client_ipv4_str[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);
					LOG("cMtcpServer: Incoming connection from %s:%d.\n", client_ipv4_str, client_addr.sin_port);
					std::string addr(client_ipv4_str);
					client[i] = new cMtcpClientHandle(sock2, addr, client_addr.sin_port, forwardserver, forwardport);
					client[i]->Start();
					break;
				}
				else if (i == (MAX_CLIENTS - 1))
				{
					LOG("cMtcpServer: Error To Much Connections to Server droping it.\n");
					close (sock2);
				}
			}
		}
		}
	}
}

cMtcpClientHandle::cMtcpClientHandle(int socket, std::string addr, int port, std::string forwardserv, int forwardp)
{
	mtcpclient = NULL;
	forwardserver = forwardserv;
	forwardport = forwardp;
	if(!forwardserver.empty() && forwardport > 0 && !mtcpclient)
	{
		mtcpclient = new cMtcpClient(forwardserver.c_str(), forwardport);
	}
	client_socket = -1;
	client_socket = socket;
	client_addr = addr;
	client_port = port;
}

cMtcpClientHandle::~cMtcpClientHandle()
{
	if(mtcpclient)
		delete mtcpclient;
	
	closeconnection();
}

void cMtcpClientHandle::closeconnection()
{
	if (client_socket > -1)
	{
		close (client_socket);
		client_socket = -1;
	}
}

bool cMtcpClientHandle::connected()
{
	if (client_socket > -1)
		return true;
	else 
		return false;
}




int cMtcpClientHandle::send(int sock, const char *data, int len)
{
   int sendcnt = 0;
   int sendret = 0;
   int sendlen = len;
   while (sendcnt < len)
     {
        sendret = ::send(sock, &data[sendcnt], sendlen, 0);
	//printf("%d Bytes send\n", sendret);
        if (sendret <= 0)
          {
             LOG("Error sending Data: %s\n", strerror(errno));
             return -1;
          }
        sendcnt = sendcnt + sendret;
        sendlen = sendlen - sendret;
     }
   return sendcnt;
}

int cMtcpClientHandle::recv(int sock, char *data, int len)
{
   int recvret = 0;
   recvret = ::recv(sock, data, len, 0);
   if (recvret == -1)
     {
        LOG("Error Receiving Data: %s\n", strerror(errno));
     }
   return recvret;
}

void cMtcpClientHandle::Body()
{
	for(;;)
	{
		this->SyncPoint();
		if (connected())
		{
		int rv;
		fd_set set;
		struct timeval timeout;
		FD_ZERO(&set);
		FD_SET(client_socket, &set);

		timeout.tv_sec = CONNECTTIMEOUT;
		timeout.tv_usec = 0;
		rv = select(client_socket + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
		{
			LOG("cServer::action: Error on Select\n"); /* an error accured */
			continue;
		}
		else if(rv == 0)
		{
		//   DEBUGLOG("cSERVER::action: Select() timeout occurred\n"); /* a timeout occured */
			LOG("cMtcpClientHandle: Client Timeout Disconnecting\n");
			closeconnection();
		}
		else
		{
			int received = recv(client_socket, recvbuffer, RECVBUFSIZE);
			if (received <= 0)
			{
				LOG("cMtcpClientHandle: 0 Bytes Received: Client closed Connection Close Socket\n");
				closeconnection();
				break;
			}
			else
			{
				std::string response = cMtcpClientHandle::handlecommand(received, recvbuffer);
				int sendret = send(client_socket, response.c_str(), response.size());
				if (sendret < 0)
					LOG("cMtcpClientHandle: Error 0 bytes send to client\n");
				
			}
		}
		}
	}
}

std::string cMtcpClientHandle::handlecommand(int lenght, char* req)
{
	//cTracerCtr *tracerctr = NULL;
	char recvmessage[RECVBUFSIZE];
	std::string recvstring(req, lenght);
	if (recvstring.back() == '\n' || recvstring.back() == '\r')
		recvstring.pop_back();
	//recvbuffer[received]='\0';
	LOG("Received Message from Client %s: %s\n",client_addr.c_str(),recvstring.c_str());
	std::vector<std::string> command;
	command = split(recvstring);
	uint command_num = command.size(); //number of strings in command
	std::string confirmation = "Not Set\n";
	int i;
	for(i=0; i<command_num;i++)
	{
		DEBUGLOG("Command %d: %s\n", i, command.at(i).c_str());
	}
//-----------------------------------------------------------------------------------------------------------------------------------	
//getreg: Return Value of Register Adress
//getreg32: Return Value of 2 Registers starting at Adress (Usefull for Values saved in 2 Registers
//setreg: Save Value as alternate in Register.
//setreg32: Save Value as alternate in 2 Register starting at Adress.
//getchargemode:  Return Charging Mode (NO, Float, Boost, Equlization) as String
//debumem: DebugOutput of Memory
//cleanmem: clean the complete Memory
//cleanalternate: clean only memory entrys set by user with setreg(32)
//getreglist: Get the Registerlist from Memory
//triggerserverrealtimeupdate
//triggerserverstatisticupdate
//triggertracerrealtimeupdate
//triggertracerstatisticupdate
//writeholdingregister
//modbussend: Send a raw Modbus Message to Tracer (Function 0x05 and 0x10 Triggers a complete Data Update on Client and Server)
//switch: Switch one of the switches defined in memmory.conf on or off
//setcoil: Set UserCoil
//getcoil: Get UserCoil
//help: Print the Commandlist
//-----------------------------------------------------------------------------------------------------------------------------------

//---getreg    Return Register Value of 16 bit Registers

if (command.at(0).compare("getreg") == 0)
	{
		if (command_num < 2)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			bool orig = false;
			if (command_num > 2)
			{
				if ((command.at(2).compare("true") == 0) || (command.at(2).compare("1") == 0))
				{
					orig = true;
				}
			}
			uint16_t adress;
			try
			{
				adress	= std::stoi(command.at(1), NULL, 16);
			}
			catch (...)
			{
				LOG("cCtrPipe: error received adress\n");
				adress = 0;
			}
			
			confirmation.clear();
			confirmation = memory.GetRegStr (adress, orig);
		}
	}
	
//getcoil     Return Coil Value
	
	else if (command.at(0).compare("getcoil") == 0)
	{
		if (command_num < 2)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			uint16_t adress;
			try
			{
				adress	= std::stoi(command.at(1), NULL, 16);
			}
			catch (...)
			{
				LOG("cCtrPipe: error received adress\n");
				adress = 0;
			}
			
			confirmation.clear();
			confirmation = memory.GetCoilStr (adress);
		}
	}
	
//---getreg32	 Return Register Value of 32 bit Register
	
	else if (command.at(0).compare("getreg32") == 0)
	{
		if (command_num < 2)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			bool orig = false;
			if (command_num > 2)
			{
				if ((command.at(2).compare("true") == 0) || (command.at(2).compare("1") == 0))
				{
					orig = true;
				}
			}
			uint16_t adress;
			try
			{
				adress	= std::stoi(command.at(1), NULL, 16);
			}
			catch (...)
			{
				LOG("cCtrPipe: error received adress\n");
				adress = 0;
			}
			confirmation.clear();
			confirmation = memory.GetRegStr32 (adress, orig);
		}
	}

//---setreg          Set Value of Register as alternate value
	
	else if (command.at(0).compare("setreg") == 0)
	{
		if (command_num < 3)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			uint16_t adress;
			try
			{
				adress	= std::stoi(command.at(1), NULL, 16);
			}
			catch (...)
			{
				LOG("cMtcpServer: error received adress\n");
				adress = 0;
			}
			confirmation.clear();
			if (memory.PutRegStr (command.at(2), adress) == -1)
			{
				confirmation = "Error\n";
			}
			else
			{
			confirmation = "OK\n";
			}
		}	
	}
	
//setcoil        Set Value of USERCOIL	
	else if (command.at(0).compare("setcoil") == 0)
	{
		if (command_num < 3)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			uint16_t adress;
			try
			{
				adress	= std::stoi(command.at(1), NULL, 16);
			}
			catch (...)
			{
				LOG("cMtcpServer: error received adress\n");
				adress = 0;
			}
			confirmation.clear();
			if (memory.PutCoilStr (command.at(2), adress) == -1)
			{
				confirmation = "Error\n";
			}
			else
			{
			confirmation = "OK\n";
			}
		}	
	}
	
//---setreg32        set Register Value of 32 bit registers
	else if (command.at(0).compare("setreg32") == 0)
	{
		if (command_num < 3)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			uint16_t adress;
			try
			{
				adress	= std::stoi(command.at(1), NULL, 16);
			}
			catch (...)
			{
				LOG("cMtcpServer: error received adress\n");
				adress = 0;
			}
			confirmation.clear();
			if (memory.PutRegStr32 (command.at(2), adress) == -1)
			{
				confirmation = "Error\n";
			}
			else
			{
				confirmation = "OK\n";
			}
		}
	}
	
//--- getchargemode--- Register 3201 - Bit D3 - D2
	
	else if (command.at(0).compare("getchargemode") == 0)
	{
		uint16_t adress = 0x3201;
		uint16_t reg;
		uint16_t result;
		const uint16_t masq = 12;  //b00001100   Masq to set all bit except bit 2 and 3 to 0
		memory.GetRegister(adress, adress, &reg, 1, true);
		result = (reg & masq); //set all bits except bit 2, 3
		result = (result >> 2);
		confirmation.clear();
		if (result == 0)
			confirmation = "No Charging\n";
		else if (result == 1)
			confirmation = "Float Charging\n";
		else if (result == 2)
			confirmation = "Boost Charging\n";
		else if  (result == 3)
			confirmation = "Equlization Charging\n";
		
		
	}
	
//---debugmem
	
	else if (command.at(0).compare("debugmem") == 0)
	{
		confirmation.clear();
		confirmation = memory.DebugLogMemory();
		
	}
	
//----cleanmem
	
	else if (command.at(0).compare("cleanmem") == 0)
	{
		confirmation.clear();
		memory.clean();
		confirmation = "OK\n";
		
	}
	
//---cleanalternate
	
	
	else if (command.at(0).compare("cleanalternate") == 0)
	{
		confirmation.clear();
		memory.cleanalternate();
		confirmation = "OK\n";
		
	}
	
//---getreglist
	
	
	else if (command.at(0).compare("getreglist") == 0)
	{
		confirmation.clear();
		confirmation = memory.GetRegListStr();
		
	}

//---triggerserverrealtimeupdate

	else if (command.at(0).compare("triggerserverrealtimeupdate") == 0)
	{
		if (cTracerCtr::IsTracerConnected())
		{
			cTracerCtr::TriggerServerRealtimeupdate();
			confirmation.clear();
			confirmation = "OK\n";
		}
		else
		{
			if(mtcpclient)
			{
				if (mtcpclient->connect() == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Connection Error\n";
					return confirmation;
				}
				if (mtcpclient->send(req, lenght) == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Send Error\n";
					return confirmation;
				}
				int recvlenght = mtcpclient->recv(recvmessage, RECVBUFSIZE);
				confirmation.clear();
				confirmation.assign(recvmessage, recvlenght);
				mtcpclient->closeconnection();
			}
			else
			{
				confirmation.clear();
				confirmation = "Tracer not locally connected\n";
			}
		}
	}
	
//---triggerserverstatisticupdate

	else if (command.at(0).compare("triggerserverstatisticupdate") == 0)
	{
		if (cTracerCtr::IsTracerConnected())
		{
			cTracerCtr::TriggerServerStatisticupdate();
			confirmation.clear();
			confirmation = "OK\n";
		}
		else
		{
			if(mtcpclient)
			{
				if (mtcpclient->connect() == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Connection Error\n";
					return confirmation;
				}
				if (mtcpclient->send(req, lenght) == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Send Error\n";
					return confirmation;
				}
				int recvlenght = mtcpclient->recv(recvmessage, RECVBUFSIZE);
				confirmation.clear();
				confirmation.assign(recvmessage, recvlenght);
				mtcpclient->closeconnection();
			}
			else
			{
				confirmation.clear();
				confirmation = "Tracer not locally connected\n";
			}
		}
	}
	
//---triggertracerrealtimeupdate

	else if (command.at(0).compare("triggertracerrealtimeupdate") == 0)
	{
		if (cTracerCtr::IsTracerConnected())
		{
			cTracerCtr::TriggerTracerRealtimeupdate();
			confirmation.clear();
			confirmation = "OK\n";
		}
		else
		{
			if(mtcpclient)
			{
				if (mtcpclient->connect() == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Connection Error\n";
					return confirmation;
				}
				if (mtcpclient->send(req, lenght) == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Send Error\n";
					return confirmation;
				}
				int recvlenght = mtcpclient->recv(recvmessage, RECVBUFSIZE);
				confirmation.clear();
				confirmation.assign(recvmessage, recvlenght);
				mtcpclient->closeconnection();
			}
			else
			{
				confirmation.clear();
				confirmation = "Tracer not locally connected\n";
			}
		}
	}
	
//---triggertracerstatisticupdate

	else if (command.at(0).compare("triggertracerstatisticupdate") == 0)
	{
		if (cTracerCtr::IsTracerConnected())
		{
			cTracerCtr::TriggerTracerStatisticupdate();
			confirmation.clear();
			confirmation = "OK\n";
		}
		else
		{
			if(mtcpclient)
			{
				if (mtcpclient->connect() == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Connection Error\n";
					return confirmation;
				}
				if (mtcpclient->send(req, lenght) == -1)
				{
					confirmation.clear();
					confirmation = "cMtcpClient: Error Forward Message to Client: Send Error\n";
					return confirmation;
				}
				int recvlenght = mtcpclient->recv(recvmessage, RECVBUFSIZE);
				confirmation.clear();
				confirmation.assign(recvmessage, recvlenght);
				mtcpclient->closeconnection();
			}
			else
			{
				confirmation.clear();
				confirmation = "Tracer not locally connected\n";
			}
		}
	}

	
//---modbussend

else if (command.at(0).compare("modbussend") == 0)
	{
		if (command_num < 2)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			
			if (cTracerCtr::IsTracerConnected())
			{
				cTracerCtr *tracerctr = NULL;
				tracerctr = cTracerCtr::GetInstance();
				if (tracerctr)
				{
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
							LOG("cMtcpServer: error can not convert Input to HEX\n");
							confirmation.clear();
							confirmation = "Error can not convert Input to HEX\n";
							return confirmation;
						}
					
						req.push_back(byte);
					}
					DEBUGLOG("Modbus Command received:\n");
					for (i = 0; i < req.size(); i++)
					{
						printf("%02X", req[i]);
					}
					printf("\n");
					uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
					int rc = 0;
					rc = tracerctr->modbus_send_request(req.data(), req.size(), rsp);
					char buffer[10];
					confirmation.clear();
					if (rc < 0)
					{
						confirmation = "Error wrong Modbus Request\n";
						return confirmation;
					}
					else
					{
						for (i = 0; i < rc; i++)
						{
							snprintf(buffer, 10, "<%02X>", rsp[i]);
							confirmation.append(buffer);
						}
						confirmation.append(1, '\n');
					}
				}
				else
				{
					confirmation.clear();
					confirmation = "Error No Instance of cTracerctr received\n";
				}
			}
			else
			{
				if(mtcpclient)
				{
					if (mtcpclient->connect() == -1)
					{
						confirmation.clear();
						confirmation = "cMtcpClient: Error Forward Message to Client: Connection Error\n";
						return confirmation;
					}
					if (mtcpclient->send(req, lenght) == -1)
					{
						confirmation.clear();
						confirmation = "cMtcpClient: Error Forward Message to Client: Send Error\n";
						return confirmation;
					}
					int recvlenght = mtcpclient->recv(recvmessage, RECVBUFSIZE);
					confirmation.clear();
					confirmation.assign(recvmessage, recvlenght);
					mtcpclient->closeconnection();
				}
				else
				{
					confirmation.clear();
					confirmation = "Tracer not locally connected\n";
				}
			}
		}
	}
	
//switch	
	else if (command.at(0).compare("switch") == 0)
	{
		if (command_num < 3)
		{
			confirmation.clear();
			confirmation = "Error wrong Parameterlist\n";
		}
		else
		{
			
			if (cTracerCtr::IsTracerConnected())
			{
				int Switch = -1;
				
				if ((command.at(2).compare("on") == 0) || (command.at(2).compare("1") == 0))
					Switch = 1;
				else if ((command.at(2).compare("off")) == 0 || (command.at(2).compare("0")) == 0)
					Switch = 0;
				else
				{
					confirmation.clear();
					confirmation = "Error wrong Parameter\n";
					return confirmation;
				}
				
				int switchret = memory.Switch(command.at(1), Switch);
				if (switchret == 1)
				{
					confirmation.clear();
					confirmation = "OK\n";
				}
				else
				{
					confirmation.clear();
					confirmation = "Error Switching\n";
				}			
			}
			else
			{
				if(mtcpclient)
				{
					if (mtcpclient->connect() == -1)
					{
						confirmation.clear();
						confirmation = "cMtcpClient: Error Forward Message to Client: Connection Error\n";
						return confirmation;
					}
					if (mtcpclient->send(req, lenght) == -1)
					{
						confirmation.clear();
						confirmation = "cMtcpClient: Error Forward Message to Client: Send Error\n";
						return confirmation;
					}
					int recvlenght = mtcpclient->recv(recvmessage, RECVBUFSIZE);
					confirmation.clear();
					confirmation.assign(recvmessage, recvlenght);
					mtcpclient->closeconnection();
				}
				else
				{
					confirmation.clear();
					confirmation = "Tracer not locally connected\n";
				}
			}
		}
	}
	
//---help
	
	
	else if (command.at(0).compare("help") == 0)
	{
		confirmation.clear();
		confirmation = PrintHelp();
	}
	
	else
	{
		LOG("cMtcpServer: Error Unknown Command\n");
		confirmation.clear();
		confirmation.append("Unknown Command use HELP for Commandlist\n");
	}
	return confirmation;	
}


std::string cMtcpClientHandle::PrintHelp()
{
	std::string confirmation;
	confirmation.clear();
	confirmation.append("Tracer mtcp Server Usage:\n");
	confirmation.append("[COMMAND] [ADRESS] [VALUE]\n");
	confirmation.append("Commandlist:\n");
	confirmation.append("getreg [Adress] [bool orig]    - Get Register Value optional: if [bool orig] is true or 1 not the alternate Value is returned\n");
	confirmation.append("getreg32 [Adress] [bool orig]  - Get Register Value for 32bit Registers. Adress must point to the L Byte and the next one must be the H Byte. optional: if [bool orig] is true or 1 not the alternate Value is returned\n");
	confirmation.append("setreg [Adress] [Value]        - Set Register Value\n");
	confirmation.append("setreg32 [Adress] [Value]      - Set Register Value for 32bit Registers. Adress must point to the L Byte and the next one must be the H Byte.\n");
	confirmation.append("getchargemode                  - Return Charging Mode (NO, Float, Boost, Equlization) as String\n");
	confirmation.append("debugmem                       - Print Output of the internal Memory Structur\n");
	confirmation.append("cleanmem                       - Clean all Values of the internal Memory Structur\n");
	confirmation.append("cleanalternate                 - Clean only User Values added by SETREG or SETREG32\n");
	confirmation.append("getreglist                     - Get Complete List of al known Registers including there Names\n");
	confirmation.append("triggerserverrealtimeupdate    - Send all Realtime Data to Server\n");
	confirmation.append("triggerserverstatisticupdate   - Send all Statistic Data to Server\n");
	confirmation.append("triggertracerrealtimeupdate    - Receive all Realtime Data from Tracer\n");
	confirmation.append("triggertracerstatisticupdate   - Receive all Statistic Data from Tracer\n");
	confirmation.append("modbussend                     - Send a raw Modbus Message to Tracer (Function 0x05 and 0x10 Triggers a complete Data update\n");
	confirmation.append("switch                         - Switch one of the switches defined in memmory.conf on or off (1 or 0)\n");
	confirmation.append("getcoil [Adress]               - Get UserCoil Value\n");
	confirmation.append("setcoil [Adress] [Value]       - Set UserCoil Value\n");
	confirmation.append("help                           - Print this Usage\n");
	return confirmation;
}	

std::vector<std::string> cMtcpClientHandle::split(const std::string& input, const std::string& separator)
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

//------------cMtcpClient


cMtcpClient::cMtcpClient(const char *serveradr, int port)
{
   connected = false;
   sock = -1;
   memset (&server, 0, sizeof (server));

   if ((addr = inet_addr( serveradr)) != INADDR_NONE)
     {
        memcpy((char*) &server.sin_addr, &addr, sizeof(addr));
     }
   else
     {
        host_info = gethostbyname(serveradr);

        if (NULL == host_info)
            {
               LOG("cMtcpClient: Error Unknwon Host: %s\n", strerror(errno));
            }

        memcpy ((char*)&server.sin_addr, host_info->h_addr, host_info->h_length);
     }

   server.sin_family = AF_INET;
   server.sin_port = htons(port);
}

cMtcpClient::~cMtcpClient()
{
   if (sock >= 0) {closeconnection();}
}


int cMtcpClient::connect()
{
   sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
     {
         LOG("cMtcpClient: Error Creating Socket: %s\n", strerror(errno));
         return -1;
     }
      const int y = 1;
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
   if (::connect(sock, (struct sockaddr*)&server, sizeof (server)) < 0)
     {
        LOG("cMtcpClient: Error Connect Socket: %s\n", strerror(errno));
        return -1;
     }
   else
     {
		connected = true;
        return 1;
     }
}

void cMtcpClient::closeconnection()
{
	if (connected && (sock > -1))
	{
		close(sock);
		sock = -1;
		connected = false;
	}
}


int cMtcpClient::send(const char *message, int len)
{
   int sendcnt = 0;
   int sendret = 0;
   int sendlen = len;
   while (sendcnt < len)
     {
        sendret = ::send(sock, &message[sendcnt], sendlen, 0);
	//printf("%d Bytes send\n", sendret);
        if (sendret <= 0)
          {
             LOG("cMtcpClient: Error sending Data: %s\n", strerror(errno));
             return -1;
          }
        sendcnt = sendcnt + sendret;
        sendlen = sendlen - sendret;
     }
   return sendcnt;
}

int cMtcpClient::recv(char *recvmessage, int len)
{
   int recvret = 0;
   recvret = ::recv(sock, recvmessage, len, 0);
   if (recvret == -1)
     {
        LOG("cMtcpClient: Error Receiving Data: %s\n", strerror(errno));
     }
   return recvret;
}