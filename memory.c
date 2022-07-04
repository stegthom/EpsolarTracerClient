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
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>

#include "memory.h"
#include "common.h"
#include <cstring>

uint16_t cMemory::RegList[MAXREG]= {0x3000, 0x3001, 0x3002, 0x3003, 0x3004, 0x3005, 0x3006, 0x3007, 0x3008, 0x300E, 0x3100, 0x3101, 0x3102, 0x3103, 0x3104, 0x3105, 0x3106, 0x3107, 0x3108, 0x3109, 0x310A, 0x310B, 0x310C, 0x310D, 0x310E, 0x310F, 0x3110, 0x3111, 0x3112, 0x3113, 0x311A, 0x311B, 0x311D, 0x3200, 0x3201, 0x3202, 0x3300, 0x3301, 0x3302, 0x3303, 0x3304, 0x3305, 0x3306, 0x3307, 0x3308, 0x3309, 0x330A, 0x330B, 0x330C, 0x330D, 0x330E, 0x330F, 0x3310, 0x3311, 0x3312, 0x3313, 0x3314, 0x3315, 0x3316, 0x3317, 0x3318, 0x3319, 0x331A, 0x331B, 0x331C, 0x331D, 0x331E, 0x3330, 0x3331, 0x3332, 0x3333, 0x3334, 0x3335, 0x3336, 0x3337, 0x9000, 0x9001, 0x9002, 0x9003, 0x9004, 0x9005, 0x9006, 0x9007, 0x9008, 0x9009, 0x900A, 0x900B, 0x900C, 0x900D, 0x900E, 0x9013, 0x9014, 0x9015, 0x9016, 0x9017, 0x9018, 0x9019, 0x901A, 0x901B, 0x901C, 0x901D, 0x901E, 0x901F, 0x9020, 0x9021, 0x903D, 0x903E, 0x903F, 0x9042, 0x9043, 0x9044, 0x9045, 0x9046, 0x9047, 0x9048, 0x9049, 0x904A, 0x904B, 0x904C, 0x904D, 0x9063, 0x9065, 0x9066, 0x9067, 0x906A, 0x906B, 0x906C, 0x906D, 0x906E, 0x906F, 0x9070, 0xFFFF};
const uint16_t cMemory::DiscreteInputList[MAXDISCRETEINPUTS]= {0x2000, 0x200C, 0xFFFF};
const uint16_t cMemory::CoilList[MAXCOILS]= {0x0000, 0x0001, 0x0002, 0x0003, 0x0005, 0x0006, 0x0013, 0x0014, 0xFFFF};

cMemory::REGISTER cMemory::Register[MAXREG];
cMemory::COIL cMemory::Coil[MAXCOILS];
cMemory::DISCRETEINPUT cMemory::DiscreteInput[MAXDISCRETEINPUTS];

std::mutex cMemory::construct_mtx;
std::mutex cMemory::mem_mtx;

bool cMemory::inited = false;
std::string cMemory::memoryfile;

std::vector<cMemory::userreg> cMemory::UserRegister;
std::vector<cMemory::usercoil> cMemory::UserCoil;
std::vector<cMemory::userswitch> cMemory::UserSwitch;
	 
int cMemory::DeviceInfo1Lenght = 0;
int cMemory::DeviceInfo2Lenght = 0;
uint8_t cMemory::DeviceInfo1[MAX_DEV_INFO_LENGHT];
uint8_t cMemory::DeviceInfo2[MAX_DEV_INFO_LENGHT];

cMemory::cMemory()
{
 construct_mtx.lock();
 if (!inited)
     {
        //clean();
	inited = true;
     }
   construct_mtx.unlock();
}

cMemory::~cMemory()
{
}

int cMemory::Save(uint8_t *resp, int lenght, int headerlenght, uint8_t *req)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   int n, m;
   switch (resp[1])
     {
      case 0x2B:
	  {
	     
	if (resp[3]==0x01)
	  {
	     DeviceInfo1Lenght = (lenght-2); //Cut off CRC16
	  }
	if (resp[3]==0x02)
	  {
	     DeviceInfo2Lenght = (lenght-2); //Cut off CRC16
	  }
	
	for (n=0; n<(lenght-2); n++)
	  {
	     if (resp[3] == 0x01)
	       {
		  DeviceInfo1[n] = resp[n];
	       }
	     if (resp[3] == 0x02)
	       {
		  DeviceInfo2[n] = resp[n];
	       }
	  }
	
	return (n+1);
	  }
	
      case 0x43:
	  {
	uint16_t startadress = MODBUS_GET_INT16_FROM_INT8(req, 2);
	int startindex = GetRegStartIndex(startadress);
	Register[startindex].Headersize = headerlenght;
	Register[startindex].HaveHeader = true;
	for (n=0; n<headerlenght; n++)
	  {
	     Register[startindex].Header[n] = resp[n];
	  }
	
	
	
	for (n=headerlenght, m=startindex; n<(lenght-2); n+=2, m++) //Don't Copy CRC16 and put 2 int8_t into an int16_t
	  {
	     Register[m].Value = MODBUS_GET_INT16_FROM_INT8(resp, n);
	  }
	return (m + 1 - startindex);
	  }
	
      case 0x02:
	  {

	uint16_t discadress = MODBUS_GET_INT16_FROM_INT8(req, 2);
	int discindex = GetDiscStartIndex(discadress);;     

	
	
	for (n=headerlenght, m=discindex; n<(lenght-2); n++, m++) 
	  {
	      DiscreteInput[m].Value = resp[n];
	  }
	return (m + 1 - discindex);
     }
   
     

      case 0x01:
	  {
	     uint16_t coiladress = MODBUS_GET_INT16_FROM_INT8(req, 2);
	     int coilindex = GetCoilStartIndex(coiladress);

	     for (n=headerlenght, m=coilindex; n<(lenght-2); n++, m++)
	       {
		  Coil[m].Value = resp[n];
	       }
	     return (m + 1 - coilindex);
	  }
	
	
 case 0x04:
 case 0x03:
     {
	// Only 1 Register at the Time allowed with Function 03 or 04 by Tracer
	uint16_t registeradress = MODBUS_GET_INT16_FROM_INT8(req, 2);
	int registerindex = GetRegStartIndex(registeradress);
	n = headerlenght;
	Register[registerindex].Value = MODBUS_GET_INT16_FROM_INT8(resp, n);
	return 1;
     }
   
   
   return 0;
}
return 0;
}

