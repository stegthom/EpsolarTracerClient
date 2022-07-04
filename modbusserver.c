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

#include "modbusserver.h"
#include "common.h"
#include "tracerctr.h"


cModbusServer::cModbusServer(int port)
{
	int i;
	for(i=0;i<MAX_MODBUS_CLIENTS;i++)
	{
		clientsock[i] = 0;
	}
   listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0)
     {
        LOG("Error Creating Server Socket: %s\n", strerror(errno));
        exit (0);
     }
   const int y = 1;
   setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

   memset (&address, 0, sizeof (address));

   address.sin_family = AF_INET;
   address.sin_addr.s_addr = htonl(INADDR_ANY);
   address.sin_port = htons(port);
   if(bind(listen_sock,(struct sockaddr*)&address, sizeof(address)) < 0)
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
cModbusServer::~cModbusServer()
{
	
   close (listen_sock);
   int i;
   for (i=0;i<MAX_MODBUS_CLIENTS;i++)
   {
	   if (clientsock[i] > 0)
	   {
		  close(clientsock[i]);
		  clientsock[i]=0;
	   }
   }
}


void cModbusServer::Body()
{
	int i;
	
	
	for(;;)
	{
		this->SyncPoint();
		
		
		int rv;
		struct timeval timeout;
		FD_ZERO(&readfds);
		FD_SET(listen_sock, &readfds);
		max_sd = listen_sock;
		
		//add child sockets to set 
		int i;
        for ( i = 0 ; i < MAX_MODBUS_CLIENTS ; i++)  
        {   
                 
            //if valid socket descriptor then add to read list 
            if(clientsock[i] > 0)  
                FD_SET( clientsock[i] , &readfds);  
                 
            //highest file descriptor number, need it for the select function 
            if(clientsock[i] > max_sd)  
                max_sd = clientsock[i];  
        }  

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		rv = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
		if(rv == -1)
		{
			LOG("cModbusServer::action: Error on Select\n"); /* an error accured */
			continue;
		}
		else if(rv == 0)
		{
		//   DEBUGLOG("cSERVER::action: Select() timeout occurred\n"); /* a timeout occured */
			//LOG("cMtcpClientHandle: Client Timeout Disconnecting\n");
			continue;
		}
		
		//If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(listen_sock, &readfds))  
        {  
            if ((new_socket = accept(listen_sock, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                continue;
            }  
             
			 //add new socket to array of sockets 
            for (i = 0; i < MAX_MODBUS_CLIENTS; i++)  
            {  
                //if position is empty 
                if( clientsock[i] == 0 )  
                {  
                    clientsock[i] = new_socket;  
                    DEBUGLOG("cModbusServer:Adding to list of sockets as %d\n" , i);  
                         
                    break;  
                }  
				else if (i == (MAX_MODBUS_CLIENTS-1))
				{
					checkclients();
					close(new_socket);
				}
            }  
        }

        //else its some IO operation on some other socket
        for (i = 0; i < MAX_MODBUS_CLIENTS; i++)  
        {  
            sd = clientsock[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, MAX_BUFFER_SIZE)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    DEBUGLOG("Modbusserver:Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    clientsock[i] = 0;  
					continue;
                }  	

			//Process Message 
                else 
                {  
                     if (process()<0)
					 {
						 DEBUGLOG("Modbusserver: Error Processing Request!");
					 }
                }  
            }  
		}
		
	}
}

int cModbusServer::process()

{
	//Request Parameters
	uint16_t id;
	uint16_t proto;
	uint16_t lenght;
	uint8_t unitid;
	uint8_t function;
	uint16_t address;
	uint16_t number;
	
	// Response Parameters
	uint16_t registers[MAX_REG];
	
	
	if (valread < 12)
		return -1;
	id = MODBUS_GET_INT16_FROM_INT8(buffer, 0);
	proto = MODBUS_GET_INT16_FROM_INT8(buffer, 2);
	lenght = MODBUS_GET_INT16_FROM_INT8(buffer, 4);
	unitid = buffer[6];
	function = buffer[7];
	address = MODBUS_GET_INT16_FROM_INT8(buffer, 8);
	number = MODBUS_GET_INT16_FROM_INT8(buffer, 10);
	
	uint8_t errorresp[9];
	MODBUS_SET_INT16_TO_INT8(errorresp, 0, id);
    MODBUS_SET_INT16_TO_INT8(errorresp, 2, proto);
	MODBUS_SET_INT16_TO_INT8(errorresp, 4, 3);
	errorresp[6] = unitid;
	errorresp[7] = (function+0x80);
	errorresp[8] = 0x02;
	
	if (function == 0x04 && number<=MAX_REG)
	{
		int result;
		result = memory.GetRegister(address, (address+(number-1)), registers, MAX_REG, true);
		if (result<0)
		{
			send(sd , errorresp , 9 , 0 );
			return -1;
		}
		uint8_t response[MAX_BUFFER_SIZE];
		uint16_t resplenght=0;
		MODBUS_SET_INT16_TO_INT8(response, 0, id);
		MODBUS_SET_INT16_TO_INT8(response, 2, proto);
		//MODBUS_SET_INT16_TO_INT8(response, 4, lenght);
		response[6]=unitid;
		resplenght++;
		response[7]=function;
		resplenght++;
		response[8]=(result*2); //byte Count 1 Reg == 2 Byte
		resplenght++;
		int index = 9;
		int i=0;
		for (i=0;i<result;i++)
		{
			MODBUS_SET_INT16_TO_INT8(response, index, registers[i]);
			index +=2;
			resplenght = resplenght+2;
		}
		MODBUS_SET_INT16_TO_INT8(response, 4, resplenght);
		send(sd , response , (resplenght+6) , 0 );
		printf("Modbusserver: Sending Response: ");
		int f=0;
		for (f=0;f<(resplenght+6);f++)
		{
			printf("<%02X>", response[f]);
		}
		printf("\n");
	}
	else
	{
		send(sd , errorresp , 9 , 0 );
		return -1;
	}
	return 1;
}	
void cModbusServer::checkclients()
{
	uint8_t buffer=0;
	int i=0;
	for (i = 0; i < MAX_MODBUS_CLIENTS; i++)  
            {  
		       if(clientsock[i] > 0)
			   {
				   if (send(clientsock[i], &buffer, 1, 0) < 0)
				   {
					   getpeername(clientsock[i] , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    DEBUGLOG("Modbusserver:Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
					   close(clientsock[i]);
					   clientsock[i]=0;
					    
				   }
			   }
			}
	
}