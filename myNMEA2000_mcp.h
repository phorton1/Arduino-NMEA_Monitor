//------------------------------------
// myNMEA2000_mcp.h
//------------------------------------

#pragma once

#include <NMEA2000_mcp.h>

class myNMEA2000_mcp : public tNMEA2000_mcp
{
public:

    myNMEA2000_mcp(
		unsigned char cs_pin,
		unsigned char clock_set = MCP_16MHz,
        unsigned char int_pin 	= 0xff,
		uint16_t rx_bufs		= MCP_CAN_RX_BUFFER_SIZE);

	void msgToSelf(const tN2kMsg &msg, uint8_t self_addr);

	bool CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf) override;
		
};	// class myNMEA2000_mcp