int cMemory::Save(data datatype, uint16_t startadress, int payloadlenght, uint8_t *payload)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	int n = 0;
	int m = 0; 

	switch (datatype)
	  {
		  case eDevInfo1:
		  {
			  if (payloadlenght > MAX_DEV_INFO_LENGHT)
			    {
					LOG("cMemory::Save: Error DeviceInfo1 > MAX_DEV_INFO_LENGHT\n");
					return -1;
				}
			  DeviceInfo1Lenght = payloadlenght;
			  for (n = 0; n < payloadlenght; n++)
			    {
					DeviceInfo1[n] = payload[n];
				}
				return (n);
		  }
		  
		  case eDevInfo2:
		  {
			  if (payloadlenght > MAX_DEV_INFO_LENGHT)
			    {
					LOG("cMemory::Save: Error DeviceInfo2 > MAX_DEV_INFO_LENGHT\n");
					return -1;
				}
			  DeviceInfo2Lenght = payloadlenght;
			  for (n = 0; n < payloadlenght; n++)
			    {
					DeviceInfo2[n] = payload[n];
				}
				return (n);
		  }
		  
		  case eReg:
		  {
			  int startindex = GetRegStartIndex(startadress);
			  for (n = 0, m=startindex; n < payloadlenght; n+=2, m++)
			    {
				   	if (m >= MAXREG)
					  {
						  LOG("cMemory::Save: Error RegisterIndex > MAXREG\n");
						  return -1;
					  }
					Register[m].Value = MODBUS_GET_INT16_FROM_INT8(payload, n);
				}
				return (m - startindex);
		  }
		  
		  case eUserReg:
		  {
			  int numuserreg, idx, startidx;
			  startidx = -1;
			  numuserreg = UserRegister.size();
			  n=0;
			  for (idx=0; idx < numuserreg; idx++)
			  {
				  if ((n+1) >= payloadlenght) //n+1 because the next Run of the loop takes 2 byte of Payload
					  break;
				  if (UserRegister.at(idx).Adress == startadress)
				  {
					  startidx = idx;
					  if ((payloadlenght/2) > (numuserreg - startidx))
					  {
						  LOG("cMemmory::Save: Error UserRegister to Save > UserRegister Memmory");
						  return -1;
					  }
					  UserRegister.at(idx).Value = MODBUS_GET_INT16_FROM_INT8(payload, n);
					  n+=2;
					  
				  }
				  if (UserRegister.at(idx).Adress > startadress && startidx >= 0)
				  {
					  UserRegister.at(idx).Value = MODBUS_GET_INT16_FROM_INT8(payload, n);
					  n+=2;
				  }
			  }
				return (n/2);
		  }
		  
		  case eCoil:
		  {
			  int coilindex = GetCoilStartIndex(startadress);
			  for (n=0, m=coilindex; n<payloadlenght; n++, m++)
               {
				 if (m >= MAXCOILS)
					  {
						  LOG("cMemory::Save: Error CoilIndex > MAXCOILS\n");
						  return -1;
					  }  
                  Coil[m].Value = payload[n];
               }
             return (m - coilindex);
		  }
		  
		  case eUserCoil:
		  {
			  int numusercoil, idx, startidx;
			  startidx = -1;
			  numusercoil = UserCoil.size();
			  n=0;
			  for (idx=0; idx < numusercoil; idx++)
			  {
				  if (n >= payloadlenght)
					  break;
				  if (UserCoil.at(idx).Adress == startadress)
				  {
					  startidx = idx;
					  if (payloadlenght > (numusercoil - startidx))
					  {
						  LOG("cMemmory::Save: Error UserCoil to Save > UserCoil Memmory");
						  return -1;
					  }
					  UserCoil.at(idx).Value = payload[n];
					  n++;
					  
				  }
				  if (UserCoil.at(idx).Adress > startadress && startidx >= 0)
				  {
					  UserCoil.at(idx).Value = payload[n];
					  n++;
				  }
			  }
				return (n);
		  }
		  
		  case eDiscInp:
		  {
			  int discindex = GetDiscStartIndex(startadress);
			  for (n=0, m=discindex; n<payloadlenght; n++, m++)
          {
			  if (m >= MAXDISCRETEINPUTS)
					  {
						  LOG("cMemory::Save: Error DiscreteInputIndex > MAXDISCRETEINPUTS\n");
						  return -1;
					  }  
              DiscreteInput[m].Value = payload[n];
          }
        return (m - discindex);
		  }
		  
		  case eHeader:
		  {
			  if (payloadlenght > MAXHEADERSIZE)
			  {
				  LOG("cMemmory::Save: Error Header Lenght > MAXHEADERSIZE\n");
				  return -1;
			  }
			  int startindex = GetRegStartIndex(startadress);
			  Register[startindex].Headersize = payloadlenght;
              Register[startindex].HaveHeader = true;
			  for (n=0; n<payloadlenght; n++)
          {
             Register[startindex].Header[n] = payload[n];
          }
		  return 1;
		  }
		  
		  case eUser:
		  {
			  int startindex = GetRegStartIndex(startadress);
			  for (n = 0, m=startindex; n < payloadlenght; n+=2, m++)
			    {
				   	if (m >= MAXREG)
					  {
						  LOG("cMemory::Save: Error RegisterIndex > MAXREG\n");
						  return -1;
					  }
					Register[m].AlternateValue = MODBUS_GET_INT16_FROM_INT8(payload, n);
					Register[m].HaveAlternateValue = true;
				}
				return (m - startindex);
		  }

}
return 0;
}

int cMemory::GenerateResponse(uint8_t *req, int reqlenght, uint8_t *result, int resultsize)
{

   if ((req[0] == 0x01) && (req[1] == 0x2B) && (req[2] == 0x0E) && (req[3] == 0x01) && (req[4] == 0x00)) // Get Device Info Basic
   {
	   return GetDeviceInfo(1, result, resultsize);
   }
  
   else if ((req[0] == 0x01) && (req[1] == 0x2B) && (req[2] == 0x0E) && (req[3] == 0x02) && (req[4] == 0x00)) // Get Device Info Regular
   {
	   return GetDeviceInfo(2, result, resultsize);
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x43) && (req[2] == 0x30) && (req[3] == 0x00) && (req[4] == 0x00) && (req[5] == 0x0F)) //Get Register 3000 - 300E
   {
	   uint16_t startadress = 0x3000;
	   uint16_t endadress = 0x300E;
	   int ret = 0;
	   ret = GetHeader(startadress, -1, result, resultsize);
	   if (ret > 0)
	   {
		   ret += GetRegister(startadress, endadress, (result + ret), (resultsize - ret), true);
	   }
	   return ret;
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x43) && (req[2] == 0x33) && (req[3] == 0x02) && (req[4] == 0x00) && (req[5] == 0x1B)) //Get Register 3302 - 331E
   {
	   uint16_t startadress = 0x3302;
	   uint16_t endadress = 0x331E;
	   int ret = 0;
	   ret = GetHeader(startadress, -1, result, resultsize);
	   if (ret > 0)
	   {
		   ret += GetRegister(startadress, endadress, (result + ret), (resultsize - ret), true);
	   }
	   return ret;
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x43) && (req[2] == 0x31) && (req[3] == 0x00) && (req[4] == 0x00) && (req[5] == 0x76)) //Get Register 3100 - 311D
   {
	   uint16_t startadress = 0x3100;
	   uint16_t endadress = 0x311D;
	   int ret = 0;
	   ret = GetHeader(startadress, -1, result, resultsize);
	   if (ret > 0)
	   {
		   ret += GetRegister(startadress, endadress, (result + ret), (resultsize - ret), true);
	   }
	   return ret;
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x43) && (req[2] == 0x32) && (req[3] == 0x00) && (req[4] == 0x00) && (req[5] == 0x04)) //Get Register 3200 - 3202
   {
	   uint16_t startadress = 0x3200;
	   uint16_t endadress = 0x3202;
	   int ret = 0;
	   ret = GetHeader(startadress, -1, result, resultsize);
	   if (ret > 0)
	   {
		   ret += GetRegister(startadress, endadress, (result + ret), (resultsize - ret), true);
	   }
	   return ret;
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x43) && (req[2] == 0x90) && (req[3] == 0x00) && (req[4] == 0x00) && (req[5] == 0x76)) //Get Register 9000 - 9070
   {
	   uint16_t startadress = 0x9000;
	   uint16_t endadress = 0x9070;
	   int ret = 0;
	   ret = GetHeader(startadress, -1, result, resultsize);
	   if (ret > 0)
	   {
		   ret += GetRegister(startadress, endadress, (result + ret), (resultsize - ret), true);
	   }
	   return ret;
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x43) && (req[2] == 0x90) && (req[3] == 0x13) && (req[4] == 0x00) && (req[5] == 0x03)) //RTC Tracer wants it in an seperate Call and we need the header Info(Function Code 43 not Documented)
   {
	   uint16_t startadress = 0x9013;
	   uint16_t endadress = 0x9015;
	   int ret = 0;
	   ret = GetHeader(startadress, -1, result, resultsize);
	   if (ret > 0)
	   {
		   ret += GetRegister(startadress, endadress, (result + ret), (resultsize - ret), true);
	   }
	   return ret;
   }
   
   else if ((req[0] == 0x01) && (req[1] == 0x02) && (req[2] == 0x20) && (req[3] == 0x00) && (req[4] == 0x00) && (req[5] == 0x01)) //Get Discrete Input 20000
   {
	   uint16_t startadress = 0x2000;
	   uint16_t endadress = 0x2000;
	   int ret = 0;
	   result[ret] = 0x01;
	   ret++;
	   result[ret] = 0x02;
	   ret++;
	   result[ret] = 0x01;
	   ret++;
	   int ret2 = GetDiscInp(startadress, endadress, (result + ret), (resultsize - ret));
	   return (ret + ret2);
   }
   
// Ignore shwitch off the discharging power like pressing [OK} on the Display   
   else if ((req[0] == 0x01) && (req[1] == 0x0F) && (req[2] == 0x00) && (req[3] == 0x02) && (req[4] == 0x00) && (req[5] == 0x01) && (req[6] == 0x01) && (req[7] == 0x00)) //switch off discharging power like on pressing [OK]
   {
	   LOG ("cDisplay Ignoring switch off discharging power\n");
	   return 0;
   }
   else
   {
	   int n = 0;
	   char tmp[100];
	   std::string reqbuffer;
	   for (n = 0; n < reqlenght; n++)
	   {
		   std::sprintf(tmp, "[%02X]", req[n]);
		   reqbuffer.append(tmp);
		   tmp[0]='\0';
	   }
	   
	   LOG ("cDisplay Unknown Request: %s\n", reqbuffer.c_str());
	   reqbuffer.clear();
	   return -2;
   }
   
   return 0;
}


