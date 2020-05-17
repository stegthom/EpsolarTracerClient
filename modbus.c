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
#include <fcntl.h>  /* File Control Definitions          */
#include <termios.h>/* POSIX Terminal Control Definitions*/
#include <unistd.h> /* UNIX Standard Definitions         */
#include <errno.h>  /* ERROR Number Definitions          */
#include <cstring>
#include <cstdlib>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <unistd.h>
#include <ctime>
#include <sys/types.h>
#include "modbus.h"
#include "common.h"

std::mutex cModbus::send_mtx;

const uint8_t cModbus::table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

const uint8_t cModbus::table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

cModbus::cModbus(std::string device, int speed, int parity, int databit, int stopbit)
{
	Mb_device = device;
	Mb_speed = speed;
	Mb_parity = parity;
	Mb_databit = databit;
	Mb_stopbit = stopbit;
	Mb_ctx = -1;
	Mb_debug = false;
	Mb_connected = false;
	int Mb_slave = 1;
}

cModbus::~cModbus()
{
	Disconnect();
}

long cModbus::Timeout(int fd, long sec, long usec)
{
		int rv;
		fd_set set;
		struct timeval timeout;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		if (sec == 0 && usec == 0)
		{
			rv = select(fd + 1, &set, NULL, NULL, NULL);
		}
		else
		{
			timeout.tv_sec = sec;
			timeout.tv_usec = usec;
			rv = select(fd + 1, &set, NULL, NULL, &timeout);
		}
		if(rv == -1)
		{
			printf("Modbus Error on Select\n"); /* an error accured */
			return -1;
		}
		else if(rv == 0)
		{
			return -2; //Timeout
		}
		else
		{
			if (usec > 0 && sec == 0)
			{
				return (usec - timeout.tv_usec); //struct Timeval is modifield only on Linux on other OS it could be wrong.
			}
			else
			{
				return 1;  //The Time of longer Timeouts does not matter e.g. Waiting for Beginning of new Requests.
			}
		}
}

int cModbus::Connect()
{
	if (Mb_connected)
		return Mb_ctx;
	  speed_t speed;

  /* open port */
  
  Mb_ctx = open(Mb_device.c_str(),O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
  if(Mb_ctx<0)
  {
    LOG("cModbus:Open device failure\n") ;
    return -1;
  }

  /* save old Terminal Settings */
  if (tcgetattr (Mb_ctx, &oldSettings) < 0)
  {
    LOG("cModbus:Can't get terminal parameters");
    return -1 ;
  }

  memset(&Mb_Settings, 0, sizeof(struct termios));
  
  
  if (Mb_speed > 19200)
	{
		T1_5 = 750; 
		T3_5 = 1750; 
	}
	else 
	{
		T1_5 = 15000000/Mb_speed; // 1T * 1.5 = T1.5
		T3_5 = 35000000/Mb_speed; // 1T * 3.5 = T3.5
	}
	
	//Overide Timeouts
	
	if (TIMEOUT_T1_5 > 0)
		T1_5 = TIMEOUT_T1_5;
	
	if (TIMEOUT_T3_5 > 0)
		T3_5 = TIMEOUT_T3_5;
  
  

  switch (Mb_speed)
  {
     case 0:
        speed = B0;
        break;
     case 50:
        speed = B50;
        break;
     case 75:
        speed = B75;
        break;
     case 110:
        speed = B110;
        break;
     case 134:
        speed = B134;
        break;
     case 150:
        speed = B150;
        break;
     case 200:
        speed = B200;
        break;
     case 300:
        speed = B300;
        break;
     case 600:
        speed = B600;
        break;
     case 1200:
        speed = B1200;
        break;
     case 1800:
        speed = B1800;
        break;
     case 2400:
        speed = B2400;
        break;
     case 4800:
        speed = B4800;
        break;
     case 9600:
        speed = B9600;
        break;
     case 19200:
        speed = B19200;
        break;
     case 38400:
        speed = B38400;
        break;
     case 57600:
        speed = B57600;
        break;
     case 115200:
        speed = B115200;
        break;
     case 230400:
        speed = B230400;
        break;
     default:
        speed = B9600;
  }
  
  if ((cfsetispeed(&Mb_Settings, speed) < 0) ||
        (cfsetospeed(&Mb_Settings, speed) < 0)) 
		{
			Disconnect();
			LOG("cModbus: Error setting Baudrate");
			return -1;
		}
  
  switch (Mb_databit)
  {
     case 7:
        Mb_Settings.c_cflag = Mb_Settings.c_cflag | CS7;
        break;
     case 8:
     default:
        Mb_Settings.c_cflag = Mb_Settings.c_cflag | CS8;
        break;
  }
  switch (Mb_parity)
  {
     case 1:
        Mb_Settings.c_cflag = Mb_Settings.c_cflag | PARENB;
        break;
     case -1:
        Mb_Settings.c_cflag = Mb_Settings.c_cflag | PARENB | PARODD;
        break;
     case 0:
     default:
        Mb_Settings.c_iflag = IGNPAR;
		Mb_Settings.c_cflag &=~ PARENB;
        break;
  }
  Mb_Settings.c_iflag &= ~ICRNL;
  
  if (Mb_parity == 0)  //parity off
  {
	  Mb_Settings.c_iflag &= ~INPCK;
  }
  else
  {
	  Mb_Settings.c_iflag |= INPCK;
  }

  
  if (Mb_stopbit==2)
     Mb_Settings.c_cflag = Mb_Settings.c_cflag | CSTOPB;
   
/* Software flow control is disabled */
Mb_Settings.c_iflag &= ~(IXON | IXOFF | IXANY);
   
   
  Mb_Settings.c_cflag = Mb_Settings.c_cflag | CLOCAL | CREAD;
  Mb_Settings.c_oflag = 0;
  Mb_Settings.c_lflag = 0; /*ICANON;*/
  
  Mb_Settings.c_line = 0; //This field isn't used on POSIX systems
  
  Mb_Settings.c_cc[VMIN]=0;
  Mb_Settings.c_cc[VTIME]=0;

  /* clean port */
  tcflush(Mb_ctx, TCIFLUSH);
  
  
  
  /* Get Notification if data arived TODO*/
  //fcntl(fd, F_SETFL, FASYNC);
  
  
  
  
  /* activate the settings port */
  if (tcsetattr(Mb_ctx,TCSANOW,&Mb_Settings) <0)
  {
    LOG("cModbus: Can't set terminal parameters ");
    return -1 ;
  }
  
			
	
	// Disable RTS Mode
	
	int flags;
	
	ioctl(Mb_ctx, TIOCMGET, &flags);
   
        flags &= ~TIOCM_RTS;
    
	ioctl(Mb_ctx, TIOCMSET, &flags);
				
  
  /* clean I & O device */
  tcflush(Mb_ctx,TCIOFLUSH);
  Mb_connected = true;
  
 
   return Mb_ctx;
}

void cModbus::Disconnect()
{
	if (Mb_ctx >= 0)
	{
		tcsetattr(Mb_ctx,TCSANOW,&oldSettings);
		close(Mb_ctx);
	}
	Mb_ctx = -1;
	Mb_connected = false;
}

uint16_t cModbus::Crc16(uint8_t *buffer, int buffer_length)
{
	uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) 
	{
        i = crc_hi ^ *buffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }

	return (crc_hi << 8 | crc_lo);
}

