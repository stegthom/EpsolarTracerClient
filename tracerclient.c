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
#define TRACERCLIENTVERSION "v0.1"

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

#include <stdlib.h>
#include <errno.h>
#include <ctime>
#include <string>
#include <string.h>
#include <signal.h>
#include "memory.h"
#include "common.h"
#include "client.h"
#include "mttp.h"
#include "display.h"
#include "mtcpserver.h"
#include "tracerctr.h"
#include "modbus.h"


//Request to send to the Tracer requiered for the Display MT50//
 uint8_t GetDeviceInfo1[] = {0x01, 0x2B, 0x0E, 0x01, 0x00}; // Get deviceInfo Basic
 uint8_t GetDeviceInfo2[] = {0x01, 0x2B, 0x0E, 0x02, 0x00}; // Get deviceInfo Regular
 uint8_t GetReg3000t300E[] = {0x01, 0x43, 0x30, 0x00, 0x00, 0x0F}; //Get Register 3000 - 300E
 uint8_t GetReg3302t331E[] = {0x01, 0x43, 0x33, 0x02, 0x00, 0x1B}; //Get Register 3302 - 331E
 uint8_t GetReg3100t311D[] = {0x01, 0x43, 0x31, 0x00, 0x00, 0x76}; //Get Register 3100 - 311D
 uint8_t GetReg3200t3202[] = {0x01, 0x43, 0x32, 0x00, 0x00, 0x04}; //Get Register 3200 - 3202
 uint8_t GetReg9000t9063[] = {0x01, 0x43, 0x90, 0x00, 0x00, 0x76}; //Get Register 9000 - 9063
 uint8_t GetRTC[] = {0x01, 0x43, 0x90, 0x13, 0x00, 0x03}; //RTC Tracer wants it in an seperate Call and we need the header Info(Function Code 43 not Documented)
 uint8_t GetDiscInp2000[] = {0x01, 0x02, 0x20, 0x00, 0x00, 0x01}; //Get Discrete Input 20000

//Request to send to the Tracer to fill the missing Registers
uint8_t GetReg3300[] = {0x01, 0x04, 0x33, 0x00, 0x00, 0x01}; //Get Register 3300
uint8_t GetReg3301[] = {0x01, 0x04, 0x33, 0x01, 0x00, 0x01}; //Get Register 3301
uint8_t GetReg331D[] = {0x01, 0x04, 0x33, 0x1D, 0x00, 0x01}; //Get Register 331D
uint8_t GetReg331E[] = {0x01, 0x04, 0x33, 0x1E, 0x00, 0x01}; //Get Register 331E
uint8_t GetDiscInp200C[] = {0x01, 0x02, 0x20, 0x0C, 0x00, 0x01}; //Get Discret Input 200C

//Request Coils
uint8_t GetCoil0000[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x01}; //Get Coil 0000
uint8_t GetCoil0001[] = {0x01, 0x01, 0x00, 0x01, 0x00, 0x01}; //Get Coil 0001
uint8_t GetCoil0002[] = {0x01, 0x01, 0x00, 0x02, 0x00, 0x01}; //Get Coil 0002
uint8_t GetCoil0003[] = {0x01, 0x01, 0x00, 0x03, 0x00, 0x01}; //Get Coil 0003
uint8_t GetCoil0005[] = {0x01, 0x01, 0x00, 0x05, 0x00, 0x01}; //Get Coil 0005
uint8_t GetCoil0006[] = {0x01, 0x01, 0x00, 0x06, 0x00, 0x01}; //Get Coil 0006
uint8_t GetCoil0013[] = {0x01, 0x01, 0x00, 0x13, 0x00, 0x01}; //Get Coil 0013
uint8_t GetCoil0014[] = {0x01, 0x01, 0x00, 0x14, 0x00, 0x01}; //Get Coil 0014



//Define Timeouts
#define TRACER_STATISTIC_TIMEOUT 600
#define TRACER_REALTIME_TIMEOUT 0
#define SERVER_STATISTIC_TIMEOUT 600
#define SERVER_REALTIME_TIMEOUT 60

