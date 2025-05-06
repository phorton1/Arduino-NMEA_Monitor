//-----------------------------------------------------
// myNM.cpp
//-----------------------------------------------------

#include "myNM.h"
#include <myDebug.h>
#include <SPI.h>
#include <N2kMessages.h>
#include <N2kMessagesEnumToStr.h>

#define dbg_mon			0		// general debugging
#define dbg_command 	0

bool myNM::m_DEBUG_BUS = DEFAULT_DEBUG_BUS;
bool myNM::m_DEBUG_SENSORS = DEFAULT_DEBUG_SENSORS;


#define BUS_COLOR "\033[37m"
	// 37 = WHITE


#if USE_HSPI
	SPIClass *hspi;
		// MOSI=13
		// MISO=12
		// SCLK=14
		// default CS = 15, we use 5
#endif


#if 0
	// I now believe that the monitor
	// (a) should not directly handle system message or advertise specifically that it does so
	// (b) should not advertise the messages it receives from sensors

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
#endif




String myNM::msgToString(const tN2kMsg &msg,  const char *prefix/*=0*/, bool with_data /* =true */)
{
	String rslt;
	if (prefix) rslt += prefix;
	rslt += String(N2kMillis());
	rslt += " : ";
	rslt += "Pri: ";
	rslt += String(msg.Priority);
	rslt += " PGN: ";
	rslt += String(msg.PGN);
	rslt += " Source: ";
	rslt += String(msg.Source);
	rslt += " Dest: ";
	rslt += String(msg.Destination);
	rslt += " Len: ";
	rslt += String(msg.DataLen);
	if (with_data)
	{
		rslt += " Data:";
		for (int i=0; i<msg.DataLen; i++)
		{
			if (i) rslt += ",";
			rslt += String(msg.Data[i],HEX);
		}
  }
  return rslt;
}