int cModbus::SendRequest(uint8_t *req, int req_lenght, uint8_t *resp)
{
	std::lock_guard<std::mutex> lock(send_mtx);
	usleep(T3_5); //Make sure we are in idle state
	if (req_lenght > (MODBUS_RTU_MAX_ADU_LENGTH -2))
	{
		LOG("Modbus: Error Send Request to long\n");
		return 0;
	}
	if (req_lenght < 1)
	{
		LOG("Modbus Request to send: %d nothing to send", req_lenght);
		return 0;
	}
	int send_lenght = req_lenght;
	uint8_t send_buffer[MODBUS_RTU_MAX_ADU_LENGTH];
	uint16_t crc;
	
	crc = Crc16(req, req_lenght);
	
	memcpy(send_buffer, req, req_lenght);
	MODBUS_SET_INT16_TO_INT8(send_buffer, send_lenght, crc);
	send_lenght = send_lenght+2;
	int send = 0;
	int rc;
	if (Mb_debug)
	{
		printf("Modbus Sending Request: ");
		int n;
		for (n=0; n<send_lenght; n++)
		{
			printf("<%02X>", send_buffer[n]);
		}
		printf("\n");
	}
	while (send < send_lenght)
	{
		rc = write(Mb_ctx, &send_buffer[send], (send_lenght - send));
		if (rc <= 0)
		{
			LOG("Modbus Error on send rc=%d\n", rc);
			return -1;
		}
		send = send + rc;
		
	}
	if (Mb_debug)
		printf("%d Bytes send\n", send);
	
	return Receive(resp, idle);
	
}

int cModbus::ReceiveRequest(uint8_t *req)
{
	return Receive(req);
}

