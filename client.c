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
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "client.h"
#include "common.h"
cClient::cClient(const char *serveradr, int port)
{
   mttpheartbeat.InitBuffer(eHeartbeat);
   connected = false;
   settimeout(true);
   LastConnecting = 0;
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
               LOG("Error Unknwon Host: %s\n", strerror(errno));
            }

        memcpy ((char*)&server.sin_addr, host_info->h_addr, host_info->h_length);
     }

   server.sin_family = AF_INET;
   server.sin_port = htons(port);
}

cClient::~cClient()
{
   if (sock >= 0) {closeconnection();}
}


int cClient::connect()
{
   sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
     {
         LOG("Error Creating Socket: %s\n", strerror(errno));
         return -1;
     }
      const int y = 1;
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
   if (::connect(sock, (struct sockaddr*)&server, sizeof (server)) < 0)
     {
        LOG("Error Connect Socket: %s\n", strerror(errno));
		close(sock);
        return -1;
     }
   else
     {
		connected = true;
		settimeout(true);
        return 1;
     }
}

void cClient::settimeout(bool reset)
{
   if (!reset)
   {
		Now = time(NULL);
		if (Now - LastServerMessage > CONNECTTIMEOUT) 
		{
			if (!stillconnected())
			{
				needreconnect = true;
			}
			else
			{
				needreconnect = false;
				LastServerMessage = time(NULL);
			}
		}
   }
   else
   {
	   LastServerMessage = time(NULL);
	   needreconnect = false;
   }
}

void cClient::closeconnection()
{
	if (connected && (sock > -1))
	{
		close(sock);
		sock = -1;
		connected = false;
	}
}

bool cClient::stillconnected()
{
	if (!connected)
		return false;
	uint8_t sendbuffer[MAX_SEND_BUFFER];
   int sendbufferlen = mttpheartbeat.GetBuffer(sendbuffer, MAX_SEND_BUFFER);
   if (::send(sock, sendbuffer, sendbufferlen, 0) < 0)
       {
			LOG("cCerver::stillconnected: Error Sending Heartbeat Message to Client\n");
			return false;
       }
	   else
		   return true;
}

int cClient::send(const uint8_t *data, int len)
{
   int sendcnt = 0;
   int sendret = 0;
   int sendlen = len;
   while (sendcnt < len)
     {
        sendret = ::send(sock, &data[sendcnt], sendlen, 0);
	printf("%d Bytes send\n", sendret);
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

int cClient::recv(uint8_t *data, int len)
{
   int recvret = 0;
   recvret = ::recv(sock, data, len, 0);
   if (recvret == -1)
     {
        LOG("Error Receiving Data: %s\n", strerror(errno));
     }
   return recvret;
}

int cClient::sendmessage(const uint8_t *data, int len)
{
	if (len > MAX_SEND_BUFFER)
	{
		LOG("cClient::sendmessage: Error Message to large for buffer");
		return -1;
	}
	struct SendMsgStruct sendmsg;
	sendmsg.lenght = len;
	
	int n;
	for(n=0; n<len; n++)
	{
		sendmsg.data[n] = data[n];
	}
	if (SendQueue.size() >= MAXSENDQUEUE)
	{
		LOG("cClient::sendmessage: To much Messages in Send Queue Deleting old Message");
		SendQueue.pop();
	}
	SendQueue.push(sendmsg);
	return len;
		
}

int cClient::recvmessage(uint8_t *data, int len)
{
	if (RecvQueue.empty())
		return 0;
	else
	{
		if (RecvQueue.front().lenght > len)
		{
			LOG("cClient::recvmessage: Error Received Message to long for buffer");
			return -1;
		}
		int n;
		int lenght = RecvQueue.front().lenght;
		for (n=0; n<lenght; n++)
		{
			data[n] = RecvQueue.front().data[n];
		}
		RecvQueue.pop();
		return lenght;
	}
}

void cClient::Body()
{
	int rv;
	fd_set set;
	struct timeval timeout;
	
	while (true)
	{
		this->SyncPoint();
		settimeout(false);
		if (needreconnect && connected)
		{
			closeconnection();
			LOG("cClient::Body: Need Reconnect No Connectin to Client");
		}
		if (!connected)
		{
			Now = time(NULL);
		if (Now - LastConnecting < 30) //only dry every 30 seconds to connect
		{
			usleep(1000000);
			continue;
		}
			connect();
			LastConnecting = time(NULL);
		}
		if (connected)
		{
			while (!SendQueue.empty())
			{
				if ((send(SendQueue.front().data, SendQueue.front().lenght)) <= 0)
					break;
				else
				{
					SendQueue.pop();
					settimeout(true);
				}
				usleep(500000); //sleep for Server to process Data correct
			}
			
			FD_ZERO(&set);
			FD_SET(sock, &set);

			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			rv = select(sock + 1, &set, NULL, NULL, &timeout);
			if(rv == -1)
			{
				LOG("cClient::Body: Error on Select\n"); /* an error accured */
				continue;
			}
			else if(rv == 0)
			{
				//DEBUGLOG("cSERVER::action: Select() timeout occurred\n"); /* a timeout occured */
				//settimeout(false); // No Data check for Timout
				continue;
			}
			else
			{
				struct RecvMsgStruct recvmsg;
				int received = recv(recvmsg.data, MAX_RECV_BUFFER);
				if (received <= 0)
				{
					DEBUGLOG("cClient::Body: 0 Bytes Received: Client closed Connection Close Socket\n");
					closeconnection();
					continue;
				}
				recvmsg.lenght = received;
				if (RecvQueue.size() >= MAXRECVQUEUE)
				{
					LOG("cClient::Body: To much Messages in Receive Queue Deleting old Message");
					RecvQueue.pop();
				}
				RecvQueue.push(recvmsg);
				settimeout(true); //Data received Reset Timeout Timer
			}
		}	
	}
}
