//-------------------------------------------
// NM.h
//-------------------------------------------
// Driving defines and externs for NMEA_Monitor sketch

#pragma once

#define dbg_mon			0
#include <myDebug.h>



#define USE_MY_MCP_CLASS	1
	// Use a derived NMEA2000_mymcp class that has a msgToSelf()
	// method that can insert NMEA2000 messages for THIS device
	// into the CANBUS rx queue so that they are acted on by the
	// library code. Primarily used to handle actisense messages
	// to THIS device (node).
	//
	// Otherwise, in order to implement THIS as a node (with product
	// information, etc), that is ALSO an actisenseReader WITH
	// a deviceList, one has to (a) parse PGN_REQUESTa, and explicitly
	// reply with product, configuration, address claims, and pgn_lists,
	// and (b) specifically all actisense) messages to the
	// deviceList::handleMsg() method.
	//
	// Not only does this duplicate much existing code, it was
	// also very complicated and long winded to learn and write.
	// So I wrote msgToSelf(). I currently am keeping the old
	// explicit code in the handleActisenseMsg() method for posterity.

	

#if USE_MY_MCP_CLASS
	#include "myNMEA2000_mcp.h"
#else
	#include <NMEA2000_mcp.h>
#endif



#define MONITOR_NMEA_ADDRESS	99
	// The default address of this device.
	// I do not currently handle it changing from the library


#define CAN_CS_PIN		5
	// I use this instead of the default ESP32 CS pin15 because
	// the default ESP32 CS pin15 is soldered on the ideaspark
	// board to the ST7789 oled display.

#define USE_HSPI		1
	// In addition, I use the ESP32 alternative HSPI for the
	// mcp2515 so that it isn't mucked with by the st7789 display
	// which is soldered to the default ESP32 VSPI pins.



// WIFI is only used to drive TELNET, which is assigned
// to myDebug::extraSerial when telnet is connected.

#define WITH_WIFI		0
	// Needed for WITH_TELNET, to connect to
	// Wifi network using my private credentials
#define WITH_TELNET		0
	// Instantiates telnet object and sets myDebug's
	// extraSerial output to telnet upon connection
	// and null upon disconnection



// The OLED monitor is not very useful.
// It is too slow to be a substitute, or hooked into, myDdebug.
// This code will be going away to be replaced with using
// the OLED to show non-scrolling state information, like
// Speed, Depth, Heading, Temperature, Wifi, etc.
//
// At this time, I use the larger ST7789 multi-color display
// as this is intended to be a somewhat "general purpose"
// monitor.
//
// The smaller fixed color SSD1306 displays might be useful
// for specific NMEA2000 devices that are more limited in
// their scope.

#define WITH_OLED_MONITOR		1
	// Use the OLED (ST7789) on an Ideaspark ESP32 for
	// slow scrolling manuall coded output.


// The DeviceList is less than completely useful.
// Maybe nice on the oled in a semi-finished deployment.

#define WITH_DEVICE_LIST	1
	// make this device also keep a list of devices
#define ADD_SELF_TO_DEVICE_LIST  1
	// and if so, add THIS device to the list with explicit
	// calls to deviceList::handleMsg() at startup.



#define WITH_SERIAL_COMMANDS	1
	// WITH_SERIAL_COMMANDS is implements a serial command
	// processor that will probably be extended. It currently supports:
	// q = put PGN_REQUEST(59904, PGN_PRODUCT_INFO) on the bus
	// i = call sendProductInfo() to put our product info on the bus


// kludges

#define BROADCAST_NMEA200_INFO	0
	// BROADCAST_NMEA200_INFO is a kludge to call SendProductInfo(),
	// SendConfigInfo, and so on at program startup, at top of loop(),
	// to make sure that other devices get it even if they are not
	// requesting it.



// Getting those out of the way, the main defines of interest
// revolve around ACTISENSE, ProductInformation, and built in
// NMEA library debugging.

#define ACTISENSE_PORT	Serial2
#define DEBUG_PORT		Serial		// should/MUST be defined
	// if these are defined, they will be opened at 115200,
	// and 921600 respectively.


// Only one of next two should be defined as 1.
// The compile should break if they are defined and ACTISENSE_PORT is not

#define TO_ACTISENSE    	1
	// implements an actisenseReader that communicates over
	// USB Serial with various laptop applications.
#define FAKE_ACTISENSE    	0
	// An attempt to understand and reply to the actisense
	// NMEAReader Applications "Get Operating Mode" proprietary
	// messags.

#define SHOW_BUS_TO_SERIAL	1
	// if neither TO_ACTISENSE or FAKE_ACTISENSE are set,
	// but this is, then the nmea2000 stream will be forwarded
	// to the DEBUG_PORT in text mode.


//----------------------------------
// end of conditional defines
//----------------------------------
// start of actual "stuff"

#if WITH_DEVICE_LIST
	#include <N2kDeviceList.h>
	extern tN2kDeviceList *device_list;
	extern void listDevices(bool force = false);
		// in nmDeviceList.cpp
	#if ADD_SELF_TO_DEVICE_LIST
		extern void addSelfToDeviceList();
	#endif

#endif


#if WITH_OLED_MONITOR
	#include <myOledMonitor.h>
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
// System requests which I need to be aware of

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


// upcoming generator PGNS

#define PGN_AGS_CONFIG_STATUS		127512L
#define PGN_AGS_STATUS				127514L

// 130316 "Temperature, Extended Range",
	//  temperature sensor with data type 14 (Exhaust Gas).

#define PGN_AC_VOLTAGE_FREQ			127747L 	// "AC Voltage / Frequency-Phase A".
	// see https://www.yachtd.com/news/  July 31, 2024 Engine Gateway YDEG-04

// in NM.cpp

#if USE_MY_MCP_CLASS
	extern myNMEA2000_mcp nmea2000;
#else
	extern tNMEA2000_mcp nmea2000;
#endif



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


	
