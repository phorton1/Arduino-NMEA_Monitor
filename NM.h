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


#define CAN_CS_PIN		5
#define USE_HSPI		1
	// use ESP32 alternative HSPI for mcp2515 so that it
	// isn't mucked with by the st7789 display

#define WITH_SERIAL2	1
	// Use Serial2 port for dbgSerial
#define WITH_OLED		0
	// Use the OLED (ST7789) on an Ideaspark ESP32 for
	// slow tiny additional output
#define WITH_WIFI		0
	// Needed for WITH_TELNET, connect to
	// Wifi network using my private credentials
#define WITH_TELNET		0
	// Instantiate telnet object and
	// Set myDebug's extraSerial output to telnet
	// upon connection and null upon disconnection


#define WITH_SERIAL_COMMANDS	1
	// implements a serial command processor
	// on dbgSerial if it's not 0


#define PASS_THRU_TO_SERIAL		1
	// Required for Actisense, but also perhaps handy
	// if not to see "Text" representation of NMEA2000 bus

// Only ONE of the next two should be defined.
// and really only if PASS_THRU_TO_SERIAL is set

#define FAKE_ACTISENSE			0
	// use fake actisense methods for debugging
#define TO_ACTISENSE			1
	// use real tActisenseReader object
	// In last two cases we set the USB Serial port to 115200 for
	// output to the ActisenseReader application and handle
	// input from USB Serial in one of these two ways


#define BROADCAST_NMEA200_INFO	0
	// Kludge to call SendProductInfo, SendConfigInfo, and
	// so on at program startup (at top of loop))?



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

		// in nmTelnet.cpp

		extern ESPTelnetStream telnet;
		extern bool telnet_connected;
		extern void init_telnet();
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


//-----------------------------------------------
// constants for debugging onNMEA2000Message()
//-----------------------------------------------

#define TEMP_ERROR_CHECK	1
	// 1 = compare subsequent temperatures and report errors

#define DISPLAY_DETAILS		1
	// 0 = off
	// 1 = show just temperatures and PGNs
	// 2 = shot detailed meessage header, temperature details, and PGNs

#define OLED_DETAILS		1
	// 0 = show only PGNs on st7789
	// 1 = show Temps and PGNS on OLED
	
