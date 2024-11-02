//-------------------------------------------
// NMEA_Monitor.cpp
//-------------------------------------------
// NOTE THAT setup_platform (compile_arduino.pm) has DEBUG_RXANY=1,
// which solved the mysterious problem of NMEA_Monitor not getting
// anything from NMEA_Sensor, by modifying the behavior of MCP_CAN::
// mcp2515_init() to allow any messages.
//
// Although the proper solution is probably to setup Extended Messages
// correctly, after experimentaiton, including setting them on both
// the Sensor and the monitor, using UL and PROGMEM constants, I could
// not get them to work, so the DEBUG_RXANY compile flag is currently
// required for the monitor (but not the sensor).
//
// from https://knowledgebase.fischerpanda.de/?p=3454
// Is it possible to switch the generator on and off via NMEA 2000®?
// 		Use the PGN 126208 to access the PGN 127512 (AGS Configuration Status)
//		in command mode. Specify the field 3 in the request. For a detailed
//		description, use the original documentation from the NMEA that has
//		to be bought from the NMEA website.
//
// The st7789 display and mcp2515 canbus module both use SPI, and there
// were conflicts when both used the same SPI device. An mcp2515 message
// gets lost, or we get bad data when writing to the display.
// We should EASILY be able to handle 50 messages a second, which is
// only about 50*12 = 600 bytes, or about 4800 bits, per second.
// The problem resolved itself when I used HSPI for the st2515.


#include <myDebug.h>
#include <SPI.h>

#define dbg_mon			0

#define CAN_CS_PIN		5
#define USE_HSPI		0
	// use ESP32 alternative HSPI for mcp2515 so that it
	// isn't mucked with by the st7789 display
	// only currently supported with HOW_BUS_NMEA2000
#define WITH_ST7789		0
#define WITH_WIFI		1
#define WITH_TELNET		1



#define HOW_BUS_MPC2515		0		// use github/autowp/arduino-mcp2515 library
#define HOW_BUS_CANBUS		1		// use github/_ttlappalainen/CAN_BUS_Shield mpc2515 library
#define HOW_BUS_NMEA2000	2		// use github/_ttlappalainen/CAN_BUS_Shield + ttlappalainen/NMEA2000 libraries

#define HOW_CAN_BUS			HOW_BUS_NMEA2000


#if USE_HSPI
	SPIClass *hspi;
		// MOSI=13
		// MISO=12
		// SCLK=14
		// default CS = 15
#endif


#if WITH_WIFI
	#include <WiFi.h>
	#include <prhPrivate.h>
	#define LED_WIFI		2	// onboard led

	#if WITH_TELNET
		#include <ESPTelnetStream.h>
	#endif
#endif

#if WITH_ST7789
	#include <myST7789Monitor.h>
	myST7789Monitor mon;
	#define WITH_ST7789_TASK	1
#endif


#if HOW_CAN_BUS == HOW_BUS_MPC2515

	#include <mcp2515.h>

	#define WITH_ACK  					0
	#define dbg_ack						1		// only used if WITH_ACK
	#define CAN_ACK_ID 					0x037
	#define MY_TEMPERATURE_CANID		0x036

	struct MCP2515 mcp2515(CAN_CS_PIN);

#elif HOW_CAN_BUS == HOW_BUS_CANBUS

	#define SEND_NODE_ID	237
	#include <mcp_can.h>
	MCP_CAN	canbus(CAN_CS_PIN);

