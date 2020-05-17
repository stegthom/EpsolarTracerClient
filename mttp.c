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
#include <vector>
#include "mttp.h"
#include <cstdint>
#include "common.h"
#include <stdio.h>
#include <cstring>
cMTTP::cMTTP(action act)
{
   inited = false;
   InitBuffer(act);
}

cMTTP::~cMTTP()
{
}

//cMTTP::commands cMTTP::decoded_commands[MAX_COMMANDS];
cMTTP::commands::commands()
{
	bool exist = false;
	enum data datatype = eNotSet;
	uint16_t startadress = 0x0000;
    int payloadlenght = 0;
	uint8_t payload[COMMAND_SIZE] = {0x00};
}
void cMTTP::InitBuffer(action act)
{
   if (act == eNone)
     {
	buffer.clear();
	buffer.reserve(BUFFER_SIZE);
	readindex = 0;
	writeindex = 0;
     }
	 
   if (act == eHeartbeat)
   {
	readindex = 0;
	writeindex = 0;
	buffer.clear();
	buffer.reserve(BUFFER_SIZE);
	buffer.push_back(DLE);
	buffer.push_back(STX);
	buffer.push_back(0x00); //Message Lenght until now High Byte
	buffer.push_back(0x07); //Message Lenght until now Low Byte
	buffer.push_back(act);
	buffer.push_back(DLE);
	buffer.push_back(ETX);
	closed = true;
	inited = true;
   }
   
   
   else if(buffer.empty() && act != eNone)
     {
	readindex = 0;
	writeindex = 0;
	buffer.reserve(BUFFER_SIZE);
	buffer.push_back(DLE);
	buffer.push_back(STX);
	buffer.push_back(0x00); //Message Lenght until now High Byte
	buffer.push_back(0x05); //Message Lenght until now Low Byte
	buffer.push_back(act);
	closed = false;
	inited = true;
     }
   else if(act != eNone)
     {
	readindex = 0;
	writeindex = 0;
	buffer.clear();
	buffer.reserve(BUFFER_SIZE);
	buffer.push_back(DLE);
    buffer.push_back(STX);
	buffer.push_back(0x00); //Message Lenght until now High Byte
	buffer.push_back(0x05); //Message Lenght until now Low Byte
	buffer.push_back(act);
	closed = false;
	inited = true;
     }
   
   
}


int cMTTP::AddCommand(data memtype, uint8_t *command, int lenght, uint16_t adress)
{
   if (closed)
     {
	LOG("cMTTP: Error Add Command Buffer already marked as closed\n");
	return -1;
     }
   uint8_t startadress[2];
   MODBUS_SET_INT16_TO_INT8(startadress, 0, adress);
   int commandlenght = 0;
   // Insert Comand Start Mark
   buffer.push_back(DLE);
   commandlenght++;
   buffer.push_back(SCD);
   commandlenght++;
   // Insert Type of Payload Data
   buffer.push_back(memtype);
   commandlenght++;
   //Insert Register Start Adress or Device Info Type(1, 2).
   if (startadress[0] == DLE)
   {
	   buffer.push_back(DLE); //Insert DLE Byte if needed
	   commandlenght++;
   }
   buffer.push_back(startadress[0]);
   commandlenght++;
   if (startadress[1] == DLE)
   {
	   buffer.push_back(DLE); //Insert DLE Byte if needed
	   commandlenght++;
   }
   buffer.push_back(startadress[1]);
   commandlenght++;
   
   // Insert Payload Data and replace every 0x10 (DLE) with 0x10 0x10 (DLE DLE).
   
   int n;
   for (n = 0; n < lenght; n++)
     {
	if (command[n] != DLE)
	  {
	     //printf("Putting data to bufer: %d\n", n);
	     buffer.push_back(command[n]);
	     commandlenght++;
	  }
	else if (command[n] == DLE)
	  {
	     //printf("Putting data to bufer: %d\n", n);
	     buffer.push_back(DLE);
	     buffer.push_back(command[n]);
	     commandlenght += 2;
	  }
     }
   
   // Insert Command End Mark
   buffer.push_back(DLE);
   commandlenght++;
   buffer.push_back(ECD);
   commandlenght++;
   
   uint16_t old_message_lenght = MODBUS_GET_INT16_FROM_INT8(buffer.data(), 2);
   uint16_t new_message_lenght = old_message_lenght + commandlenght;
   MODBUS_SET_INT16_TO_INT8(buffer.data(), 2, new_message_lenght);
   return commandlenght;
}

