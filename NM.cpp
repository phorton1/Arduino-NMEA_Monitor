//-----------------------------------------------------
// NM.cpp
//-----------------------------------------------------
// Implementation of main variables and methods externed in NM.h

#include "NM.h"
#include <SPI.h>
#include <N2kMessages.h>
#include <N2kMessagesEnumToStr.h>

#define MONITOR_NMEA_ADDRESS	99


#if USE_HSPI
	SPIClass *hspi;
		// MOSI=13
		// MISO=12
		// SCLK=14
		// default CS = 15, we use 5
#endif


tNMEA2000_mcp nmea2000(CAN_CS_PIN,MCP_8MHz);

// I now believe that the monitor
// (a) should not handle system message or advertise specifically that it does so
// (b) should not try to enumerate the messages it sends or receives from clients

const unsigned long AllMessages[] = {
	#if 0
		PGN_REQUEST,
		PGN_ADDRESS_CLAIM,
		PGN_PGN_LIST,
		PGN_HEARTBEAT,
		PGN_PRODUCT_INFO,
		PGN_DEVICE_CONFIG,
	#endif
	#if 0
		PGN_TEMPERATURE,
		PGN_HEADING,
		PGN_SPEED,
		PGN_DEPTH,
	#endif
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

	// set Device Mode and it's address(99)

	nmea2000.SetMode(tNMEA2000::N2km_ListenAndNode, MONITOR_NMEA_ADDRESS);
		// N2km_NodeOnly
		// N2km_ListenAndNode *
		// N2km_ListenAndSend **
		// N2km_ListenOnly
		// N2km_SendOnly

	#if PASS_THRU_TO_SERIAL

		nmea2000.SetForwardStream(&Serial);

		#if TO_ACTISENSE || FAKE_ACTISENSE
			nmea2000.SetForwardType(tNMEA2000::fwdt_Actisense);
		#else
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

		// I now believe that the monitor
		// (a) should not handle system message or advertise specifically that it does so
		// (b) should not try to enumerate the messages it sends or receives from clients

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




//----------------------------
// actisense
//----------------------------


#if TO_ACTISENSE
	tActisenseReader actisenseReader;

	void handleActisenseMsg(const tN2kMsg &msg)
	{
		if (dbgSerial)
		{
			dbgSerial->print("*");
			msg.Print(dbgSerial);
		}
		display(dbg_mon,"ACTISENSE(%d) source(%d) dest(%d) priority(%d)",
			msg.PGN,
			msg.Source,
			msg.Destination,
			msg.Priority);

		// forward broadcast and non-self messages to bus

		if (msg.Destination == 255 ||
			msg.Destination != MONITOR_NMEA_ADDRESS)
		{
			nmea2000.SendMsg(msg,-1);
			// return;
		}


		// handle broadcast and self messages

		if (msg.Destination == 255 ||
			msg.Destination == MONITOR_NMEA_ADDRESS)
		{
			display(dbg_mon,"    handling PGN(%d) to self(%d)",msg.PGN,MONITOR_NMEA_ADDRESS);

			#if 0

				// a perhaps better implementation based on calling CheckKnownMessages()
				// and switch statement from the guts of HandleReceivedSystemMessage()
				// however, these methods are all protected and I would need to either
				// derive a class or modify the library in order to call them

				bool is_sys_msg;
				bool is_fast_packet;
				if (nmea2000.CheckKnownMessage(msg.PGN,is_sys_msg,is_fast_packet))
				{
					display(dbg_mon,"    known PGN(%d) is_sys(%d) is_fast(%d)",msg.PGN,is_sys_msg,is_fast_packet);
					if (is_sys_msg)
					{
						switch (msg.PGN)
						{
							case 59392L: /*ISO Acknowledgement*/
								break;
							case 59904L: /*ISO Request*/
								nmea2000.HandleISORequest(msg);
								break;
							case 60928L: /*ISO Address Claim*/
								nmea2000.HandleISOAddressClaim(msg);
								break;
							case 65240L: /*Commanded Address*/
								nmea2000.HandleCommandedAddress(msg);
								break;
						#if !defined(N2K_NO_GROUP_FUNCTION_SUPPORT)
							case 126208L: /*NMEA Request/Command/Acknowledge group function*/
								nmea2000.HandleGroupFunction(msg);
								break;
						#endif
						}
					}
				}

			#else

				// one would think that the NMEA library automatically handled
				// all of these somehow.  I think for a while it did.
				//
				// I suspect it doesn't because I am trying to be BOTH an actisense
				// interface AND a node-device on the net, and that is not an expected
				// usage of the object model.
				//
				// In otherwords, no-one expected that an actisenseReader thing
				// would also want to be an NMEA200 device.  Note that in none
				// of the actisenseReader examples does Timo setup an NMEA2000 device.
				//
				// In any case, this is the only way I have found for my device
				// product information to "show up" anywhere on the laptop
				// when I am an actisenseReader, by explicitly handling these
				// messages.
				//
				// I still suspect it will be broken for "group" requests,
				// and I still haven't played with the DeviceList object with
				// the idea of caching other devices on this object and/or
				// displaying them somehow (on the OLED) for reference.

				if (msg.PGN == PGN_REQUEST)		// PGN_REQUEST == 59904L
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
							// Library handles these.
							//
							// I think I need to figure out "Grouip Functions" and
							// the NMEA200 library N2kDeviceList object.

							case PGN_PGN_LIST:
								nmea2000.SendTxPGNList(msg.Source,0);	// 0 == iDev);
								nmea2000.SendRxPGNList(msg.Source,0);	// 0 == iDev);

								// nmea2000.SendMsg(msg,-1);
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
			#endif	// 0
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

	if (dbgSerial)
		msg.Print(dbgSerial);

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

	// new heading,speed,depth sensor not shown on oled
	// and no error checking is done

	#if DISPLAY_DETAILS == 1

		uint8_t sid;
		double d1,d2,d3;

		if (msg.PGN == PGN_HEADING)
		{
			// heading is in radians
			tN2kHeadingReference ref;
			if (ParseN2kPGN127250(msg, sid, d1,d2,d3, ref))
				display(dbg_mon,"heading(%d) : %0.3f degrees",msg_counter,RadToDeg(d1));
			else
				my_error("Parsing PGN128267",0);
		}
		else if (msg.PGN == PGN_SPEED)
		{
			// speed is in meters/second
			tN2kSpeedWaterReferenceType SWRT;
			if (ParseN2kPGN128259(msg, sid, d1, d2, SWRT))
				display(dbg_mon,"speed(%d) : %0.3f kts",msg_counter,msToKnots(d1));
			else
				my_error("Parsing PGN128267",0);
		}
		else if (msg.PGN == PGN_DEPTH)
		{
			// depth is in meters
			if (ParseN2kPGN128267(msg,sid,d1,d2,d3))
				display(dbg_mon,"depth(%d) : %0.3f meters",msg_counter,d1);
			else
				my_error("Parsing PGN128267",0);
		}

		else
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



