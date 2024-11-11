//--------------------------------------
// nmDeviceList.cpp
//--------------------------------------

#include "myNM.h"
#include <myDebug.h>

#define dbg_dl  0


#define GCOLOR "\033[92m"

static void displayIntList(const char *prefix, const unsigned long *list)
{
    uint8_t i;
    if (list)
    {
        Serial.print(GCOLOR);
        Serial.print(prefix);
        for (i=0; list[i]!=0; i++)
        {
            if (i>0) Serial.print(", ");
            Serial.print(list[i]);
        }
        Serial.println();
        if (my_nm.m_telnet_connected)
        {
            my_nm.m_telnet->print(GCOLOR);
            my_nm.m_telnet->print(prefix);
            for (i=0; list[i]!=0; i++)
            {
                if (i>0) my_nm.m_telnet->print(", ");
                my_nm.m_telnet->print(list[i]);
            }
            my_nm.m_telnet->println();
        }
    }
}


static void displayDevice(const tNMEA2000::tDevice *pDevice)
{
    if (pDevice == 0) return;

    display(0,"----------------------------------------------------------------------",0);
    if (pDevice->GetModelID())
    display(0,"%s",pDevice->GetModelID());
    display(0,"    Source: %d",                     pDevice->GetSource());
    display(0,"    Manufacturer code:  %d",         pDevice->GetManufacturerCode());
    display(0,"    Unique number:      %d",         pDevice->GetUniqueNumber());
    if (pDevice->GetSwCode())
        display(0,"    Software version:   %s",     pDevice->GetSwCode());
    if (pDevice->GetModelVersion())
        display(0,"    Model version:      %s",     pDevice->GetModelVersion());
    if (pDevice->GetManufacturerInformation())
        display(0,"    Manufacturer Info:  %s",     pDevice->GetManufacturerInformation());
    if (pDevice->GetInstallationDescription1())
        display(0,"    Installation1:      %s",     pDevice->GetInstallationDescription1());
    if (pDevice->GetInstallationDescription2())
        display(0,"    Installation2:      %s",     pDevice->GetInstallationDescription2());
    displayIntList("    Transmit PGNs :",           pDevice->GetTransmitPGNs());
    displayIntList("    Receive PGNs  :",           pDevice->GetReceivePGNs());
    display(0,"",0);
}


void myNM::listDevices()
{
    display(0,"******************************** DEVICE LIST **********************************",0);
    for (uint8_t i=0; i< N2kMaxBusDevices; i++)
    {
        displayDevice(m_device_list->FindDeviceBySource(i));
    }
}


//----------------------------------------
// addSelfToDeviceList()
//----------------------------------------
// Add self to the device list by creating the
// relevant messages and giving them to the
// device list to parse.
//
// It'd sure be nice if we could do this better.
// I don't want to use my NM.cpp constants, but
// rather GET the values from the nmea2000 object
// even though it's not Open yet.
//
// I *may* be able to take advantage of myNMEA2000_mcp
// derived class, and I *might* need to derive my own
// device list, but I haven't after trying to deal with
// the weird protected: schemes, I give up.  Protected
// members with public accessors.  Way obstruficated
// headers.  If it's an implementation variable, effing
// hide it in the cpp file.
//
// Finally, foe the 3rd case, it's a waste of my time trying
// to figure out how to pass my own PGN lists
// this way.  I'd have to extern NMEA2000.cpp implementation
// variables, and then use my own constant lists here
// because the author made everything protected, wheras
// it could have simply been const for safety.
//
// I almost would write my own device list, and then
// I start think of re-writing the whole library.
// It works.  I'll leave it as is.  You don't see
// any PGN's on my SELF object, and it sends everyone
// else on the bus, except me, 4 * 2 times, a request
// for my PGN lists.  Sheesh

void myNM::addSelfToDeviceList()
{
    for (int i=0; i<3; i++)
    {
        tN2kMsg msg;
        display(dbg_dl,"addSelfToDeviceList(%d)",i);

        switch (i)
        {
            case 0 :
            {
                const tNMEA2000::tDeviceInformation info = GetDeviceInformation();
                SetN2kPGN60928(msg,
                    info.GetUniqueNumber(),
                    info.GetManufacturerCode(),
                    info.GetDeviceFunction(),
                    info.GetDeviceClass(),  // << 1?
                    info.GetDeviceInstance(),
                    info.GetSystemInstance(),
                    info.GetIndustryGroup() );
                msg.Source = MONITOR_NMEA_ADDRESS;
                m_device_list->HandleMsg(msg);
                break;
            }
            case 1 :
            {
                bool info_progmem;
                const tNMEA2000::tProductInformation *info = GetProductInformation(0,info_progmem);   //idev, b_progmem
                if (info)
                {
                    SetN2kPGN126996(msg,
                        info->N2kVersion,
                        info->ProductCode,
                        info->N2kModelID,
                        info->N2kSwCode,
                        info->N2kModelVersion,
                        info->N2kModelSerialCode,
                        info->CertificationLevel,
                        info->LoadEquivalency);
                    msg.Source = MONITOR_NMEA_ADDRESS;
                    m_device_list->HandleMsg(msg);
                }
                else
                    my_error(0,"COULD NOT addSelfToDeviceList(%d) PRODUCT INFO",i);
                break;
            }
            case 2 :
            {
                #define MAX_BUF  32
                char inst1[MAX_BUF+1];
                char inst2[MAX_BUF+1];
                char manuf[MAX_BUF+1];

                GetInstallationDescription1(inst1, MAX_BUF);
                GetInstallationDescription2(inst2, MAX_BUF);
                GetManufacturerInformation(manuf, MAX_BUF);

                SetN2kPGN126998(msg,inst1,inst2,manuf,false);
                    // bool UsePgm=false

                msg.Source = MONITOR_NMEA_ADDRESS;
                m_device_list->HandleMsg(msg);
                break;
            }
        }   // switch
    }   // for
}   // addSelfToDeviceList()
