//-------------------------------------------
// NMEA_Monitor.cpp
//-------------------------------------------

#include "myNM.h"
#include <myDebug.h>

#define dbg_mon 	0

#define INT_PIN_NONE	0xff

myNM my_nm(CAN_CS_PIN,MCP_8MHz,INT_PIN_NONE,MCP_CAN_RX_BUFFER_SIZE);



void setup()
{
	Serial.begin(921600);
	delay(2000);
	Serial.println("WTF");
	display(dbg_mon,"NMEA_Monitor.ino setup() started",0);
	proc_entry();
	
	my_nm.setup(
		DEFAULT_WITH_ACTISENSE,
		DEFAULT_WITH_OLED,
		DEFAULT_WITH_TELNET,
		DEFAULT_WITH_DEVICE_LIST,
		DEFAULT_ADD_SELF_TO_DEVICE_LIST,
		DEFAULT_BROADCAST_NMEA200_INFO  );

	proc_leave();
	display(dbg_mon,"NMEA_Monitor.ino setup() finished",0);
}


void loop()
{
	my_nm.loop();
}

