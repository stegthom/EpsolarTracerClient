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
#ifndef __SERVER_H
#define __SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include "memory.h"
#include "mttp.h"
#include "common.h"

#define RECVBUFSIZE 8192
#define MAXDECODINGLOOPS 100 //if something wrong in recvbuffer to proteect the Server to hang infinite in the processing loop.
#define CONNECTTIMEOUT 100 //Timeout in seconds before a Still Alive Message is send.

class cServer
{
 private:
   struct sockaddr_in server, client;
//   struct hostent *host_info;
//   unsigned long addr;
   int sock1, sock2;
   uint8_t recvbuffer[RECVBUFSIZE];
   unsigned int len;
   cMTTP mttp;
   cMTTP mttpheartbeat;//(eHeartbeat);
   cMemory memory;
   bool connected;
   bool needreconnect;
   time_t Now, LastClientMessage;
   int waitclient();
   bool stillconnected();
   void closeconnection();
   void settimeout(bool reset);

 public:
   cServer(int port);
   ~cServer();
//   int connect();
//   int createsocket();
//   int closesocket();
   int send(int sock, const uint8_t *data, int len);
   int recv(int sock, uint8_t *data, int len);
   
   int action();
};

#endif //_SERVER_H