#elif HOW_CAN_BUS == HOW_BUS_NMEA2000

	#include <NMEA2000_mcp.h>
	#include <N2kMessages.h>
	#include <N2kMessagesEnumToStr.h>

	#define PASS_THRU_TO_SERIAL		1
	#define TO_ACTISENSE			1
	#define BROADCAST_NMEA200_INFO	0

	// forked and added API to pass the CAN_500KBPS baudrate

	tNMEA2000_mcp nmea2000(CAN_CS_PIN,MCP_8MHz,CAN_500KBPS);

	#define PGN_REQUEST					59904L
	#define PGN_ADDRESS_CLAIM			60928L
	#define PGN_PGN_LIST				126464L
	#define PGN_HEARTBEAT				126993L
	#define PGN_PRODUCT_INFO			126996L
	#define PGN_DEVICE_CONFIG			126998L
	#define PGN_TEMPERATURE    			130316L

	const unsigned long AllMessages[] = {
		PGN_REQUEST,
		PGN_ADDRESS_CLAIM,
		PGN_PGN_LIST,
		PGN_HEARTBEAT,
		PGN_PRODUCT_INFO,
		PGN_DEVICE_CONFIG,
		PGN_TEMPERATURE,
		0};


	#if TO_ACTISENSE

		#include <ActisenseReader.h>
		tActisenseReader actisenseReader;

		void handleActisenseMsg(const tN2kMsg &msg)
		{
			// N2kMsg.Print(&Serial);
			display(dbg_mon,"ACTISENSE(%d) source(%d) dest(%d) priority(%d)",
				msg.PGN,
				msg.Source,
				msg.Destination,
				msg.Priority);

			#if 0
				if (msg.Destination == 255)
					nmea2000.SendMsg(msg,-1);
				// forward the message to everyone ?!?
			#endif
			
			// one would think that the NMEA library automatically handled
			// all of these somehow.  Perhaps if I derived and was able to
			// call protected members it would.  For now I am going to
			// try hardwiring some things

			if (msg.PGN == PGN_REQUEST)	// PGN_REQUEST == 59904L
			{
				unsigned long requested_pgn;
				if (ParseN2kPGN59904(msg, requested_pgn))
				{
					display(dbg_mon,"    requested_pgn=%d",requested_pgn);
					switch (requested_pgn)
					{
						case PGN_ADDRESS_CLAIM:
							nmea2000.SendIsoAddressClaim();
							break;
						case PGN_PRODUCT_INFO:
							nmea2000.SendProductInformation();
							break;
						case PGN_DEVICE_CONFIG:
							nmea2000.SendConfigurationInformation();
							break;

						// there is no parsePGN126464() method, and there
						// is only one request for two different types of lists,
						// althouigh there are separate API methods for sending them.
						// Furthermore the documenttion says specifically that the
						// Library handles these. First I will try to just put it
						// on the net and and see what happens.
						//
						// Next I think I need to figure out "Grouip Functions" and
						// the NMEA200 library N2kDeviceList object.
						//
						// Aoon I qill nwws ro begin factoring this code into myNMEA2000 library

						case PGN_PGN_LIST:
							nmea2000.SendMsg(msg,-1);
							break;
							//	case 2:
							//		nmea2000.SendTxPGNList(255,0,false);
							//		break;
							//	case 3:
							//		nmea2000.SendRxPGNList(255,0,false);
							//		break;
					}
				}
				else
					my_error("Error parsing PGN(%d)",msg.PGN);
			}

			//
		}

	#endif



#endif


