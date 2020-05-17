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

#include "server.h"
#include "common.h"
#include "mttp.h"
cServer::cServer(int port)
{
   //mttp = new cMTTP(eNone);
   mttpheartbeat.InitBuffer(eHeartbeat);
   connected = false;
   settimeout(true);
   sock1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock1 < 0)
     {
        LOG("Error Creating Server Socket: %s\n", strerror(errno));
        exit (0);
     }
   const int y = 1;
   setsockopt(sock1, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

   memset (&server, 0, sizeof (server));

   server.sin_family = AF_INET;
   server.sin_addr.s_addr = htonl(INADDR_ANY);
   server.sin_port = htons(port);
   if(bind(sock1,(struct sockaddr*)&server, sizeof( server)) < 0)
     {
        LOG("Error Bind Server Socket to port %s\n", strerror(errno));
        exit (0);
     }
   if(listen(sock1, 5) == -1 )
     {
        LOG("Error Listen on Server Socket: %s\n", strerror(errno));
        exit (0);
     }
}
cServer::~cServer()
{
   closeconnection();
   close (sock1);
}


int cServer::waitclient()
{
	int rv;
	fd_set set;
	struct timeval timeout;
	FD_ZERO(&set);
	FD_SET(sock1, &set);

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	rv = select(sock1 + 1, &set, NULL, NULL, &timeout);
	if(rv == -1)
	{
	    LOG("Error on Select\n"); /* an error accured */
	    return -1;
	}
	else if(rv == 0)
	{
	    //DEBUGLOG("cSERVER::action: Select() timeout occurred\n"); /* a timeout occured */
	    return 0;
	}
	else
	{
		len = sizeof(client);
        sock2 = accept(sock1, (struct sockaddr*)&client, &len);
        if (sock2 < 0)
          {
             LOG("Error Socket accept: %s\n", strerror(errno));
          }
		connected = true;
		settimeout(true);
		return 1;
	}
}

bool cServer::stillconnected()
{
	if (!connected)
		return false;
	uint8_t sendbuffer[MAX_SEND_BUFFER];
   int sendbufferlen = mttpheartbeat.GetBuffer(sendbuffer, MAX_SEND_BUFFER);
   if (send(sock2, sendbuffer, sendbufferlen) < 0)
       {
			LOG("cServer::stillconnected: Error Sending Heartbeat Message to Client\n");
			return false;
       }
	   else
		   return true;
}

void cServer::closeconnection()
{
	if (connected && (sock2 > -1))
	{
		close(sock2);
		sock2 = -1;
		connected = false;
	}
}


void cServer::settimeout(bool reset)
{
   if (!reset)
   {
		Now = time(NULL);
		if (Now - LastClientMessage > CONNECTTIMEOUT) 
		{
			if (!stillconnected())
			{
				needreconnect = true;
			}
			else
			{
				needreconnect = false;
				LastClientMessage = time(NULL);
			}
		}
   }
   else
   {
	   LastClientMessage = time(NULL);
	   needreconnect = false;
   }
}
int cServer::send(int sock, const uint8_t *data, int len)
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

int cServer::recv(int sock, uint8_t *data, int len)
{
   int recvret = 0;
   recvret = ::recv(sock, data, len, 0);
   if (recvret == -1)
     {
        LOG("Error Receiving Data: %s\n", strerror(errno));
     }
   return recvret;
}

int cServer::action()
{
  if (needreconnect && connected)
  {
	  LOG("cServer::action: Need Reconnect No Connectin to Client\n");
	  closeconnection();
  }
  
  if (!connected)
  {
	  if (waitclient() < 1)
	  {
		  return 0;
	  }
  }
  if (connected)
  {
	 // LOG("I am Connected!\n");
    int rv;
	fd_set set;
	struct timeval timeout;
	FD_ZERO(&set);
	FD_SET(sock2, &set);

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	rv = select(sock2 + 1, &set, NULL, NULL, &timeout);
	if(rv == -1)
	{
	    LOG("cServer::action: Error on Select\n"); /* an error accured */
	    return -1;
	}
	else if(rv == 0)
	{
	 //   DEBUGLOG("cSERVER::action: Select() timeout occurred\n"); /* a timeout occured */
		settimeout(false); // No Data check for Timout
	    return 0;
	}
	else
	{
 //       len = sizeof(client);
 //       sock2 = accept(sock1, (struct sockaddr*)&client, &len);
 //       if (sock2 < 0)
 //         {
 //            LOG("Error Socket accept: %s\n", strerror(errno));
 //         }
		
		
        int received = recv(sock2, recvbuffer, RECVBUFSIZE);
		if (received <= 0)
		{
			DEBUGLOG("cServer::action: 0 Bytes Received: Client closed Connection Close Socket\n");
			closeconnection();
			return 0;
		}
		settimeout(true); //Data received Reset Timeout Timer
		int processed = 0;
		data memtype;
		uint16_t startadress;
		uint8_t command[COMMAND_SIZE];
		int commandret;
		int saveret;
        DEBUGLOG("Daten empfangen: %d\n", received);
        int n = 0, m = 0;
        for (n = 0; n<MAXDECODINGLOOPS; n++)
          {
             if (processed < received)
			 {
				 processed = mttp.AddMessage((recvbuffer + processed), (received - processed));
				 DEBUGLOG("cMTTP: %d bytes processed\n", processed);
				 for (m = 0; m < MAX_COMMANDS; m++)
				 {
					 memtype = eNotSet;
					 startadress = 0;
					 commandret = 0;
					 std::memset(command, 0, COMMAND_SIZE);
					 commandret = mttp.GetCommand(&memtype, &startadress, command, COMMAND_SIZE, m);
					 if (commandret < 0)
						 break;
					 else if (commandret == 0)
						 continue;
					 else
					 {
						 saveret = memory.Save(memtype, startadress, commandret, command);
						 if (saveret < 0)
						 {
						     LOG("cServer::action: Error Save Registers\n");
						 }
						 else
						 {
							 DEBUGLOG("cServer::action: %d Registers Saved\n", saveret);
						 }
					 }
				 }
			 }
			 else
			 {
				 break;
			 }
					 
					 
          }
	

//        printf("\n");
//        close(sock2);
	}
  }
  return 1;
}