cMemory memory;
cClient *client = NULL;
cDisplay *display = NULL;
cMtcpServer *mtcpserver = NULL;
cTracerCtr *tracerctr = NULL;


time_t Now, LastTracerStatisticUpdate, LastTracerRealtimeUpdate;
time_t LastServerRealtimeUpdate, LastServerStatisticUpdate;

int ReadTracerInfo1();
int ReadTracerInfo2();
int ReadRatedData();
int ReadRealTimeData();
int ReadRealTimeStatus();
int ReadStatisticData();
int ReadSettingData();
int ReadCoils();
int ReadDiscreteInput();
void SaveTimestamp();

static bool KillMe = false;

static void SignalHandler(int signum)
{
   KillMe = true;
}

int SendMem(data memtype, uint16_t from = 0x0000, uint16_t to = 0xFFFF, bool send = true);
void SetTimeout();

int main( int argc, char* argv[] )
{
   memory.clean();
   std::string tracerdev;
   std::string displaydev;
   std::string server;
   int serverport = 0;
   int mtcpserverport = 0;
   bool daemonmode = false;
   bool displayhelp = false;
   bool displayversion = false;
   std::string confdir;
   
   static struct option long_options[] = {
      { "tracerdev",  required_argument, NULL, 'T' },
      { "displaydev", required_argument, NULL, 'D' },
      { "server",     required_argument, NULL, 's' },
      { "serverport", required_argument, NULL, 'p' },
	  { "mtcpport",   required_argument, NULL, 'm' },
	  { "config", 	  required_argument, NULL, 'c' },
	  { "daemon",     no_argument,       NULL, 'd' },
      { "help",       no_argument,       NULL, 'h' },
      { "version",    no_argument,       NULL, 'v' },
      { NULL }
};

int c;
while ((c = getopt_long(argc, argv, "T:D:s:p:m:c:dhv", long_options, NULL)) != -1)
     {
	switch (c)
	  {
	   case 'T': tracerdev = optarg;
	             break;
	   case 'D': displaydev = optarg;
	             break;
	   case 's': server = optarg;
	             break;
	   case 'p': serverport = atoi(optarg); break;
	   case 'd': daemonmode = true; break;
	   case 'h': displayhelp = true; break;
	   case 'v': displayversion = true; break;
	   case 'm': mtcpserverport = atoi(optarg); break;
	   case 'c': confdir = optarg; break;
	   default: exit(0);
	  }
     }
   if (tracerdev.empty())
     {
	LOG("Error Tracer device not set\n");
	displayhelp = true;
     }
   
   if (displayhelp || displayversion)
     {
	if (displayhelp)
	  {
	     printf("Usage: tracerclient [OPTIONS]\n\n"
		    "-T PATH,      --tracerdev=PATH       Device Path of the Tracer Solar Charge Controller\n"
		    "-D PATH,      --displaydev=PATH      Device Path of the Display MT50\n"
		    "-s IP,        --server=IP            The Server IP Adress or Hostname\n"
		    "-p PORT,      --serverport=PORT      The Server Port\n"
			"-m PORT,      --mtcpport=PORT        The Server Port of the Controlling Interface\n"
			"-c Config Dir --config               Config Directory that contain memory.conf\n"
			"-d,           --daemon               Run as Daemon\n"
		    "-h,           --help                 Show Usage\n"
		    "-v            --version              Show version Info\n"
		    "\n");
	  }
	
	if (displayversion)
	  {
	     printf("Tracerclient-%s\n",TRACERCLIENTVERSION);
	  }
	exit(0);
     }
   
   if (daemonmode)
     if (daemon(0, 0) == -1)
       {
	  LOG("Tracerclient: Error Run as Daemon: %s\n", strerror(errno));
	  exit (0);
       }
   

   if (!server.empty() && serverport > 0)
   client = new cClient(server.c_str(), serverport);

   if (!displaydev.empty())
		  display = new cDisplay(displaydev);
	  
	  
	if (mtcpserverport > 0)
		  mtcpserver = new cMtcpServer(mtcpserverport);
	
	if (!confdir.empty())
		  memory.addconfdir(confdir);
	  
	if (display)
			 display->Start();
		 
		 
	if (client)
			client->Start();
		
	if (mtcpserver)
		  mtcpserver->Start();

   cTracerCtr::CreateInstance(true, tracerdev);
   tracerctr = cTracerCtr::GetInstance();

signal(SIGINT, SignalHandler);
signal(SIGHUP, SignalHandler);
signal(SIGTERM, SignalHandler);


bool NeedTracerRealtimeupdate;
bool NeedTracerStatisticupdate;
bool NeedServerRealtimeupdate;
bool NeedServerStatisticupdate;

//---------MainLoop

   while (!KillMe)
     {
// Make sure that the Triggered Updates will be processed in the right order. 
// That why set the Values from TracerCtr on top of the Loop
 NeedTracerRealtimeupdate = tracerctr->NeedTracerRealtimeupdate();
 NeedTracerStatisticupdate = tracerctr->NeedTracerStatisticupdate();
 NeedServerRealtimeupdate = tracerctr->NeedServerRealtimeupdate();
 NeedServerStatisticupdate = tracerctr->NeedServerStatisticupdate();
 //--------Read Tracer Data
	
	int n;
if (NeedTracerRealtimeupdate)
     {
	for (n = 0; n<4; n++)
	  {
	    if (ReadRealTimeData() < 0)
	       {
	          LOG("Retry Reading\n");
	          continue;
	       }
	    else
	       {
			   SaveTimestamp();
	          break;
	       }
	   }
	
	for (n = 0; n<4; n++)
          {
             if (ReadRealTimeStatus() < 0)
               {
                  LOG("Retry Reading\n");
                  continue;
               }
             else
               {
                  break;
               }
          }
	
//------------- We need also statistic Data in Realtime because there are some Realtime Data required by Display MT50	
		  for (n = 0; n<4; n++)
          {
             if (ReadStatisticData() < 0)
               {
                  LOG("Retry Reading\n");
                  continue;
               }
             else
               {
                  break;
               }
          }
//-------------------		  
		  
	LastTracerRealtimeUpdate = time(NULL);
	tracerctr->ResetTracerRealtimeupdate();
     }
if (NeedTracerStatisticupdate)
   {
      for (n = 0; n<4; n++)
	  {
	     if (ReadTracerInfo1() < 0)
	       {
		  LOG("Retry Reading\n");
		  continue;
	       }
	     else
	       {
		  break;
	       }
	  }
	  
	  for (n = 0; n<4; n++)
	  {
	     if (ReadTracerInfo2() < 0)
	       {
		  LOG("Retry Reading\n");
		  continue;
	       }
	     else
	       {
		  break;
	       }
	  }

	for (n = 0; n<4; n++)
          {
             if (ReadRatedData() < 0)
               {
                  LOG("Retry Reading\n");
                  continue;
               }
             else
               {
                  break;
               }
          }



	for (n = 0; n<4; n++)
          {
             if (ReadSettingData() < 0)
               {
                  LOG("Retry Reading\n");
                  continue;
               }
             else
               {
                  break;
               }
          }
/*  Coils are write only
	for (n = 0; n<4; n++)
          {
             if (ReadCoils() < 0)
               {
                  LOG("Retry Reading\n");
                  continue;
               }
             else
               {
                  break;
               }
          }
*/
	for (n = 0; n<4; n++)
          {
             if (ReadDiscreteInput() < 0)
               {
                  LOG("Retry Reading\n");
                  continue;
               }
             else
               {
                  break;
               }
          }
      LastTracerStatisticUpdate = time(NULL);
      tracerctr->ResetTracerStatisticupdate();
   }
	
//--------------Send Data to Server
if (NeedServerStatisticupdate && client)
   {
	if (SendMem(eAll) < 0)  //send complete Memory Data
	  LOG("Error Sending Complete Memory Data\n");
        else
	{
	   LastServerStatisticUpdate = time(NULL);
	   LastServerRealtimeUpdate = time(NULL);
	   tracerctr->ResetServerStatisticupdate();
	   tracerctr->ResetServerRealtimeupdate();
	}
      
   }
	
if (NeedServerRealtimeupdate && client)
   {
        int sucess = 0;
	if (SendMem(eReg, 0x3100, 0x31FF, false) < 0) //send Realtime Data
	  LOG("Error sending Regitster 3100 - 31FF\n");
        else sucess++;
        if (SendMem(eReg, 0x3200, 0x32FF, false) < 0) //send Realtime Status
	  LOG("Error sending Regitster 3200 - 32FF\n");
        else sucess++;
		//------------- We need also statistic Data in Realtime because there are some Realtime Data required by Display MT50
		if (SendMem(eReg, 0x3302, 0x33FF, false) < 0) //send Realtime Status
	  LOG("Error sending Regitster 3302 - 32FF\n");
	    else sucess++;
		//----------------
		
		//----------Sending Alternate Values
		if (SendMem(eUser, 0x3100, 0x31FF, false) < 0) //send Realtime Data
	  LOG("Error sending Regitster 3100 - 31FF\n");
        else sucess++;
        if (SendMem(eUser, 0x3200, 0x32FF, false) < 0) //send Realtime Status
	  LOG("Error sending Regitster 3200 - 32FF\n");
        else sucess++;
		//------------- We need also statistic Data in Realtime because there are some Realtime Data required by Display MT50
		if (SendMem(eUser, 0x3302, 0x33FF, true) < 0) //send Realtime Status  //The last call to SendMem sends the buffer
	  LOG("Error sending Regitster 3302 - 32FF\n");
	    else sucess++;
		//----------------
		
		
	if (sucess == 6)
	  {
	     LastServerRealtimeUpdate = time(NULL);
         tracerctr->ResetServerRealtimeupdate();
	  }
      
	  if (SendMem(eUserReg, 0x4000, 0x6FFF, true) < 0) //send Realtime UserRegister from 0x4000 to 0x6FFF
	  LOG("Error sending UserRegitster 4000 - 6FFF\n");
	  
	  if (SendMem(eUserCoil, 0x0020, 0x00FF, true) < 0) //Sending UserCoils
	  LOG("Error sending UserCoil 0020 - 00FF\n"); 
   }
	
	
	SetTimeout(); //Set the timeout like defined in tracerclient.c
	usleep(10000);

     }
   

   LOG("Shutting down Tracerclient\n");
   
   if (client)
   {
	   LOG("Killing Client Thread\n");
	   client->Abort();
	   client->Join();
	   delete client;
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
	 if (tracerctr)
	 {
	   tracerctr->DeleteInstance();
	   tracerctr = NULL;
	 }
	 LOG("Tracerclient Shutdown complete\n");
   return 0;
   
}

int ReadTracerInfo1()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
   
   rc = 0;
   rc = tracerctr->modbus_send_request(GetDeviceInfo1, 5 * sizeof(uint8_t), rsp);

   if (rc <= 0)
     {
	LOG("Error Receive DeviceInfo1");
	return -1;
     }
   else if (rc > 0)
     {
	if (memory.Save(rsp, rc, 0, GetDeviceInfo1) < 0)
	    {
	       LOG("Error Saving DeviceInfo1\n");
	       return -1;
	    }
     }
	 return 1;
}
	 