#if WITH_WIFI
#if WITH_TELNET

    static bool telnet_connected = false;


    ESPTelnetStream telnet;


    static void onTelnetConnect(String ip)
    {
        display(dbg_mon,"Telnet connected from %s",ip.c_str());
		#if WITH_ST7789
			mon.println("TELNET(%s)",ip.c_str());
		#endif

        telnet_connected = true;

		// this #if would be complicated for all cases,
		// so I am only checking TO_ACTISENSE
		#if TO_ACTISENSE
			// turn on as primary port in myDebug.h
			dbgSerial = &telnet;
		#else
			// turn on as 2nd port in myDebug.h
			extraSerial = &telnet;
		#endif

		telnet.println("Welcome to the NMEA Monitor");
    }

    void onTelnetDisconnect(String ip)
    {
		// this #if would be complicated for all cases,
		// so I am only checking TO_ACTISENSE
		#if TO_ACTISENSE
			// turn off primary port in myDebug.h
			dbgSerial = 0;
		#else
			// turn off 2nd port in myDebug.h
			extraSerial = 0;
		#endif
		
		telnet_connected = false;

        display(dbg_mon,"Telnet disconnect",0);
		#if WITH_ST7789
			mon.println("TELNET DISCONNECT");
		#endif
    }

    void onTelnetReconnect(String ip)
    {
        display(dbg_mon,"Telnet reconnected from %s",ip.c_str());
		#if WITH_ST7789
			mon.println("RE-TELNET(%s)",ip.c_str());
		#endif

        telnet_connected = true;

		// this #if would be complicated for all cases,
		// so I am only checking TO_ACTISENSE
		#if TO_ACTISENSE
			// turn on as primary port in myDebug.h
			dbgSerial = &telnet;
		#else
			// turn on as 2nd port in myDebug.h
			extraSerial = &telnet;
		#endif

		telnet.println("Welcome back to the NMEA Monitor");
    }

    void onTelnetConnectionAttempt(String ip)
    {
        display(dbg_mon,"Telnet attempt from %s",ip.c_str());
		#if WITH_ST7789
			mon.println("TELNET TRY(%s)",ip.c_str());
		#endif
    }

    void onTelnetReceived(String command)
    {
		// getting blank lines for some reason
		#if 0
			display(dbg_mon,"Telnet: %s",command.c_str());
			#if WITH_ST7789
				mon.println(command.c_str());
			#endif
		#endif
	}


    void init_telnet()
    {
        display(dbg_mon,"Starting Telnet",0);
		#if WITH_ST7789
			mon.println("Starting Telnet");
		#endif
        telnet.onConnect(onTelnetConnect);
        telnet.onConnectionAttempt(onTelnetConnectionAttempt);
        telnet.onReconnect(onTelnetReconnect);
        telnet.onDisconnect(onTelnetDisconnect);
        telnet.onInputReceived(onTelnetReceived);
		telnet.begin();
    }

#endif	// WITH_TELNET
#endif	// WITH_WIFI


//---------------------------------
// onNMEA2000Message
//---------------------------------
// grrrr - sometimes we get invalid messages
// i.e. PGN==0, or that parse but contain invalid results
// i.e. PGN=130316 temperatureC = -100000000000 ?!?!?!

#if HOW_CAN_BUS == HOW_BUS_NMEA2000

	void onNMEA2000Message(const tN2kMsg &msg)
	{
		#define TEMP_ERROR_CHECK	1
			// 1 = compare subsequent temperatures and report errors

		#define DISPLAY_DETAILS		1
			// 0 = off
			// 1 = show just temperatures and PGNs
			// 2 = shot detailed meessage header, temperature details, and PGNs

		#define ST7789_DETAILS		0
			// 0 = show only PGNs on st7789
			// 1 = show Temps and PGNS on ST7789

		static uint32_t msg_counter = 0;
		msg_counter++;

		#if DISPLAY_DETAILS == 2
			display(dbg_mon,"onNMEA2000 message(%d) priority(%d) source(%d) dest(%d) len(%d)",
				msg.PGN,
				msg.Priority,
				msg.Source,
				msg.Destination,
				msg.DataLen);
			// also timestamp (ms since start [max 49days]) of the NMEA2000 message
			// unsigned long MsgTime;
		#endif

		if (msg.PGN == PGN_TEMPERATURE)
		{
			uint8_t sid;
			uint8_t instance;
			tN2kTempSource source;
			double temperature1;
			double temperature2;

			bool rslt = ParseN2kPGN130316(
				msg,
				sid,
				instance,
				source,
                temperature1,
				temperature2);

			if (rslt)
			{
				#if TEMP_ERROR_CHECK 	// check for missing values
					static bool check_started = 0;
					static int last_temp = 0;
					int temp = temperature1;

					if (check_started &&
						last_temp != temp+1 &&
						last_temp != temp-1)
					{
						my_error("BAD_TEMP(%d) %d != %d",msg_counter,last_temp,temp);
					}

					last_temp = temp;
					check_started = true;
				#endif


				float temperature = KelvinToC(temperature1);
					// temperature2 == N2kDoubleNA

				#if DISPLAY_DETAILS == 2
					display(dbg_mon,"%s(%d) inst(%d) : %0.3fC",
						N2kEnumTypeToStr(source),
						msg_counter,
						instance,
						temperature);
				#elif DISPLAY_DETAILS == 1
					display(dbg_mon,"temp(%d) : %0.3fC",msg_counter,temperature);
				#endif

				if (!temperature1 || (temperature1 == N2kDoubleNA))
				{
					my_error("BAD_TEMP(%d) %0.3f",msg_counter,temperature1);
					return;
				}

				#if WITH_ST7789 && ST7789_DETAILS
					mon.println("temp(%d) %0.3fC",msg_counter,temperature);
				#endif
			}
			else
				my_error("PARSE_PGN(%d)",msg_counter);
		}
		else
		{
			// known invalid valid messages

			if (msg.PGN == 0 || (
				msg.PGN != 60928L &&
				msg.PGN != 126993L))
			{
				my_error("BAD PGN(%d)=%d",msg_counter,msg.PGN);
			}
			else
			{
				#if DISPLAY_DETAILS == 1
					display(dbg_mon,"PGN(%d)=%d",msg_counter,msg.PGN);
				#endif

				#if WITH_ST7789
					mon.println("PGN(%d)=%d",msg_counter,msg.PGN);
				#endif
			}
		}
	}
