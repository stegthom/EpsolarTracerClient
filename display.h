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
#ifndef __DISPLAY_H
#define __DISPLAY_H

//#include <modbus.h>
#include <string>
#include "memory.h"
#include "common.h"
#include "modbus.h"
#include "mtcpserver.h"
#include "threadwrapper/ThreadWrapper.h"

class cDisplay : public ThreadWrapper
{
	private:
	//modbus_t *ctx;
	cModbus *ctx;
	cMemory memory;
	cMtcpClient *mtcpclient;
	
	
	public:
	cDisplay(std::string displaydev, std::string forwardserv = "", int forwardp=0);
	virtual ~cDisplay();
//	void action();
	
	protected:
	void Body() override;
};

#endif //__DISPLAY_H