int ReadTracerInfo2()
{
	int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
   rc = 0;
   rc = tracerctr->modbus_send_request(GetDeviceInfo2, 5 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
        LOG("Error Receive DeviceInfo2\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 0, GetDeviceInfo2) < 0)
       	  {
             LOG("Error Saving DeviceInfo2\n");
	     return -1;
	  }
     }
return 1;
}

   
int ReadRatedData()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
   rc = 0;
   rc = tracerctr->modbus_send_request(GetReg3000t300E, 6 * sizeof(uint8_t), rsp);
 
if (rc <= 0)
     {
        LOG("Error Receive Register 3000\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 5, GetReg3000t300E) < 0)
	  {
             LOG("Error Saving Register 3000\n");
	     return -1;
	  }
     }
return 1;
}
   
int ReadRealTimeData()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
   rc = 0;
   rc = tracerctr->modbus_send_request(GetReg3100t311D, 6 * sizeof(uint8_t), rsp);
 
if (rc <= 0)
     {
        LOG("Error Receive Register 3100\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 18, GetReg3100t311D) < 0)
	  {
              LOG("Error Saving Register 3100\n");
	      return -1;
	  }
	
     }
return 1;
}
int ReadRealTimeStatus()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
   rc = 0;
   rc = tracerctr->modbus_send_request(GetReg3200t3202, 6 * sizeof(uint8_t), rsp);
 
 if (rc <= 0)
     {
        LOG("Error Receive Register 3200\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 4, GetReg3200t3202) < 0)
	  {
              LOG("Error Saving Register 3200\n");
	      return -1;
	  }
	
     }
return 1;
}

int ReadStatisticData()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
	rc = 0;
	rc = tracerctr->modbus_send_request(GetReg3302t331E, 6 * sizeof(uint8_t), rsp);
  
if (rc <= 0)
     {
        LOG("Error Receive Register 3302\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 7, GetReg3302t331E) < 0)
	  {
              LOG("Error Saving 3302\n");
	      return -1;
	  }
	
     }
	 
	 
	  rc = 0;
	  rc = tracerctr->modbus_send_request(GetReg3300, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
	LOG("Error Receive Register 3300\n");
        return -1;
     }
   else if (rc > 0)
     {
	if (memory.Save(rsp, rc, 3, GetReg3300) < 0)
	  {
	     LOG("Error Saving Register 3300\n");
	     return -1;
	  }
     }
   
   
   rc = 0;
   rc = tracerctr->modbus_send_request(GetReg3301, 6 * sizeof(uint8_t), rsp);
  
   if (rc <= 0)
     {
	LOG("Error Receive Register 3301\n");
        return -1;
     }
   else if (rc > 0)
     {
	if (memory.Save(rsp, rc, 3, GetReg3301) < 0)
	  {
	      LOG("Error Saving Register 3301\n");
	      return -1;
	  }
     }
   
   rc = 0;
   rc = tracerctr->modbus_send_request(GetReg331D, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
	LOG("Error Receive Register 331D\n");
        return -1;
     }
   else if (rc > 0)
     {
	if (memory.Save(rsp, rc, 3, GetReg331D) < 0)
	  {
	      LOG("Error Saving Register 331D\n");
	      return -1;
	  }
     }
   
   rc = 0;
   rc = tracerctr->modbus_send_request(GetReg331E, 6 * sizeof(uint8_t), rsp);
  
   if (rc <= 0)
     {
	LOG("Error Receive Register 331E\n");
        return -1;
     }
   else if (rc > 0)
     {
	if (memory.Save(rsp, rc, 3, GetReg331E) < 0)
	  {
	     LOG("Error Saving Register 331E\n");
	     return -1;
	  }
     }
return 1;
}

int ReadSettingData()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
	rc = 0;
	rc = tracerctr->modbus_send_request(GetReg9000t9063, 6 * sizeof(uint8_t), rsp);
 
 if (rc <= 0)
     {
        LOG("Error Receive Register 9000\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 18, GetReg9000t9063) < 0)
	  {
              LOG("Error Saving Register 9000\n");
	      return -1;
	  }
	
     }
	 
	   rc = 0;
	   rc = tracerctr->modbus_send_request(GetRTC, 6 * sizeof(uint8_t), rsp);
  
 if (rc <= 0)
     {
        LOG("Error Receive RTC\n");
     return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 4, GetRTC) < 0)
	  {
             LOG("Error Saving RTC\n");
	  return -1;
	  }
     }