#endif






//-----------------------------------------------------------------
// setup
//-----------------------------------------------------------------

void setup()
{
	// this #if would be complicated for all cases,
	// so I am only checking TO_ACTISENSE
	#if TO_ACTISENSE
		// turn off primary port in myDebug.h
		// so no debugging goes to the serial port
		dbgSerial = 0;
		Serial.begin(115200);
	#else
		Serial.begin(921600);
		delay(2000);
		Serial.println("WTF");
	#endif

	display(dbg_mon,"NMEA_Monitor.ino setup(%d) started",HOW_CAN_BUS);
	
	#if WITH_ST7789
		mon.init(1,WITH_ST7789_TASK);
			// font_size,with_task,with_display,rotation
		mon.println("NMEA_Monitor(%d)",HOW_CAN_BUS);
	#endif

	#if WITH_WIFI
		pinMode(LED_WIFI,OUTPUT);
		digitalWrite(LED_WIFI,0);
		
		WiFi.mode(WIFI_STA); //Optional
		WiFi.begin(private_ssid, private_pass);
		display(dbg_mon,"Connecting to %s",private_ssid);
		delay(500);
		bool connected = (WiFi.status() == WL_CONNECTED);

		int wifi_wait = 0;
		while (!connected && wifi_wait++<10)
		{
			display(dbg_mon,"    connect wait(%d)",wifi_wait);
			delay(1000);
			connected = (WiFi.status() == WL_CONNECTED);
		}

		if (connected)
		{
			digitalWrite(LED_WIFI,1);
			String ip = WiFi.localIP().toString();
			display(dbg_mon,"Connecting to %s at %s",private_ssid,ip.c_str());
			#if WITH_ST7789
				mon.println("IP: %s",ip.c_str());
			#endif

			#if WITH_TELNET
				init_telnet();
			#endif
		}
		else
		{
			my_error("Could not connect to %s",private_ssid);
			#if WITH_ST7789
				mon.println("WIFI ERROR!!!");
			#endif
		}

	#endif

	#if USE_HSPI
		hspi = new SPIClass(HSPI);
	#endif


	#if HOW_CAN_BUS == HOW_BUS_MPC2515

		SPI.begin();
		mcp2515.reset();
		mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
		mcp2515.setNormalMode();

	#elif HOW_CAN_BUS == HOW_BUS_CANBUS

		byte err = canbus.begin(CAN_500KBPS, MCP_8MHz);
		if (err != CAN_OK)
			my_error("canbus.begin() err=%d",err);

	#elif HOW_CAN_BUS == HOW_BUS_NMEA2000

		// set buffer sizes

		#if 0
			nmea2000.SetN2kCANSendFrameBufSize(150);
			nmea2000.SetN2kCANReceiveFrameBufSize(150);
		#endif

		// set device information

		#if 1
			// the product information doesnt seem correct
			// in actisense
			//     LEN are both 1 (50ma)
			//	   ModelID is "Arduino N2K->PC"
			//	   Softare ID is "1.0.0.0"
			//	   Hardware ID is "1.0.0"
			// perhaps it is the INSTANCES or the
			// device number ...

			nmea2000.SetProductInformation(
				"prh_model_100",            // Manufacturer's Model serial code
				100,                        // Manufacturer's uint8_t product code
				"ESP32 NMEA Monitor",       // Manufacturer's Model ID
				"prh_sw_100.0",             // Manufacturer's Software version code
				"prh_mv_100.0",             // Manufacturer's uint8_t Model version
				3,                          // LoadEquivalency uint8_t 3=150ma; Default=1. x * 50 mA
				2101,                       // N2kVersion Default=2101
				1,                          // CertificationLevel Default=1
				0                           // iDev (int) index of the device on \ref Devices
				);
			nmea2000.SetConfigurationInformation(
				"prhSystems",           // ManufacturerInformation
				"MonitorInstall1",      // InstallationDescription1
				"MonitorInstall2"       // InstallationDescription2
				);
			nmea2000.SetDeviceInformation(
				1230100, // uint32_t Unique number. Use e.g. Serial number.
				130,     // uint8_t  Device function=Analog to NMEA 2000 Gateway. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
				25,      // uint8_t  Device class=Inter/Intranetwork Device. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
				2046     // uint16_t choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
				);

			// note that we typically use iDev=0 for creating THIS device
			// in each implementation; it is internal to the NME2000 library
			// and not broadcast.

		#endif


		// set Device Mode and handle pass through

		nmea2000.SetMode(tNMEA2000::N2km_ListenAndNode, 99);
			// N2km_NodeOnly
			// N2km_ListenAndNode *
			// N2km_ListenAndSend **
			// N2km_ListenOnly
			// N2km_SendOnly

		#if PASS_THRU_TO_SERIAL

			nmea2000.SetForwardStream(&Serial);

			// for actiSense program, we comment out the following line
			// for viewing in serial monitor, we leave it in

			#if !TO_ACTISENSE
				nmea2000.SetForwardType(tNMEA2000::fwdt_Text); 	// Show bus data in clear text
			#endif

			nmea2000.SetForwardOwnMessages(true);	// !TO_ACTISENSE);
				// read/write streams are the same, so dont forward own messages?

		#else
			nmea2000.EnableForward(false);
		#endif


		// nmea2000.SetDebugMode(tNMEA2000::dm_ClearText);

		#if 1
			// I could not get this to eliminate need for DEBUG_RXANY
			// compiile flag in the Monitor
			nmea2000.ExtendReceiveMessages(AllMessages);
			nmea2000.ExtendTransmitMessages(AllMessages);
		#endif

		nmea2000.SetMsgHandler(onNMEA2000Message);

		#if USE_HSPI
			nmea2000.SetSPI(hspi);
		#endif


		bool ok = nmea2000.Open();
		if (!ok)
			my_error("nmea2000.Open() failed",0);


		#if TO_ACTISENSE
			actisenseReader.SetReadStream(&Serial);
			actisenseReader.SetDefaultSource(75);
			actisenseReader.SetMsgHandler(handleActisenseMsg);
		#endif

	#endif	// HOW_CAN_BUS == HOW_BUS_NMEA2000

	display(dbg_mon,"NMEA_Monitor.ino setup() finished",0);

}



