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
#ifndef __MODBUS_H
#define __MODBUS_H
#include <termios.h>
#include <string>
#include <stdint.h>
#include <mutex>

#define TIMEOUT_T1_5 1750 //Overide Inter char Timeout 0 not Overide
#define TIMEOUT_T3_5 0 //Overide Frame Timeout 0 not overide


#define MODBUS_RTU_MAX_ADU_LENGTH 256

class cModbus
{
	private:
	static std::mutex send_mtx;
	std::string Mb_device;
	int Mb_speed;
	int Mb_parity;
	int Mb_databit;
	int Mb_stopbit;
	int Mb_ctx;
	bool Mb_debug;
	bool Mb_connected;
	uint8_t Mb_slave;
	enum eState { initial, waiting, processing, idle };
	eState Mb_state;
	struct termios oldSettings;
	struct termios Mb_Settings;
	
	unsigned long T1_5; // inter character time out in microseconds
	unsigned long T3_5; // frame delay in microseconds
	
	long Timeout(int fd, long sec, long usec);
	uint16_t Crc16(uint8_t *buffer, int buffer_length);
	int Receive(uint8_t *buffer, eState input_state = initial); //Receive Message from Master / Slave. As a Master input_state is idle there is no need for waiting T3_5 Timeout
	
	static const uint8_t table_crc_hi[];
	static const uint8_t table_crc_lo[];
	
	public:
	cModbus(std::string device, int speed, int parity, int databit, int stopbit);
	~cModbus();
	int Connect();
	void SetDebug(bool debug) { Mb_debug = debug; }
	void Disconnect();
	void SetSlave(uint8_t slave) { Mb_slave = slave; }
	int SendRequest(uint8_t *req, int req_lenght, uint8_t *resp); //Send Request ant wait for Response. Return lenght of Response, 0 on Timeout, -1 on Failure.
	int ReceiveRequest(uint8_t *req); //Wait for Request and return lenght of Request. 0 on Timeout, and -1 on Failure.
	int SendConfirmation(uint8_t *conf, int conf_lenght); //Send Confirmation and return number of bytes Send. -1 on Failure.
	
};

/**
 * UTILS FUNCTIONS
 * Copied from Libmodbus.org
 **/

#define MODBUS_GET_HIGH_BYTE(data) (((data) >> 8) & 0xFF)
#define MODBUS_GET_LOW_BYTE(data) ((data) & 0xFF)
#define MODBUS_GET_INT64_FROM_INT16(tab_int16, index) \
    (((int64_t)tab_int16[(index)    ] << 48) + \
     ((int64_t)tab_int16[(index) + 1] << 32) + \
     ((int64_t)tab_int16[(index) + 2] << 16) + \
      (int64_t)tab_int16[(index) + 3])
#define MODBUS_GET_INT32_FROM_INT16(tab_int16, index) ((tab_int16[(index)] << 16) + tab_int16[(index) + 1])
#define MODBUS_GET_INT16_FROM_INT8(tab_int8, index) ((tab_int8[(index)] << 8) + tab_int8[(index) + 1])
#define MODBUS_SET_INT16_TO_INT8(tab_int8, index, value) \
    do { \
        tab_int8[(index)] = (value) >> 8;  \
        tab_int8[(index) + 1] = (value) & 0xFF; \
    } while (0)
#define MODBUS_SET_INT32_TO_INT16(tab_int16, index, value) \
    do { \
        tab_int16[(index)    ] = (value) >> 16; \
        tab_int16[(index) + 1] = (value); \
    } while (0)
#define MODBUS_SET_INT64_TO_INT16(tab_int16, index, value) \
    do { \
        tab_int16[(index)    ] = (value) >> 48; \
        tab_int16[(index) + 1] = (value) >> 32; \
        tab_int16[(index) + 2] = (value) >> 16; \
        tab_int16[(index) + 3] = (value); \
    } while (0)


#endif //_MODBUS_H