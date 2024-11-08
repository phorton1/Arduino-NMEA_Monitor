# NMEA_Monitor - History

This readme describes the things I went through to get my
NMEA_Sensor and NMEA_Monitor working with the NMEA2000
and associated libraries.

It is mostly reflective of my first few weeks of struggling
with the libraries and hardware.  It is not presented in a
purely chronological order but rather a mix of the history,
and my impressions, at various points in time.



## Pure CANBUS

I started off by getting two ESP32's to talk to each other over CANBUS with mpc2515's
using the **autowp/mcp2515** library.  That was relatively easy, taking perhaps 1/2 day.

Then I tried to figure out how to work with the **ttlappalainen/NMEA2000** library.

Although there are some useful example programs and a variety of documentation available
for the libraries, reaching an understanding of how it all works was not at all trivial
or straight forward, and, as of this writing, I am still learning about the system.

I will merely mention that I had to spend 2-3 days trying to understand the *NMEA2000*
library object model, before I really understood that I needed to use the
**ttlappalainen/CAN_BUS_Shield** library in order to work with NMEA2000 and the
mcp2515.

So I now have CANBUS_Sensor.ino and CANBUS_Monitor.ino programs (which currently
exist in a private repo) working, and talking to each other, over pure CANBUS,
without yet involving any NMEA2000 stuff.

And once again, although I present it as a simple fact, it was many days after
that, as I was struggling with the NMEA2000 stuff, and much testing, that I assured
myself that these two pure-CANBUS progams could talk to each other using **either**
the *mcp2515* library **or** the *CAN_BUS_Shield* library.


## Rant about first exposure to NMEA2000 library

Please understand that I appreciate the effort of Timo and the many other
contributors to the NMEA2000 libraries and documentation efforts.
Nonetheless, this was my experience.

I have been a C++ programmer for over 40 years.  I have written millions
of lines of C++ and worked on some of the worlds most complex, and profitable
programs since 1978.  I am not perfect, and I have a lot of my own style
that does not necessarily correspond to modern day best practices.

Having said that.  Here are some personal rants about reading
this code base.

I REALLY don't like the way the classes start with private: sections, have many
intermingled private and protected sections, and then somewhere in the middle
(after you finally find it) there is a public: section that starts the API.

I dont use a sophisticated development environment.  I READ THE CODE.


### Everything is UpperCase and difficult to type

There is no distinction or naming convention for member variables, global
variables, global methods, helper methods, etc.  Almost all members variable,
parameters, methods, are capitalized.

I prefer to use lower case for the first letter of these. With capitals
delineating words within an identifier. I prefer that member variables
start with a prefix like "m_".  Personally I use "g_" for global variables,
and tend to use a leading underscore ("_") for private methods.

Another complaint is about the overuse of N2k as a prefix.  It is difficult to
type.  For instance, when trying to use copy-and-paste to get started from
an example, you might see a fuction like this:

```
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  int iHandler;
  // Find handler
  for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
  if (NMEA2000Handlers[iHandler].PGN!=0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg);
  }
}
```