void cMemory::cleanalternate()
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	int n;
	for (n = 0; n < MAXREG; n++)
     {
		 Register[n].HaveAlternateValue = false;
		 Register[n].AlternateValue = 0xFFFF;
	 }
}

void cMemory::clean()
{
	std::lock_guard<std::mutex> lock(mem_mtx);

   int n, m;

   for (n = 0; n < MAXREG; n++)
     {
	Register[n].empty = true;
	Register[n].HaveHeader = false;
	Register[n].HaveAlternateValue = false;
	Register[n].Adress = 0xFFFF;
	Register[n].Value = 0xFFFF;
	Register[n].AlternateValue = 0xFFFF;
	Register[n].Headersize = 0;
     }
   n = 0;
   while (RegList[n] != 0xFFFF)
     {
	if (n == MAXREG)
	  break;
	Register[n].Adress = RegList[n];
	Register[n].empty = false;
	if (Register[n].Adress == 0x3110 || Register[n].Adress == 0x3111 || Register[n].Adress == 0x3112 
	|| Register[n].Adress == 0x311B || Register[n].Adress == 0x331B || Register[n].Adress == 0x331C 
	|| Register[n].Adress == 0x331D || Register[n].Adress == 0x331E || Register[n].Adress == 0x9018)
	{
		Register[n].issigned = true;
	}
	else
	{
		Register[n].issigned = false;
	}
		
	
	n++;

     }

   for (n = 0; n < MAXDISCRETEINPUTS; n++)
     {

	DiscreteInput[n].empty = true;
	DiscreteInput[n].Adress = 0xFFFF;
	DiscreteInput[n].Value = 0xFF;
     }
   n = 0;
   while (DiscreteInputList[n] != 0xFFFF)
     {
	if (n == MAXDISCRETEINPUTS)
	  break;
	DiscreteInput[n].Adress = DiscreteInputList[n];
	DiscreteInput[n].empty = false;
	n++;
     }
   

   for (n = 0; n < MAXCOILS; n++)
     {
	Coil[n].empty = true;
	Coil[n].Adress = 0xFFFF;
	Coil[n].Value = 0xFF;
     }
   n = 0;
   while (CoilList[n] != 0xFFFF)
     {
	if (n == MAXCOILS)
	  break;
	Coil[n].Adress = CoilList[n];
	Coil[n].empty = false;
	n++;
     }
   
   for (n = 0; n < MAX_DEV_INFO_LENGHT; n++)
     {
	DeviceInfo1[n] = 0xFF;
     }
   for (n = 0; n < MAX_DEV_INFO_LENGHT; n++)
     {
	DeviceInfo2[n] = 0xFF;
     }
	 AddRegisterProperties();
	 
	 if (UserRegister.size() > 0)
	 {
		 UserRegister.clear();
	 }
	 if (UserCoil.size() > 0)
	 {
		 UserCoil.clear();
	 }
	 if (UserSwitch.size() > 0)
	 {
		 UserSwitch.clear();
	 }
	 if (!memoryfile.empty())
	 {
		 readconfig(memoryfile, false);
	 }
}

uint16_t cMemory::GetRegisterAdress(int index, bool lock)
{
	if (lock)
		std::lock_guard<std::mutex> lock(mem_mtx);
   if (index >= MAXREG)
     {
	LOG("cMemory: Error Get Register Adress Index out of range\n");
	return 0;
     }
   else
     {
	return Register[index].Adress;
     }
}

   

int cMemory::GetRegStartIndex(uint16_t registeradress)
{
   if (registeradress == 0x0000)
     return 0;
   int n;
   for (n = 0; n < MAXREG; n++)
     {
	if (Register[n].Adress  == registeradress)
	  {
	     return n;
	  }
	else if (Register[n].empty == true)
	  break;
	else
	  {
	     continue;
	  }
     }
   return -1;
}

int cMemory::GetDiscStartIndex(uint16_t discadress)
{
   if (discadress == 0X0000)
     return 0;
   int n;
   for (n = 0; n < MAXDISCRETEINPUTS; n++)
     {
	if (DiscreteInput[n].Adress == discadress)
	  {
	     return n;
	  }
	else if (DiscreteInput[n].empty == true)
	  break;
	else
	  {
	     continue;
	  }
     }
   return -1;
}

int cMemory::GetCoilStartIndex(uint16_t coiladress)
{
   if (coiladress == 0x0000)
     return 0;
   int n;
   for (n = 0; n < MAXCOILS; n++)
     {
	if (Coil[n].Adress == coiladress)
	  {
	     return n;
	  }
	else if (Coil[n].empty == true)
	  break;
	else
	  {
	     continue;
	  }
     }
   return -1;
}


