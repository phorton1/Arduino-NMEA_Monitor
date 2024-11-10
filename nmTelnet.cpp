//-------------------------------------------
// nmTelnet.cpp
//-------------------------------------------
// implementation of telnet methods externed in NM.h

#include "NM.h"


#if WITH_WIFI
#if WITH_TELNET

    bool telnet_connected = false;

    ESPTelnetStream telnet;

    static void onTelnetConnect(String ip)
    {
        display(dbg_mon,"Telnet connected from %s",ip.c_str());
		#if WITH_OLED_MONITOR
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
		#if WITH_OLED_MONITOR
			mon.println("TELNET DISCONNECT");
		#endif
    }

    void onTelnetReconnect(String ip)
    {
        display(dbg_mon,"Telnet reconnected from %s",ip.c_str());
		#if WITH_OLED_MONITOR
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
		#if WITH_OLED_MONITOR
			mon.println("TELNET TRY(%s)",ip.c_str());
		#endif
    }

    void onTelnetReceived(String command)
    {
		// getting blank lines for some reason
		#if 0
			display(dbg_mon,"Telnet: %s",command.c_str());
			#if WITH_OLED_MONITOR
				mon.println(command.c_str());
			#endif
		#endif
	}


    void init_telnet()
    {
        display(dbg_mon,"Starting Telnet",0);
		#if WITH_OLED_MONITOR
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


