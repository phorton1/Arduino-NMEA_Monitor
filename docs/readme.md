# NMEA_Monitor

An ESP32 NMEA 2000 monitor/gateway.

- Currently uses an mpc2515 CAN module
- Supports an st7889 oled display
- Has WiFi and Telnet Accessibility
- Can communicate with my NMEA_Sensor example
- Can communicate (act as a gateway) to a Win10 Actisense Reader application over USB Serial


## Pre NMEA2000 library notes

I started off by getting two ESP32's to talk to each other over CANBUS with mpc2515's
using the **audowp mcp2515** library.  That was relatively easy, perhaps 1/2 day

Then I tried to figure out how to work with NMEA 2000, specifically, the
**ttlappalainen/NMEA2000** library on github.  Although there seem to be useful
examples at first blush, in reality, the library, and it's usage are very complicated.
It took me several days to understand the architecture well enough to implement
a relatively simple NMEA2000 Sensor and Monitor, and subsequently, it has taken
much more time to try to refine those so that they work well enough with the
**Actisense NMEA Reader** that I will feel confident to try to attach the Monitor
to my Fishcher Panda Generator to see if I can talk to it.

The **first step** was to implement simple CANBUS communication using the
*ttlappalainen/CAN_Bus_Shield* library, which I renamed to *NMEA2000_mcp2515* on
my machine, for reasons I might explain later.  When I got that working, I then
tried to move up to the **NMEA2000** level.

Before I did that, however, I played around with other things like creating
my own **DS18B20 temperature sensor library** and **ST7789 oled display** library.
Eventually, for simplicity, I removed the actual temperature sensor from the
NMEA_Sensor application, and just let it send a series of known temperatures.
However, I had hopes of still using the ST7789 display in some meaningful
manner.

It wasn't until (much) later, after I had NMEA2000 basically working that
I had to go back and really take a look at the interactions between the
st7789 display and the mcp2515 module, both of which use SPI, and spend
at least a full day just resolving the conflicts by using the ESP32's
2nd SPI Device (HSPI) for the mcp2515, since the st7789 display is
soldered to the board using the regular ESP32 (VSPI) SPI device.
That, and many other dealings with timings due to Serial display
debugging, and so, before I wrote this document.

Nonetheless I had the CAN_BUS_Shield library working with my little
demo as I then tried the NMEA2000 library.


## NMEA2000 Library Notes

I will try to be kind to Timo in these notes.  There is a vast amount of
work that he and other have done to get the NMEA2000 Library to where it
is, so that I could even try it.

Having said that, although the author and a few other websites imply that
the library is somewhat easy to use, and seemingly give useful examples, upon closer
examination, neither of those are quite accurate.  The library is actually
very complicated to use, and the examples assume a relatively large amount
of knowledge about NMEA2000 and the library itself, and are not anywhere
clearly explained.

Likewise, the documentation is limited, once again assuming significant
amounts of knowledge about NMEA2000 and the library itself.  There is a
single short tutorial, consisting of one or two paragraphs, with instructions
to compile a given example program after changing various #defines in
the source code.  The author mostly assumes the use of a specific Arduino
board, and doesn't really give any kind of an overview of how the example
fits into the larger picture of an NMEA2000 network.   It relies heavily
upon assumed knowledge about other programs, like Actisense Reader, which
are themselves propriatary, or OpenSkipper, which is a hugely complicated
project of its own.

So, here I try to leave a few breadcrumbs for someone that might follow
a similar path that I have taken.


### Basic Library Concepts

The NMEA2000 library can work with many different MPU and interfaces.
Some modules essentially convert the CANBUS signals into serial TX/RX
signals that are handled by peripherals in the silicon of various
MPUs.  Teensies and ESP32's, for example, have internal CANBUS peripherals.

I didn't have any of those Serial CANBUS modules when I started.
I have ordered some, but it takes weeks for things to get here.
I had two mpc2515 CANBUS modules that I had bought years ago and had never tried.
Hence why I started with the autowp mcp2515 library.

Although Timo also provides an NMEA2000_esp32 library, and of course, that
was the first thing I looked at, it took me a while to understand that
the esp32 library was only for those Serial CANBUS modules hooked to
RX/TX pins on the ESP32.  It then took me an additional while to figure
out how to get the mpc2515 working with the NMEA2000 library.


