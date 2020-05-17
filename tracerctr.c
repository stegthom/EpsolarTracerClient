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
#include <string>

#include "modbus.h"
#include "common.h"
#include "tracerctr.h"


bool cTracerCtr::NeedTracerStatistic = true;
bool cTracerCtr::NeedTracerRealtime = true;
bool cTracerCtr::NeedServerStatistic = true;
bool cTracerCtr::NeedServerRealtime = true;
bool cTracerCtr::TracerConnected = false;
cTracerCtr *cTracerCtr::Instance = NULL;
void cTracerCtr::CreateInstance(bool tracerconnected, std::string tracerdev)
{
	if (!Instance)
	{
		Instance = new cTracerCtr(tracerconnected, tracerdev);
	}
}

void cTracerCtr::DeleteInstance()
{
	if (Instance)
	{
		delete Instance;
		Instance = NULL;
	}
}

cTracerCtr* cTracerCtr::GetInstance()
{
	if (!Instance)
		CreateInstance();
	return Instance;
}
	

cTracerCtr::cTracerCtr(bool tracerconnected, std::string tracerdev)
{
	TracerConnected = tracerconnected;
	ctx = NULL;
	if (TracerConnected && !tracerdev.empty())
	{
		//ctx = modbus_new_rtu(tracerdev.c_str(), 115200, 'N', 8,1);
		ctx = new cModbus(tracerdev, 115200, 0, 8, 1);
		ctx->SetSlave(1);
	
		if (ctx->Connect() == -1)
		{
			LOG("Modbus: Failed to connect the context\n");
			exit(0);
		}
		//modbus_set_slave(ctx, TRACER_ID);
   
		#ifdef DEBUG
		ctx->SetDebug(true);
		//modbus_set_debug(ctx, 1);
		#endif
   
		//modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
		//if (modbus_connect(ctx) == -1)
		//{
			//LOG("Modbus: Unable to connect: %s\n", modbus_strerror(errno));
			//modbus_free(ctx);
			//exit(0);
		//}
	}
}

cTracerCtr::~cTracerCtr()
{
	if (ctx)
	{
		delete ctx;
		//modbus_close(ctx);
		//modbus_free(ctx);
		ctx = NULL;
	}
}

int cTracerCtr::modbus_send_request(uint8_t *req, int reqlen, uint8_t *rsp)
{
	int rc = 0;
	if (TracerConnected && ctx)
	{
		std::lock_guard<std::mutex> lock(modbus_mtx);
		//modbus_send_raw_request(ctx, req, reqlen);
		//rc = modbus_receive_raw_confirmation(ctx, rsp);
		rc = ctx->SendRequest(req, reqlen, rsp);
		if (rc >= 2)
		{
			// If we write some Registers or Coils on the Tracer Controller trigger a complete Update on Client and Server
			if (rsp[1] == 0x05 || rsp[1] == 0x10 || rsp[1] == 0x0F || rsp[1] == 0x06)
			{
				TriggerTracerStatisticupdate();
				TriggerTracerRealtimeupdate();
				TriggerServerStatisticupdate();
				TriggerServerRealtimeupdate();
			}
		}
		else if (rc == -1)
		{
			LOG("cTracerCtr: Error Sending Request\n");
			LOG("cTracerCtr: Reconnect Context\n");
			ctx->Disconnect();
			usleep(2000000);
			int retry = 0;
			for (retry = 0; retry<20; retry++)
			{
				if (ctx->Connect() >= 0)
					break;
				ctx->Disconnect();
				usleep(2000000);
			}
			if (retry == 20)
			{
				LOG("cTracerCtr: Can not Reconnect to Trscerdev Exit Programm\n");
				exit(0);
			}
		}
	}
	return rc;
}

