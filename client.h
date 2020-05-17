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
#ifndef __CLIENT_H
#define __CLIENT_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <queue>
#include <ctime>
#include "threadwrapper/ThreadWrapper.h"
#include "mttp.h"


#define CONNECTTIMEOUT 100
#define MAXSENDQUEUE 50
#define MAXRECVQUEUE 50
class cClient : public ThreadWrapper
{
 private:
   struct SendMsgStruct {
    int lenght; 
    uint8_t data[MAX_SEND_BUFFER];
   };
   
   struct RecvMsgStruct {
    int lenght; 
    uint8_t data[MAX_RECV_BUFFER];
   }RecvMsg;
   
   std::queue<SendMsgStruct> SendQueue;
   std::queue<RecvMsgStruct> RecvQueue;
   
   struct sockaddr_in server;
   struct hostent *host_info;
   unsigned long addr;
   int sock;
   bool connected;
   bool needreconnect;
   time_t Now, LastServerMessage, LastConnecting;
   //uint8_t *data;
   //int datalen;
   cMTTP mttp;
   cMTTP mttpheartbeat;//(eHeartbeat);
   bool stillconnected();
   void closeconnection();
   int connect();
   int createsocket();
   void settimeout(bool reset);
   int send(const uint8_t *data, int len);
   int recv(uint8_t *data, int len);

 public:
   cClient(const char *serveradr, int port);
   int sendmessage(const uint8_t *data, int len);
   int recvmessage(uint8_t *data, int len);
   virtual ~cClient();
   
   
 protected:
   void Body() override;
};

#endif //_CLIENT_H