int cMTTP::GetCommand(data *memtype, uint16_t *startadress, uint8_t *command, int commandsize, int index)
{
	
if (index >= MAX_COMMANDS)
	  {
		  LOG("cMTTP::GetCommand: index Out of Range\n");
	      return -1;
	  }
	if (!decoded_commands[index].exist)
	  {
		  DEBUGLOG("cMTTP::GetCommand: Command with index %d does not exist\n", index);
		  return -1;
	  }
	if (decoded_commands[index].payloadlenght > commandsize)
	  {
		  LOG("cMTTP::GetCommand: Error *command Buffer to small for Command\n");
	      return -1;
	  }
	else
	  {
		  *memtype = decoded_commands[index].datatype;
		  *startadress = decoded_commands[index].startadress;
		  std::memcpy (command, decoded_commands[index].payload, decoded_commands[index].payloadlenght);
		  
		  //decoded_commands[index].payloadlenght = 0;
		  //decoded_commands[index].startadress = 0;
		  
		  #if 0
		  //-----------------Debug CommandBuffer----------------------------
		  DEBUGLOG("Command index: %d\n", index);
		  DEBUGLOG("Command Memtype: %02X\n", decoded_commands[index].datatype);
		  DEBUGLOG("Command Startadress: %04X\n", decoded_commands[index].startadress);
		  DEBUGLOG("Command Payloadlenght: %d\n", decoded_commands[index].payloadlenght);
		  DEBUGLOG("Command Exist: %d\n", decoded_commands[index].exist);
		  DEBUGLOG("Command Payload:\n");
		  int z=0;
		  for (z = 0; z < decoded_commands[index].payloadlenght; z++)
		  {
			  printf("%02X ", decoded_commands[index].payload[z]);
		  }
		  printf("\n");
		  
		  //---------------------End Debug-------------------------------
		  #endif
		  decoded_commands[index].exist = false;
		  return decoded_commands[index].payloadlenght;
	  }

}

int cMTTP::GetBuffer(uint8_t *sendbuffer, int sendbufferlen)
{
   if(!inited)
     {
	LOG("cMTTP: GetBufer not inited\n");
	return -1;
     }
   
   if(!closed)
     CloseBuffer();
   
   int buffsize = buffer.size();
   if (buffsize == 0)
     {
	LOG("cMTTP: Error GetBuffer Buffer empty\n");
	return -1;
     }
   else if (sendbufferlen < buffsize)
     {
	LOG("cMTTP: Error SendBuffer to small\n");
	return -1;
     }
   
   else
     {
	std::copy(buffer.begin(), buffer.end(), sendbuffer);
     }
   return buffsize;
}

