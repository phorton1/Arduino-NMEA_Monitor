//-----------------------------------------------------
//fakeActisense.cpp
//-----------------------------------------------------
// Implementation of doFakeActisense() serial listener
// and non-standard broadcastNMEA2000Info()

#include "NM.h"
#include <SPI.h>
#include <N2kMessages.h>
// #include <N2kMessagesEnumToStr.h>


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