So, we have a lower case 't' as the prefix for a type, followed by upper case, then a number,
then a lower case, then an uppercase, then two lower cases.  I have to hit and release the
shift key FIVE times just to type that.  Then (it's a very small method) an ugly parameter
name of &N2kMsg, when simply &msg would have been a lot easier on the eyes and fingers.

Another thing I don't like is the use of all caps for variable names.
All the programs have an NME2000 variable, which I for me would be better
as simply "nmea2000".


Sometimes its N2k, sometimes it's NEMA2000.

There are 100's of methods with copy and paste comments.  For me it is too bad that
people expect that a machine will look at your code.  I am a human, and I READ
the code WITH MY EYES.

I appreciate all of the hard work that went into these libraries, but making them
more complicated and difficult to read, understand, and work with is not
the best approach in my opinion.



### Terminology Issues

Some of the terminology in the NMEA libraries strikes me as incorrect and/or
just plain backwards.  That could be because I am a native English speaker,
and the authors, in many cases, are not.  But just to give one example.

In the NMEA2000 library there are example programs for a "TemperatureMonitor" and a "WindMonitor".
My understanding of the term "monitor" is a device that monitors (pays attention to) something.
I believe these would be better called a "TemperatureSensor" and a "WindSensor".

From the Oxford dictionary

> **Monitor** - a device used for observing, checking, or keeping a continuous record
of something. i.e. "a heart monitor"

So, in this terminology a "monitor" is something that Observers, Logs, and/or conveys
information for a human or other downstream system. TemperatureMonitor
and WindMonitor do not observe, check, or keep a continuous record of these values.

It would have helped my understanding more easily if these had been called
TemperatureSensor and WindSensor. I know this is trivial and pedantic.
But it goes to the fact that as a completely new reader of this library,
trying to understand the relationship of objects, it took me precious time
to understand that these were SENDERS and not RECEIVERS of information.


### All Things to All People for All Time

This is a personal peeve I have to many libraries and open source projects
these days.

The libraries suffer from trying to be **all things to all people for all time**,
which is fine if you're Microsoft and have 1000's of engineers and people to
write documentation and support software over its lifecycle, but not so great
for complex libraries written by a relatively small group of engineers.

As an example, the NMEA2000 library includes task scheduling methods and
objects including one called **tN2kSyncScheduler**.  The tN2kSyncScheduler
is used internally by the NMEA2000 library to schedule *Hearbeat PGN*
transmissions.

Some, or most, of the example programs make use of these scheduling
objects and methods.   Yet they are not documented well enough to
be understandable.  And, perhaps more importantly, it is not clear
at all that they are **not necessary**, and are mostly substitutes
for utilization of a *millis()* timer which would arguably better
be left up to the author, and the specific MPU being used.

Timers and task scheduling, in my opinion, should either (a) not
be part of an NMEA library, or (b) they should be documented
and supported in detail, with meaningful information, across
all platforms (MPUS) which the library claims to support.

It will take me more time to understand how these work,
or **do not work as expected** going forward.

On just one MPU, the ESP32, a whole book could be written
about task scheduling.  Putting this in the NMEA2000 library
makes it more complicated without necessarily making it
easier to understand and use.


### Basic Object Model

The basic object model is another example.

The **NMEA2000_CAN.h** file, with enumerated object constatns, is a really **bad idea**.

Enumerating classes and behaviors not present in a library is a layer violation.
This is a violation of basic software architecture principals where *base objects
should know **nothing** about classes derived from them*.

The NMEA2000 library should not know about the NMEA2000_mcp, NMEA2000_esp,
NMEA2000_teensy, or any other future libraries that might be built on top
of NMEA2000.

Imagine if Adafruit had tried to enumerate all derived classes in their Adafruite_GFX
library.  As if every new screen or display IC that is invented must then be enumerated
in the base class.

It is an approach guaranted to be limiting and unsupportable over time, and
an approach that actually makes it **more difficult** to understanding and use
as opposed to making it *easier* as the well-intentioned author would like.

Personally, this alone added several days of work to my initial foray
into NMEA2000, as I tried to follow the documentation and use **NMEA2000_CAN.h**.

Grrr :-)

It is just a complicated way of understanding that you must install,
and use, the proper derived class.

In the WindMonitor example, for instance, there is only this line to guide you

```
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
```

Which is, at best, entirely misleading.  My first attempt is with an ESP32
and an mcp2515 module.   Do you think it automatically figured that out?

LOLOLOL

In fact, the usage model in including NMEA2000_CAN.h
is that you must FIRST define HOW you are implementing the CAN device,


Here is the top portion of NMEA2000_CAN.h:

