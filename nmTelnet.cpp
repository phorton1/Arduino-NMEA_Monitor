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

#define dbg_wifi	0
#define dbg_telnet 	0

#define USE_SSID	apartment_ssid
#define USE_PASS	private_pass

#define CHECK_WIFI_TIME			60000
#define CONNECT_TIMEOUT			10000
#define DISCONNECT_TIMEOUT		10000
#define SHORT_TRY_ATTEMPTS		10
#define RECONNECT_DELAY_SHORT	10000		// 10 seconds for tries 1 thru 5
#define RECONNECT_DELAY_LONG	180000		// 3 minutes therafter


static volatile bool g_wifi_connected;
static uint32_t g_connect_try_count;
static uint32_t g_connect_count;
static uint32_t g_connect_fail_count;
static uint32_t g_connect_delay;
static uint32_t g_connect_fail_time;
static uint32_t g_last_wifi_check;


//---------------------------------
// connectWifi()
//---------------------------------

static void connectWifi()
{
	g_wifi_connected = false;
	g_connect_try_count++;
	g_connect_delay = 0;
	g_connect_fail_time = 0;

	display(dbg_wifi,"Connecting try(%d) to %s",
		g_connect_try_count,
		USE_SSID);
	proc_entry();

	// set mode

    wifi_mode_t mode = WiFi.getMode();
	if (mode != WIFI_AP_STA)
	{
		display(dbg_wifi,"setting WiFi.mode(WIFI_STA)",0);
		WiFi.mode(WIFI_STA);
		delay(400);
	}

	// disconnect

	if (WiFi.status() == WL_CONNECTED)
	{
		display(dbg_wifi,"Disconnecting station ...",0);

		WiFi.disconnect();   // station only
		delay(400);

		int wait_count = 0;
		uint32_t start = millis();
		while (WiFi.status() == WL_CONNECTED)
		{
			if (millis() - start >= DISCONNECT_TIMEOUT)
			{
				warning(dbg_wifi,"Could not discconnect station",0);
				break;
			}
			display(dbg_wifi,0,"    waiting disconnect(%d)",++wait_count);
			delay(1000);
		}
	}

	// connect

	WiFi.begin(USE_SSID, USE_PASS);
	delay(400);

	int wait_count = 0;
	uint32_t start = millis();
	while (WiFi.status() != WL_CONNECTED)
	{
		if (millis() - start >= CONNECT_TIMEOUT)
		{
			my_error("Could not connect to %s",USE_SSID);
			break;
		}
		display(dbg_wifi,"    waiting connect(%d)",++wait_count);
		delay(1000);
	}

	// handle result

	if (WiFi.status() == WL_CONNECTED)
	{
		g_wifi_connected = true;
		g_connect_count++;
		g_connect_try_count = 0;

		String ip = WiFi.localIP().toString();
		display(dbg_wifi,"Connected(%d) to %s at %s",
			g_connect_count,
			USE_SSID,
			ip.c_str());
	}
	else
	{
		g_connect_fail_count++;
		if (g_connect_try_count > SHORT_TRY_ATTEMPTS)
			g_connect_delay = RECONNECT_DELAY_LONG;
		else
			g_connect_delay = RECONNECT_DELAY_SHORT;
		g_connect_fail_time = millis();

		warning(0,"Could not Connect(%d) to %s (retry in %d seconds)",
			g_connect_try_count,
			USE_SSID,
			g_connect_delay / 1000);
	}

	display(dbg_wifi,"total tries(%d) success(%d) fails(%d)",
		g_connect_count + g_connect_fail_count,
		g_connect_count,
		g_connect_fail_count);

	proc_leave();
}




//---------------------------------------------------
// telnet handlers
//---------------------------------------------------

void myNM::onTelnetConnect(String ip)
{
	warning(dbg_telnet,"Telnet connected from %s",ip.c_str());
	extraSerial = my_nm.m_telnet;
	my_nm.m_telnet->println("Welcome to the NMEA Monitor");
	my_nm.m_telnet->print(getCommandUsage().c_str());

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

//	not using this with ESPTelnetStream
//	void myNM::onTelnetReceived(String command)
//	{
//		// getting a lot of blank lines
//		if (command != "")
//			warning(dbg_telnet,"Telnet: %s",command.c_str());
//	}


//---------------------------------------------------
// initTelnet()
//---------------------------------------------------

void myNM::initTelnet()
{
	display(dbg_telnet,"initTelnet()",0);
	m_telnet = new ESPTELNET_CLASS();
	m_telnet->onConnect(onTelnetConnect);
	m_telnet->onConnectionAttempt(onTelnetConnectionAttempt);
	m_telnet->onReconnect(onTelnetReconnect);
	m_telnet->onDisconnect(onTelnetDisconnect);

	// note that we start the task before calling m_telnet->begin()
	// and I have no idea what happens if myEspTelnetStream() task
	// calls it's loop() outside of begin() to stop()

	// Must run on ESP32_CORE_ARDUINO==1
	// Cannot run on ESP32_CORE_OTHER==0
	// see notes in bilgeAlarm.cpp lcdPrint()

	#define USE_CORE 	1
	display(0,"starting telnetTask pinned to core %d",USE_CORE);
	xTaskCreatePinnedToCore(
		telnetTask,		// task
		"telnetTask",	// name
		4096,			// stack
		m_telnet,		// param
		5,  			// priority
		NULL,   		// handle
		USE_CORE);		// core

	connectWifi();
	if (g_wifi_connected)
	{
		display(dbg_telnet,"starting telnet from initTelnet()",0);
		m_telnet->begin();
	}
}



//---------------------------------------------------
// telnetTask
//---------------------------------------------------

void myNM::telnetTask(void *telnet_ptr)
{
	delay(1000);
	display(0,"starting telnetTask loop on core(%d)",xPortGetCoreID());
	ESPTELNET_CLASS *telnet = (ESPTELNET_CLASS *) telnet_ptr;
	while (1)
	{
		vTaskDelay(1);

		uint32_t now = millis();
		if (WiFi.status() == WL_CONNECTED)
		{
			telnet->loop();
			#if USE_MY_ESP_TELNET
				telnet->flushOutput();
			#endif
		}

		if (g_connect_delay &&
			now - g_connect_fail_time >= g_connect_delay)
		{
			g_connect_delay = 0;
			warning(dbg_wifi,"calling connectWifi() from telnetTask()",0);
			connectWifi();
			if (g_wifi_connected)
			{
				display(dbg_telnet,"starting telnet from telnetTask()",0);
				telnet->begin();
			}
		}
		else if (
			g_wifi_connected &&
			!g_connect_delay &&
			WiFi.status() != WL_CONNECTED)
		{
			g_connect_delay = RECONNECT_DELAY_SHORT;
			g_connect_fail_time = now;
			warning(dbg_wifi,"Wifi connection lost in telnetTask() .. reconnecting in %d seconds",g_connect_delay/1000);
			display(dbg_telnet,"stopping telnet from telnetTask()",0);
			extraSerial = 0;
			telnet->stop();
		}

		if (now - g_last_wifi_check > CHECK_WIFI_TIME)
		{
			g_last_wifi_check = now;
			display(dbg_wifi,"WIFI check %s cur_try(%d) successes(%d) fails(%d)",
				g_wifi_connected ? "CONNECTED" : "NOT CONNECTED",
				g_connect_try_count,
				g_connect_count,
				g_connect_fail_count);
		}
	}
}