### NMEA2000_mcp Library

It turns out that to use the NMEA2000 library with the mcp2515 you need
THREE libraries.  First you need the **CAN_BUS_Shield** library which
Timo did not, per se, write, but merely ported into his project. Second,
you need the basic **NMEA2000** library.  What was not clear was that
you need the **NMEA2000_mcp** library, and that, in fact, THAT is the object
that you really create.

```
#define CAN_CS_PIN 5
#include <NMEA2000_mcp.h>
tNMEA2000_mcp nmea2000(CAN_CS_PIN,MCP_8MHz,CAN_500KBPS);
```

I have to use a non-standard CS pin becaue the standard ESP32 CS (15)
is soldered to the st7899 dispoay.

But, importantly, it took me a full day to figure out the simple include
and ctor, and then some.  tNMEA2000_mcp is **derived** from the *tNMEA2000**
class, so it actually sits above the main library as the entry point.
But the exmples are SO complicated.

For exmaple, there is a file, NMEA2000_CAN.h in the main library, that
several exmaples use.  And it can be made to include the NMEA2000_mcp
library by the akward use of a compile time define.

```
#define USE_N2K_CAN 1  // for use with SPI and MCP2515 can bus controller
#define USE_N2K_CAN 2  // for use with due based CAN
#define USE_N2K_CAN 3  // for use with Teensy 3.1/3.2/3.5/3.6 boards
#define USE_N2K_CAN 4  // for use with avr boards
#define USE_N2K_CAN 5  // for use with socketCAN (linux, etc) systems
#define USE_N2K_CAN 6  // for use with MBED (ARM) systems
#define USE_N2K_CAN 7  // for use with ESP32
#define USE_N2K_CAN 8  // for use with Teensy 3.1/3.2/3.5/3.6/4.0/4.1 boards
#define USE_N2K_CAN 9  // for use with Arduino CAN API (e.g. UNO R4 or Portenta C33)
```

This is presumably so that you can write an application independent of the
MPU and/or CAN module. OK, I'll buy that :-)  But in practice it turned out
to be very hard to figure out that, even though you change the define, you
MUST ALSO change the object declaration (i.e. use tNMEA2000_mcp for USE_N2K_CAN==1),
at least to the degree that you *might* want to access mcp2515 specifics in
your implementaiton.

Which it turned out that I very much needed to do to figure it out and get it working.
My mpc2515 module has an **8Mhz** crystal soldered to it.   All of Timo's exmaples,
when you finally get to the mpc215, have the 16Mhz constant, not the 8Mhz one in their
ctor.  It was an act of faith to change it in the tNMEA2000_mcp ctor.

But worse, all of my previous examples, **already working code**, were using a 500kbs CANBUS
bus speed, and when I could not get anything to work with the NMEA200, I **had** to fall back
to the working code and work my way back up from the autowp mpc2515 library, make sure it could
talk to the CAN_BUS_Shield library, and then use one of those to determine if my NMEA_Sensor
(transmitter) was at fault or my NMEA_Monitor was at fault.   That's when, through laborious
tracing of the mountain of code, I realized, not being able to sleep one night, that the
problem was probably the bus speed.   It turns out that, although the CAN_BUS_Shield library
allows for parameterization of the bus speed in it's ctor, the NMEA2000_mcp ctor called it
with a hardwired bus speed of 250kbps.

This is about when I forked all three libraries, so that I could have a stable starting point,
as I modified the NMEA2000_mcp ctor and object so that I couild pass in 500kps, so that I
could at least see if CAN packets were being sent by the Sensor, and then could send CAN
packets and see if anything happened on the Monitor side.

Sheesh, it took almost two full days to get a simple 130316 (temperature) packet from
the Sensor to the Monitor.  But not before I had to also learn, the hard way, about the
basic object model;

BTW, I hate the method and parameter names, the order of declarations, and lack of
clarity in the H files of all of these libraries.   Mountains of stuff, private,
protected, private, public, private, with large re-gurgitated comment sections making
it hard to look at.

The Sensor builds a specific 130316 message using SetN2kPGN130316(), and calls SendMsg:

