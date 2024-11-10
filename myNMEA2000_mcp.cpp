//------------------------------------
// myNMEA2000_mcp.cpp
//------------------------------------

#include <myDebug.h>
#include "myNMEA2000_mcp.h"

#define DEBUG_MYMCP	1


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
// From NMEA2000.cpp:

extern unsigned long N2ktoCanID(unsigned char priority, unsigned long PGN, unsigned long Source, unsigned char Destination);
//	{
//	  unsigned char CanIdPF = (unsigned char) (PGN >> 8);
//
//	  if (CanIdPF < 240) {  // PDU1 format
//	     if ( (PGN & 0xff) != 0 ) return 0;  // for PDU1 format PGN lowest byte has to be 0 for the destination.
//	     return ( ((unsigned long)(priority & 0x7))<<26 | PGN<<8 | ((unsigned long)Destination)<<8 | (unsigned long)Source);
//	  } else { // PDU2 format
//	     return ( ((unsigned long)(priority & 0x7))<<26 | PGN<<8 | (unsigned long)Source);
//	  }
//	}

typedef struct  {
    uint32_t id; 		// CAN identifier
    uint8_t  dlc; 		// Data Length Code
    uint8_t  data[8]; 	// Data bytes
} CANFrame;

static CANFrame static_frame;
static volatile bool frame_pending;


static void convertToCan(CANFrame *frame, const tN2kMsg &msg, uint8_t self_addr)
{
	frame->id = N2ktoCanID(
		msg.Priority,
		msg.PGN,
		msg.Source,
		99); 	// msg.Destination );
	frame->dlc = msg.DataLen;
	memcpy(frame->data, msg.Data, msg.DataLen);
}


void myNMEA2000_mcp::msgToSelf(const tN2kMsg &msg, uint8_t self_addr)
{
	#if DEBUG_MYMCP
		if (dbgSerial)
		{
			// 95 = BRIGHT MAGENTA
			dbgSerial->print("\033[95mSELF: ");
			msg.Print(dbgSerial);
		}
	#endif

	if (frame_pending)
	{
		my_error("myNMEA2000_mcp::msgToSelf() Self Frame Pending!!",0);
		return;
	}

	convertToCan(&static_frame,msg,self_addr);
	frame_pending = true;
	ParseMessages();
}



//-----------------------------------
// overriden CANGetFrame() method
//-----------------------------------

bool myNMEA2000_mcp::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)	// override;
{
	if (frame_pending)
	{
		#if DEBUG_MYMCP
			// 95 = BRIGHT MAGENTA
			if (dbgSerial)
				dbgSerial->print("\033[95m    Returning static frame\r\n");
		#endif
		
		id = static_frame.id;
		len = static_frame.dlc;
		memcpy(buf,static_frame.data,len);
		frame_pending = false;
		return true;
	}

	return tNMEA2000_mcp::CANGetFrame(id,len,buf);

}

