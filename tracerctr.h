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
#ifndef __TRACERCTR_H
#define __TRACERCTR_H

#include <sys/types.h>
#include <unistd.h>
#include <ctime>
//#include <modbus.h>
#include <string>
#include <mutex>

#include "common.h"
#include "modbus.h"

class cTracerCtr
{
	private:
		std::mutex modbus_mtx;  // protects Modbus send/receive
		static bool NeedTracerStatistic;
		static bool NeedTracerRealtime;
		static bool NeedServerStatistic;
		static bool NeedServerRealtime;
		static bool TracerConnected; //Tracer Solar Controler is connected
		//modbus_t *ctx;
		cModbus *ctx;
		static cTracerCtr *Instance;
		cTracerCtr(bool tracerconnected = false, std::string tracerdev = "");
		~cTracerCtr();
		
	
	public:
		static void CreateInstance(bool tracerconnected = false, std::string tracerdev = "");
		void DeleteInstance();
		static cTracerCtr* GetInstance();
		bool NeedTracerStatisticupdate() { return NeedTracerStatistic; }
		bool NeedTracerRealtimeupdate() { return NeedTracerRealtime; }
		bool NeedServerStatisticupdate() { return NeedServerStatistic; }
		bool NeedServerRealtimeupdate() { return NeedServerRealtime; }
		void ResetTracerStatisticupdate() { NeedTracerStatistic = false; }
		void ResetTracerRealtimeupdate() { NeedTracerRealtime = false; }
		void ResetServerStatisticupdate() { NeedServerStatistic = false; }
		void ResetServerRealtimeupdate()  { NeedServerRealtime = false; }
		static void TriggerTracerStatisticupdate() { NeedTracerStatistic = true; }
		static void TriggerTracerRealtimeupdate() { NeedTracerRealtime = true; }
		static void TriggerServerStatisticupdate() { NeedServerStatistic = true; }
		static void TriggerServerRealtimeupdate()  { NeedServerRealtime = true; }
		static bool IsTracerConnected() { return TracerConnected; }
		int modbus_send_request(uint8_t *req, int reqlen, uint8_t *rsp); //send Modbus Request and Receive response. Return Response lenght
	

};

#endif //_TRACERCTR_H