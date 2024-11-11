//-------------------------------------------
// nmTelnet.cpp
//-------------------------------------------
// To do this correctly, we should check for STA_DISCONNECTED
// and use a reconnect timer, and restart the Telnet when it fails.

// However the immediate errors I am seeing are not because
// a lost STATION connection, but rather a failure in the console
// to write to the socket.
// I notice I get a lot of failures with current console timing
// of about 10 seconds to send 0's for heartbeat check.
// But that if I call client.stop() it recovers ...

#include "myNM.h"
#include <myDebug.h>
#include <prhPrivate.h>

#define dbg_telnet -0


void myNM::onTelnetConnect(String ip)
{
	warning(dbg_telnet,"Telnet connected from %s",ip.c_str());
	extraSerial = my_nm.m_telnet;
	my_nm.m_telnet->println("Welcome to the NMEA Monitor");
}

void myNM::onTelnetDisconnect(String ip)
{
	extraSerial = 0;
	warning(dbg_telnet,"Telnet disconnect",0);
}

void myNM::onTelnetReconnect(String ip)
{
	warning(dbg_telnet,"Telnet reconnected from %s",ip.c_str());
	extraSerial = my_nm.m_telnet;
	my_nm.m_telnet->println("Welcome back to the NMEA Monitor");
}

void myNM::onTelnetConnectionAttempt(String ip)
{
	warning(dbg_telnet,"Telnet attempt from %s",ip.c_str());
}


//	void myNM::onTelnetReceived(String command)
//	{
//		// getting a lot of blank lines
//		if (command != "")
//			warning(dbg_telnet,"Telnet: %s",command.c_str());
//	}


void myNM::initTelnet()
{
	#define USE_SSID	apartment_ssid

	WiFi.mode(WIFI_STA); //Optional
	WiFi.begin(USE_SSID, private_pass);
	display(dbg_telnet,"Connecting to %s",USE_SSID);
	delay(500);
	bool connected = (WiFi.status() == WL_CONNECTED);

	int wifi_wait = 0;
	while (!connected && wifi_wait++<10)
	{
		display(dbg_telnet,"    connect wait(%d)",wifi_wait);
		delay(1000);
		connected = (WiFi.status() == WL_CONNECTED);
	}

	if (connected)
	{
		String ip = WiFi.localIP().toString();
		display(dbg_telnet,"Connecting to %s at %s",USE_SSID,ip.c_str());
	}
	else
	{
		my_error("Could not connect to %s",USE_SSID);
	}

	if (connected)
	{
		display(dbg_telnet,"Starting Telnet",0);
		my_nm.m_telnet = new ESPTELNET_CLASS();
		my_nm.m_telnet->onConnect(onTelnetConnect);
		my_nm.m_telnet->onConnectionAttempt(onTelnetConnectionAttempt);
		my_nm.m_telnet->onReconnect(onTelnetReconnect);
		my_nm.m_telnet->onDisconnect(onTelnetDisconnect);
		// my_nm.m_telnet->onInputReceived(onTelnetReceived);
			// I am using stream operators for input

		#if USE_MY_ESP_TELNET
			my_nm.m_telnet->init();
				// called init() becaue ESPTelnetStream::begin()
				// is not virutal
		#else
			my_nm.m_telnet->begin();
		#endif
	}
}


void myNM::telnetLoop()
{
	if (!m_telnet)
		return;

	#if USE_MY_ESP_TELNET
		#if !WITH_MY_ESP_TELNET_TASK
			m_telnet->loop();
		#endif
	#else
		m_telnet->loop();
	#endif

	if (extraSerial && !m_telnet->isConnected())
	{
		extraSerial = 0;
		warning(dbg_telnet,"Telnet disconnected in telnetLoop()",0);
	}

	// my console regularly sends a  0x00 hearbeat
	// switching to use extraSerial and this method might go away

	if (extraSerial && extraSerial->available())
	{
		uint8_t byte = extraSerial->read();
		if (byte)
			handleSerialChar(byte,true);
	}
}