return 1;
}

int ReadCoils()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
	rc = 0;
	rc = tracerctr->modbus_send_request(GetCoil0000, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0000\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0000) < 0)
          {
             LOG("Error Saving Coil 0000\n");
             return -1;
	  }
     }

rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0001, 6 * sizeof(uint8_t), rsp);
  
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0001\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0001) < 0)
          {
             LOG("Error Saving Coil 0001\n");
             return -1;
	  }
     }

rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0002, 6 * sizeof(uint8_t), rsp);
  
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0002\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0002) < 0)
          {
             LOG("Error Saving Coil 0002\n");
             return -1;
	  }
     }

rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0003, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0003\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0003) < 0)
          {
             LOG("Error Saving Coil 0003\n");
          return -1;
	  }
     }

rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0005, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0005\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0005) < 0)
          {
             LOG("Error Saving Coil 0005\n");
             return -1;
	  }
     }
rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0006, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0006\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0006) < 0)
          {
             LOG("Error Saving Coil 0006\n");
             return -1;
	  }
     }

rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0013, 6 * sizeof(uint8_t), rsp);
 
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0013\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0013) < 0)
          {
             LOG("Error Saving Coil 0013\n");
             return -1;
	  }
     }

rc = 0;
rc = tracerctr->modbus_send_request(GetCoil0014, 6 * sizeof(uint8_t), rsp);
  
   if (rc <= 0)
     {
        LOG("Error Receive Coil 0014\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetCoil0014) < 0)
          {
             LOG("Error Saving Coil 0014\n");
             return -1;
	  }
     }
return 1;
}

