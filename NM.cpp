//-----------------------------------------------------
// NM.cpp
//-----------------------------------------------------
// Implementation of main variables and methods externed in NM.h

#include "NM.h"
#include <SPI.h>
#include <N2kMessages.h>
#include <N2kMessagesEnumToStr.h>


#if USE_HSPI
	SPIClass *hspi;
		// MOSI=13
		// MISO=12
		// SCLK=14
		// default CS = 15, we use 5
#endif


tNMEA2000_mcp nmea2000(CAN_CS_PIN,MCP_8MHz);

const unsigned long AllMessages[] = {
	PGN_REQUEST,
	PGN_ADDRESS_CLAIM,
	PGN_PGN_LIST,
	PGN_HEARTBEAT,
	PGN_PRODUCT_INFO,
	PGN_DEVICE_CONFIG,
	PGN_TEMPERATURE,
	0};


//-----------------------------------------------------------------
// setup
//-----------------------------------------------------------------

void nmea2000_setup()
	// A bunch of things can be tried
	// set buffer sizes
{
	display(dbg_mon,"nmea2000_setup() started",0);
	proc_leave();

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
		hspi = new SPIClass(HSPI);
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

	proc_leave();
	display(dbg_mon,"nmea2000_setup() finished",0);
}



//-----------------------------------------------------------------
// initial message broadcasting
//-----------------------------------------------------------------

#if BROADCAST_NMEA200_INFO

	void broadcastNMEA2000Info()
	{
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
		}
	}
#endif	// BROADCAST_NMEA200_INFO




//-----------------------------------------------------------------
// loop()
//-----------------------------------------------------------------

#if FAKE_ACTISENSE

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


	static const uint8_t requestGetOperatingMode[] 	= {4,	0xa1, 0x01, 0x11, 0x4d};


	static const uint8_t replyGetOperatingMode[]   	= {6, 	0xa1, 0x03, 0x11, 0x02, 0x00, 0x49};
		// from ydnu92.pdf
		// from AI: const uint8_t replyGetOperatingMode[]   	= {8, 	0xa1, 0x01, 0x01, 0x5C};

	static const uint8_t *requests[] = {
		requestGetOperatingMode,
		0 };

	static const uint8_t *replies[] = {
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
					#if WITH_OLED
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
			#if WITH_OLED
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
		#if WITH_OLED
			mon.println(active_str);
		#endif

		handle_request();
		clearbuf(0);
	}


	void doFakeActisense()
	{
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
	}	// doFakeActisense

#endif	// FAKE_ACTISENSE





//----------------------------
// actisense
//----------------------------


#if TO_ACTISENSE
	tActisenseReader actisenseReader;

	void handleActisenseMsg(const tN2kMsg &msg)
	{
		// N2kMsg.Print(&Serial);
		display(dbg_mon,"ACTISENSE(%d) source(%d) dest(%d) priority(%d)",
			msg.PGN,
			msg.Source,
			msg.Destination,
			msg.Priority);

		#if 1
			nmea2000.SendMsg(msg,-1);
			return;
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
	}

#endif



//---------------------------------
// onNMEA2000Message
//---------------------------------
// grrrr - sometimes we get invalid messages
// i.e. PGN==0, or that parse but contain invalid results
// i.e. PGN=130316 temperatureC = -100000000000 ?!?!?!

void onNMEA2000Message(const tN2kMsg &msg)
{
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

			#if WITH_OLED && OLED_DETAILS
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

			#if WITH_OLED
				mon.println("PGN(%d)=%d",msg_counter,msg.PGN);
			#endif
		}
	}
}