```
//  Just include this <NMEA2000_CAN.h> to your project and it will
//  automatically select suitable CAN library according to board
//  selected on Arduino development environment. You can still force
//  library by adding one of next defines before including NMEA2000_CAN.h:

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

Which then goes on to explain that you will need to install additional libraries
and coordinate them with the setting of USE_N2K_CAN.

As if you are going to write a program then arbitrarily switch back and forth
betwene a 16K 16Mhz Arduino and a dual core 240Mhz ESP32 with Wifi and BT.

LOLOL, sigh.


### Untested

It later became clear to me that Timo has/had a focus on certain Arduino devices,
although he recommends Teensies and ESP32's almost everytime he mentions an Arduino,
and that the example programs in no way explain how to use the various devices.

I think it was unfortunate (for me) that the only CANBUS module I had when I started
this was an mpc2515 module.  And my initial MPU decision was an ESP32.  It actually
took me the best part of 2 days to even realize that I did not need, or want the
NMEA2000_esp32 library for that configuration.

As I am revisiting this rant, I am in the middle of trying to understand why
(I had to figure out) that I had to provide a "DEBUG_RXANY=1" compile definition
to get my NEMA_Monitor program to even begin working.     I mean this is after
more than a week of looking at these libraries.

After I got the autowp/mcp2515 library working, and even after I got the
CAN_BUS_Shield library working (misnamed, it's not a shield in my case).
between two ESP32's, after I implemented my NMEA_Sensor and this NMEA_Monitor
program, they wouldn't talk to each other.  I tried everything. Pulled my hair out.

Finally after DIGGING into mpc_can.cpp and seeing this nowhere-documented
but likely looking define, when I added -D DEBUG_RXANY=1 to my build,
all of a sudden they started talking!

I still have no clue how it's *supposed* to work.  One would think that
setting the "ExtendReceiveMessages" in my nmea2000 object would somehow
translate to setting recieve filters and masks on the mcp2515, but I can
see no evidence whatsoever that ANYTHING in NMEA2000 or NMEA2000_mcp libraries
EVER calls the (apparently crucial if you don't define DEBUG_RXANY=1)
MCP_CAN (mcp2515) init_Mask() or init_Filt() methods.

Plus, just for more obscurity you cant even GET TO the mcp_can member
object on the tNEMA2000_mcp object because it's private (and once again
weirdly named):

```
private:
    MCP_CAN N2kCAN;
```

If it was my code that would AT LEAST be protected and would be named
much clearer:

```
protected:
    NEMA2000_mcp2515 m_mcp2515;
```

I guess what happened is that Timo was *planning* to merge the CAN_BUS_Shield
library into the NMEA2000_mcp repository and elimitating the CAN_BUS_Shield repo,
but he never got that far and just left the labrynth for future unsuspecting
programmers, like me, to decipher.


## Moving Forward

After I had my two ESP32 talking to each other I felt the need to
talk to some kind of a reference device, so I dove into *Actisense*
and the **Actisense Reader**

It took me several days to understand the architecture well enough to implement
a relatively simple NMEA2000 Sensor and Monitor, and subsequently, it has taken
much more time to try to refine those so that they work well enough with the
**Actisense NMEA Reader** that I will feel confident to try to attach the Monitor
to my Fischer Panda Generator to see if I can talk to it.

### Pre-Actisense

The **first step**, above, was to implement simple CANBUS communication.
When I got that working, I then tried to move up to the **NMEA2000** level.

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


### Documentation Limitations

I will try to be kind to Timo in these notes.  There is a vast amount of
work that he and other have done to get the NMEA2000 Library to where it
is, so that I could even try it.

The documentation is limited, assuming significant amounts of "forward"
knowledge about NMEA2000 and the library itself.

There is not really a good tutorial on how to get started, nor a
decent architectural overiview document that descrbes how it all
fits together.  *I am considereing writing these**.

There is a single short tutorial, consisting of one or two paragraphs,
with instructions to compile a given example program after changing
various #defines in the source code.  The author mostly assumes the
use of a specific Arduino board, and doesn't really give any kind of an
overview of how the example fits into the larger picture of an NMEA2000 network.
The example itself relies heavily upon assumed knowledge about other programs,
like Actisense Reader, which is a propriatary program using a propriatary
undocumented protocol, or OpenSkipper, which is a hugely complicated
project of its own.

So, here I try to leave a few breadcrumbs for someone that might follow
a similar path that I have taken.


### Basic Library Concepts

The NMEA2000 library can work with many different MPUs and interfaces.
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
tNMEA2000_mcp nmea2000(CAN_CS_PIN,MCP_8MHz);
```

I have to use a non-standard CS pin becaue the standard ESP32 CS (15)
is soldered to the st7899 dispoay.

Importantly, it took me a full day to figure out the simple include
and ctor, and then some.  tNMEA2000_mcp is **derived** from the *tNMEA2000**
class, and yet, with compile definitions, the NMEA2000 library *knows*
about the NMEA2000_mcp library.  The exmples are SO lacking in any overview.

Which it turned out that I very much needed to to figure all of this out
just to get a simple two node network test working.

