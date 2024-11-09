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
	#if WITH_SERIAL2
		Serial2.begin(921600);
		delay(1000);
		dbgSerial = &Serial2;
	#endif

	#if FAKE_ACTISENSE || TO_ACTISENSE
		// turn off primary port in myDebug.h
		// so no debugging goes to the serial port
		// and initialize at 115200
		#if !WITH_SERIAL2
			dbgSerial = 0;
		#endif
		Serial.begin(115200);
	#else
		Serial.begin(921600);
		delay(2000);
		Serial.println("WTF");
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
				display(0,"Sending PGN_PRODUCT_INFO request",0);
				tN2kMsg msg;

				// This is a global method, NOT a member method
				// but is only inches from the messy public API.
				// I know ... they prefer automatic documentation generators ... I don't.
				// I prefer easy to read H files.
				
				SetN2kPGN59904(msg, 255, PGN_PRODUCT_INFO);
					// 2nd param is "dest". I thought 255 meant "broadcast"
					// dest(255) did not work until I fixed my code and added DEBUG_RXANY=1 to Sensor
					// dest(22) thereafter worked as expected

				nmea2000.SendMsg(msg, 0);
					// the 2nd parameter is the "idx" and should be 0, I think, as we are sending
					// 		from THIS nmea2000 (NMEA_Monitor)
					// I initially had a bad parameter (PGN_PRODUCT_INFO) but it compiled
					//		and did not work.
					// After changing it to correct "msg" and using DEBUG_RXANY=1 to the sensor
					//		it worked!
					// The message shows up both in my sent debugging output,
					// 		the sensors debugging output, and both see the reply
					//		as well.
			}
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

}	// loop()

