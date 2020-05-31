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
#ifndef __MEMORY_H
#define __MEMORY_H

#include <string>
#include <mutex>
#include <vector>
#include "common.h"
#include "modbus.h"

#define MAXUSRREG 50
#define MAXUSRCOILS 20

#define MAXREG 180 // should be caculated with MAXUSRREG
#define MAXHEADERSIZE 30
#define MAXHEADERS 10
#define MAXDISCRETEINPUTS 3
#define MAXCOILS 29 //should be calculated with MAXUSRCOILS
#define MAX_DEV_INFO_LENGHT 100

#define MINUSERREG 0x3400
#define MAXUSERREG 0x8FFF
#define MINUSERCOIL 0x0020
#define MAXUSERCOIL 0x00FF

class cMemory
{
   
 private:
      static std::mutex construct_mtx;
	  static std::mutex mem_mtx;
	  static bool inited;
	  static std::string memoryfile;
// public:
      static struct REGISTER
     {
	bool empty;
	bool HaveHeader;
	bool HaveAlternateValue;
	bool issigned;
	uint16_t Adress;
	uint16_t Value;
	uint16_t AlternateValue;
	uint16_t Headersize;
	uint8_t Header[MAXHEADERSIZE];
	std::string RegName;
	std::string Description;
	std::string Unit;
	int Multiplier;
     } Register[MAXREG];
   
      static struct COIL
     {
	bool empty;
	uint16_t Adress;
	uint8_t Value;
	std::string Description;
     } Coil[MAXCOILS];
   
      static struct DISCRETEINPUT
     {
	bool empty;
	uint16_t Adress;
	uint8_t Value;
	std::string Description;
     } DiscreteInput[MAXDISCRETEINPUTS];
	 
	 struct userreg
	 {
		 uint16_t Adress;
		 uint16_t Value;
		 std::string RegName;
		 std::string Description;
		 bool issigned;
		 bool operator < (const userreg &iData) const
			{
				return Adress < iData.Adress;
			}
	 };
	 
	 struct usercoil
	 {
		 uint16_t Adress;
		 uint8_t Value;
		 std::string CoilName;
		 std::string Description;
		 bool operator < (const usercoil &iData) const
			{
				return Adress < iData.Adress;
			}
	 };
	 
	 struct userswitch
	 {
		 std::string Name;
		 std::string OnCommand;
		 std::string OffCommand;
	 };
	 
	 static std::vector<userreg> UserRegister;
	 static std::vector<usercoil> UserCoil;
	 static std::vector<userswitch> UserSwitch;
	
      static uint16_t RegList[MAXREG]; //Adresses for all holding and input Registers
//      static uint16_t HeaderList[MAXHEADERS];  // starting Adress Reg for the coresponding Header
//      static uint16_t Reg[MAXREG]; //Register memory fo the coresponding RegList[] adress
//      static uint8_t RegHeader[MAXHEADERS][MAXHEADERSIZE];  //Header for the coresponding HeaderList[]   RegHeader[index][(0==size,memory...]
      static const uint16_t DiscreteInputList[MAXDISCRETEINPUTS]; //Adresses for all Dicrete Inputs
//      static uint8_t DiscreteInputHeader[MAXDISCRETEINPUTS][MAXHEADERSIZE];  // Headers for Discrete Inputs from DicreteInputList[]  DiscreteInputHeader[size][momory]
//      static uint8_t DiscreteInput[MAXDISCRETEINPUTS]; //Discrete Input Memory from DiscreteInputList
      static const uint16_t CoilList[MAXCOILS]; //Adresses for all Coil Registers
//      static uint8_t CoilHeader[MAXCOILS][MAXHEADERSIZE]; //Headers for Coil from CoilList[] CoilHeader[size][memory]
//      static uint8_t Coil[MAXCOILS]; //Coil Memory from CoilList
      static int DeviceInfo1Lenght; //Lenght of received Device Info 1 complete with Header
      static uint8_t DeviceInfo1[MAX_DEV_INFO_LENGHT]; //Device Info1 with header
      static int DeviceInfo2Lenght; //Lenght of received Device Info 2 complete wi$
      static uint8_t DeviceInfo2[MAX_DEV_INFO_LENGHT]; //Device Info2 with header
	  
	  // Search in HeaderList[] for the headeradress and return the Index Number
      // if not exist a new one will be generated if create is true.
      // if headeradress == 0 the first index is returned.
//      int GetHeaderIndex(uint16_t headeradress, bool create);
      
      
          
      
      
      // Search in DiscreteInputList[] for registeradress and return the IndexNumber
      // If discadress == 0 the first index is returned
      int GetDiscStartIndex(uint16_t discadress);
      
      // Search in CoilList[] for registeradress and return the IndexNumber
      // if coiladress == 0 the first index is returned
      int GetCoilStartIndex(uint16_t coiladress);
	  
 //public:
	  // Add Register Names, Unit...
	  void AddRegisterProperties();
   public:  
      cMemory();
      ~cMemory();

