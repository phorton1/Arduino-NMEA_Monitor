//-------------------------------------------
// myNM.h
//-------------------------------------------

#pragma once

#define NMEA2000_CLASS	tNMEA2000_mcp

#include <NMEA2000_mcp.h>
#include <N2kDeviceList.h>



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
		bool with_device_list 		 = DEFAULT_WITH_DEVICE_LIST,
		bool add_self_to_device_list = DEFAULT_ADD_SELF_TO_DEVICE_LIST,
		bool broadcast_nmea_info 	 = DEFAULT_BROADCAST_NMEA200_INFO  );

	void loop();

	// utilities

	static String msgToString(const tN2kMsg &msg, const char *prefix=0, bool with_data=true);
		// prefix may be 0, used to identify source of message for display purposes
		// will be prepended to string if provided
	static String getCommandUsage();

	static bool m_DEBUG_BUS;
	static bool m_DEBUG_SENSORS;
	

protected:

	tN2kDeviceList *m_device_list;

	bool m_broadcase_nmea_info;
	
	static void onBusMessage(const tN2kMsg &msg);
	void broadcastNMEA2000Info();
	void handleSerialChar(uint8_t byte, bool telnet=false);

	void listDevices();
	void addSelfToDeviceList();
		// in nmDeviceList.cpp
	
}; 	// class myNM


extern myNM my_nm;
	// in NMEA_Monitor.ino




