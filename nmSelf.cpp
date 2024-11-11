//------------------------------------
// nmSelf.cpp
//------------------------------------
// contains implementation of msgToSelf() and
// related override of CANGetFrame()

#include "myNM.h"
#include <myDebug.h>

#define MSG_TO_SELF_COLOR 	"\033[95m"
	// 95 = BRIGHT MAGENTA


//---------------------------------------
// Class to copy a message
//---------------------------------------
// with method to provide it's canID

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

		void assign(const tN2kMsg &msg, uint8_t dest_addr)
		{
			Source 		= msg.Source;
			Priority 	= msg.Priority;
			PGN 		= msg.PGN;
			DataLen 	= msg.DataLen;
			Destination = dest_addr;
			MsgTime 	= msg.MsgTime;
			ResetData();
			memcpy(Data,msg.Data,DataLen);
		}

		uint32_t canId()
		{
			return N2ktoCanID(Priority,PGN,Source,Destination);
		}

};	 // class copyMsg



//-------------------------------------------------
// implementation
//-------------------------------------------------

static copyMsg pending;
static volatile bool in_msg;
static uint8_t num_frames;
static uint8_t cur_frame;
static uint8_t cur_byte;
static uint8_t frame_counter;


void myNM::msgToSelf(const tN2kMsg &msg, uint8_t dest_addr)
{
	if (m_DEBUG_SELF)
		display_color(MSG_TO_SELF_COLOR,0,
			"msgToSelf(%d) addr(%d) len=%d",
			msg.PGN,
			dest_addr,
			msg.DataLen);

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
	pending.assign(msg,dest_addr);
	in_msg = 0;

	ParseMessages();
}



//-----------------------------------
// overriden CANGetFrame() method
//-----------------------------------

bool myNM::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)	// override;
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

			if (m_DEBUG_SELF)
				display_color(MSG_TO_SELF_COLOR,0,
					"    CANGetFrame(%d) %d/%d PGN(%d) cur_byte=%d",
					frame_counter,
					cur_frame,
					num_frames,
					pending.PGN,
					cur_byte);
		}
		else if (pending.DataLen<=8 && !IsFastPacket(pending))
		{
			// send single frame
			write = len = pending.DataLen;
			memcpy(buf,pending.Data,len);

			if (m_DEBUG_SELF)
				display_color(MSG_TO_SELF_COLOR,0,
					"    CANGetFrame single frame PGN(%d) len=%d",
					pending.PGN,
					len);
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

			if (m_DEBUG_SELF)
				display_color(MSG_TO_SELF_COLOR,0,
					"    CANGetFrame(%d) zero/%d PGN(%d) DataLen=%d",
					frame_counter,
					num_frames,
					pending.PGN,
					pending.DataLen);
		}

		cur_frame++;
		cur_byte += write;
		if (cur_frame >= num_frames)
		{
			if (m_DEBUG_SELF)
				display_color(MSG_TO_SELF_COLOR,0,"    CANGetFrame DONE",0);
			frame_counter++;
			pending.PGN = 0; 	// done
		}
		return true;
	}

	// default behavior: call base class

	return NMEA2000_CLASS::CANGetFrame(id,len,buf);

}

