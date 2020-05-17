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

// MTTP  -> My Tracer Transport Protokoll
//<DLE><STX><2Byte Message Lenght><1Byte Action><DLE><SCD><enum data><2Byte RegStartAdress or devInfo type><Paylod with all DLE replaced by DLE DLE><DLE><ECD><DLE><SCD>...<DLE><ETX>

#ifndef __MTTP_H
#define __MTTP_H

#define BUFFER_SIZE 500
#define COMMAND_SIZE 500
#define MAX_COMMANDS 50

#define DLE 0x10 // Escape Byte
#define STX 0x02 // Start of Text
#define ETX 0x03 // End of Text
#define SCD 0x04 // start of Command
#define ECD 0x05 // End of Command

#include <vector>
#include "memory.h"
#include <cstdint>
#include "common.h"
#include <list>


class cMTTP
{
 private:
   std::vector<uint8_t> buffer;
   int readindex;
   int writeindex;
   bool closed;
   bool inited;
   struct commands
     {
	bool exist = false;
	enum data datatype;
	uint16_t startadress;
	int payloadlenght;
	uint8_t payload[COMMAND_SIZE];
	commands();
     } decoded_commands[MAX_COMMANDS];
	 std::list<commands> decoded_commands_list;
	
   
 public:
   cMTTP(action act = eNone); // Create buffer for action if action is eNone its used as a read buffer to decode
   ~cMTTP();
   void InitBuffer(action act); //Clear the Buffer and Add protocol specific Bytes to the Buffer. if action is none nothing will be added
   int AddCommand(data memtype, uint8_t *command, int lenght, uint16_t adress); //Add a Command to Buffer
   int GetCommand(data *memtype, uint16_t *startadress, uint8_t *command, int commandsize, int index); // Get command[index] from Buffer. Return number of payloadlenght on sucess 0 if Buffer[index] is empty and -1 on Failure. index should be smaler than MAX_COMMANDS
   int GetBuffer(uint8_t *sendbuffer, int sendbufferlen); //Get Buffer filled with Commands to send to Server
   int AddMessage(uint8_t *message, int lenght); //Add a complete Message received from Client and decodes it. Call GetCommand to receive the Command one by one. Return Number of Bytes processed.
   void CloseBuffer();

};
   
     
   


#endif //__MTTP_H