My mpc2515 module has an **8Mhz** crystal soldered to it.   All of Timo's exmaples,
when you finally get to the mpc215, have the 16Mhz constant, not the 8Mhz one in their
ctor.  It was an act of faith to change it in the tNMEA2000_mcp ctor.

Sheesh, it took almost two full days to get a simple 130316 (temperature) packet from
the Sensor to the Monitor.  But not before I had to also learn, the hard way, about the
basic object model;

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

Whew.  OK, I understand. There are hundreds of PGNa and 4 methods for each.

**TODO QUESTION:** Is a table driven API to PGN's possible?




## Actisense Reader One

So far, all of the beginner's tutorial documentation I could find was basically
presented in a few lines of sparse text in one of Timo's documents.  Then we *merely*
install this app (proprietary 3rd party app), set afew variables, and we should
see it in the Actisense reader, and I guess everybody can go home at that point.

That's basically where Timo's "tutorial" and any kind of overview documentation, end.

LOLOL.

I downloaded and installed this propriatary program, still learning, not understanding,
but lo and behold, the temperature made it from the Sensor, to the Monitor, to the USB
Serial port, to the program and showed up on my laptop.

But unfortunately that's where both the documentation and the examples and
any kind of systemic overview of the concepts and architure of the libraries
and any applications yu might write simply end, and you are completely on your
own to reverse engineer thousands of lines of code, hundreds of messages,
dozens of files, multiple libraries, and, once again, underlying it all, a
completely opaque set of proprietary protocols.



### Actisense Reader Two

I spent most of a full day fruitlessly trying to understand and use the NMEA2000 library
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

**IMPORTANT** - I now use iDev=0 parameter in the setup call SetProductInformation() so that
subsequently, upon startup, or if queried, I can call SendProductInformation() with its default *idx=0"
to send it to the network.

It looks like when the App **recieves** an address claim it then **requests** product information.

I have since been able to receive a PGN_REQUEST(59904) for PGN 60928 - Get Product Information
and figured out that I need to call SendProductInformation() in response.

Of course, I had to sort through the 17 different signatures for "SendProductInformation"
and undeerstand the device idx thing, before I figured out how to get this working.

My program now has the option of **sending out** ProductInformation and ConfigInformation
when I boot. BTW, It's better to not use the multi-packet boolean parameter
in any of these calls, as the App (Actisense Reader) seems to miss them occasionally.
By using multi-packet=false, they get sent as one big packet each which seems to work better.

The library seems to imply that it manages several devices, but I have yet to figure out how
that works.



## Final History Notes

I now feel as if I can move on past this *history.md* document and begin
writing **real** documentation and code.  Before I do, this section
contains a last few notes of where I am at this point in time, and
a few snippets of code that I thought were important enough to document.

- I am getting the correct Device, Product and Configuration information in the App
- My programs (the Monitor) is not responding to the Apps requests, specifically for Product Information
- The App asks once per second for an undocumented PGN "Get Operting Mode" which I don' what to do with.
- I can see the bytes coming from the App to the monitor over USB
- I don't know what to do next
- I'm not sure if the monitor should be agregating devices or not

Also see
[actisense_junk.txt](actisense_junk.txt) and
[AI_CONVERSTATION.docx](AI_CONVERSTATION.docx) and


### Setup Calls

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


### Network Device Enumeration (PGNS) General

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
  its Product Information.  It does not happen automatically.
- **PGN(126998) - Device Configuration** - This message is sent by a device to broadcast
  its Device Configuration Information. It does not happen automatically.
- **PGN_REQUEST(59904,xxx)*** - PGN Request - This message is sent to ask devices to respond with a particular
  PGN(xxx).  You may need to learn about this to get anything nice to show in the Actisense Reader app.


Other interesting PGNS include

- 126992 - System Time


### Behavior notes

In reaponse to a Address Claim, The App sends out REQUEST_PGN(59904==0xea00) for
PGN(60928) Address Claim (even though the AI I talked to said this is a bad practice).

Weird.

I include a full conversation I had with an AI about this stuff at
[AI_CONVERSTATION.docx](AI_CONVERSTATION.docx)

Hours and Hours and Hours.  If only someone had thought to make it simpler, and
more clear rather than trying, and failing, to be everything to everyone forever.


### Dispatching from NMEA_Monitor

After now successfully parsing actisense requests, and getting the REQUEST_PGN(Address Claim),
how should the Monitor go about dispatching it?

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


