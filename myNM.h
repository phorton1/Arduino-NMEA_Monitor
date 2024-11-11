//-------------------------------------------
// myNM.h
//-------------------------------------------

#pragma once

#define USE_MY_ESP_TELNET		 1

#define NMEA2000_CLASS	tNMEA2000_mcp

#include <NMEA2000_mcp.h>
#include <N2kDeviceList.h>
#include <ActisenseReader.h>
#include <WiFi.h>

#if USE_MY_ESP_TELNET
	#include <myESPTelnetStream.h>
	#define ESPTELNET_CLASS myESPTelnetStream
#else
	#include <ESPTelnetStream.h>
	#define ESPTELNET_CLASS ESPTelnetStream
#endif


//------------------------------
// PGNS
//------------------------------

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


//---------------------------------
// compile defines
//---------------------------------

#define MONITOR_NMEA_ADDRESS	99
	// The default address of this device.
	// I do not currently handle it changing from the library
	// globally defined to spread myNM class implementation
	// across several cpp files.


#define CAN_CS_PIN		5
	// I use this instead of the default ESP32 CS pin15 because
	// the default ESP32 CS pin15 is soldered on the ideaspark
	// board to the ST7789 oled display.
#define USE_HSPI		1
	// In addition, I use the ESP32 alternative HSPI for the
	// mcp2515 so that it isn't mucked with by the st7789 display
	// which is soldered to the default ESP32 VSPI pins.


//---------------------------------
// NVS Defaults
//---------------------------------
// These are defaults that can subsquently be changed
// and stored to NVS vis the serial command processor

#define DEFAULT_WITH_ACTISENSE	1
	// Actisense requirres an FDTI232 adapter on Serial2
	//`The main Serial port is always used for debugging
	// output and the serial command processor.

#define DEFAULT_WITH_OLED		1
	// implements ST7789 oled to act as a small system monitor display

#define DEFAULT_WITH_TELNET		1
	// WIFI is only used to drive TELNET, which is assigned
	// to myDebug::extraSerial when telnet is connected.
	// Connects using myPrivate.h credentials, then nstantiates
	// telnet object and sets myDebug's extraSerial output to telnet
	// upon connection and null upon disconnection.

#define DEFAULT_WITH_DEVICE_LIST	1
	// make this device also keep a list of devices

#define DEFAULT_ADD_SELF_TO_DEVICE_LIST  1
	// and if so, add THIS device to the list with explicit
	// calls to deviceList::handleMsg() at startup.

#define DEFAULT_BROADCAST_NMEA200_INFO	0
	// BROADCAST_NMEA200_INFO is a kludge to call SendProductInfo(),
	// SendConfigInfo, and so on at program startup, at top of loop(),
	// to make sure that other devices get it even if they are not
	// requesting it.

#define DEFAULT_DEBUG_BUS	1
	// Will show messages that are not otherwise explicitly handled
	// by onBusMessage() in WHITE
#define DEFAULT_DEBUG_ACTISENSE	0
	// Will show messages that are recieved via ACTISENSE in CYAN
#define DEFAULT_DEBUG_SELF	0
	// Will show messages that are sent with msgToSelf(), and
	// the processing in CANGetGrame() self in MAGENTA
#define DEFAULT_DEBUG_SENSORS	1
	// Will show parsed sensor mssages as they come into onBusMessage()
	

//---------------------------------
// class definition
//---------------------------------

class myNM : public NMEA2000_CLASS
	// statically constructed, so by default, all members are 0
{
public:

	myNM(
		unsigned char cs_pin 	= CAN_CS_PIN,
		unsigned char clock_set = MCP_8MHz,
        unsigned char int_pin 	= 0xff,
		uint16_t rx_bufs		= MCP_CAN_RX_BUFFER_SIZE) :
		NMEA2000_CLASS(cs_pin,clock_set,int_pin,rx_bufs)
	{}

	void setup(
		bool with_actisense			 = DEFAULT_WITH_ACTISENSE,
		bool with_oled 				 = DEFAULT_WITH_OLED,
		bool with_telnet 			 = DEFAULT_WITH_TELNET,
		bool with_device_list 		 = DEFAULT_WITH_DEVICE_LIST,
		bool add_self_to_device_list = DEFAULT_ADD_SELF_TO_DEVICE_LIST,
		bool broadcast_nmea_info 	 = DEFAULT_BROADCAST_NMEA200_INFO  );

	void loop();

	// utilities

	static String msgToString(const tN2kMsg &msg, const char *prefix=0, bool with_data=true);
		// prefix may be 0, used to identify source of message for display purposes
		// will be prepended to string if provided

	static bool m_DEBUG_BUS;
	static bool m_DEBUG_SELF;
	static bool m_DEBUG_ACTISENSE;
	static bool m_DEBUG_SENSORS;
	

protected:

	ESPTELNET_CLASS *m_telnet;
	tN2kDeviceList *m_device_list;
	tActisenseReader *m_actisense_reader;

	bool m_broadcase_nmea_info;
	
	static void onBusMessage(const tN2kMsg &msg);
	static void onActisenseMessage(const tN2kMsg &msg);
	void broadcastNMEA2000Info();
	void handleSerialChar(uint8_t byte, bool telnet=false);


	void initOled();
		// in nmOled.cpp

	void listDevices();
	void addSelfToDeviceList();
		// in nmDeviceList.cpp

	void msgToSelf(const tN2kMsg &msg, uint8_t dest_addr);
	bool CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf) override;
		// in nmSelf.cpp
		// These two methods implement msgToSelf() method that can insert
		// NMEA2000 messages for THIS device into the CANBUS rx queue so that
		// they are acted on by the library code. Primarily used to handle actisense
		// messages to THIS device (node).
		//
		// Otherwise, in order to implement THIS as a node (with product
		// information, etc), that is ALSO an actisenseReader WITH
		// a deviceList, one has to (a) parse PGN_REQUESTa, and explicitly
		// reply with product, configuration, address claims, and pgn_lists,
		// and (b) specifically all actisense) messages to the
		// deviceList::handleMsg() method.

	// telnet implementation in nmTelnet.cpp

	void initTelnet();
	static void onTelnetConnect(String ip);
	static void onTelnetDisconnect(String ip);
	static void onTelnetReconnect(String ip);
	static void onTelnetConnectionAttempt(String ip);
	
}; 	// class myNM


extern myNM my_nm;
	// in NMEA_Monitor.ino




