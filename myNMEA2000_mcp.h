//------------------------------------
// myNMEA2000_mcp.h
//------------------------------------

#pragma once

#include <NMEA2000_mcp.h>

class myNMEA2000_mcp : public tNMEA2000_mcp
{
public:

    myNMEA2000_mcp(
		unsigned char _N2k_CAN_CS_pin,
		unsigned char _N2k_CAN_clockset = MCP_16MHz,
        unsigned char _N2k_CAN_int_pin = 0xff,
		uint16_t _rx_frame_buf_size=MCP_CAN_RX_BUFFER_SIZE);

	void msgToSelf(const tN2kMsg &msg, uint8_t self_addr);

	bool CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf) override;


	//	const tInternalDevice *getInternalDevice(int idx=0) const
	//	{
	//		if (idx < DeviceCount)
	//			return &Devices[idx];
	//		return 0;
	//	}

		
};	// class myNMEA2000_mcp
