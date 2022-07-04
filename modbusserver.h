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
#ifndef __MODBUSSERVER_H
#define __MODBUSSERVER_H
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

#define MAX_MODBUS_CLIENTS 20
#define MAX_BUFFER_SIZE  256
#define MAX_REG 100

class cModbusServer : public ThreadWrapper
{
 private:
   struct sockaddr_in address;
   int listen_sock;
   int clientsock[MAX_MODBUS_CLIENTS];
   int max_sd, new_socket;
   fd_set readfds;
   int addrlen, sd, valread;
   uint8_t buffer[MAX_BUFFER_SIZE];
   int process();
   void checkclients();
   cMemory memory;
 public:
   cModbusServer(int port);
   virtual ~cModbusServer();
 
   protected:
	void Body() override;
};




#endif //_MODBUSSERVER_H