int cModbus::Receive(uint8_t *buffer, eState input_state)
{
	uint8_t read_buffer[MODBUS_RTU_MAX_ADU_LENGTH];                
	int readbuffer_index = 0;
	Mb_state = input_state;
	char discardbyte;
	bool Complete = false;
   for (;;)
     {
		 if (readbuffer_index == MODBUS_RTU_MAX_ADU_LENGTH)
			 break;
		 int rc;
		 if(Mb_state == initial)
		 {
			 rc = Timeout(Mb_ctx, 0, T3_5);
			 if (rc == -1)
			 {
				 LOG("Modbus: Error on Select()\n");
				 return -1;
			 }
			 else if (rc == -2)  //Timeout
			 {
				 Mb_state = idle;
				 if (Mb_debug)
					printf("Modbus Timeout expired switching from initial to idle State\n");
				 continue;
			 }
			 else if (rc >= 0)
			 {
				 if (read(Mb_ctx, &discardbyte, 1) > 0)
				 {
					 if (Mb_debug)
						printf("Modbus beginning of request not found discard Byte: %02X\n", discardbyte);
					 continue;
				 }
				 else 
				 {
					 LOG("Modbus: Error on Read and discard Byte\n");
					 return -1;
				 }
			 }
		 }
			 
			 if(Mb_state == idle)
			 {
				 rc = Timeout(Mb_ctx, 5, 0); //Block until a byte receives for reading Max 5 Seconds
				 if (rc == -1)
				{
					LOG("Modbus: Error on Select()\n");
					return -1;
				}
				else if (rc == -2)  //Timeout
				{
					if (Mb_debug)
						printf("Modbus Timeout expired in idle state\n");
					return 0;
				}
				else if (rc >= 0)
				{
					if (read(Mb_ctx, &read_buffer[readbuffer_index], 1) == 1)
					{
						Mb_state = processing;  //first byte received switching to processing 
						if (Mb_debug)
							printf("Modbus new Request received: <%02X>", read_buffer[readbuffer_index]);
						readbuffer_index++;
						continue;
					}
					else 
					{
						LOG("Modbus: Error on Read Byte\n");
						return -1;
					}
				}
			 }
			 
			 if (Mb_state == processing)
			 {
				 rc = Timeout(Mb_ctx, 0, T3_5); //After Timeout T3_5 a Full Request shold be received
				 if (rc == -1)
					{
						LOG("Modbus: Error on Select()\n");
						return -1;
					}
				else if (rc == -2)  //Timeout
					{
						if (Mb_debug)
						{
							printf("\n");
							printf("%d bytes received\n", readbuffer_index);
						}
						Mb_state = idle;
						Complete = true;
						break;
					}	
				else if (rc > T1_5) //Inter Char Timout expired Request uncomplete
					{
						if (Mb_debug)
							printf("Inter Char Timeout expired Request uncomplete discarding!   Timeout: %d\n", rc);
						readbuffer_index = 0;
						Mb_state = initial;
						continue;
					}
				else
					{
						if (read(Mb_ctx, &read_buffer[readbuffer_index], 1) == 1)
						{
							if (Mb_debug)
								printf("<%02X>", read_buffer[readbuffer_index]);
							readbuffer_index++;
							continue;
						}
						else 
						{
							LOG("Modbus: Error on Read Byte\n");
							return -1;
						}
					}
			 }
	 }
	 
	 
	 if (Complete && readbuffer_index > 3)
	 {
		 if (read_buffer[0] != Mb_slave)
		 {
			 if (Mb_debug)
				 printf("Modbus Request for Slave ID: %d not %d\n", read_buffer[0], Mb_slave);
			return 0;
		 }
		 uint16_t crc;
		 uint16_t crc_received;
		 crc = Crc16(read_buffer, (readbuffer_index-2));
		 crc_received = MODBUS_GET_INT16_FROM_INT8(read_buffer, (readbuffer_index-2));
		 if (crc_received == crc)
		 {
			memcpy(buffer, read_buffer, (readbuffer_index));
			return (readbuffer_index);
		 }
		 else
		 {
			 if (Mb_debug)
				 printf("Modbus CRC Received: %04X != CRC Caculated: %04X\n", crc_received, crc);
			 return 0;
		 }
	 }
	 else 
	 {
		 return 0;
	 }
	 
	 
}

int cModbus::SendConfirmation(uint8_t *conf, int conf_lenght)
{
	if (conf_lenght > (MODBUS_RTU_MAX_ADU_LENGTH -2))
	{
		LOG("Modbus: Error Confirmation to long\n");
		return 0;
	}
	int send_lenght = conf_lenght;
	uint8_t send_buffer[MODBUS_RTU_MAX_ADU_LENGTH];
	uint16_t crc;
	
	crc = Crc16(conf, conf_lenght);
	
	memcpy(send_buffer, conf, conf_lenght);
	MODBUS_SET_INT16_TO_INT8(send_buffer, send_lenght, crc);
	send_lenght = send_lenght+2;
	int send = 0;
	int rc;
	if (Mb_debug)
	{
		printf("Modbus Sending Confirmation: ");
		int n;
		for (n=0; n<send_lenght; n++)
		{
			printf("<%02X>", send_buffer[n]);
		}
		printf("\n");
	}
	while (send < send_lenght)
	{
		rc = write(Mb_ctx, &send_buffer[send], (send_lenght - send));
		if (rc <= 0)
		{
			LOG("Modbus Error on send\n");
			return 0;
		}
		send = send + rc;
	}
	if (Mb_debug)
			printf("%d Bytes send\n", send);
		return send;
}