//-----------------------------------------------------------------
// loop()
//-----------------------------------------------------------------

#define DEBUG_ACTIVE	0

#define MAX_ACTIVE_BUF	32

static bool active_escape;
static bool in_active_msg;
static int active_ptr;
static uint8_t active_buf[MAX_ACTIVE_BUF];
static uint32_t active_counter;


#define DLE	0x10
#define STX 0x02
#define ETX 0x03


const uint8_t requestGetOperatingMode[] 	= {4,	0xa1, 0x01, 0x11, 0x4d};


const uint8_t replyGetOperatingMode[]   	= {6, 	0xa1, 0x03, 0x11, 0x02, 0x00, 0x49};
	// from ydnu92.pdf
	// from AI: const uint8_t replyGetOperatingMode[]   	= {8, 	0xa1, 0x01, 0x01, 0x5C};


const uint8_t *requests[] = {
	requestGetOperatingMode,
	0 };

const uint8_t *replies[] = {
	replyGetOperatingMode };



static void handle_request()
{
	uint8_t idx = 0;
	const uint8_t **request = requests;
	while (*request)
	{
		const uint8_t *ptr = *request;
		uint8_t len = *ptr++;
		if (len == active_ptr)		// if actisense buffer request has same length as const
		{
			bool match = true;
			for (int i=0; i<len; i++)
			{
				if (ptr[i] != active_buf[i])
				{
					i = len;
					match = false;
				}
			}

			if (match)
			{
				const uint8_t *reply = replies[idx];
				uint8_t reply_len = *reply++;
				Serial.write(DLE);
				Serial.write(STX);
				for (int i=0; i<reply_len; i++)
				{
					Serial.write(reply[i]);
				}
				Serial.write(DLE);
				Serial.write(ETX);
				Serial.write(0x0D);
				Serial.write(0x0A);
				display(0,"SEND_REPLY",0);
				#if WITH_ST7789
					mon.println("SEND_REPLY");
				#endif
			}
		}

		idx++;
		request++;
	}


}

