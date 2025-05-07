//-------------------------------------------
// NMEA_Monitor.cpp
//-------------------------------------------
// Note that the monitor needs DEBUG_RXANY compile flag.
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


void loop()
{
	my_nm.loop();
}

