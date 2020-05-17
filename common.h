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
#ifndef __COMMON_H
#define __COMMON_H
#ifdef DEBUG
#define DEBUGLOG(a...) printf("DEGUG: " a )
#else
#define DEBUGLOG(a...)
#endif
#define LOG(a...) printf("LOG: " a)

//#define TRACER_ID 1

#define MAX_SEND_BUFFER 8192
#define MAX_RECV_BUFFER 8192



enum data:uint8_t 
{
   eNotSet = 0x00, eAll = 0x30, eDevInfo1 = 0x31, eDevInfo2 = 0x32, eReg = 0x33, eCoil = 0x34, eDiscInp = 0x35, eHeader = 0x36, eUser = 0x37, eUserReg = 0x38, eUserCoil = 0x39
};

enum action:uint8_t
{
   eNone = 0x20, ePut = 0x21, eGet = 0x22, eHeartbeat = 0x29
};


#endif  //__COMMON_H