void cMemory::AddRegisterProperties()
{
	int n;
	for (n = 0; n < MAXREG; n++)
	{
		if (Register[n].Adress == 0xFFFF)
			break;
		
		if (Register[n].Adress == 0x3000)
		{
			Register[n].RegName = "Charging equipment rated input voltage";
			//Register[n].Description = Register[n].RegName;
			//Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		
		else if (Register[n].Adress == 0x3001)
		{
			Register[n].RegName = "Charging equipment rated input current";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3002)
		{
			Register[n].RegName = "Charging equipment rated input power L";
			Register[n].Description = "Charging equipment rated input power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3003)
		{
			Register[n].RegName = "Charging equipment rated input power H";
			Register[n].Description = "Charging equipment rated input power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3004)
		{
			Register[n].RegName = "Charging equipment rated output voltage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3005)
		{
			Register[n].RegName = "Charging equipment rated output current";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3006)
		{
			Register[n].RegName = "Charging equipment rated output power L";
			Register[n].Description = "Charging equipment rated output power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3007)
		{
			Register[n].RegName = "Charging equipment rated output power H";
			Register[n].Description = "Charging equipment rated output power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3008)
		{
			Register[n].RegName = "Charging mode";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x300E)
		{
			Register[n].RegName = "Rated output current of load";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3100)
		{
			Register[n].RegName = "Charging equipment input voltage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3101)
		{
			Register[n].RegName = "Charging equipment input current";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3102)
		{
			Register[n].RegName = "Charging equipment input power L";
			Register[n].Description = "Charging equipment input power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3103)
		{
			Register[n].RegName = "Charging equipment input power H";
			Register[n].Description = "Charging equipment input power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3104)
		{
			Register[n].RegName = "Charging equipment output voltage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3105)
		{
			Register[n].RegName = "Charging equipment output current";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3106)
		{
			Register[n].RegName = "Charging equipment output power L";
			Register[n].Description = "Charging equipment output power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3107)
		{
			Register[n].RegName = "Charging equipment output power H";
			Register[n].Description = "Charging equipment output power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x310C)
		{
			Register[n].RegName = "Disharging equipment output voltage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x310D)
		{
			Register[n].RegName = "Disharging equipment output current";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x310E)
		{
			Register[n].RegName = "Disharging equipment output power L";
			Register[n].Description = "Disharging equipment output power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x310F)
		{
			Register[n].RegName = "Disharging equipment output power H";
			Register[n].Description = "Disharging equipment output power";
			Register[n].Unit = "W";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3110)
		{
			Register[n].RegName = "Battery Temperature";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3111)
		{
			Register[n].RegName = "Temperature inside equipment";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3112)
		{
			Register[n].RegName = "Power components temperature";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x311A)
		{
			Register[n].RegName = "Batterie SOC";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x311B)
		{
			Register[n].RegName = "Remote battery temperature";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x311D)
		{
			Register[n].RegName = "Battery real rated power";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3200)
		{
			Register[n].RegName = "Battery status";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3201)
		{
			Register[n].RegName = "Charging equipment status";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3300)
		{
			Register[n].RegName = "Maximum input volt (PV) today";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3301)
		{
			Register[n].RegName = "Minimum input volt (PV) today";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3302)
		{
			Register[n].RegName = "Maximum battery volt today";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3303)
		{
			Register[n].RegName = "Minimum battery volt today";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3304)
		{
			Register[n].RegName = "Consumed energy today L";
			Register[n].Description = "Consumed energy today";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3305)
		{
			Register[n].RegName = "Consumed energy today H";
			Register[n].Description = "Consumed energy today";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3306)
		{
			Register[n].RegName = "Consumed energy this month L";
			Register[n].Description = "Consumed energy this month";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3307)
		{
			Register[n].RegName = "Consumed energy this month H";
			Register[n].Description = "Consumed energy this month";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3308)
		{
			Register[n].RegName = "Consumed energy this year L";
			Register[n].Description = "Consumed energy this year";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3309)
		{
			Register[n].RegName = "Consumed energy this year H";
			Register[n].Description = "Consumed energy this year";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x330A)
		{
			Register[n].RegName = "Total consumed energy L";
			Register[n].Description = "Total consumed energy";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x330B)
		{
			Register[n].RegName = "Total consumed energy H";
			Register[n].Description = "Total consumed energy";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x330C)
		{
			Register[n].RegName = "Generated energy today L";
			Register[n].Description = "Generated energy today";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x330D)
		{
			Register[n].RegName = "Generated energy today H";
			Register[n].Description = "Generated energy today";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x330E)
		{
			Register[n].RegName = "Generated energy this month L";
			Register[n].Description = "Generated energy this month";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x330F)
		{
			Register[n].RegName = "Generated energy this month H";
			Register[n].Description = "Generated energy this month";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3310)
		{
			Register[n].RegName = "Generated energy this year L";
			Register[n].Description = "Generated energy this year";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3311)
		{
			Register[n].RegName = "Generated energy this year H";
			Register[n].Description = "Generated energy this year";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3312)
		{
			Register[n].RegName = "Total generated energy L";
			Register[n].Description = "Total generated energy";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3313)
		{
			Register[n].RegName = "Total generated energy H";
			Register[n].Description = "Total generated energy";
			Register[n].Unit = "KWH";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3314)
		{
			Register[n].RegName = "Carbon dioxide reduction L";
			Register[n].Description = "Carbon dioxide reduction";
			Register[n].Unit = "Ton";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3315)
		{
			Register[n].RegName = "Carbon dioxide reduction H";
			Register[n].Description = "Carbon dioxide reduction";
			Register[n].Unit = "Ton";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x331B)
		{
			Register[n].RegName = "Battery Net Current L";
			Register[n].Description = "Battery Net Current";
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x331C)
		{
			Register[n].RegName = "Battery Net Current H";
			Register[n].Description = "Battery Net Current";
			Register[n].Unit = "A";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x331D)
		{
			Register[n].RegName = "Battery Temp";
			Register[n].Description = "Battery Temp";
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x331E)
		{
			Register[n].RegName = "Ambient Temp";
			Register[n].Description = "Ambient Temp";
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x3330)
		{
			Register[n].RegName = "Last Update Day";
			Register[n].Description = "Last Update Day";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3331)
		{
			Register[n].RegName = "Last Update Month";
			Register[n].Description = "Last Update Month";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3332)
		{
			Register[n].RegName = "Last Update Year";
			Register[n].Description = "Last Update Year";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3333)
		{
			Register[n].RegName = "Last Update Hour";
			Register[n].Description = "Last Update Hour";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3334)
		{
			Register[n].RegName = "Last Update minute";
			Register[n].Description = "Last Update minute";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3335)
		{
			Register[n].RegName = "Last Update second";
			Register[n].Description = "Last Update second";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3336)
		{
			Register[n].RegName = "Last Update Unix Timestamp L";
			Register[n].Description = "Last Update Unix Timestamp L";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x3337)
		{
			Register[n].RegName = "Last Update Unix Timestamp H";
			Register[n].Description = "Last Update Unix Timestamp H";
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x900A)
		{
			Register[n].RegName = "Low voltage reconnect";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x900B)
		{
			Register[n].RegName = "Under voltage recover";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x900C)
		{
			Register[n].RegName = "Under voltage warning";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x900D)
		{
			Register[n].RegName = "Low voltage reconect";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x900E)
		{
			Register[n].RegName = "Discharging limit voltage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9013)
		{
			Register[n].RegName = "Real time clock";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9014)
		{
			Register[n].RegName = "Real time clock";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9015)
		{
			Register[n].RegName = "Real time clock";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9016)
		{
			Register[n].RegName = "Equalization charging cycle";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9017)
		{
			Register[n].RegName = "Battery temperature warning upper limit";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9018)
		{
			Register[n].RegName = "Battery temperature warning lower limit";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9019)
		{
			Register[n].RegName = "Controller inner temperature upper limit";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x901A)
		{
			Register[n].RegName = "Controller inner temperature upper limit recover";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x901B)
		{
			Register[n].RegName = "Power component temperature upper limit";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x901C)
		{
			Register[n].RegName = "Power component temperature upper limit recover";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "C";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x901D)
		{
			Register[n].RegName = "Line Impedance";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "miliohm";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x901F)
		{
			Register[n].RegName = "";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x901F)
		{
			Register[n].RegName = "Light signal startup (night) delay time";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9020)
		{
			Register[n].RegName = "Day Time Threshold Volt.(DTTV)";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "V";
			Register[n].Multiplier = 100;
		}
		else if (Register[n].Adress == 0x9021)
		{
			Register[n].RegName = "Light signal turn off(day) delay time";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x903D)
		{
			Register[n].RegName = "Load controling modes";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x903E)
		{
			Register[n].RegName = "Working time length 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x903F)
		{
			Register[n].RegName = "Working time length 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9042)
		{
			Register[n].RegName = "Turn on timing 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "sec";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9043)
		{
			Register[n].RegName = "Turn on timing 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9044)
		{
			Register[n].RegName = "Turn on timing 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "hour";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9045)
		{
			Register[n].RegName = "Turn off timing 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "sec";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9046)
		{
			Register[n].RegName = "Turn off timing 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9047)
		{
			Register[n].RegName = "Turn off timing 1";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "hour";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9048)
		{
			Register[n].RegName = "Turn on timing 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "sec";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9049)
		{
			Register[n].RegName = "Turn on timing 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x904A)
		{
			Register[n].RegName = "Turn on timing 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "hour";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x904B)
		{
			Register[n].RegName = "Turn off timing 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "sec";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x904C)
		{
			Register[n].RegName = "Turn off timing 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x904D)
		{
			Register[n].RegName = "Turn off timing 2";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "hour";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9065)
		{
			Register[n].RegName = "Lenght of night";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9067)
		{
			Register[n].RegName = "Battery rated voltage code";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9069)
		{
			Register[n].RegName = "Load timing control selection";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x906A)
		{
			Register[n].RegName = "Default Load On/Off in manual mode";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x906B)
		{
			Register[n].RegName = "Equalize duration";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x906C)
		{
			Register[n].RegName = "Boost duration";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "min";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x906D)
		{
			Register[n].RegName = "Discharging percentage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "%";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x906E)
		{
			Register[n].RegName = "Charging percentage";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "%";
			Register[n].Multiplier = 1;
		}
		else if (Register[n].Adress == 0x9070)
		{
			Register[n].RegName = "Management modes of battery charging and discharging";
			Register[n].Description = Register[n].RegName;
			Register[n].Unit = "";
			Register[n].Multiplier = 1;
		}
		
}
}


int cMemory::GetRegister(uint16_t startadress, uint16_t endadress, uint16_t *reg, int reglenght, bool alternate)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   int startindex = GetRegStartIndex(startadress);
   if (startindex < 0)
     {
	LOG("cMemory: Register Start Adress not found: <%04X>\n",startadress);
	return -1;
     }
   
   int index, n;
   n = 0;
   for (index = startindex; index < MAXREG; index++)
	{
	   if (Register[index].Adress > endadress || Register[index].empty == true)
	     break;
	   else if  (n >= reglenght)
	     {
		LOG("cMemmory: Array *reg to short\n");
		return -1;
	     }
	   
	   else
	     {
			if (alternate && Register[index].HaveAlternateValue)
				reg[n] = Register[index].AlternateValue;
			else
				reg[n] = Register[index].Value;
		n++;
	     }
	   
	}
   
	     
   return n;
}

int cMemory::GetRegister(uint16_t startadress, uint16_t endadress, uint8_t *reg, int reglenght, bool alternate)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   int startindex = GetRegStartIndex(startadress);
   if (startindex < 0)
     {
	LOG("cMemory: Registe Start Adress not found\n");
	return -1;
     }
   
   int index, n;
   n = 0;
   for (index = startindex; index < MAXREG; index++)
	{
	   if (Register[index].Adress > endadress || Register[index].empty == true)
	     break;
	   else if  (n >= reglenght)
	     {
		LOG("cMemmory: Array *reg to short\n");
		return -1;
	     }
	   
	   else
	     {
			if (alternate && Register[index].HaveAlternateValue)
				MODBUS_SET_INT16_TO_INT8(reg, n, Register[index].AlternateValue);
			else
				MODBUS_SET_INT16_TO_INT8(reg, n, Register[index].Value);
		n=n+2;
	     }
	   
	}
   
	     
   return n;
}

int cMemory::GetUserRegister(uint16_t startadress, uint16_t endadress, uint16_t *reg, int reglenght, uint16_t *firstadress)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   int numuserreg;
   bool beginningfound = false;
   numuserreg = UserRegister.size();
   
   int index, n;
   n = 0;
   for (index = 0; index < numuserreg; index++)
	{
		if  (n >= reglenght)
		{
			LOG("cMemmory: Array *reg to short\n");
			return -1;
		}
		if (UserRegister.at(index).Adress >= startadress && UserRegister.at(index).Adress <= endadress)
		{
			if (!beginningfound)
			{
				*firstadress = UserRegister.at(index).Adress;
				beginningfound = true;
			}
			reg[n] = UserRegister.at(index).Value;
			n++;
		}
	}
	return n;
}

int cMemory::GetCoil(uint16_t startadress, uint16_t endadress, uint8_t *coil, int coillenght)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   int startindex = GetCoilStartIndex(startadress);
   if (startindex < 0)
     {
	LOG("cMemory: Coil Start Adress not found\n");
	return -1;
     }
   
   int index, n;
   n = 0;
   for (index = startindex; index < MAXCOILS; index++)
     {
	if (Coil[index].Adress > endadress || Coil[index].empty == true)
	  break;
	else if (n >= coillenght)
	  {
	     LOG("cMemmory: Array *coil to short\n");
	     return -1;
	  }
	else
	  {
	     coil[n] = Coil[index].Value;
	     n++;
	  }
     }
   return n;
}

int cMemory::GetUserCoil(uint16_t startadress, uint16_t endadress, uint8_t *coil, int coillenght, uint16_t *firstadress)

{
	std::lock_guard<std::mutex> lock(mem_mtx);
	int numusercoil;
   bool beginningfound = false;
   numusercoil = UserCoil.size();
   
   int index, n;
   n = 0;
   for (index = 0; index < numusercoil; index++)
	{
		if  (n >= coillenght)
		{
			LOG("cMemmory: Array *coil to short\n");
			return -1;
		}
		if (UserCoil.at(index).Adress >= startadress && UserCoil.at(index).Adress <= endadress)
		{
			if (!beginningfound)
			{
				*firstadress = UserCoil.at(index).Adress;
				beginningfound = true;
			}
			coil[n] = UserCoil.at(index).Value;
			n++;
		}
	}
	return n;
	

}



int cMemory::GetDiscInp(uint16_t startadress, uint16_t endadress, uint8_t *discinp, int discinplenght)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   int startindex = GetDiscStartIndex(startadress);
   if (startindex < 0)
     {
	LOG("cMemory: Discrete Input Start Adress not found\n");
	return -1;
     }
   
   int index, n;
   n = 0;
   for (index = startindex; index < MAXDISCRETEINPUTS; index++)
     {
	if ((DiscreteInput[index].Value > endadress) || (DiscreteInput[index].empty == true))
	  break;
	else if (n >= discinplenght)
	  {
	     LOG("cMemmory: Array *discinp to short\n");
	     return -1;
	  }
	else
	  {
	     discinp[n] = DiscreteInput[index].Value;
	     n++;
	  }
     }
   return n;
   
}

int cMemory::GetHeader(uint16_t adress, int index, uint8_t *header, int headerlenght)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   if (index > -1)
     {
	if (index >= MAXREG)
	  {
	     LOG("cMemory: Header Index Out of range\n");
	     return -1;
	  }
	else if (Register[index].empty == true || Register[index].HaveHeader == false)
	  return 0;
	else
	  {
	     int n = 0; 
//	     int m = 0;
	     int lenght = Register[index].Headersize;
	     for (n=0; n<lenght; n++)
	       {
		  if (n > headerlenght)
		    {
		       LOG("cMemory: Array for Header to short!\n");
		       return -1;
		    }
		  
		  header[n] = Register[index].Header[n];
	       }
	     return (n);
	  }
     }
   else if (adress < 0xFFFF)
     {
	int index = GetRegStartIndex(adress);
	if (index < 0)
	  {
	     LOG("cMemory: Header Adress not found\n");
	     return -1;
	  }
	else if (Register[index].HaveHeader == false)
	  return 0;
	int lenght = Register[index].Headersize;
	int n = 0;
//	int m = 0;
	for (n=0; n < lenght; n++)
	  {
	     if (n >= headerlenght)
	       {
	           LOG("cMemory: Array for Header to short!\n");
	           return -1;
	       }
	     
	     header[n] = Register[index].Header[n];
	  }
	return (n);
     }
   else
     {
	LOG("cMemory: Error Get Header wrong parameter\n"); 
	return -1;
     }
}

int cMemory::GetDeviceInfo(int type, uint8_t *deviceinfo, int deviceinfolenght)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   if (type == 1)
     {
	int n = 0;
	for (n=0; n < DeviceInfo1Lenght; n++)
	  {
	     if (n >= deviceinfolenght)
	       {
		  LOG("cMemory: Array *deviceinfo to short\n");
		  return -1;
	       }
	     deviceinfo[n] = DeviceInfo1[n];
	  }
	return DeviceInfo1Lenght;
     }
   else if (type == 2)
     {
	int n = 0;
	for (n=0; n < DeviceInfo2Lenght; n++)
	  {
	     if (n >= deviceinfolenght)
	       {
		  LOG("cMemory: Array *deviceinfo to short\n");
		  return -1;
	       }
	     deviceinfo[n] = DeviceInfo2[n];
	  }
	return DeviceInfo2Lenght;
     }
   else
     {
	LOG("cMemory: GetDeviceInfo unknown type\n");
	return -1;
     }
   
}


std::string cMemory::GetRegStr (uint16_t adress, bool orig)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	int n = -1;
	std::string value;
	
	if (adress >= MINUSERREG && adress <= MAXUSERREG)
	{
		int numuserreg;
		numuserreg = UserRegister.size();
		if (numuserreg >0)
		{
			int n;
			for (n=0; n<numuserreg; n++)
			{
				if (UserRegister.at(n).Adress == adress)
				{
					if (!UserRegister.at(n).issigned)
					{
						try
							{
								value = std::to_string(UserRegister.at(n).Value);
							}
						catch (...)
							{
								LOG("cMemmory::GetRegStr: Error Convert to string");
							}
					}
					else
					{
						int16_t tmp;
						std::memcpy(&tmp, &UserRegister.at(n).Value, sizeof(tmp));
						try
						{
							value = std::to_string(tmp);
						}
						catch (...)
							{
								LOG("cMemmory::GetRegStr: Error Convert to string");
							}
					}
					break;
				}
			}
		}
	}
	else
	{
		if (adress > 0)
			n = GetRegStartIndex(adress);
		if (n >= 0)
		{
			if (!Register[n].issigned)
			{
				try
				{
				if (Register[n].HaveAlternateValue && !orig)
					value = std::to_string(Register[n].AlternateValue);
				else
					value = std::to_string(Register[n].Value);
				}
				catch (...)
				{
					LOG("cMemmory::GetRegStr: Error Convert to string");
				}
			}
			else
			{
				int16_t tmp;
				if (Register[n].HaveAlternateValue && !orig)
					std::memcpy(&tmp, &Register[n].AlternateValue, sizeof(tmp));
				else
					std::memcpy(&tmp, &Register[n].Value, sizeof(tmp));
				try
				{
					value = std::to_string(tmp);
			   
				}
				catch (...)
				{
					LOG("cMemmory::GetRegStr: Error Convert to string");
				}
			}
		}
	}
	value.append(1, '\n');
	return value;
}

std::string cMemory::GetCoilStr (uint16_t adress)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	int n = -1;
	std::string value;
	
	if (adress >= MINUSERCOIL && adress <= MAXUSERCOIL)
	{
		int numusercoil;
		numusercoil = UserCoil.size();
		if (numusercoil >0)
		{
			int n;
			for (n=0; n<numusercoil; n++)
			{
				if (UserCoil.at(n).Adress == adress)
				{
					
					try
						{
							value = std::to_string(UserCoil.at(n).Value);
						}
					catch (...)
						{
							LOG("cMemmory::GetCoilStr: Error Convert to string");
						}
					break;
				}
			}
		}
	}
	value.append(1, '\n');
	return value;
}
	
std::string cMemory::GetRegStr32 (uint16_t adress, bool orig)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	int n[2] = {-1, -1};
	std::string value;
	
	if (adress >= MINUSERREG && adress <= MAXUSERREG)
	{
		int numuserreg;
		numuserreg = UserRegister.size();
		uint16_t tmp[2];
		uint32_t tmp32;
		int i, f;
		bool Signed = false;
		f = 0;
		for (i=0; i<numuserreg; i++)
		{
			if (UserRegister.at(i).Adress == adress)
			{
				tmp[1] = UserRegister.at(i).Value; //low Byte second for the libmodbus Macro
				Signed = UserRegister.at(i).issigned; 
				f++;
			}
			else if (UserRegister.at(i).Adress == (adress+1))
			{
				tmp[0] = UserRegister.at(i).Value; //hight Byte first for the libmodbus Macro
				Signed = UserRegister.at(i).issigned;
				f++;
			}
			if (f == 2)
				break;
		}
		tmp32 = MODBUS_GET_INT32_FROM_INT16(tmp, 0);
		if (!Signed && f == 2)
		{
			try
			{
				   value = std::to_string(tmp32);
			}
			catch (...)
			{
				LOG("cMemmory::GetRegStr32: Error Convert to string");
			}
		}
		else if (Signed && f == 2)
		{
			int32_t tmpsigned;
			std::memcpy(&tmpsigned, &tmp32, sizeof(tmpsigned));
			try
			{
				value = std::to_string(tmpsigned);
			   
			}
			catch (...)
			{
				LOG("cMemmory::GetRegStr32: Error Convert to string");
			}
		}
	}
	
	else
	{
		if (adress > 0)
		{
			n[1] = GetRegStartIndex(adress); //low Byte second for the libmodbus Macro
			n[0] = GetRegStartIndex((adress+1));  //hight Byte first for the libmodbus Macro
		}
		if (n[0] >= 0  && n[1] >= 0)
		{
			uint16_t tmp[2];
			uint32_t tmp32;
			if (Register[n[0]].HaveAlternateValue && Register[n[1]].HaveAlternateValue && !orig)
			{
				tmp[0] = Register[n[0]].AlternateValue;
				tmp[1] = Register[n[1]].AlternateValue;
			}
			else
			{
				tmp[0] = Register[n[0]].Value;
				tmp[1] = Register[n[1]].Value;
			}
			tmp32 = MODBUS_GET_INT32_FROM_INT16(tmp, 0);
			if (!Register[n[0]].issigned && !Register[n[1]].issigned)
			{
				try
				{
					value = std::to_string(tmp32);
				}
				catch (...)
				{
					LOG("cMemmory::GetRegStr32: Error Convert to string");
				}
			}
			else
			{
				int32_t tmpsigned;
				std::memcpy(&tmpsigned, &tmp32, sizeof(tmpsigned));
				try
				{
					value = std::to_string(tmpsigned);
			   
				}
				catch (...)
				{
					LOG("cMemmory::GetRegStr32: Error Convert to string");
				}
			}
		}
	}
	value.append(1, '\n');
	return value;
}
	

int cMemory::PutRegStr (std::string value, uint16_t adress)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	if (adress >= MINUSERREG && adress <= MAXUSERREG)
	{
		int i;
		uint16_t tmp;
		int numuserreg;
		numuserreg = UserRegister.size();
		for (i=0; i<numuserreg; i++)
		{
			if (UserRegister.at(i).Adress == adress)
			{
				try
				{
					tmp = std::stoi(value, NULL, 10);
				}
				catch (...)
				{
					LOG("cMemmory::PutRegStr: Error Convert string to int");
					return -1;
				}
				UserRegister.at(i).Value = tmp;
				return 1;
			}
		}
	}
	else
	{
		int n = -1;
		uint16_t tmp;
		if (adress > 0)
			n = GetRegStartIndex(adress);
		if (n >= 0)
		{
			try
			{
				tmp = std::stoi(value, NULL, 10);
			}
			catch (...)
			{
				LOG("cMemmory::PutRegStr: Error Convert string to int");
				return -1;
			}
			Register[n].HaveAlternateValue = true;
			Register[n].AlternateValue = tmp;
			return 1;
		}
	}
		return -1;
}	

int cMemory::PutRegStr32 (std::string value, uint16_t adress)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	if (adress >= MINUSERREG && adress <= MAXUSERREG)
	{
		uint32_t tmp;
		uint16_t tmp16[2];
		int i;
		int numuserreg;
		int idx = -1;
		numuserreg = UserRegister.size();
		for (i=0; i<numuserreg; i++)
		{
			if (UserRegister.at(i).Adress == adress)
			{
				idx = i;
				if ((idx+1) < numuserreg)
					break;
				else
					return -1;
			}
		}
		if (idx >= 0)
		{
			try
			{
				tmp = std::stoi(value, NULL, 10);
			}
			catch (...)
			{
				LOG("cMemmory::PutRegStr32: Error Convert string to int");
				return -1;
			}
			MODBUS_SET_INT32_TO_INT16(tmp16, 0, tmp);
			UserRegister.at(idx).Value = tmp16[1]; //Low Byte
			idx++;
			UserRegister.at(idx).Value = tmp16[0]; //High Byte
			return 1;
		}
	}
	
	else
	
	{
		int n = -1;
		uint32_t tmp;
		uint16_t tmp16[2];
		if (adress > 0)
			n = GetRegStartIndex(adress);
		if (n >= 0 && (GetRegStartIndex((adress+1))) >= 0)
		{
			try
			{
				tmp = std::stoi(value, NULL, 10);
			}
			catch (...)
			{
				LOG("cMemmory::PutRegStr32: Error Convert string to int");
				return -1;
			}
			MODBUS_SET_INT32_TO_INT16(tmp16, 0, tmp);
			Register[n].HaveAlternateValue = true;
			Register[n].AlternateValue = tmp16[1];  //Low Byte
			n++;
			Register[n].HaveAlternateValue = true;
			Register[n].AlternateValue = tmp16[0]; //high Byte
			return 1;
		}
	}
		return -1;
}

int cMemory::PutCoilStr (std::string value, uint16_t adress)
{
	std::lock_guard<std::mutex> lock(mem_mtx);
	if (adress >= MINUSERCOIL && adress <= MAXUSERCOIL)
	{
		int i;
		uint8_t tmp;
		int numusercoil;
		numusercoil = UserCoil.size();
		for (i=0; i<numusercoil; i++)
		{
			if (UserCoil.at(i).Adress == adress)
			{
				try
				{
					tmp = std::stoi(value, NULL, 10);
				}
				catch (...)
				{
					LOG("cMemmory::PutRegStr: Error Convert string to int");
					return -1;
				}
				UserCoil.at(i).Value = tmp;
				return 1;
			}
		}
	}
	return -1;
}

	
std::string cMemory::DebugLogMemory()
{
	std::lock_guard<std::mutex> lock(mem_mtx);
   std::string debugmem;
   const int tmplen = 1024;
   char tmp[tmplen];
   tmp[0] = '\0';
   int n, m;
   for (n = 0; n < MAXREG; n++)
       {
	  if (Register[n].issigned)
	  {
	     snprintf(tmp, tmplen, "Register %04X = %04X signed\n", Register[n].Adress, Register[n].Value);
	     debugmem.append(tmp);
	     tmp[0] = '\0';
		 if (Register[n].HaveAlternateValue)
		 {
			snprintf(tmp, tmplen, "Register %04X Alternate Value = %04X signed\n", Register[n].Adress, Register[n].AlternateValue);
			debugmem.append(tmp);
			tmp[0] = '\0';
		 }
	  }
	  else
	  {
		 snprintf(tmp, tmplen, "Register %04X = %04X\n", Register[n].Adress, Register[n].Value);
	     debugmem.append(tmp);
	     tmp[0] = '\0';
		 if (Register[n].HaveAlternateValue)
		 {
			snprintf(tmp, tmplen, "Register %04X Alternate Value = %04X\n", Register[n].Adress, Register[n].AlternateValue);
			debugmem.append(tmp);
			tmp[0] = '\0';
		 }
	  }
       }
   for (n = 0; n < MAXREG; n++)
     {
	if (Register[n].HaveHeader)
	  {
	     
	     snprintf(tmp, tmplen, "Register Header for Adress %04X with lenght %d:", Register[n].Adress, Register[n].Headersize);
		 debugmem.append(tmp);
	     tmp[0] = '\0';
		 
	     for (m = 0; m < MAXHEADERSIZE; m++)
	         {
	           snprintf (tmp, tmplen, " %02X", Register[n].Header[m]);
			   debugmem.append(tmp);
	           tmp[0] = '\0';
	         }
	      debugmem.append(1, '\n');
	  }
	
     }
   for (n = 0; n < MAXDISCRETEINPUTS; n++)
     {
	snprintf(tmp, tmplen, "Discrete Input %04X = %02X\n", DiscreteInput[n].Adress, DiscreteInput[n].Value);
	debugmem.append(tmp);
	tmp[0] = '\0';

     }
   for (n = 0; n < MAXCOILS; n++)
     {
        snprintf(tmp, tmplen, "Coil %04X = %02X\n", Coil[n].Adress, Coil[n].Value);
		debugmem.append(tmp);
	    tmp[0] = '\0';

      }
   snprintf(tmp, tmplen, "Device Info1: Lenght = %d = ", DeviceInfo1Lenght);
   debugmem.append(tmp);
   tmp[0] = '\0';
   
   for (n = 0; n < MAX_DEV_INFO_LENGHT; n++)
     {
	snprintf(tmp, tmplen, " %02X", DeviceInfo1[n]);
	debugmem.append(tmp);
	tmp[0] = '\0';
     }
   debugmem.append(1, '\n');
   
   snprintf(tmp, tmplen, "Device Info2: Lenght = %d = ", DeviceInfo2Lenght);
   debugmem.append(tmp);
   tmp[0] = '\0';
   
   for (n = 0; n < MAX_DEV_INFO_LENGHT; n++)
     {
	 snprintf(tmp, tmplen, " %02X", DeviceInfo2[n]);
	 debugmem.append(tmp);
	 tmp[0] = '\0';
     }
   
      debugmem.append(1, '\n');
	  
	  int numuserreg;
	  numuserreg = UserRegister.size();
	  if (numuserreg > 0)
	  {
		  int n;
		  for (n=0; n<numuserreg; n++)
		  {
			  snprintf(tmp, tmplen, "UserRegister %04X = %04X", UserRegister.at(n).Adress, UserRegister.at(n).Value);
			  debugmem.append(tmp);
			  if (UserRegister.at(n).issigned == true)
				  debugmem.append(" signed\n");
			  else
				  debugmem.append(1, '\n');
		  }
	  }
		
	  int numusercoil;
	  numusercoil = UserCoil.size();
	  if (numusercoil > 0)
	  {
		  int n;
		  for (n=0; n<numusercoil; n++)
		  {
			  snprintf(tmp, tmplen, "UserCoil %04X = %02X\n", UserCoil.at(n).Adress, UserCoil.at(n).Value);
			  debugmem.append(tmp);
			  
		  }
	  }		
	  
	  return debugmem;
}
	
std::string cMemory::GetRegListStr()
{
   std::lock_guard<std::mutex> lock(mem_mtx);
   std::string reglist;
   const int tmplen = 1024;
   char tmp[tmplen];
   tmp[0] = '\0';
   int n;
   for (n = 0; n < MAXREG; n++)
	  {
		  if (Register[n].Adress == 0xFFFF)
			  break;
	     snprintf(tmp, tmplen, "%04X	%s\n", Register[n].Adress, Register[n].RegName.c_str());
	     reglist.append(tmp);
	     tmp[0] = '\0';
	  }
	  int numuserreg;
	  numuserreg = UserRegister.size();
	  if (numuserreg > 0)
	  {
		  reglist.append("\nUser Registers:\n");
		  for (n=0; n<numuserreg; n++)
		  {
			  snprintf(tmp, tmplen, "%04X	%s\n", UserRegister.at(n).Adress, UserRegister.at(n).RegName.c_str());
			  reglist.append(tmp);
			  tmp[0] = '\0';
		  }
	  }
	  return reglist;
}
	
bool cMemory::RegisterHaveAlternateValue (uint16_t adress)
{
	int idx;
	idx = GetRegStartIndex(adress);
	if (idx >= 0)
		return Register[idx].HaveAlternateValue;
	else
		return false;
}

void cMemory::addconfdir(std::string confdir)
{
	std::string file = confdir;
	if (file.back() != '/')
		file.append("/");
	file.append("memory.conf");
	memoryfile = file;
	readconfig(memoryfile);
	
}

void cMemory::readconfig(std::string conffile, bool lock)
{
	if (lock)
		std::lock_guard<std::mutex> lock(mem_mtx);
	int type = 0; //1 = Register 2 = Coil 3 = Switch
	uint16_t address = 0;
	int issigned = 0; //1 = False 2 = True
	std::string description;
	std::string oncommand;
	std::string offcommand;
	std::string switchname;
	std::string regname;
	std::string coilname;
    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile (conffile.c_str());
    if (cFile.is_open())
    {
        std::string line;
		std::size_t delimiterPos;
		std::string name;
		std::string value;
        while(getline(cFile, line)){
            //line.erase(std::remove_if(line.begin(), line.end(), isspace),
            //                     line.end());
			
				
			delimiterPos = line.find("=");
			if (delimiterPos != std::string::npos)
			{
				name = line.substr(0, delimiterPos);
				value = line.substr(delimiterPos + 1);
				name.erase(std::remove_if(name.begin(), name.end(), isspace),
                                 name.end());
				if ((name.compare("description") != 0) && (name.compare("oncommand") != 0) && (name.compare("offcommand") != 0))
				{
					value.erase(std::remove_if(value.begin(), value.end(), isspace),
                                 value.end());
				}
			}
			else
			{
				line.erase(std::remove_if(line.begin(), line.end(), isspace),
                                 line.end());
			}
			
			
            if(line[0] == '#' || line.empty())
                continue;
			
			if (line.compare("[REGISTER]") == 0)
			{
				LOG("Register Found\n");
				if (type == 0)
				{
					type = 1;
					address = 0;
					issigned = 0;
					description.clear();
					regname.clear();
					continue;
				}
				else 
				{
					LOG("Error In Register Configfile\n");
					continue;
				}
			}
			
			if (line.compare("[COIL]") == 0)
			{
				LOG("Coil Found\n");
				if (type == 0)
				{
					type = 2;
					address = 0;
					issigned = 0;
					description.clear();
					coilname.clear();
					continue;
				}
				else 
				{
					LOG("Error In Register Configfile\n");
					continue;
				}
			}
			
			if (line.compare("[SWITCH]") == 0)
			{
				LOG("Switch Found\n");
				if (type == 0)
				{
					type = 3;
					address = 0;
					issigned = 0;
					description.clear();
					oncommand.clear();
					offcommand.clear();
					switchname.clear();
					continue;
				}
				else 
				{
					LOG("Error In Register Configfile\n");
					continue;
				}
			}
			
			if (line.compare("[/REGISTER]") == 0)
			{
				LOG("End of Register Found\n");
				if (type == 1 && address > 0 && !description.empty() && !regname.empty())
				{
					//Save Register allowed adresses MINUSERREG to MAXUSERREG (don't conflict with tracer adresses)
					if ((address >= MINUSERREG) && (address <= MAXUSERREG))
					{
						userreg Register;
						Register.Adress = address;
						Register.RegName = regname;
						Register.Description = description;
						Register.Value = 0xFFFF;
						if (issigned == 2)
							Register.issigned = true;
					    else
							Register.issigned = false;
						
						if (UserRegister.empty())
						UserRegister.push_back(Register);
					    else
						{
							int size = UserRegister.size();
							bool present = false;
							int i;
							for (i = 0; i < size; i++)
							{
								if (UserRegister.at(i).Adress == address)
								{
									present = true;
									break;
								}
							}
							if (present == false)
							{
								UserRegister.push_back(Register);
							}
							else
							{
								LOG("Error UserRegister %04X already present\n", address);
							}
						}
						
					}
					else
					{
						LOG("Error Userreg address %04X not allowed (valid range from MINUSERREG to MAXUSERREG)\n", address);
					}
					type = 0;
					address = 0;
					issigned = 0;
					description.clear();
					oncommand.clear();
					offcommand.clear();
					switchname.clear();
					regname.clear();
					coilname.clear();
					continue;
						
					
				}
				else if (type == 1)
				{
					LOG("Error In Register Configfile\n");
					type = 0;
					continue;
				}
				else 
				{
					LOG("Error In Register Configfile\n");
					continue;
				}
			}
			
			if (line.compare("[/COIL]") == 0)
			{
				LOG("End of Coil Found\n");
				if (type == 2 && address > 0 && !description.empty() && !coilname.empty())
				{
					//Save Coil allowed adresses MINUSERCOIL to MAXUSERCOIL (don't conflict with tracer adresses)
					if ((address >= MINUSERCOIL) && (address <= MAXUSERCOIL))
					{
						usercoil Coil;
						Coil.Adress = address;
						Coil.CoilName = coilname;
						Coil.Description = description;
						Coil.Value = 0xFF;
						
						if (UserCoil.empty())
						UserCoil.push_back(Coil);
					    else
						{
							int size = UserCoil.size();
							bool present = false;
							int i;
							for (i = 0; i < size; i++)
							{
								if (UserCoil.at(i).Adress == address)
								{
									present = true;
									break;
								}
							}
							if (present == false)
							{
								UserCoil.push_back(Coil);
							}
							else
							{
								LOG("Error UserCoil %04X already present\n", address);
							}
						}
						
					}
					else
					{
						LOG("Error UserCoil address %04X not allowed (valid range from MINUSERCOIL to MAXUSERCOIL)\n", address);
					}
					type = 0;
					address = 0;
					issigned = 0;
					oncommand.clear();
					offcommand.clear();
					switchname.clear();
					description.clear();
					regname.clear();
					coilname.clear();
					continue;
				}
				else if (type == 2)
				{
					LOG("Error In Register Configfile\n");
					type = 0;
					continue;
				}
				else 
				{
					LOG("Error In Register Configfile\n");
					continue;
				}
			}
			
            if (line.compare("[/SWITCH]") == 0)
			{
				LOG("End of Switch Found\n");
				if (type == 3 && !switchname.empty() && !oncommand.empty() && !offcommand.empty() && !description.empty())
				{
					//Save Switch
					
						userswitch uswitch;
						uswitch.OnCommand = oncommand;
						uswitch.OffCommand = offcommand;
						uswitch.Name = switchname;
						
						if (UserSwitch.empty())
						UserSwitch.push_back(uswitch);
					    else
						{
							int size = UserSwitch.size();
							bool present = false;
							int i;
							for (i = 0; i < size; i++)
							{
								if ((UserSwitch.at(i).Name.compare(switchname)) == 0)
								{
									present = true;
									break;
								}
							}
							if (present == false)
							{
								UserSwitch.push_back(uswitch);
							}
							else
							{
								LOG("Error UserSwitch %s already present\n", switchname.c_str());
							}
						}
						
					
					
					type = 0;
					address = 0;
					issigned = 0;
					description.clear();
					oncommand.clear();
					offcommand.clear();
					switchname.clear();
					regname.clear();
					coilname.clear();
					continue;
				}
				else if (type == 3)
				{
					LOG("Error In Register Configfile\n");
					type = 0;
					continue;
				}
				else 
				{
					LOG("Error In Register Configfile\n");
					continue;
				}
			}
			
			
			if (type > 0)
			{
				if (name.compare("address") == 0)
				{
					if (address > 0)
					{
						LOG("Error in Register Configfile (Address already present)\n");
						continue;
					}
			try
			{
				address	= std::stoi(value, NULL, 16);
			}
			catch (...)
			{
				LOG("Error in Register Configfile (wrong address)\n");
				address = 0;
				continue;
			}
			LOG("address found: %04X\n", address);
			}
			
			if (name.compare("description") == 0)
			{
				if (!description.empty())
				{
					LOG("Error in Register Configfile (Description already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (Description empty)\n");
					continue;
				}
				
				description.assign(value);
				description.erase(0,description.find_first_not_of(" "));
				LOG("description found: %s\n", description.c_str());
			}
			
			if (name.compare("switchname") == 0)
			{
				if (!switchname.empty())
				{
					LOG("Error in Register Configfile (switchname already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (switchname empty)\n");
					continue;
				}
				
				switchname.assign(value);
				switchname.erase(0,switchname.find_first_not_of(" "));
				LOG("switchname found: %s\n", switchname.c_str());
			}
			
			if (name.compare("regname") == 0)
			{
				if (!regname.empty())
				{
					LOG("Error in Register Configfile (regname already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (regname empty)\n");
					continue;
				}
				
				regname.assign(value);
				regname.erase(0,regname.find_first_not_of(" "));
				LOG("regnamename found: %s\n", regname.c_str());
			}
			
			if (name.compare("coilname") == 0)
			{
				if (!coilname.empty())
				{
					LOG("Error in Register Configfile (coilname already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (coilname empty)\n");
					continue;
				}
				
				coilname.assign(value);
				coilname.erase(0,coilname.find_first_not_of(" "));
				LOG("coilname found: %s\n", coilname.c_str());
			}
			
			
			if (name.compare("oncommand") == 0)
			{
				if (!oncommand.empty())
				{
					LOG("Error in Register Configfile (oncommand already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (oncommand empty)\n");
					continue;
				}
				
				oncommand.assign(value);
				oncommand.erase(0,oncommand.find_first_not_of(" "));
				LOG("oncommand found: %s\n", oncommand.c_str());
			}
			
			if (name.compare("offcommand") == 0)
			{
				if (!offcommand.empty())
				{
					LOG("Error in Register Configfile (offcommand already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (offcommand empty)\n");
					continue;
				}
				
				offcommand.assign(value);
				offcommand.erase(0,offcommand.find_first_not_of(" "));
				LOG("offcommand found: %s\n", offcommand.c_str());
			}
			
			if (name.compare("issigned") == 0)
			{
				if (issigned != 0)
				{
					LOG("Error in Register Configfile (issigned already present)\n");
					continue;
				}
				if (value.empty())
				{
					LOG("Error in Register Configfile (issigned empty)\n");
					continue;
				}
				
				if (value.compare("true") == 0)
				{
					issigned = 2;
					LOG("Setting issigned to true\n");
				}
				else
				{
					issigned = 1;
					LOG("Setting issigned to false\n");
				}
				
			}
				
        }
        }
		
		std::sort(UserRegister.begin(), UserRegister.end());
		
		std::sort(UserCoil.begin(), UserCoil.end());
		
		LOG ("Config File read:\n");
		int numreg;
		numreg = UserRegister.size();
		LOG ("%d User Registers added:\n", numreg);
		int n;
		for (n=0; n<numreg; n++)
		{
			LOG("[REGISTER]\n");
			LOG("Address: %04X\n", UserRegister.at(n).Adress);
			LOG("Regname: %s\n", UserRegister.at(n).RegName.c_str());
			LOG("Description: %s\n", UserRegister.at(n).Description.c_str());
			LOG("Value: %04X\n", UserRegister.at(n).Value);
			LOG("IsSigned: %d\n", UserRegister.at(n).issigned);
			LOG("[/REGISTER]\n\n");
		}
		int numcoil;
		numcoil = UserCoil.size();
		LOG ("%d User Coils added:\n", numcoil);
		for (n=0; n<numcoil; n++)
		{
			LOG("[COIL]\n");
			LOG("Address: %04X\n", UserCoil.at(n).Adress);
			LOG("Coilname: %s\n", UserCoil.at(n).CoilName.c_str());
			LOG("Description: %s\n", UserCoil.at(n).Description.c_str());
			LOG("Value: %04X\n", UserCoil.at(n).Value);
			LOG("[/Coil]\n\n");
		}
		
		int numswitch;
		numswitch = UserSwitch.size();
		LOG ("%d User Switch added:\n", numswitch);
		for (n=0; n<numswitch; n++)
		{
			LOG("[SWITCH]\n");
			LOG("Name: %s\n", UserSwitch.at(n).Name.c_str());
			LOG("OnCommand: %s\n", UserSwitch.at(n).OnCommand.c_str());
			LOG("OffCommand: %s\n", UserSwitch.at(n).OffCommand.c_str());
			LOG("[/Switch]\n\n");
		}
		
		}
    else {
        LOG("Couldn't open config file for reading.\n");
    }
}

int cMemory::Switch(std::string switchname, int onoff)
{
	int numuserswitch;
	numuserswitch = UserSwitch.size();
	int n;
	for(n=0; n<numuserswitch; n++)
	{
		if (UserSwitch.at(n).Name.compare(switchname) == 0)
		{
			if (onoff == 1)
			{
				system(UserSwitch.at(n).OnCommand.c_str());
				return 1;
			}
			else if (onoff == 0)
			{
				system(UserSwitch.at(n).OffCommand.c_str());
				return 1;
			}
			else
				return -1;
		}
	}
	return -1;
}
	       
   
   
	


  