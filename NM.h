//-------------------------------------------
// NM.h
//-------------------------------------------
// Driving defines and externs for NMEA_Monitor sketch

#pragma once

#define dbg_mon			0

#include <myDebug.h>
#include <NMEA2000_mcp.h>
// #include <N2kMessages.h>
// #include <N2kMessagesEnumToStr.h>
// #include <SPI.h>


#define MONITOR_NMEA_ADDRESS	99


#define CAN_CS_PIN		5
#define USE_HSPI		1
	// use ESP32 alternative HSPI for mcp2515 so that it
	// isn't mucked with by the st7789 display

// WIFI is only used to drive TELNET, which is only assigned
// to myDebug::extraSerial when telnet is connected.

#define WITH_WIFI		0
	// Needed for WITH_TELNET, connect to
	// Wifi network using my private credentials
#define WITH_TELNET		0
	// Instantiate telnet object and
	// Set myDebug's extraSerial output to telnet
	// upon connection and null upon disconnection


// The OLED display as a monitor is not very useful,
// plus it's backwards, in a sense, where it would be, perhaps
// better to think of it as an alternative myDebug stream.
// Otherwise, paritcularly if I went to the SSD1306,
// the oled is probably a "special" display for limited
// information (i.e. heading/speed/depth/temperature)
// or maybe a brief device list, or generator details when
// I learn them, but it's not very useful as a scrolling
// output device.

#define WITH_OLED		0
	// Use the OLED (ST7789) on an Ideaspark ESP32 for
	// slow tiny additional output


// The DeviceList is less than completely useful.
// Maybe nice on the oled in a semi-finished deployment.

#define WITH_DEVICE_LIST	0
	// make this device also keep a list of  devices


// kludges

#define WITH_SERIAL_COMMANDS	1
	// WITH_SERIAL_COMMANDS is a kludge that currently
	// q = put PGN_REQUEST(59904, PGN_PRODUCT_INFO) on the bus
	// i = call sendProductInfo() to put our product info on the bus

#define BROADCAST_NMEA200_INFO	0
	// BROADCAST_NMEA200_INFO is a kludge to call SendProductInfo(),
	// SendConfigInfo, and so on at program startup, at top of loop(),
	// to make sure that other devices get it even if they are not
	// requesting it.

// Getting those out of the way, the main defines of interest
// revolve around ACTISENSE, ProductInformation, and debugging.
//
// With this next mod I am going to change it so that TO_ACTISENSE
// is synonymous with WITH_SERIAL2 and that the actisense port
// is that SERIAL2 at 115200, and that I basically never change
// the USB Serial port as the debugger/programmer, although the
// option of showing the NMEA2000 stream by forwarding to it
// remains.



#define ACTISENSE_PORT	Serial2
#define DEBUG_PORT		Serial
	// if these are defined, they will be opened at 115200,
	// and 921600 respectively.

#define TO_ACTISENSE    	1
#define FAKE_ACTISENSE    	0
	// only one of the above should be defined as 1
	// compile will break if they are defined an ACTISENSE_PORT is not

#define SHOW_BUS_TO_SERIAL	1
	// if neither TO_ACTISENSE or FAKE_ACTISENSE are set,
	// but this is, then the nmea2000 stream will be forwarded
	// to the DEBUG_PORT in text mode.


// OLD:
//
//	#define WITH_SERIAL2	1
//		// Use Serial2 port for dbgSerial
//	#define PASS_THRU_TO_SERIAL		1
//		// Required for Actisense, but also perhaps handy
//		// if not to see "Text" representation of NMEA2000 bus
//
//	// Only ONE of the next two should be defined.
//	// and really only if PASS_THRU_TO_SERIAL is set
//
//	#define FAKE_ACTISENSE			0
//		// use fake actisense methods for debugging
//	#define TO_ACTISENSE			1
//		// use real tActisenseReader object
//		// In last two cases we set the USB Serial port to 115200 for
//		// output to the ActisenseReader application and handle
//		// input from USB Serial in one of these two ways





//----------------------------------
// end of conditional defines
//----------------------------------
// start of actual "stuff"

#if WITH_DEVICE_LIST
	#include <N2kDeviceList.h>
	extern tN2kDeviceList *device_list;
	extern void listDevices(bool force = false);
		// in nmDeviceList.cpp
#endif


#if WITH_OLED
	#include <myOledMonitor.h>
	#define WITH_OLED_TASK		1
	extern myOledMonitor mon;
#endif


#if WITH_WIFI
	#include <WiFi.h>
	#define LED_WIFI		2	// onboard led
	#if WITH_TELNET
		#include <ESPTelnetStream.h>
		extern ESPTelnetStream telnet;
		extern bool telnet_connected;
		extern void init_telnet();
			// in nmTelnet.cpp
	#endif
#endif


//------------------------------------------
// NMEA2000 library
//------------------------------------------

#define PGN_REQUEST					59904L
#define PGN_ADDRESS_CLAIM			60928L
#define PGN_PGN_LIST				126464L
#define PGN_HEARTBEAT				126993L
#define PGN_PRODUCT_INFO			126996L
#define PGN_DEVICE_CONFIG			126998L
#define PGN_TEMPERATURE    			130316L
#define PGN_HEADING					127250L
#define PGN_SPEED					128259L
#define PGN_DEPTH					128267L

// in NM.cpp

extern tNMEA2000_mcp nmea2000;
extern const unsigned long AllMessages[];

extern void nmea2000_setup();
extern void onNMEA2000Message(const tN2kMsg &msg);

extern void broadcastNMEA2000Info();
	// in factActisense.cpp


#if TO_ACTISENSE
	#include <ActisenseReader.h>
	extern tActisenseReader actisenseReader;
	extern void handleActisenseMsg(const tN2kMsg &msg);
#endif

#if FAKE_ACTISENSE
	// in factActisense.cpp
	extern void doFakeActisense();
#endif


	
