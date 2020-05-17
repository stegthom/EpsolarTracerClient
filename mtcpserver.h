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
#ifndef __MTCPSERVER_H
#define __MTCPSERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include "memory.h"
#include "common.h"
#include <string>
#include <vector>
#include "threadwrapper/ThreadWrapper.h"

#define RECVBUFSIZE 8192

//#define MAXDECODINGLOOPS 100 //if something wrong in recvbuffer to proteect the Server to hang infinite in the processing loop.
#define CONNECTTIMEOUT 100
#define MAX_CLIENTS 100

class cMtcpClient
{
  private:
   struct sockaddr_in server;
   struct hostent *host_info;
   unsigned long addr;
   int sock;
   bool connected;
   int createsocket();
   
 public:
   int connect();
   void closeconnection();
   int send(const char *message, int len);
   int recv(char *recvmessage, int len);
   cMtcpClient(const char *serveradr, int port);
   ~cMtcpClient();
};

class cMtcpClientHandle : public ThreadWrapper
{
  private:
    std::string client_addr;
	int client_port;
	int client_socket;
	char recvbuffer[RECVBUFSIZE];
	cMemory memory;
	void closeconnection();
	std::string handlecommand(int lenght, char* req); //handle command with int lenght and returns an Answer string
	std::string PrintHelp();
	std::vector<std::string> split(const std::string& input, const std::string& separator = " ");
	cMtcpClient *mtcpclient;
	std::string forwardserver;
	int forwardport;
   public:
	cMtcpClientHandle(int socket, std::string addr, int port, std::string forwardserv, int forwardp);
	virtual ~cMtcpClientHandle();
	bool connected();
	int send(int sock, const char *message, int len);
    int recv(int sock, char *message, int len);
	
  protected:
    void Body() override;
};	



class cMtcpServer : public ThreadWrapper
{
	friend class cMtcpClientHandle;
 private:
   cMtcpClientHandle *client[MAX_CLIENTS];
   struct sockaddr_in server_addr, client_addr;
   int listen_sock;
   int sock2;
   unsigned int len;
   std::string forwardserver;
   int forwardport;
   
 public:
   cMtcpServer(int port, std::string forwardserv = "", int forwardp=0);
   virtual ~cMtcpServer();
 
   protected:
	void Body() override;
};




#endif //_MTCPSERVER_H