int ReadDiscreteInput()
{
   int rc;
   uint8_t rsp[MODBUS_RTU_MAX_ADU_LENGTH];
	   rc = 0;
	   rc = tracerctr->modbus_send_request(GetDiscInp2000, 6 * sizeof(uint8_t), rsp);
 
if (rc <= 0)
     {
        LOG("Error Receive Discrete Input 2000\n");
        return -1;
     }
   else if (rc > 0)
     {
        if (memory.Save(rsp, rc, 3, GetDiscInp2000) < 0)
	  {
              LOG("Error Saving Discrete Input 2000\n");
	      return -1;
	  }
	
     }

  
   
   rc = 0;
   rc = tracerctr->modbus_send_request(GetDiscInp200C, 6 * sizeof(uint8_t), rsp);
  
   if (rc <= 0)
     {
	LOG("Error Receive Discrete Input 200C\n");
        return -1;
     }
   else if (rc > 0)
     {
	if (memory.Save(rsp, rc, 3, GetDiscInp200C) < 0)
	  {
	     LOG("Error Saving Discrete Input 200C\n");
	     return -1;
	  }
     }
return 1;
}


int SendMem(data memtype, uint16_t from, uint16_t to, bool send)
{
   static cMTTP MTTP(ePut);
   switch (memtype)
     {
      case eAll:
      case eDevInfo1:
	  {
	     uint8_t deviceinfo[MAX_DEV_INFO_LENGHT];
	     int lenght = memory.GetDeviceInfo(1, deviceinfo, MAX_DEV_INFO_LENGHT);
	     if (lenght < 0)
	       {
		  LOG("Error Get DeviceInfo1\n");
		  //return -1;
	       }
	     MTTP.AddCommand(eDevInfo1, deviceinfo, lenght, 1);
	     
	     if (memtype != eAll) break;
	  }
	
	case eDevInfo2:
	  {
	     uint8_t deviceinfo[MAX_DEV_INFO_LENGHT];
	     int lenght = memory.GetDeviceInfo(2, deviceinfo, MAX_DEV_INFO_LENGHT);
	     if (lenght < 0)  
	       {
		  LOG("Error Get DeviceInfo2\n");
		  //return -1;
	       }
	     MTTP.AddCommand(eDevInfo2, deviceinfo, lenght, 2);
	     if (memtype != eAll) break;
	  }
	
      case eReg:
	  {
	     uint16_t reg[MAXREG];
	     uint8_t reg8_t[(MAXREG*2)];
	     int numreg = memory.GetRegister(from, to, reg, MAXREG);
	     if (numreg < 0)
	       {
		  LOG("Error Get Register\n");
		  //return -1;
	       }
	     int n; 
	     int m = 0;
	     for (n=0; n<numreg; n++)
	       {
		  MODBUS_SET_INT16_TO_INT8(reg8_t, m, reg[n]);
		  m = m+2;
	       }
		 if (numreg > 0)  
			MTTP.AddCommand(eReg, reg8_t, (numreg*2), from);
	     if (memtype != eAll) break;
	  }
	
      case eCoil:
	  {
	     uint8_t coil[MAXCOILS];
	     int numcoils = memory.GetCoil(from, to, coil, MAXCOILS);
	     if (numcoils < 0)
	       {
		  LOG("Error Get Coil\n");
		  //return -1;
	       }
		   if (numcoils > 0)
			MTTP.AddCommand(eCoil, coil, numcoils, from);
	     if (memtype != eAll) break;
	  }
	
      case eDiscInp:
	  {
	     uint8_t discinp[MAXDISCRETEINPUTS];
	     int numdiscinp = memory.GetDiscInp(from, to, discinp, MAXDISCRETEINPUTS);
	     if (numdiscinp < 0)
	       {
		  LOG("Error Get Discrete Inputs\n");
		  //return -1;
	       }
		  if (numdiscinp > 0) 
			MTTP.AddCommand(eDiscInp, discinp, numdiscinp, from);
	     if (memtype != eAll) break;
	  }
	
      case eHeader:
	  {
	     if (memtype == eAll)
	       {
		  int n;
		  for (n=0; n<MAXREG; n++)
		    {
		       uint8_t header[MAXHEADERSIZE];
		       int lenght = memory.GetHeader(0xFFFF, n, header, MAXHEADERSIZE);
		       if (lenght < 0)
			 {
			    LOG("Error Get Header\n");
			    //return -1;
			 }
		       else if (lenght == 0)
			 continue;
		       else
			 {
			    uint16_t headeradress;
			    headeradress = memory.GetRegisterAdress(n, true);
			    if (!headeradress)
			      {
				 LOG("MTTP AddCommand Header Adress not found Skipping Header");
				 break;
			      }
			    
			    MTTP.AddCommand(eHeader, header, lenght, headeradress);
			 }
		    }
	       }
	     else
	       {
		  uint8_t header[MAXHEADERSIZE];
		  int lenght = memory.GetHeader(from, -1, header, MAXHEADERSIZE);
		  if(lenght < 0)
		    {
		       LOG("Error Get single Header\n");
		       //return -1;
		    }
		  else if (lenght == 0)
		    {
		       LOG("Header: %04X not found\n", from);
		    }
		  else
		    {
		       MTTP.AddCommand(eHeader, header, lenght, from);
		    }
	       }	  
	     if (memtype != eAll) break;
	  }
	
      case eUser:
	  {
	     int n;
		 int startidx = memory.GetRegStartIndex(from);
		 uint16_t adress;
		 uint8_t reg[2];
		 for (n=startidx; n<MAXREG; n++)
		 {
			adress = memory.GetRegisterAdress(n, true);
			if (adress > to)
				break;
			if (memory.RegisterHaveAlternateValue(adress))
			{
				memory.GetRegister(adress, adress, reg, 2, true);
				MTTP.AddCommand(eUser, reg, 2, adress);
			}
		 }
		 if (memtype != eAll) break;
	  }
	  
	  case eUserReg:
	  {
	     uint16_t reg[MAXREG];
	     uint8_t reg8_t[(MAXREG*2)];
		 uint16_t firstadress;
	     int numreg = memory.GetUserRegister(from, to, reg, MAXREG, &firstadress);
	     if (numreg < 0)
	       {
		  LOG("Error Get UserRegister\n");
		  //return -1;
	       }
	     int n; 
	     int m = 0;
	     for (n=0; n<numreg; n++)
	       {
				MODBUS_SET_INT16_TO_INT8(reg8_t, m, reg[n]);
				m = m+2;
	       }
		   if (numreg > 0)
				MTTP.AddCommand(eUserReg, reg8_t, (numreg*2), firstadress);
	     if (memtype != eAll) break;
	  }
	  
	  case eUserCoil:
	  {
		  uint8_t coil[MAXCOILS];
		  uint16_t firstadress;
		  int numcoils = memory.GetUserCoil(from, to, coil, MAXCOILS, &firstadress);
		  if (numcoils < 0)
		  {
			  LOG("Error Get UserCoil\n");
			  //return -1;
		  }
		  if (numcoils > 0)
			MTTP.AddCommand(eUserCoil, coil, numcoils, firstadress);
		  if (memtype != eAll) break;
	  }
	
      default:
	  break;
     }
	 
	 if (send)
	 {
   
		uint8_t sendbuffer[MAX_SEND_BUFFER];
		int sendbufferlen = MTTP.GetBuffer(sendbuffer, MAX_SEND_BUFFER);
		MTTP.InitBuffer(ePut);
		printf("Debug Sendbuffer:\n");
		int n;
		for (n = 0; n < sendbufferlen; n++)
			{
			if(sendbuffer[n] == DLE)
			printf("<DLE>");
			else if(sendbuffer[n] == STX)
			printf("<STX>");
			else if(sendbuffer[n] == ETX)
			printf("<ETX>");
			else if(sendbuffer[n] == SCD)
			printf("<SCD>");
			else if(sendbuffer[n] == ECD)
			printf("<ECD>");
			else
			printf("<%02X>", sendbuffer[n]);
			}
		printf("\n");
		printf("%d\n", n);
 
		if (client->sendmessage(sendbuffer, sendbufferlen) < 0)
			{
			LOG("Error Sending Data to Server\n");
			return -1;
			}

	 }
   
   return 1;
}

