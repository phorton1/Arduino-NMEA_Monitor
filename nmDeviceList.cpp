//--------------------------------------
// nmDeviceList.cpp
//--------------------------------------

#include "NM.h"

#if WITH_DEVICE_LIST

    #define START_DELAY_MS    8000

	tN2kDeviceList *device_list;
        // instantiated in NM.cpp::setup_NMEA2000()


    void printText(const char *text, bool with_cr=true)
    {
      if (text) Serial.print(text);
      if (with_cr) Serial.println();
    }


    static void printUlongList(const char *prefix, const unsigned long *list)
    {
        uint8_t i;
        if (list)
        {
            Serial.print(prefix);
            for (i=0; list[i]!=0; i++)
            {
                if (i>0) Serial.print(", ");
                Serial.print(list[i]);
            }
            Serial.println();
        }
    }


    static void printDevice(const tNMEA2000::tDevice *pDevice)
    {
        if ( pDevice == 0 ) return;

        Serial.println("----------------------------------------------------------------------");
        Serial.println(pDevice->GetModelID());
        Serial.print("    Source: ");                       Serial.println(pDevice->GetSource());
        Serial.print("    Manufacturer code:        ");     Serial.println(pDevice->GetManufacturerCode());
        Serial.print("    Unique number:            ");     Serial.println(pDevice->GetUniqueNumber());
        Serial.print("    Software version:         ");     Serial.println(pDevice->GetSwCode());
        Serial.print("    Model version:            ");     Serial.println(pDevice->GetModelVersion());
        Serial.print("    Manufacturer Information: ");     printText(pDevice->GetManufacturerInformation());
        Serial.print("    Installation description1: ");    printText(pDevice->GetInstallationDescription1());
        Serial.print("    Installation description2: ");    printText(pDevice->GetInstallationDescription2());
        printUlongList("    Transmit PGNs :",               pDevice->GetTransmitPGNs());
        printUlongList("    Receive PGNs  :",               pDevice->GetReceivePGNs());
        Serial.println();
    }


    // extern
    void listDevices(bool force /*= false*/)
    {
        static bool started = false;
        static uint32_t start_time = 0;
        if (!started)
        {
            uint32_t now = millis();
            if (!start_time)
                start_time = now;
            else if (now - start_time > START_DELAY_MS)
                started = true;
            return;
        }

        // return if !force & list has not changed
        
        if (!force && !device_list->ReadResetIsListUpdated())
            return;

        Serial.println();
        Serial.println("**********************************************************************");
        for (uint8_t i=0; i< N2kMaxBusDevices; i++)
        {
            printDevice(device_list->FindDeviceBySource(i));
        }
    }


#endif  // WITH_DEVICE_LIST
