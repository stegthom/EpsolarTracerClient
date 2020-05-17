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
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>

#include "display.h"
#include "tracerctr.h"


cDisplay::cDisplay(std::string displaydev, std::string forwardserv, int forwardp)
{
	ctx = NULL;
	mtcpclient = NULL;
	//ctx = modbus_new_rtu(displaydev.c_str(), 115200, 'N', 8, 1);
	ctx = new cModbus(displaydev, 115200, 0, 8, 1);
	ctx->SetSlave(1);
	
	
	
	if (ctx->Connect() == -1)
     {
         LOG("cDisplay: Failed to connect the context\n");
		 exit(0);
     }
	 //modbus_set_slave(ctx, TRACER_ID);
	 #ifdef DEBUG
	 //modbus_set_debug(ctx, 1);
	 ctx->SetDebug(true);
	 #endif
	 if(!forwardserv.empty() && forwardp > 0 && !mtcpclient)
	{
		mtcpclient = new cMtcpClient(forwardserv.c_str(), forwardp);
	}
	 
	// if (modbus_connect(ctx) == -1)
     //{
       //     LOG("cDisplay:Unable to connect: %s\n", modbus_strerror(errno));
         //   modbus_free(ctx);
			//exit(0);
    // }
}

cDisplay::~cDisplay()
{
	if (ctx)
	{
		delete ctx;
		//modbus_close(ctx);
		//modbus_free(ctx);
		ctx = NULL;
	}
	if (mtcpclient)
	{
		delete mtcpclient;
		mtcpclient = NULL;
	}
}

void cDisplay::Body()
{
	uint8_t query[MODBUS_RTU_MAX_ADU_LENGTH];
	int rc, replylenght;
	
	while (true)
	{
		this->SyncPoint();
		rc = 0;
		replylenght = 0;
		
		
	//rc = modbus_receive(ctx, query);
	rc = ctx->ReceiveRequest(query);
	if (rc>0)
	{
		uint8_t reply[MODBUS_RTU_MAX_ADU_LENGTH];
		replylenght = memory.GenerateResponse(query, rc, reply, MODBUS_RTU_MAX_ADU_LENGTH);
		if (replylenght >0)
		{
			//modbus_send_raw_reply(ctx, reply, replylenght);
			ctx->SendConfirmation(reply, replylenght);
		}
		else if (replylenght == -2) //Unknown Request forward to tracer Controler
		{
			if (cTracerCtr::IsTracerConnected())
			{
				cTracerCtr *tracerctr;
				tracerctr = cTracerCtr::GetInstance();
				if (tracerctr && rc>2)
					replylenght = tracerctr->modbus_send_request(query, (rc - 2), reply);
			}
			else
			{
				if (mtcpclient)
				{
					std::string sendbuffer("modbussend ");
					char buffer[3];
					char recvmessage[1024];
					int n = 0;
					for (n=0; n<(rc-2); n++)
					{
						snprintf(buffer, 3, "%02X", query[n]);
						sendbuffer.append(buffer);
					}
					if (mtcpclient->connect() == -1)
					{
						LOG("cDisplay: Error Forward Message to Client: Connection Error\n");
					}
					LOG("cDisplay: Forward Request to Tracerclient: %s\n", sendbuffer.c_str());
					if (mtcpclient->send(sendbuffer.c_str(),sendbuffer.size()) == -1)
					{
						LOG("cDisplay: Error Forward Message to Client: Send Error\n");
					}
					int recvlenght = mtcpclient->recv(recvmessage, 1024);
					mtcpclient->closeconnection();
					//std::string recvstring(recvmessage, recvlenght);
					std::string message;
					for(n=0; n<recvlenght; n++)
					{
						if (recvmessage[n] != '<' && recvmessage[n] != '>')  //Remove Formating Character from string
						{
							message.push_back(recvmessage[n]);
						}
					}
					int i;
					uint8_t byte;
					replylenght = 0;
					LOG("cDisplay: Message from Tracerclient received: %s\n", message.c_str());
					
					
					for (i = 0; i < (message.size()-1); i += 2) 
					{
						//printf("replylenght: %d\n", replylenght);
						//printf("recvlenght: %d of %d\n", i, recvlenght);
						std::string byteString = message.substr(i, 2);
						try
						{
							byte = (uint8_t) stoi(byteString, NULL, 16);  //covert string to HEX Value
						}
						catch (...)
						{
							LOG("cDisplay: error can not convert Client Message to HEX\n");
						}
					
						reply[replylenght] = byte;
						replylenght++;
					}
					
				}
				else
				{
					LOG("cDisplay: Can Not Forward Unknown Message to Tracer Controler. (Not Connected and no Mtcp Forward Server specified");
				}
		}
		if (replylenght > 2)
		{
			//modbus_send_raw_reply(ctx, reply, replylenght);
			ctx->SendConfirmation(reply, (replylenght-2)); //cutoff crc Received from tracer Controller because it will be appended by SendConfirmation
		}
	}
	}
	
	else if (rc == 0)
	{
		LOG("cDisplay: No Request received\n");
		
	}
	else
	{
		LOG("cDisplay: Error Receive Request\n");
		//modbus_flush(ctx);
		LOG("cDisplay: Reconnect Context\n");
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
			LOG("cDisplay: Can not Reconnect to Display Exit Programm\n");
			exit(0);
		}

	}
	}
}
