//--------------------------------------
// nmDeviceList.cpp
//--------------------------------------

#include "NM.h"

#if WITH_DEVICE_LIST

    #define dbg_dl  0

    #define START_DELAY_MS    8000

	tN2kDeviceList *device_list;
        // instantiated in NM.cpp::setup_NMEA2000()


    void printText(const char *text, bool with_cr=true)
    {
        if (text) dbgSerial->print(text);
        if (with_cr) dbgSerial->println();
    }


    static void printUlongList(const char *prefix, const unsigned long *list)
    {
        uint8_t i;
        if (list)
        {
            dbgSerial->print(prefix);
            for (i=0; list[i]!=0; i++)
            {
                if (i>0) dbgSerial->print(", ");
                dbgSerial->print(list[i]);
            }
            dbgSerial->println();
        }
    }


    static void printDevice(const tNMEA2000::tDevice *pDevice)
    {
        if (pDevice == 0) return;
        if (!dbgSerial) return;

        dbgSerial->println("----------------------------------------------------------------------");
        dbgSerial->println(pDevice->GetModelID());
        dbgSerial->print("    Source: ");                       dbgSerial->println(pDevice->GetSource());
        dbgSerial->print("    Manufacturer code:        ");     dbgSerial->println(pDevice->GetManufacturerCode());
        dbgSerial->print("    Unique number:            ");     dbgSerial->println(pDevice->GetUniqueNumber());
        dbgSerial->print("    Software version:         ");     dbgSerial->println(pDevice->GetSwCode());
        dbgSerial->print("    Model version:            ");     dbgSerial->println(pDevice->GetModelVersion());
        dbgSerial->print("    Manufacturer Information: ");     printText(pDevice->GetManufacturerInformation());
        dbgSerial->print("    Installation description1: ");    printText(pDevice->GetInstallationDescription1());
        dbgSerial->print("    Installation description2: ");    printText(pDevice->GetInstallationDescription2());
        printUlongList("    Transmit PGNs :",               pDevice->GetTransmitPGNs());
        printUlongList("    Receive PGNs  :",               pDevice->GetReceivePGNs());
        dbgSerial->println();
    }


    // extern
    void listDevices(bool force /*= false*/)
    {
        if (!dbgSerial) return;

        static bool started = false;
        static uint32_t start_time = 0;
        if (!started)
        {
            uint32_t now = millis();
            if (!start_time)
            {
                start_time = now;
            }
            else if (now - start_time > START_DELAY_MS)
                started = true;
        }


        if (!started)
        {
            // This was a vestigial attempt to speed up
            // the enumeration when on a quiet bus. 
            #if 0
                tN2kMsg msg(0);
                device_list->HandleMsg(msg);
            #endif
            return;
        }
        
        
        // return if !force & list has not changed
        
        if (!force && !device_list->ReadResetIsListUpdated())
            return;

        dbgSerial->println();
        dbgSerial->println("**********************************************************************");
        for (uint8_t i=0; i< N2kMaxBusDevices; i++)
        {
            printDevice(device_list->FindDeviceBySource(i));
        }
    }


	#if ADD_SELF_TO_DEVICE_LIST

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

		void addSelfToDeviceList()
        {
            for (int i=0; i<3; i++)
            {
                tN2kMsg msg;
                display(dbg_dl,"addSelfToDeviceList(%d)",i);

                switch (i)
                {
                    case 0 :
                    {
                        const tNMEA2000::tDeviceInformation info = nmea2000.GetDeviceInformation();
                        SetN2kPGN60928(msg,
                            info.GetUniqueNumber(),
                            info.GetManufacturerCode(),
                            info.GetDeviceFunction(),
                            info.GetDeviceClass(),  // << 1?
                            info.GetDeviceInstance(),
                            info.GetSystemInstance(),
                            info.GetIndustryGroup() );
                        msg.Source = MONITOR_NMEA_ADDRESS;
                        device_list->HandleMsg(msg);
                        break;
                    }
                    case 1 :
                    {
                        bool info_progmem;
                        const tNMEA2000::tProductInformation *info = nmea2000.GetProductInformation(0,info_progmem);   //idev, b_progmem
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
                            device_list->HandleMsg(msg);
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

                        nmea2000.GetInstallationDescription1(inst1, MAX_BUF);
                        nmea2000.GetInstallationDescription2(inst2, MAX_BUF);
                        nmea2000.GetManufacturerInformation(manuf, MAX_BUF);

                        SetN2kPGN126998(msg,inst1,inst2,manuf,false);
                            // bool UsePgm=false

                        msg.Source = MONITOR_NMEA_ADDRESS;
                        device_list->HandleMsg(msg);
                        break;
                    }
                }   // switch
            }   // for
        }   // addSelfToDeviceList()

	#endif  // ADD_SELF_TO_DEVICE_LIST

#endif  // WITH_DEVICE_LIST
