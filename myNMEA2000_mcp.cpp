//------------------------------------
// myNMEA2000_mcp.cpp
//------------------------------------

#include <myDebug.h>
#include "myNMEA2000_mcp.h"

#define DEBUG_MYMCP	0

#define COLOR 	"\033[95m"
	// 95 = BRIGHT MAGENTA


myNMEA2000_mcp::myNMEA2000_mcp(
	unsigned char _N2k_CAN_CS_pin,
	unsigned char _N2k_CAN_clockset /* = MCP_16MHz */,
	unsigned char _N2k_CAN_int_pin  /* = 0xff */,
	uint16_t _rx_frame_buf_size		/* = MCP_CAN_RX_BUFFER_SIZE */) :
	tNMEA2000_mcp(
		_N2k_CAN_CS_pin,
		_N2k_CAN_clockset,
		_N2k_CAN_int_pin,
		_rx_frame_buf_size)
{}



//-------------------------
// Conversion code
//-------------------------

extern unsigned long N2ktoCanID(unsigned char priority, unsigned long PGN, unsigned long Source, unsigned char Destination);
	// From NMEA2000.cpp:

class copyMsg: public tN2kMsg
{
	public:

		copyMsg() : tN2kMsg(0,0,0,0) {}

		void clear()
		{
			PGN = 0;
		}

		void assign(const tN2kMsg &msg, uint8_t dest)
		{
			Source 		= msg.Source;
			Priority 	= msg.Priority;
			PGN 		= msg.PGN;
			DataLen 	= msg.DataLen;
			Destination = dest;
			MsgTime 	= msg.MsgTime;
			ResetData();
			memcpy(Data,msg.Data,DataLen);
		}

		uint32_t canId()
		{
			return N2ktoCanID(Priority,PGN,Source,Destination);
		}

};	 // class copyMsg


static copyMsg pending;
static volatile bool in_msg;
static uint8_t num_frames;
static uint8_t cur_frame;
static uint8_t cur_byte;
static uint8_t frame_counter;


void myNMEA2000_mcp::msgToSelf(const tN2kMsg &msg, uint8_t self_addr)
{
	#if DEBUG_MYMCP
		dbgSerial->print(COLOR);
		dbgSerial->print("msgToSelf(");
		dbgSerial->print(msg.PGN);
		dbgSerial->print(") len=");
		dbgSerial->print(msg.DataLen);
		dbgSerial->print("\r\n");
	#endif

	if (pending.PGN)
	{
		my_error("myNMEA2000_mcp::msgToSelf() pending.PGN!!",0);
		return;
	}
	if (in_msg)
	{
		my_error("myNMEA2000_mcp::msgToSelf() in_msg!!",0);
		return;
	}

	in_msg = 1;
	cur_byte = 0;
	cur_frame = 0;
	num_frames = 0;
	pending.assign(msg,self_addr);
	in_msg = 0;

	ParseMessages();
}



//-----------------------------------
// overriden CANGetFrame() method
//-----------------------------------

bool myNMEA2000_mcp::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)	// override;
{
	if (pending.PGN)
	{
		id = pending.canId();
		int write = 0;
		
		if (num_frames)	// send next 7 bytes
		{
			write = (pending.DataLen - cur_byte) > 7 ? 7 :
				(pending.DataLen - cur_byte);
			len = write + 1;

			buf[0] = (frame_counter & 0x7) << 5;
			buf[0] |= (cur_frame & 0x1f);
			memcpy(&buf[1],&pending.Data[cur_byte],write);
			for (int i=write; i<7; i++)
			{
				buf[i+1] = 0xff;
			}

			#if DEBUG_MYMCP
				dbgSerial->print(COLOR);
				dbgSerial->print("    CANGetFrame(");
				dbgSerial->print(frame_counter);
				dbgSerial->print(") ");
				dbgSerial->print(cur_frame);
				dbgSerial->print("/");
				dbgSerial->print(num_frames);
				dbgSerial->print(" frames PGN(");
				dbgSerial->print(pending.PGN);
				dbgSerial->print(") cur_byte=");
				dbgSerial->print(cur_byte);
				dbgSerial->print("\r\n");
			#endif

		}
		else if (pending.DataLen<=8 && !IsFastPacket(pending))
		{
			// send single frame
			write = len = pending.DataLen;
			memcpy(buf,pending.Data,len);

			#if DEBUG_MYMCP
				dbgSerial->print(COLOR);
				dbgSerial->print("    CANGetFrame single frame PGN(");
				dbgSerial->print(pending.PGN);
				dbgSerial->print(") len=");
				dbgSerial->print(len);
				dbgSerial->print("\r\n");
			#endif
		}
		else // send 0th frame of 6 bytes
		{
			// 6=1, 7..13=2, 14..20=3

			num_frames = pending.DataLen>6 ? 1 + (pending.DataLen/7) : 1;
			write = pending.DataLen > 6 ? 6 : pending.DataLen;
			len = write + 2;

			buf[0] = (frame_counter & 0x7) << 5;
			// buf[0] |= (cur_frame & 0xf);
			buf[1] = pending.DataLen;
			memcpy(&buf[2],pending.Data,write);

			#if DEBUG_MYMCP
				dbgSerial->print(COLOR);
				dbgSerial->print("    CANGetFrame(");
				dbgSerial->print(frame_counter);
				dbgSerial->print(") zero/");
				dbgSerial->print(num_frames);
				dbgSerial->print(" frame PGN(");
				dbgSerial->print(pending.PGN);
				dbgSerial->print(") DataLen=");
				dbgSerial->print(pending.DataLen);
				dbgSerial->print("\r\n");
			#endif
		}

		cur_frame++;
		cur_byte += write;
		if (cur_frame >= num_frames)
		{
			#if DEBUG_MYMCP
				dbgSerial->print(COLOR);
				dbgSerial->print("    CANGetFrame DONE\r\n");
			#endif
			frame_counter++;
			pending.PGN = 0; 	// done
		}
		return true;
	}

	// default behavior: call base class
	
	return tNMEA2000_mcp::CANGetFrame(id,len,buf);

}