```
	tN2kMsg mzg; 	// it's a class, not a structure!
	SetN2kPGN130316(
		mzg,
		255,// unsigned char SID; 255 indicates "unused"
		93,	// unsigned char TempInstance "should be unique per device-PGN"
		N2kts_RefridgerationTemperature, // tN2kTempSource enumerated type
		CToKelvin(tempDouble),
		N2kDoubleNA	// double SetTemperature
	);

	// SIDS have the semantic meaning of tying a number of messages together to
	// 		one point in time.  If used, they should start at 0, and recyle after 252
	// 		253 and 254 are reserved.

	nmea2000.SendMsg(mzg);
```

Those comments I added were hard fought for.

The Monitor sets an "onMessage" callback and calls ParseMessages() tightly from loop().
When the monitor gets a message, it calls onMessage() and you can call a specific
parser to interpret the message.

```
...
nmea2000.SetMsgHandler(onNMEA2000Message);
...
void onNMEA2000Message(const tN2kMsg &msg)
{
	if (msg.PGN == PGN_TEMPERATURE)
	{
		uint8_t sid;
		uint8_t instance;
		tN2kTempSource source;
		double temperature1;
		double temperature2;

		bool rslt = ParseN2kPGN130316(
			msg,
			sid,
			instance,
			source,
			temperature1,
			temperature2);
```

Whew.  OK, I understand, there are hundreds of messages.

After that was somewhat working reliably, I decided to try the **Actisense** stuff.



## Actisense Reader One

So far, all of this document was basically presented in a few lines of sparse
text in one of Timo's documents.  Then we *merely* install this app, set a
few variables, and we should see it in the Actisense reader, and I guess
everybody can go home at that point.

LOLOL.

I downloaded and installed this propriatary program, still learning, not understanding,
but lo and behold, the temperature made it from the Sensor, to the Monitor, to the USB
Serial port, to the program and showed up on my laptop.

Then I changed one thing, I don't know what it was, and it stopped working, and it
took me 12 hours to figure out that it was either the baud rate problem or the SPI
problem, before I was able to get it to work again.

After some sleep I was satisified that Actisense was sort of working.

But unfortunately that's where both the documentation and the examples and
any kind of systemic overview of the concepts and artiectures involved just
simple end, and you are completely on your own to reverse engineer thousands
of line of code, hundreds of messages, dozens of files, multiple libraries,
and, once again, underlying it all, a completely opaque set of proprietary
protocols.



## Actisense Reader Two

I spent most of yesterday fruitlessly trying to understand and use the NMEA2000 library
ActisenseReader object.  Writing my own code to see if I was getting any bytes.  Struggling
with the tiny ST7789 display as my only debugging output when the USB Serial is in "ACTISENSE"
mode (its a binary, proprietary protocol - pretty simple wrapper, but no docs anywhere).

I ended up adding **WiFi** and **Telnet** to my ESP32 apps so that I could have an alternative
Serial port for debugging.  I *could* have used Serial2 on the ESP32 and a SerialUSB converter
module, but I really don't want more wires and USB cables to my laptop.

The story goes like this.

- The Actinsense App shows the temperature.
- Apparently when you "Open" the nmea2000 object, it sends out an "address claim" message
- The address claim message includes the values from your setup() SetDeviceInformation() call.
- Then the ActisenseApp shows it as a Device with that stuff.
- But the App does not, by default show any Product Information or Configuration Information

*Insert Hours of trying to figure out NMEA2000 modes, ForwardOwnMessages(), etc, etc, etc, etc, etc, etc*
as I added Product Information, Configuration Information, Instances, and so on to both programs.

Note once again that the "address claim" is only sent out by the library **once right after nmea2000,Open()**,
and that it **never automaticaly sends the Product or Configuration information**,

**IMPORTANT** - I typically use iDev=0 parameter in the setup call SetProductInformation() so that
subsequently, upon startup, or if queried, I can call SendProductInformation() with its default *idx=0"
to send it to the network.


It looks like when the App **recieves** an address claim it then **requests** product information.
However I have not at all been able to figure out how to get the NMEA2000 library ActisenseReader object
to work and/or hook it up to the nmea2000 object so that it responds to those requests.  There are
implications in Library docs and comments that it should work, but no concrete descriptions, and
it did not work out of the box.

