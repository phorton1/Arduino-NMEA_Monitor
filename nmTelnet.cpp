//-------------------------------------------
// nmTelnet.cpp
//-------------------------------------------

#include "myNM.h"
#include <myDebug.h>
#include <prhPrivate.h>

#define dbg_telnet -0


static void onTelnetConnect(String ip)
{
	display(dbg_telnet,"Telnet connected from %s",ip.c_str());
	my_nm.m_telnet_connected = true;
	extraSerial = my_nm.m_telnet;
	my_nm.m_telnet->println("Welcome to the NMEA Monitor");
}

static void onTelnetDisconnect(String ip)
{
	extraSerial = 0;
	my_nm.m_telnet_connected = false;
	display(dbg_telnet,"Telnet disconnect",0);
}

static void onTelnetReconnect(String ip)
{
	display(dbg_telnet,"Telnet reconnected from %s",ip.c_str());
	my_nm.m_telnet_connected = true;
	extraSerial = my_nm.m_telnet;
	my_nm.m_telnet->println("Welcome back to the NMEA Monitor");
}

static void onTelnetConnectionAttempt(String ip)
{
	display(dbg_telnet,"Telnet attempt from %s",ip.c_str());
}


static void onTelnetReceived(String command)
{
	// getting blank lines for some reason
	#if 0
		display(dbg_telnet,"Telnet: %s",command.c_str());
	#endif
}


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
		my_nm.m_telnet = new ESPTelnetStream();
		my_nm.m_telnet->onConnect(onTelnetConnect);
		my_nm.m_telnet->onConnectionAttempt(onTelnetConnectionAttempt);
		my_nm.m_telnet->onReconnect(onTelnetReconnect);
		my_nm.m_telnet->onDisconnect(onTelnetDisconnect);
		my_nm.m_telnet->onInputReceived(onTelnetReceived);
		my_nm.m_telnet->begin();
	}
}



