//-------------------------------------------
// NMEA_Monitor.cpp
//-------------------------------------------

#include "NM.h"

#if WITH_WIFI
	#include <prhPrivate.h>
#endif

#if WITH_OLED
	myOledMonitor mon(DRIVER_ST7789_320x170,1,WITH_OLED_TASK);
		// we always want the smallest font(1) in this program
#endif



//----------------------------------------------
// setup
//----------------------------------------------

void setup()
{
	#ifdef DEBUG_PORT
		DEBUG_PORT.begin(921600);
		dbgSerial = &DEBUG_PORT;
	#endif

	#ifdef ACTISENSE_PORT
		ACTISENSE_PORT.begin(115200);
	#endif

	#ifdef DEBUG_PORT
		delay(2000);
		dbgSerial->println("WTF");
	#endif


	display(dbg_mon,"NMEA_Monitor.ino setup() started",0);
	proc_entry();
	
	#if WITH_OLED
		mon.init(1);
			// rotation = 1
			// with_display = false;
		mon.println("NMEA_Monitor()");
	#endif

	#if WITH_WIFI
		pinMode(LED_WIFI,OUTPUT);
		digitalWrite(LED_WIFI,0);
		
		#define USE_SSID	apartment_ssid

		WiFi.mode(WIFI_STA); //Optional
		WiFi.begin(USE_SSID, private_pass);
		display(dbg_mon,"Connecting to %s",USE_SSID);
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
			display(dbg_mon,"Connecting to %s at %s",USE_SSID,ip.c_str());
			#if WITH_OLED
				mon.println("IP: %s",ip.c_str());
			#endif

			#if WITH_TELNET
				init_telnet();
			#endif
		}
		else
		{
			my_error("Could not connect to %s",USE_SSID);
			#if WITH_OLED
				mon.println("WIFI ERROR!!!");
			#endif
		}

	#endif	// WITH_WIFI

	//-------------------
	// NMEA2000 setup
	//--------------------

	nmea2000_setup();

	proc_leave();
	display(dbg_mon,"NMEA_Monitor.ino setup() finished",0);
}



#if WITH_SERIAL_COMMANDS

	static void handleSerialCommands()
	{
		if (!dbgSerial)
			return;
		
		if (dbgSerial->available())
		{
			uint8_t byte = dbgSerial->read();

			if (byte == 'q')
			{
				display(0,"Sending PGN_REQUEST(PGN_PRODUCT_INFO) message",0);
				tN2kMsg msg;
				SetN2kPGN59904(msg, 255, PGN_PRODUCT_INFO);
				nmea2000.SendMsg(msg, 0);
					// 0 == idx of THIS device by my setup()
			}
			else if (byte == 'i')
			{
				display(0,"calling SendProductInformation()",0);
				nmea2000.SendProductInformation();

				// experimental
				// try to add self to device_list
				// via PGN_PRODUCT_INFO ... maybe need address claim first  

				#if WITH_DEVICE_LIST
					if (device_list)
					{
						tN2kMsg msg;

						#define MAXBUF 32
						char model_id[MAXBUF+1];
						char sw_code[MAXBUF+1];
						char model_version[MAXBUF+1];
						char model_serial[MAXBUF+1];

						nmea2000.GetModelID(model_id, MAXBUF);
						nmea2000.GetSwCode(sw_code,  MAXBUF);
						nmea2000.GetModelVersion(model_version,  MAXBUF);
						nmea2000.GetModelSerialCode(model_serial,  MAXBUF);

						SetN2kPGN126996(msg,
							nmea2000.GetN2kVersion(),
							nmea2000.GetProductCode(),
							model_id,
							sw_code,
							model_version,
							model_serial,
							nmea2000.GetCertificationLevel(),
							nmea2000.GetLoadEquivalency() );

						msg.Source = MONITOR_NMEA_ADDRESS;

						device_list->HandleMsg(msg);
					}
				#endif
			}
			#if WITH_DEVICE_LIST
				else if (byte == 'l')
				{
					display(0,"calling listDevices(true)",0);
					listDevices(true);
				}
			#endif
			else
			{
				warning(0,"unhandled serial char(0x%02x)='%c'",byte,byte>=32?byte:' ');
			}
		}
	}

#endif




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

	#if BROADCAST_NMEA200_INFO
		broadcastNMEA2000Info();
	#endif

	nmea2000.ParseMessages();

	#if WITH_SERIAL_COMMANDS
		handleSerialCommands();
	#endif

	#if FAKE_ACTISENSE
		doFakeActisense();
	#elif TO_ACTISENSE
		actisenseReader.ParseMessages();
	#endif

	#if WITH_DEVICE_LIST
		listDevices(false);
	#endif

}	// loop()

