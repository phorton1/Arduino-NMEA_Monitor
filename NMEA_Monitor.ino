//-------------------------------------------
// NMEA_Monitor.cpp
//-------------------------------------------
// Note that the MCP2515 monitor needs DEBUG_RXANY compile flag.
// Pins to	MCP2515 Canbus module	pin 1 == VBUS
// Currently wired same as the NMEA_SENSOR, using lregular SPI
//	   on DEV0_board_with_holes
// Pins to	MCP2515 Canbus module	pin 1 == VBUS
//
//		module			ESP32
//
//		(7) INT			GPIO22			orange
//		(6) SCK			GPIO18 (SCK)	white
//		(5) (MO)SI		GPIO23 (MOSI)  	yellow
//		(4) (MI)SO		GPIO19 (MISO)	green
// 		(3) CS			GPIO5  (CS)		blue
// 		(2) GND			GND				black
//		(1) VCC 	 	VBUS			red
//
// Previouis IdeaSpark st7789 module
//
//		module			ESP32
//
//		(1) VCC 	 	(1)  VBUS			red
// 		(2) GND			(2)  GND			brown
//		(5) (MO)SI		(3)  GPIO13 (MOSO)  green		alt HSPI
//		(4) (MI)SO		(4)  GPIO12 (MISO)	yellow		alt HSPI
//		(6) SCK			(5)  GPIO14 (SCLK)	blue		alt HSPI
// 		(3) CS			(23) GPIO5	altCS	orange		15 is alt HSP-CS, so 5 (normal SPI CS) is 'alt' for this
//		(7) INT			nc

#include "myNM.h"
#include <myDebug.h>

#define dbg_mon 	0


myNM my_nm;


void setup()
{
	Serial.begin(921600);
	delay(1000);
	Serial.println("WTF");
	delay(1000);

	const char *how = USE_NMEA2000_ESP ? "ESP32" : "MCP";
	display(dbg_mon,"NMEA_Monitor.ino setup(%s) v1.0 started",how);
	proc_entry();
	
	my_nm.setup(
		DEFAULT_WITH_DEVICE_LIST,
		DEFAULT_ADD_SELF_TO_DEVICE_LIST,
		DEFAULT_BROADCAST_NMEA200_INFO  );

	proc_leave();
	display(dbg_mon,"NMEA_Monitor.ino setup() finished",0);
}



//--------------------------------------------
// low level canbus output debug display
//--------------------------------------------

#define DEBUG_MCP2515_OUT_LOW	0

#if DEBUG_MCP2515_OUT_LOW
	// overrides weakly linked prh_dbg_mcp2515_write() method
	// in #ifdef PRH_MODS in /libraries/CAN_BUS_Shield/mcp_can.cpp
	// to circular buffer the bytes written in the interrupt handler
	// and display() the in the main thread loop() method.
	// This was useful in the initial debugging of NMEA2000 to my Raymarine E80 MFD.

	#define MAX_CAN_MESSAGES	100

	typedef struct
	{
		byte len;
		byte first[4];
		byte buf[8];
	} dbg_msg_t;


	static volatile bool in_debug = 0;
	static int can_head = 0;
	static int can_tail = 0;
	static dbg_msg_t dbg_msgs[MAX_CAN_MESSAGES];


	void prh_dbg_mcp2515_write(byte *first, volatile const byte *buf, byte len)
	{
		int new_head = can_head  + 1;
		if (new_head >= MAX_CAN_MESSAGES)
			new_head = 0;
		if (new_head == can_tail)
		{
			my_error("DBG_CAN_MSG BUFFER OVERFLOW",0);
			return;
		}

		in_debug = 1;
		dbg_msg_t *msg = &dbg_msgs[can_head++];
		if (can_head >= MAX_CAN_MESSAGES)
			can_head = 0;

		msg->len = len;
		memcpy(msg->first,first,4);
		memcpy(msg->buf,(const void*) buf,len);
		in_debug = 0;
	}


	static void show_dbg_can_messages()
	{
		int head = can_head;
		if (in_debug)
			return;

		while (can_tail != head)
		{
			dbg_msg_t *msg = &dbg_msgs[can_tail++];
			if (can_tail >= MAX_CAN_MESSAGES)
				can_tail = 0;

			static char obuf[80];
			static int ocounter = 0;
			sprintf(obuf,"(%d) --> %02x%02x%02x%02x ",
				ocounter++,
				msg->first[0],
				msg->first[1],
				msg->first[2],
				msg->first[3]);

			int olen = strlen(obuf);
			for (int i=0; i<msg->len; i++)
			{
				sprintf(&obuf[olen],"%02x ",msg->buf[i]);
				olen += 3;
			}
			Serial.println(obuf);
		}
	}
#endif



//--------------------------------------------
// loop()
//--------------------------------------------


void loop()
{
	#if DEBUG_MCP2515_OUT_LOW
		show_dbg_can_messages();
	#endif

	my_nm.loop();
}

