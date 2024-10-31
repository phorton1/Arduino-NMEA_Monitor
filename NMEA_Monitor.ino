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

#include <myDebug.h>

#define HOW_BUS_MPC2515		0		// use github/autowp/arduino-mcp2515 library
#define HOW_BUS_CANBUS		1		// use github/_ttlappalainen/CAN_BUS_Shield mpc2515 library
#define HOW_BUS_NMEA2000	2		// use github/_ttlappalainen/CAN_BUS_Shield + ttlappalainen/NMEA2000 libraries

#define HOW_CAN_BUS			HOW_BUS_NMEA2000


#define dbg_mon			0

#define CAN_CS_PIN		5


#if HOW_CAN_BUS == HOW_BUS_MPC2515

	#include <mcp2515.h>
	#include <SPI.h>

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

	#define PASS_THRU_TO_SERIAL		0
	#define TO_ACTISENSE			0

	#if TO_ACTISENSE
		#define dbg_mon			1
	#endif

	#define PGN_TEMPERATURE    			130316L

	// forked and added API to pass the CAN_500KBPS baudrate

	tNMEA2000_mcp nmea2000(CAN_CS_PIN,MCP_8MHz,CAN_500KBPS);

	const unsigned long ReceiveMessages[] = {PGN_TEMPERATURE,0};
	
#endif




#if HOW_CAN_BUS == HOW_BUS_NMEA2000

	void onNMEA2000Message(const tN2kMsg &N2kMsg)
	{
		#define SHOW_DETAILS	0

		#if SHOW_DETAILS
			display(dbg_mon,"onNMEA2000 message(%d) priority(%d) source(%d) dest(%d) len(%d)",
				N2kMsg.PGN,
				N2kMsg.Priority,
				N2kMsg.Source,
				N2kMsg.Destination,
				N2kMsg.DataLen);
			// also timestamp (ms since start [max 49days]) of the NMEA2000 message
			// unsigned long MsgTime;
		#else
			display(dbg_mon,"onNMEA2000 message(%d)",N2kMsg.PGN);
		#endif


		if (N2kMsg.PGN == PGN_TEMPERATURE)
		{
			uint8_t sid;
			uint8_t instance;
			tN2kTempSource source;
			double temperature1;
			double temperature2;

			bool rslt = ParseN2kPGN130316(
				N2kMsg,
				sid,
				instance,
				source,
                temperature1,
				temperature2);

			if (rslt)
			{
				float temperature = KelvinToC(temperature1);
				// temperature2 == N2kDoubleNA

				static uint32_t msg_counter = 0;
				msg_counter++;

				#if SHOW_DETAILS
					display(dbg_mon,"%s(%d) inst(%d) : %0.3fC",
						N2kEnumTypeToStr(source),
						msg_counter,
						instance,
						temperature);
				#else
					display(dbg_mon,"temp(%d) : %0.3fC",msg_counter,temperature);
				#endif
			}
			else
				my_error("Error parsing PGN(%d)",PGN_TEMPERATURE);
		}
	}
#endif




void setup()
{
	#if HOW_CAN_BUS == HOW_BUS_NMEA2000
		#if TO_ACTISENSE
			Serial.begin(115200);
		#else
			Serial.begin(921600);
		#endif
	#else
		Serial.begin(921600);
	#endif

	delay(2000);
	display(dbg_mon,"NMEA_Monitor.ino setup(%d) started",HOW_CAN_BUS);
	Serial.println("WTF");
	

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

		// set some device information from ArduinoGateway example

		#if 0

			nmea2000.SetN2kCANSendFrameBufSize(150);
			nmea2000.SetN2kCANReceiveFrameBufSize(150);
			nmea2000.SetProductInformation(
				"00000001", // Manufacturer's Model serial code
				100, // Manufacturer's product code
				"Arduino Gateway",  // Manufacturer's Model ID
				"1.0.0.2 (2018-01-10)",  // Manufacturer's Software version code
				"1.0.0.0 (2017-01-06)" // Manufacturer's Model version
				);
			nmea2000.SetDeviceInformation(
				1, // Unique number. Use e.g. Serial number.
				130, // Device function=Analog to NMEA 2000 Gateway. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
				25, // Device class=Inter/Intranetwork Device. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
				2046 // Just choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
				);

		#endif


		// from DataDisplay.ino example

		nmea2000.SetMode(tNMEA2000::N2km_ListenOnly);
			// N2km_ListenAndNode

		#if PASS_THRU_TO_SERIAL

			nmea2000.SetForwardStream(&Serial);

			// for actiSense program, we comment out the following line
			// for viewing in serial monitor, we leave it in

			#if !TO_ACTISENSE
				nmea2000.SetForwardType(tNMEA2000::fwdt_Text); 	// Show bus data in clear text
			#endif

			// nmea2000.SetForwardOwnMessages(false);
				// read/write streams are the same, so dont forward own messages?

		#else
			nmea2000.EnableForward(false);
		#endif


		// nmea2000.SetDebugMode(tNMEA2000::dm_ClearText);

		#if 0
			// I could not get this to eliminate need for DEBUG_RXANY
			// compiile flag in the Monitor
			nmea2000.ExtendReceiveMessages(ReceiveMessages);
		#endif

		nmea2000.SetMsgHandler(onNMEA2000Message);

		bool ok = nmea2000.Open();
		if (!ok)
			my_error("nmea2000.Open() failed",0);

	#endif	// HOW_CAN_BUS == HOW_BUS_NMEA2000

	display(dbg_mon,"NMEA_Monitor.ino setup() finished",0);

}


void loop()
{
	#if HOW_CAN_BUS == HOW_BUS_NMEA2000

		static bool started = false;
		if (!started)
		{
			started = true;
			display(dbg_mon,"loop() started",0);
		}
		nmea2000.ParseMessages();

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