void myNM::setup(
	bool with_device_list 		 /* = DEFAULT_WITH_DEVICE_LIST */,
	bool add_self_to_device_list /* = DEFAULT_ADD_SELF_TO_DEVICE_LIST */,
	bool broadcast_nmea_info 	 /* = DEFAULT_BROADCAST_NMEA200_INFO */ )
{
	m_broadcase_nmea_info = broadcast_nmea_info;

	proc_entry();

	//--------------------------------------
	// nmea setup
	//--------------------------------------

	#if 0	// the program works well enough with the defaults of 50
		SetN2kCANSendFrameBufSize(150);
		SetN2kCANReceiveFrameBufSize(150);
	#endif

	// set device information
	// I am not currently calling SetDeviceInstance() but it's working "ok"

	SetProductInformation(
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
	SetConfigurationInformation(
		"prhSystems",           // ManufacturerInformation
		"MonitorInstall1",      // InstallationDescription1
		"MonitorInstall2"       // InstallationDescription2
		);
	SetDeviceInformation(
		1230100, // uint32_t Unique number. Use e.g. Serial number.
		130,     // uint8_t  Device function=Analog to NMEA 2000 Gateway. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
		25,      // uint8_t  Device class=Inter/Intranetwork Device. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
		2046     // uint16_t choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
		);

	// note that I typically use iDev=0 for creating THIS device
	// in each implementation; it is internal to the NME2000 library
	// and not broadcast.

	// set Device Mode and it's address(99)

	SetMode(tNMEA2000::N2km_ListenAndNode, MONITOR_NMEA_ADDRESS);
		// N2km_NodeOnly
		// N2km_ListenAndNode *
		// N2km_ListenAndSend **
		// N2km_ListenOnly
		// N2km_SendOnly

	// configure forwarding or disable it
	// Note about SetForwardOwnMessages.  Not clear about this example comment:
	// "read/write streams are the same, so dont forward own messages", so
	// I keep playing with true/false in both forwards

	#if 0
		SetForwardType(tNMEA2000::fwdt_Text); 	// Show bus data in clear text
		SetForwardStream(&Serial);
		SetForwardOwnMessages(true);
	#else
		EnableForward(false);
	#endif


	#if 0
		// I could not get this to eliminate need for DEBUG_RXANY=1
		// compiile flag in the Monitor. See notes elsewhere.
		ExtendReceiveMessages(AllMessages);
		ExtendTransmitMessages(AllMessages);
	#endif

	SetMsgHandler(onBusMessage);

	#if USE_HSPI
		hspi = new SPIClass(HSPI);
		SetSPI(hspi);
	#endif

	if (with_device_list)
	{
		display(dbg_mon,"creating deviceList",0);
		m_device_list = new tN2kDeviceList(this);
		if (add_self_to_device_list)
			addSelfToDeviceList();
	}

	//----------------------------
	// OPEN THE CANBUS
	//----------------------------

	bool ok = Open();
	if (!ok)
		my_error("NMEA2000::Open() failed",0);


	proc_leave();
	display(dbg_mon,"myNM::setup() finished",0);

	display(dbg_mon,"myNM::setup() started",0);
	Serial.print(getCommandUsage().c_str());

}	// myNM::setup()



//---------------------------------
// utilities
//---------------------------------

void myNM::broadcastNMEA2000Info()
{
	#define NUM_INFOS		4
	#define MSG_SEND_TIME	2000
	static int info_sent;
	static uint32_t last_send_time;

	uint32_t now = millis();
	if (info_sent < NUM_INFOS && now - last_send_time > MSG_SEND_TIME)
	{
		last_send_time = now;
		ParseMessages(); // Keep parsing messages

		// at this time I have not figured out the actisense reader, and how to
		// get the whole system to work so that when it asks for device configuration(s)
		// and stuff, we send it stuff.  However, this code explicitly sends some info
		// at boot, and I have seen the results get to the reader!

		switch (info_sent)
		{
			case 0:
				SendProductInformation(
					255,	// unsigned char Destination,
					0,		// only device
					false);	// bool UseTP);
				break;
			case 1:
				SendConfigurationInformation(255,0,false);
				break;
			case 2:
				SendTxPGNList(255,0,false);
				break;
			case 3:
				SendRxPGNList(255,0,false);	// empty right now for the sensor
				break;
		}

		info_sent++;
		ParseMessages();
	}
}




String myNM::getCommandUsage()
{
	String rslt = "Usage:\r\n";
	rslt += "    ? == Show this Help Message\r\n";
	rslt += "    l == List Devices\r\n";
	rslt += "    b == toggle DEBUG_BUS cur=";
	rslt += String(m_DEBUG_BUS);
	rslt += "\r\n";
	rslt += "    c == toggle DEBUG_SENSORS cur=";
	rslt += String(m_DEBUG_SENSORS);
	rslt += "\r\n";
	rslt += "    i == Send out Product Information\r\n";
	rslt += "    q == Send out PGN_REQUEST for Product Information\r\n";
	rslt += "    m == Show Memory Usage\r\n";
	rslt += "    x! = Reboot\r\n";
	return rslt;
}



void myNM::handleSerialChar(uint8_t byte, bool telnet)
{
	static uint8_t pending_command;

	if (byte == '!')
	{
		if (pending_command == 'x')
		{
			warning(0,"REBOOTING !!!",0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			ESP.restart();
			while (1) {}
		}
		else
		{
			warning(dbg_command,"unhandled exclamation pending='%c'",
				pending_command > 32 ? pending_command : ' ');
		}
	}
	else if (pending_command)
	{
		warning(dbg_command,"Canceling pending '%c' command",
			pending_command > 32 ? pending_command : ' ');
		pending_command = 0;
	}

	if (byte == '?')
	{
		if (telnet)
			extraSerial->print(getCommandUsage().c_str());
		else
			dbgSerial->print(getCommandUsage().c_str());
	}
	else if (byte == 'q')
	{
		display(dbg_command,"Sending PGN_REQUEST(PGN_PRODUCT_INFO) message",0);
		tN2kMsg msg;
		SetN2kPGN59904(msg, 255, PGN_PRODUCT_INFO);
		SendMsg(msg, 0);
			// 0 == idx of THIS device by my setup()
	}
	else if (byte == 'i')
	{
		display(dbg_command,"calling SendProductInformation()",0);
		SendProductInformation();
	}
	else if (byte == 'l')
	{
		if (m_device_list)
		{
			display(dbg_command,"calling listDevices()",0);
			listDevices();
		}
		else
			my_error("NO DEVICE LIST!!",0);
	}

	else if (byte == 'b')
	{
		m_DEBUG_BUS = !m_DEBUG_BUS;
		display(dbg_command,"DEBUG_BUS=%d",m_DEBUG_BUS);
	}
	else if (byte == 'c')
	{
		m_DEBUG_SENSORS = !m_DEBUG_SENSORS;
		display(dbg_command,"DEBUG_SENSORS=%d",m_DEBUG_SENSORS);
	}
	else if (byte == 'm')
	{
		uint32_t mem_free = xPortGetFreeHeapSize() / 1024;
		uint32_t mem_min = xPortGetMinimumEverFreeHeapSize() / 1024;
		display(0,"MEMORY Free: %d KB   Min: %d KB",mem_free,mem_min);
	}
	else if (byte == 'x')
	{
		warning(0,"PRESS '!' to Confirm Reboot!!!",0);
		pending_command = 'x';
	}

	else
	{
		warning(dbg_command,"unhandled serial char(0x%02x)='%c'",byte,byte>=32?byte:' ');
	}
}



//---------------------------------
// onBusMessage
//---------------------------------
// Handles sensor messages and can show others from the bus
// i.e. PGN=130316 temperatureC = -100000000000 ?!?!?!

// static
void myNM::onBusMessage(const tN2kMsg &msg)
{
	bool msg_handled = false;
	static uint32_t msg_counter = 0;
	msg_counter++;

	if (msg.Destination == 255 ||
		msg.Destination == MONITOR_NMEA_ADDRESS)
	{
		uint8_t sid;
		double d1,d2,d3;

		if (msg.PGN == PGN_HEADING)
		{
			// heading is in radians
			tN2kHeadingReference ref;
			if (ParseN2kPGN127250(msg, sid, d1,d2,d3, ref))
			{
				msg_handled = true;
				if (m_DEBUG_SENSORS)
					display(0,"heading(%d) : %0.3f degrees",msg_counter,RadToDeg(d1));
			}
			else
				my_error("Parsing PGN_HEADING(128267)",0);
		}
		else if (msg.PGN == PGN_SPEED)
		{
			// speed is in meters/second
			tN2kSpeedWaterReferenceType SWRT;
			if (ParseN2kPGN128259(msg, sid, d1, d2, SWRT))
			{
				msg_handled = true;
				if (m_DEBUG_SENSORS)
					display(0,"speed(%d) : %0.3f kts",msg_counter,msToKnots(d1));
			}
			else
				my_error("Parsing PGN_SPEED(128267)",0);
		}
		else if (msg.PGN == PGN_DEPTH)
		{
			// depth is in meters
			if (ParseN2kPGN128267(msg,sid,d1,d2,d3))
			{
				msg_handled = true;
				if (m_DEBUG_SENSORS)
					display(0,"depth(%d) : %0.3f meters",msg_counter,d1);
			}
			else
				my_error("Parsing PGN_DEPTH(128267)",0);
		}
		else if (msg.PGN == PGN_TEMPERATURE)
		{
			// temperature is in kelvin
			uint8_t instance;
			tN2kTempSource source;

			if (ParseN2kPGN130316(msg,sid,instance,source,d1,d2))
			{
				msg_handled = true;
				if (m_DEBUG_SENSORS)
					display(0,"temp(%d) : %0.3fC",msg_counter,KelvinToC(d1));
			}
			else
				my_error("Parsing PGN_TEMPERATURE(130316)",0);
		}
	}	// 255 or MONITOR_NMEA_ADDRESS


	// show unhandled messages as BUS: in white

	if (!msg_handled && m_DEBUG_BUS)
	{
		display_string(BUS_COLOR,0,msgToString(msg,"BUS: ").c_str());
	}

}	// onBusMessage()




//-----------------------------------
// loop()
//-----------------------------------


void myNM::loop()
{
	static bool started = false;
	if (!started)
	{
		started = true;
		display(dbg_mon,"myNM::loop() started",0);
	}

	if (m_broadcase_nmea_info)
		broadcastNMEA2000Info();

	ParseMessages();

	// These probably get moved to a task too ..
	// so the only thing in the loop are NMEA message things
	// or, THEY get moved to a task ... and then don't need
	// a separate task for telnet/wifi?
	
	if (Serial.available())
	{
		uint8_t byte = Serial.read();
		handleSerialChar(byte,false);
	}

	#if 0	// bad idea in time critical loop
		delay(2);
	#endif

}