Finally, after sorting out the 17 different signatures for "SendProductInformation" I figured out
how it works (sort of).  And the same for SendConfigurationInformation, SendTxPGNList, and SendRxPGNList.

**I now send those out when I boot**.  BTW, It's better to not use the multi-packet boolean parameter
in any of these calls, as the App seems to miss them occasionally, so I send em as one big packet each.


The library seems to imply that it manages several devices, but I have yet to figure out how
that works.



## Status

- I am getting the correct Device, Product and Configuration information in the App
- My programs (the Monitor) is not responding to the Apps requests, specifically for Product Information
- The App asks once per second for an undocumented PGN "Get Operting Mode"
- I can see the bytes coming from the App to the monitor over USB
- I don't know what to do next
- I'm not sure if the monitor should be agregating devices or not


Here's some example setup() calls:

``` c++
nmea2000.SetProductInformation(
    "prh_model_100",            // Manufacturer's Model serial code
    100,                        // Manufacturer's uint8_t product code
    "ESP32 NMEA Monitor",       // Manufacturer's Model ID
    "prh_sw_1.0",               // Manufacturer's Software version code
    "prh_mv_1.0",               // Manufacturer's uint8_t Model version
    3,                          // LoadEquivalency uint8_t 3=150ma; Default=1. x * 50 mA
    2101,                       // N2kVersion Default=2101
    1,                          // CertificationLevel Default=1
    0                           // iDev (int) index of the device on \ref Devices
    );
nmea2000.SetConfigurationInformation(
    "prhSystems",           // ManufacturerInformation
    "MonitorInstall1",      // InstallationDescription1
    "MonitorInstall2"       // InstallationDescription2
    );
nmea2000.SetDeviceInformation(
    1230100, // uint32_t Unique number. Use e.g. Serial number.
    130,     // uint8_t  Device function=Analog to NMEA 2000 Gateway. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    25,      // uint8_t  Device class=Inter/Intranetwork Device. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    2046     // uint16_t choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );
```


## Network Device Enumeration (PGNS) General

- **PGN(60928) - Address Claim** - when a device starts up, as per the standard, it **must**
  broadcast a PGN(60928) Address Claim message.  If no other device replies (within 250 ms?)
  then it can assume, at least temporarily, that it "owns" the address it has claimed.
  If other devices reply, arbitration will likely need to take place (based on some
  *priority* scheme) to resolve the address conflict.  The PGN(60928) sends out the data
  that you provide in your setup() SetDeviceInformation() call.
- **PGN(126993) - Heartbeat** - the Library appears to send out a regular and repeating PGN(126992)
  Hearbeat message.  This notifies eveyone on the network of their continued presence,
  and *could* be used by a device that wishes to enumerate all devices to subsequently
  do requests to obtain more information about specific devices. Strangly the App doesn't do that.
- **PGN(126996) - Product Information** - This message is sent by a device to broadcast
  its Product Information.
- **PGN(126998) - Device Configuration** - This message is sent by a device to broadcast
  its Device Configuration Information.
- **PGN(126996) - PGN Request - This message is sent to ask devices to respond with a particular
  PGN.


Other interesting PGNS include

- 126992 - System Time


In reaponse to a Address Claim, The App sends out REQUEST_PGN(59904==0xea00) for
PGN(60928) Address Claim (even though the AI I talked to said this is a bad practice).

Weird.

### Dispatching from NMEA_Monitor

After now successfully parsing actisense requests, and getting the REQUEST_PGN(Address Claim),
how should the Monitor go about dispatching it.

Clearly it depends on the destination and source.  Lets add those to the debugging.


### FIRST COMPLETE ACTISENSE REPLY!!!

See AI_CONVERSATION.docx for a long conversation I had with an AI about
how the NMEA2000 protocol should work with regards to device enumeration.

I finally got the App's log window show a "Complete" status to one of its
actisense messages being sent via the serial port to the NMEA_Monitor.
NMEA_Monitor listens for PGN_REQUEST(Address Claim) and calls nmea2000.SendIsoAddressClaim()
in response.  Timing is an issue as when I was running a Telnet session
to the NMEA_Monitor the response would *bypass* the log window (resulting in
a timeout) although it would still show up in the App's DataView window.