      // Search in RegList[] for registerindex and return the IndexNumber
      // if registeradress == 0 the first index is returned. if not found -1 is returned
      int GetRegStartIndex(uint16_t registeradress);     

	 //Return the Register Adress with the given index. Must be locked if called outside from cMemory and not currently locked.
      uint16_t GetRegisterAdress(int index, bool lock);
	  
	  //Save the Registers from TracerControler Modbus Protocol in the matching Arrays received from *resp with the lenght
      // returns Number of Saved Register or -1 in case of a Failure
      int Save(uint8_t *resp, int lenght, int headerlenght, uint8_t *req);
	  
	  //Save the Registers received from Client with the mttp.
	  int Save(data datatype, uint16_t startadress, int payloadlenght, uint8_t *payload);
      
      //Generate Response for the received *req with int lenght and stores it in *result
      // Request *req must be [MODBUS_RTU_MAX_ADU_LENGTH], and is needed for Register adresses. Returns lenght of generated Response
	  //Return -2 for Unknown Request (Modify Settings from Display)
      int GenerateResponse(uint8_t *req, int reqlenght, uint8_t *result, int resultsize);
      
	  //Clean all Alternate Values in Register
      void cleanalternate();
	  
	  //Fill all arrays wizh 0xff
      void clean();
      
   
      // Get Register from startadress to endadress and save result in *reg with lenght of reglenght. *reg is an Array large enought to hold result. 
	  // If alternate is true and a alternate Value is saved this will be returned.
      int GetRegister(uint16_t startadress, uint16_t endadress, uint16_t *reg, int reglenght, bool alternate = false);
	  
	  // Put the Registers into an uint8_t Buffer
	  // If alternate is true and a alternate Value is saved this will be returned.
	  int GetRegister(uint16_t startadress, uint16_t endadress, uint8_t *reg, int reglenght, bool alternate = false);
	  
	  //Get UserRegister from startadress to endadress and save result in *reg with lenght of reglenght. *reg is an Array large enought to hold result.
	  //Save the Adress of the first found Register to *firstadress
	  int GetUserRegister(uint16_t startadress, uint16_t endadress, uint16_t *reg, int reglenght, uint16_t *firstadress);

      // Get Coil from startadress to endadress and save result in *coil with lenght of coil lenght. *coil is an Array large enought to hold result.
      int GetCoil(uint16_t startadress, uint16_t endadress, uint8_t *coil, int coillenght);
	  
	  // Get UserCoil from startadress to endadress and save result in *coil with lenght of coil lenght. *coil is an Array large enought to hold result. 
	  int GetUserCoil(uint16_t startadress, uint16_t endadress, uint8_t *coil, int coillenght, uint16_t *firstadress);
   
      // Get Discrete Input from startadress to endadress and save result in *discinp with lenght of discinp lenght. *discinp is an Array large enought to hold result.
      int GetDiscInp(uint16_t startadress, uint16_t endadress, uint8_t *discinp, int discinplenght);
   
      // Get Header for uint16_t Adress or with the int index from Array and save result in *Header with lenght of Header lenght. *header is an Array large enought to hold result.
      //if int index is -1 it will ignored. if uint16_t adress is 0xFFFF it will ignored. only one from both parameters must be used.
      //return -1 in case of Error or 0 if no more header at index or adress found.
      int GetHeader(uint16_t adress, int index, uint8_t *header, int headerlenght);
   
      // Get Device Info from type 1 or 2 like received from tracer and save it in *deviceinfo with array lenght of int deviceinfolenght.
      int GetDeviceInfo(int type, uint8_t *deviceinfo, int deviceinfolenght);
	  
	  // Return a std::string containing the Register Value
	  std::string GetRegStr (uint16_t adress, bool orig = false);
	  
	  // Return a std::string containing the Register Value of double sized Registers starting at adress. The Low Byte Register must e the first.
	  std::string GetRegStr32 (uint16_t adress, bool orig = false);
	  
	  // Return a std::string containing the Coil Value
	  std::string GetCoilStr (uint16_t adress);
	  
	  //Save the string value as alternate Value in Register
	  int PutRegStr (std::string value, uint16_t adress);
	  
	  //Save the string value as alternate Value in the first 2 Registers starting at adress
	  int PutRegStr32 (std::string value, uint16_t adress);
	  
	  //Save the string value in UserCoil
	  int PutCoilStr (std::string value, uint16_t adress);
	  
	  //Return if the given Register have a Alternate Value:
	  bool RegisterHaveAlternateValue (uint16_t adress);
	  
	  //Return a std::string containing All Regesters with there Adress and Name.
	  std::string GetRegListStr();
    
   
      //void DebugLogMemory();
	  
	  std::string DebugLogMemory();
	  
	  //Add Configdirectory and search for memory.conf that contain custom Registers
	  void addconfdir(std::string confdir);
	  
	  //Read memory.conf in Confdir and add user Regs if present
	  void readconfig(std::string conffile, bool lock=true);
	  
	  //Switch on off the switches defined in memmory.conf
	  int Switch(std::string switchname, int onoff);
};

#endif //__MEMORY_H