void SetTimeout()
{
   
   Now = time(NULL);
   if (Now - LastTracerStatisticUpdate > TRACER_STATISTIC_TIMEOUT) {tracerctr->TriggerTracerStatisticupdate();}
   if (Now - LastTracerRealtimeUpdate > TRACER_REALTIME_TIMEOUT) {tracerctr->TriggerTracerRealtimeupdate();}
   if (Now - LastServerRealtimeUpdate > SERVER_REALTIME_TIMEOUT) {tracerctr->TriggerServerRealtimeupdate();}
   if (Now - LastServerStatisticUpdate > SERVER_STATISTIC_TIMEOUT) {tracerctr->TriggerServerStatisticupdate();}
}

void SaveTimestamp()
{
	uint16_t day;
	uint16_t mon;
	uint16_t year;
	uint16_t hour;
	uint16_t min;
	uint8_t payload[10];
	time_t Timestamp;
	uint16_t startadress;
	startadress = 0x3330;
    tm *now;
    Timestamp = time(0);
    now = localtime(&Timestamp);
	day = now->tm_mday;
	mon = (now->tm_mon+1);
	year = (now->tm_year+1900);
	hour = now->tm_hour;
	min = now->tm_min;
	
	MODBUS_SET_INT16_TO_INT8(payload, 0, day);
	MODBUS_SET_INT16_TO_INT8(payload, 2, mon);
	MODBUS_SET_INT16_TO_INT8(payload, 4, year);
	MODBUS_SET_INT16_TO_INT8(payload, 6, hour);
	MODBUS_SET_INT16_TO_INT8(payload, 8, min);
	
	memory.Save(eReg, startadress, 10, payload);
}

	
	
   
	
   