static void clearbuf(const char *msg)
{
	if (msg)
	{
		display(0,"clearbuf(%s)",msg);
		#if WITH_ST7789
			mon.println("clearbuf(%s)",msg);
		#endif
	}

	active_escape = 0;
	in_active_msg = 0;
	active_ptr = 0;
}

static void addbyte(uint8_t byte)
{
	if (active_ptr<MAX_ACTIVE_BUF)
		active_buf[active_ptr++] = byte;
	else
		clearbuf("OVERFLOW");
}

static void showbuf()
{
	#define MAX_ACTIVE_STR	156
	static char active_str[MAX_ACTIVE_STR + 1];

	sprintf(active_str,"msg(%d) ",active_counter++);
	int len = strlen(active_str);
	for (int i=0; i<active_ptr && len<MAX_ACTIVE_STR-4; i++)
	{
		sprintf(&active_str[len]," %02x",active_buf[i]);
		len = strlen(active_str);
	}

	display(0,"actisense msg: %s",active_str);
	#if WITH_ST7789
		mon.println(active_str);
	#endif

	handle_request();
	clearbuf(0);
}




void loop()
{
	static bool started = false;
	if (!started)
	{
		started = true;
		display(dbg_mon,"loop() started",0);
	}

	#if WITH_WIFI
	#if WITH_TELNET
		telnet.loop();
	#endif
	#endif

	#if HOW_CAN_BUS == HOW_BUS_NMEA2000

		#if BROADCAST_NMEA200_INFO
			#define NUM_INFOS		4
			#define MSG_SEND_TIME	2000
			static int info_sent;
			static uint32_t last_send_time;
	
			uint32_t now = millis();
			if (info_sent < NUM_INFOS && now - last_send_time > MSG_SEND_TIME)
			{
				last_send_time = now;
				nmea2000.ParseMessages(); // Keep parsing messages

				// at this time I have not figured out the actisense reader, and how to
				// get the whole system to work so that when it asks for device configuration(s)
				// and stuff, we send it stuff.  However, this code explicitly sends some info
				// at boot, and I have seen the results get to the reader!

				switch (info_sent)
				{
					case 0:
						nmea2000.SendProductInformation(
							255,	// unsigned char Destination,
							0,		// only device
							false);	// bool UseTP);
						break;
					case 1:
						nmea2000.SendConfigurationInformation(255,0,false);
						break;
					case 2:
						nmea2000.SendTxPGNList(255,0,false);
						break;
					case 3:
						nmea2000.SendRxPGNList(255,0,false);	// empty right now for the sensor
						break;
				}

				info_sent++;
				nmea2000.ParseMessages();
				return;
			}
		#endif	// BROADCAST_NMEA200_INFO

		// fast loop
		nmea2000.ParseMessages();

		#if 0
			// Actisense Format:
			// Data: <10><02><93><length (1)><priority (1)><PGN (3)><destination (1)><source (1)><time (4)><len (1)><data (len)><CRC (1)><10><03>
			// Request: <10><02><94><length (1)><priority (1)><PGN (3)><destination (1)><len (1)><data (len)><CRC (1)><10><03>

			// I believe that 0x10 0x02 starts a message and 0x10 0x03 ends it
			// and in between there may be 0x10 0x10 for an escaped 0x10

			static int serial_count = 0;
			if (Serial.available())
			{
				uint8_t byte = Serial.read();
				serial_count++;

				#if DEBUG_ACTIVE
					mon.println("Serial(%d)=%02x",serial_count,byte);
				#endif

				if (byte == 0x10)	// DLE
				{
					if (active_escape)
					{
						active_escape = false;
						if (in_active_msg)
							addbyte(byte);
						else
							clearbuf("BOGUS 0x10");
					}
					else
						active_escape = true;
				}
				else if (byte == 0x02)	// STX
				{
					if (active_escape)
					{
						active_escape = 0;
						if (in_active_msg)
							clearbuf("BOGUS 0x02");
						else
						{
							#if DEBUG_ACTIVE
								mon.println("START MSG");
							#endif
							in_active_msg = 1;
						}
					}
					else if (in_active_msg)
						addbyte(byte);
				}
				else if (byte == 0x03)	// ETX
				{
					if (active_escape)
					{
						active_escape = 0;
						if (!in_active_msg)
							clearbuf("BOGUS 0x03");
						else
						{
							#if DEBUG_ACTIVE
								mon.println("END_MSG");
							#endif
							showbuf();
						}
					}
					else if (in_active_msg)
						addbyte(byte);
				}
				else if (in_active_msg)
					addbyte(byte);
				else
					clearbuf("BOGUS BYTE");
			}
		#elif TO_ACTISENSE
			actisenseReader.ParseMessages();
		#endif
		
	#elif HOW_CAN_BUS == HOW_BUS_CANBUS

		// assuming that we receive all messages if we don't set any filters
		if (CAN_MSGAVAIL  == canbus.checkReceive())
		{
			// huh? - we typically get an error on the 0th read
			// but if we we don't continue and call readMsgBuf,
			// the error persists.  Thereafter, we don't get the
			// error again.
			
			uint32_t id = canbus.getCanId();
			display(0,"canbus.getCanId(%d) = 0x%08x",id,id);

			uint8_t len;
			uint8_t buf[32];
			canbus.readMsgBuf(&len,buf);
				// RETURNS the length as specified in the message!!
			display_bytes(0,"    canbus.readMsgBuf",buf,len);

			if (id == SEND_NODE_ID)
			{
				float temperatureC;
				memcpy(&temperatureC,buf,4);
				static uint32_t counter;
				counter++;
				display(0,"    Temp(%d): %0.3fC",counter,temperatureC);
			}
		}

	#elif HOW_CAN_BUS == HOW_BUS_MPC2515

		struct can_frame canMsg;
		if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK)
		{
			display(0,"canMsg.id(%d)",canMsg.can_id);
			display_bytes(0,"canMsg",(uint8_t *)&canMsg,sizeof(canMsg));

			if (canMsg.can_id == MY_TEMPERATURE_CANID)  // Check if the message is from the sender
			{
				float temperatureC;
				uint8_t *ptr = (uint8_t *)&temperatureC;
				*ptr++ = canMsg.data[0];
				*ptr++ = canMsg.data[1];
				*ptr++ = canMsg.data[2];
				*ptr++ = canMsg.data[3];

				#if WITH_ACK
					// Send acknowledgment
					canMsg.can_id  = CAN_ACK_ID;  // Use ACK ID
					canMsg.can_dlc = 0;           // No data needed for ACK
					mcp2515.sendMessage(&canMsg);
					display(dbg_ack,"ACK sent",0);
				#endif
				
				static uint32_t counter;
				counter++;
				display(0,"Temp(%d): %0.3fC",counter,temperatureC);
			}
		}

	#endif	// HOW_CAN_BUS == HOW_BUS_MPC2515
}