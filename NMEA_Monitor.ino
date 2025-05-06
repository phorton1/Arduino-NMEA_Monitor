//-------------------------------------------
// NMEA_Monitor.cpp
//-------------------------------------------
// Note that the monitor needs DEBUG_RXANY compile flag.
// Pins to	MCP2515 Canbus module	pin 1 == VBUS
// IdeaSpark st7789 module
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

#define dbg_mon 	1

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