int cMTTP::AddMessage(uint8_t *message, int lenght)
{
   int n;
   bool dlefound, stxfound, etxfound, scdfound, ecdfound;
   dlefound = false;
   stxfound = false;
   etxfound = false;
   scdfound = false;
   ecdfound = false;
   int cmdindex = -1;
   int cmdheaderindex = 0;
   int payloadlenght = 0;
   int headerindex = -1; //Needed to check we are in the header and skip DLE checking
   for (n = 0; n < MAX_COMMANDS; n++)
     {
	if (decoded_commands[n].exist)
	  continue;
	else 
	  {
	     cmdindex = n;
	     break;
	  }
     }
   if (cmdindex == -1)
     {
	LOG("cMTTP::AddMessage Error Command Buffer overrun\n");
	return 0;
     }
   
   
   for (n = 0; n < lenght; n++)
     {
	switch (message[n])
      {
       case DLE:
	  {
		  if (headerindex == 0 || headerindex == 1 || headerindex == 2) // Skip control Byte checking in the message header (known lenhgt)
			  break;
	     if (!dlefound)
	       {
		  dlefound = true;
		  continue;
	       }
	     else
	       {
		  dlefound = false;
		  break;
	       }
	  }
      case STX:
	  {
	     if (dlefound)
	       {
		  stxfound = true;
		  dlefound = false;
		  headerindex = 0;
		  continue;
	       }
	     else
	       break;
	  }
      case ETX:
	  {
	     if (dlefound)
	       {
			   if (!stxfound)
			   {
				   LOG("cMTTP::AddMessage Error End of Message found with out Start of Message (ignore it)\n");
			       dlefound = false;
				   break;
			   }
		  etxfound = true;
		  dlefound = false;
		  headerindex = 0;
		  return (n+1);
	       }
	     else 
	       break;
	  }
      case SCD:
	  {
	     if (dlefound)
	       {
		  if (!scdfound)
		    {
		      scdfound = true;
		      dlefound = false;
			}
			else
			 {
				 LOG("cMTTP::AddMessage Error wrong Command received ( [SCD][SCD] )\n");
			     return (n+1);
			 }
		  if (ecdfound)
		    {
				cmdindex++;
				ecdfound = false;
			
		    if (cmdindex >= MAX_COMMANDS)
		      {
				  LOG("cMTTP::AddMessage Error Command Buffer overrun\n");
				  return (n+1);
			  }
			}
		  continue;
	       }
	     else
	       break;
	  }
      case ECD:
	  {
	     if (dlefound)
	       {
		  scdfound = false;
		  ecdfound = true;
		  dlefound = false;
		  payloadlenght = 0;
		  cmdheaderindex = 0;
		  //cmdindex++;
		  //if (cmdindex >= MAX_COMMANDS)
		    //{
		      // LOG("cMTTP::AddMessage Error Command Buffer overrun");
		       //return cmdindex;
		    //}
		  
		  
		  continue;
	       }
	     else
	       break;
	  }
	
	   
      default:
	break;
	  	  
     }
	if (stxfound && !scdfound)
	  {
	     if (headerindex == 0)
		   {
			   headerindex++;
		       continue; //ignore MessageLenght High Byte
		   }
		  
		 else if (headerindex == 1)
		   {
			   headerindex++;
			   continue; //ignore Messagelenght Low Byte
		   }
		   
		 else if (headerindex == 2)
		   {
			   headerindex++;
		       continue; //ignore Action
		   }
		   
		 else if (headerindex  > 2)
		   {
			   LOG("cMTTP::AddMessage Error [SCD] expecded. Ignore Byte\n");
		       continue;
		   }
			   
	  }
	  
	 if (stxfound && scdfound)
	   
	   {
		   if (cmdheaderindex == 0)
		     {
			 decoded_commands[cmdindex].datatype = (data)message[n];
				 cmdheaderindex++;
				 continue;
			 }
		    
		   else if (cmdheaderindex == 1)
		     {
				 decoded_commands[cmdindex].startadress = MODBUS_GET_INT16_FROM_INT8(message, n);
				 cmdheaderindex++;
				 continue;
			 }
		   else if (cmdheaderindex == 2)
		     {
				 decoded_commands[cmdindex].payloadlenght = 0;
				 decoded_commands[cmdindex].exist = true;
				 cmdheaderindex++;
				 continue; // startadress low byte already processed above
			 }
		   else 
		     {
				 decoded_commands[cmdindex].payload[payloadlenght] = message[n];
//				 DEBUGLOG("cMTTP::AddMessage: index: %d payload: %02X saved\n",cmdindex, message[n]); 
				 payloadlenght++;
				 decoded_commands[cmdindex].payloadlenght = payloadlenght;
			     continue;
			 }
     
     }
   
	 } 
	
   LOG("cMTTP::AddMessage Warning End of Message not found\n");	  
   return (n+1);
}



void cMTTP::CloseBuffer()
{
   if (inited && !closed)
     {
	buffer.push_back(DLE);
	buffer.push_back(ETX);
	uint16_t old_message_lenght = MODBUS_GET_INT16_FROM_INT8(buffer.data(), 2);
	uint16_t new_message_lenght = old_message_lenght + 2;
	MODBUS_SET_INT16_TO_INT8(buffer.data(), 2, new_message_lenght);
	
	closed = true; 
     }
   else if (!inited)
     LOG("cMTTP: Error Close Buffer not Inited\n");
   else if (closed)
     LOG("cMTTP: Error Close Buffer already closed\n");
}
