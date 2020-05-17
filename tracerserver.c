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
#define TRACERSERVERVERSION "v0.1"

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <ctime>
#include <string>
#include <string.h>
#include <csignal>
#include "memory.h"
#include "common.h"
#include "server.h"
#include "mttp.h"
#include "display.h"
#include "mtcpserver.h"

cMemory memory;
cServer *server = NULL;
cDisplay *display = NULL;
cMtcpServer *mtcpserver = NULL;
cMtcpClient *mtcpclient = NULL;

static bool KillMe = false;

static void SignalHandler(int signum)
{
   KillMe = true;
}

int main( int argc, char* argv[] )
{
	memory.clean();
	std::string displaydev;
	int serverport = 0;
	int mtcpserverport = 0;
	bool daemonmode = false;
    bool displayhelp = false;
    bool displayversion = false;
	std::string mtcpforwardip;
	int mtcpforwardport = 0;
	std::string confdir;
	
	static struct option long_options[] = {
      { "displaydev", 		required_argument, NULL, 'D' },
      { "serverport", 		required_argument, NULL, 'p' },
	  { "mtcpport",   		required_argument, NULL, 'm' },
	  { "mtcpforwardip",   	required_argument, NULL, 'i' },
	  { "mtcpforwardport",  required_argument, NULL, 'x' },
	  { "config", 		    required_argument, NULL, 'c' },
	  { "daemon",     		no_argument,       NULL, 'd' },
      { "help",       		no_argument,       NULL, 'h' },
      { "version",    		no_argument,       NULL, 'v' },
      { NULL }
      };

int c;
while ((c = getopt_long(argc, argv, "D:p:m:i:x:c:dhv", long_options, NULL)) != -1)
     {
	switch (c)
	  {
	   case 'D': displaydev = optarg;
	             break;
	   case 'p': serverport = atoi(optarg); break;
	   case 'd': daemonmode = true; break;
	   case 'h': displayhelp = true; break;
	   case 'v': displayversion = true; break;
	   case 'm': mtcpserverport = atoi(optarg); break;
	   case 'i': mtcpforwardip = optarg; break;
	   case 'x': mtcpforwardport = atoi(optarg); break;
	   case 'c': confdir = optarg; break;
	   default: exit(0);
	  }
     }
	 
	 if (serverport > 0)
		  server = new cServer(serverport);
	  else
	  {
		  LOG("Tracerserver: No Serverport given\n");
		  displayhelp = true;
	  }
	 
	 if (displayhelp || displayversion)
     {
	if (displayhelp)
	  {
	     printf("Usage: tracerserver [OPTIONS]\n\n"
		    "-D PATH,      --displaydev=PATH      Device Path of the Display MT50\n"
		    "-p PORT,      --serverport=PORT      The Server Port to bind\n"
			"-i IP		   --mtcpforwardip        Forward Commands to the tracerclient which can not handled locally\n"
			"-x PORT       --mtcpforwardport      Forward Commands to the tracerclient which can not handled locally\n"
			"-m PORT,      --mtcpserverport=PORT  The Server Port of the Controlling Interface\n"
			"-c Config Dir --config               Config Directory that contain memory.conf\n"
			"-d,           --daemon               Run as Daemon\n"
		    "-h,           --help                 Show Usage\n"
		    "-v            --version              Show version Info\n"
		    "\n");
	  }
	
	if (displayversion)
	  {
	     printf("Tracerserver-%s\n",TRACERSERVERVERSION);
	  }
	exit(0);
     }
	 
	if (daemonmode)
    if (daemon(0, 0) == -1)
      {
	  LOG("Tracerserver: Error Run as Daemon: %s\n", strerror(errno));
	  exit (0);
      } 
	  
	  
	  if (!displaydev.empty())
		  display = new cDisplay(displaydev, mtcpforwardip, mtcpforwardport);
	  
	  if (mtcpserverport > 0)
		  mtcpserver = new cMtcpServer(mtcpserverport, mtcpforwardip, mtcpforwardport);
	  
	  if (!confdir.empty())
		  memory.addconfdir(confdir);
	  
	  
	  
	  if (display)
			 display->Start();
		 
	  if (mtcpserver)
		  mtcpserver->Start();
	  
	  if(!mtcpforwardip.empty() && mtcpforwardport > 0 && !mtcpclient)
	  {
		  mtcpclient = new cMtcpClient(mtcpforwardip.c_str(), mtcpforwardport);
	  }
	  
	  if (mtcpclient)
	  {
		  std::string sendbuffer("triggerserverstatisticupdate");
		  char recvmessage[1024];
		  
		  if (mtcpclient->connect() == -1)
		  {
			  LOG("Tracerserver: Error Send Message to Client(TriggerUpdate): Connection Error\n");
		  }
		  else
		  {
			LOG("Tracerserver: Send Message to Tracerclient: %s\n", sendbuffer.c_str());
			if (mtcpclient->send(sendbuffer.c_str(),sendbuffer.size()) == -1)
			{
				LOG("Tracerserver: Error Send Message to Client: Send Error\n");
			}
			int recvlenght = mtcpclient->recv(recvmessage, 1024);
			if (recvlenght > 0)
			{
				LOG("Tracerserver: Message from Tracerclient received: %s\n", recvmessage);
			}
			else 
				LOG("Tracerserver Error Receive Confirmation from Tracerclient\n");
		
			mtcpclient->closeconnection();
		  }
	  }
	  
signal(SIGINT, SignalHandler);
signal(SIGHUP, SignalHandler);
signal(SIGTERM, SignalHandler);	

	
//---------MainLoop

   while (!KillMe)
     {
		 //-----------------Read Data from Client
		 if (server)
			 server->action();
		 
		 //-----------------Print Data to Display MT50

	 }
	 
	 LOG("Shutting down Tracerserver\n");
	 if (server)
	 {
		 delete server;
	 }
	 
	 if (display)
	 {
		 LOG("Killing Display Thread\n");
		 display->Abort();
		 display->Join();
		 delete display;
	 }
	 
	 if (mtcpserver)
	 {
		 LOG("Killing MTCPServer Thread\n");
		 mtcpserver->Abort();
		 mtcpserver->Join();
		 delete mtcpserver;
	 }
	 if (mtcpclient)
	 {
		 delete mtcpclient;
	 }
	 LOG("Tracerserver Shutdown complete\n");
	 return 0;
}
		 
		 
	  