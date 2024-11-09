# NMEA_Monitor


Notes after [history.md](history.md)


## Handling Actisense GetProductInformation request

I have not figured out how you are supposed to hook the ActisenseReader
upto the library so that the library will do what it advertises, which
is supposedly to respond to standard PGN Requests.

- I have verified that my monitor's ActisenseReader object receives
  the GetProductInformation PGN_REQUEST(59904,126996) from the App.
- I have proven that I *can* respond to the request manually
  by calling nmea2000.SendProductInformation() in response.
- I have also built support for other "standard" PGNs
  but thus far they have never been called by the App.

These are the PGNS my current monitor's ActisenseReader callback
method responds to:

- PGN_ADDRESS_CLAIM			60928L
- PGN_PRODUCT_INFO			126996L
- PGN_DEVICE_CONFIG			126998L
- PGN_PGN_LIST				126464L



### Try SendMsg(actisens_msg,-1) to buss

There is a define in my monitor program which allows me,
instead of explicitly replying, to "put the msg on the bus" by
calling nmea2000.SendMsg(msg,-1).

I was kind of hoping that the Library would then respond.
It didn't.  I double checked and I have nmea200.SetForwardOwnMessages(true)
in my setup() method.

So, this morning I dig into

- under what circumstances does the Library handle GetProductInfoRequet, and
- how am I best supposed to get that to happen with an ActisenseReader object

One big question is whether or not I need to create and maintain a
DeviceList object, as perhaps IT is responsible for aggregating devices
and responding with multiple GetProductInfo requests.

But it seems that AT LEAST my monitor should somehow respond with
it's own product information, so that comes first.

### Library Exploration SendProductInformation()

I shouldn't have to do this.

Grep for SendProductInformation() shows a variety of places where it is called.

> This is automatically used by class. You only need to use this, if
you want to write your own behavior for providing product information.

Really?  And how, kind sir, am I supposed to get that to happen?

> "Currently Product Information and Configuration Information will we
pended on ISO request. This is because specially for broadcasted
response it may take a while, when higher priority devices sends
their response."

Once again, very difficult to understand.  I can find no
overview anywhere about these "pending" messages are or
if they are important, the core, of what I am looking for.

**How is this library supposed to work as a system**.

It sends out Heartbeats, that must mean that something is
hooked up correctly.

Continuing wasting my time looking for answers.

Maybe I should implement a serial command interface so that
the monitor can send the sensor a request and see what happens?

I will keep browsing through the miles of code before deciding
the next line of code I write.

Possible next greps:

- RespondISORequest()
- SendPendingInformation()
- tN2kGroupFunctionHandlerForPGN126996::HandleRequest()


> See document https://web.archive.org/web/20150910070107/http://www.nmea.org/Assets/20140710%20nmea-2000-060928%20iso%20address%20claim%20pgn%20corrigendum.pdf
For requirements for handling Group function request for PGN 60928

Sure, why not send me entirely back to square 1.
But these comments are not for me, they are for Timo to remember how he got there.



### grep for SendPendingInformation()

This looks promising as a part of a scheduled response handler.
**Its called directly from ParseMesages()** and so looks like its
part of the baked in request handler.

Actually, a closer look at ParseMessages() is warranted.

```
    if ( OpenState!=os_Open ) {
      if ( !(Open() && OpenState==os_Open) ) return;  // Can not do much
    }

    if (dbMode != dm_None) return; // No much to do here, when in Debug mode

    SendFrames();
    SendPendingInformation();
#if defined(DEBUG_NMEA2000_ISR)
    TestISR();
#endif

    while (FramesRead<MaxReadFramesOnParse && CANGetFrame(canId,len,buf) ) {           // check if data coming
        FramesRead++;
        N2kMsgDbgStart("Received frame, can ID:"); N2kMsgDbg(canId); N2kMsgDbg(" len:"); N2kMsgDbg(len); N2kMsgDbg(" data:"); DbgPrintBuf(len,buf,false); N2kMsgDbgln();
        MsgIndex=SetN2kCANBufMsg(canId,len,buf);
        if (MsgIndex<MaxN2kCANMsgs) {
          if ( !HandleReceivedSystemMessage(MsgIndex) ) {
            N2kMsgDbgStart(" - Non system message, MsgIndex: "); N2kMsgDbgln(MsgIndex);
            ForwardMessage(N2kCANMsgBuf[MsgIndex]);
          }
//          N2kCANMsgBuf[MsgIndex].N2kMsg.Print(Serial);
          RunMessageHandlers(N2kCANMsgBuf[MsgIndex].N2kMsg);
          N2kCANMsgBuf[MsgIndex].FreeMessage();
          N2kMsgDbgStart(" - Free message, MsgIndex: "); N2kMsgDbg(MsgIndex); N2kMsgDbgln();
        }
    }

#if !defined(N2K_NO_HEARTBEAT_SUPPORT)
    SendHeartbeat();
```

Note that parseMessages() calls SendHeartbeat() directly.

I think I can presume that SendPendingInformation() will do something
if a request is pending, but I'm guessing that the call to RunMessageHandlers()
wouild be what enqueues the pending request.

By the way, these are all PUBLIC methods. 100's of em.
But lol, mpc_can device is private. ok.


From the implementation of RunMessageHandlers()
I see that tMsgHandler's are a linked list of objects
and that a (protected member) pointer variable called
**MsgHandlers** is the list that is used..


The MsgHandlers variable is initialize to 0 in the NMEA2000 ctor()
and referenced in

- RunMessageHandler()
- AttachMsgHandler()
- DetachMsgHandler()

So, now I'm guessing that there will be an AttachMsgHandleer() someplace that
has been called.

### grep for AttachMsgHandler()

Unfortunately for me, it appears as if AttachMsgHandler() is
only intended as a client API and is used in a couple of
example programs as such.   It does not appear to be part
of any automatic internal system for handling requests.

### grep for HandleReceivedSystemMessage()

Oops, I think I missed the main entry point for this
in the implementation of ParseMessages().   In ITs
implementation I note this:

```
bool tNMEA2000::HandleReceivedSystemMessage(int MsgIndex) {
  bool result=false;

   if ( N2kMode==N2km_SendOnly || N2kMode==N2km_ListenAndSend ) return result;

    if ( N2kCANMsgBuf[MsgIndex].SystemMessage ) {
      if ( ForwardSystemMessages() ) ForwardMessage(N2kCANMsgBuf[MsgIndex].N2kMsg);
      if ( N2kMode!=N2km_ListenOnly ) { // Note that in listen only mode we will not inform us to the bus
        switch (N2kCANMsgBuf[MsgIndex].N2kMsg.PGN) {
          case 59392L: /*ISO Acknowledgement*/
            break;
          case 59904L: /*ISO Request*/
            HandleISORequest(N2kCANMsgBuf[MsgIndex].N2kMsg);
            break;
          case 60928L: /*ISO Address Claim*/
            HandleISOAddressClaim(N2kCANMsgBuf[MsgIndex].N2kMsg);
            break;
          case 65240L: /*Commanded Address*/
            HandleCommandedAddress(N2kCANMsgBuf[MsgIndex].N2kMsg);
            break;
#if !defined(N2K_NO_GROUP_FUNCTION_SUPPORT)
          case 126208L: /*NMEA Request/Command/Acknowledge group function*/
            HandleGroupFunction(N2kCANMsgBuf[MsgIndex].N2kMsg);
            break;
#endif
        }
      }
      result=true;
    }

  return result;
}
```

I am N2km_ListenAndNode, so make it past the short return.
Apparently the message buffer object knows if it is a SystemMessage.
I will start by assuming this presumed boolean member variable is correct.
Which means that one of the likely candidates for handling the message
will be called.  So I believe this is at the core of the automatic
handling.

However, I don't see, yet, if there is a connection between me
calling SendMessage(actisense_message) and that message making
it back into the devices bus.

So, once again, I cannot see the NMEA2000 forward bus

- on telnet because it only goes to the serial port at this time.
- because I am using actisense forwarding and
- relying on the App to send the Info Request


I *could* turn off actisense but then there would be no-one to
send the request.  So I am back in the lion's jaws.


### Timing Issues - Telnet

I *may* hook up a USBSerial Adapter to the ESP32's Serial2 port
because I am sure that using the ST7789 display and/or Telnet
introduce timing delays which prevent the system from working
correctly.

However, with existing Telnet I can *at least* see that I am
receiving the GetProductInfo request, although, at this time
I cannot see if it is "on the bus".

Another option might be to use UDP rather than TCP/IP packets,
but then I need to write UDP Perl app?



## tN2kDeviceList


(It's not a typedef - its a class wouild have been better as n2kDeviceList or even just N2DeviceList)

Do I need it?



## What do I do next

Do I:

- start playing with the DeviceList object?
- modify my program to have serial command input to send the request to the NMEA_Sensor?
- tee or otherwise get the NMEA bus to showup on telnet somehow?
- implement a 2nd debugging serial monitor to solve the previous two?
- keep looking at code trying to figure out what the author was thinking
  and how it is *supposed* to work.

My strongest idea is to tend towards a Serial2 debugging monitor
and by first implementing a Serial command processor to try to
query the existing NMEA_Sensor for its product information, then
to code-up and wire-up a USB Serial board and see what happens.

### Serial command processor

q = query product information PGN_REQUEST(59904,126996)

First attempt: no joy. Perhaps thats because the temperature sensor is not listening
Nope. It is set to N2km_ListenAndNode.

Do I need to specify it's destination (22) specifically?
Am I using the wrong device index in building th emessage?


Grrr ... need to try DEBUG_RXANY in the sensor ...

AFTER WHICH the sensor received a 126993 heartbeat from the monitor


### NMEA_Sensor got, and responded to PGN_REQUEST(59904,126996)!!

I had to set DEBUG_RXANY.

Sadly, and/or strangely, I had initially made a typo in my code
and the compiler accepted it:

```
    nmea2000.SendMsg(PGN_PRODUCT_INFO, 0);
    // should have been
    nmea2000.SendMsg(msg, 0);
```

I guess there is a signature for SendMsg that takes two integers.

### Now implement Serial2 debugging

and turn of the ST7789, WIFI, and Telnet. to further explore what
I get, and what I should do with it, when TO_ACTISENSE=1

I *could* implement a teensy program instead of using the FDTI module.




## GRRRRR Fragile (stupid) Actisense Reader Application

Yesterday, I'm sure, that when I had TO_ACTISENSE defined, and
the App was freshly started started, and it had heartbeats from
both the NMEA_Monitor and the NMEA_Sensor program, that if I
rebooted the NMEA_Sensor, causing it to send out an address claim,
that upon receiving that address claim, the App would then turn
around and request Product Information.

Today the App no longer requests Product Information.

Instead it requests Address Claims (something that the
AI specifically told me is a non-NMEA2000 standard way
of enumerating devices on the bus).

So now I am totally eff'd, because the whole point of all
of today's work was merely to figure out how to get the
Monitor to properly reply to an Actisense GetProductInfo
request, but the stupid app, for some reason I will probably
never know, no longer sends one.

I am about sick and tired of considering the Actisense Reader
Application to be my reference standard for trying to figure
this stuff out.

Sheesh.


In addition, since the monitor is putting it's own messages
on the buss, the stupid Address Claim request gets echoed
back to the App, when what I really wanted was for it to
get forwarded to the Sensor so that BOTH the Sensor and
Monitor would reply with the product information, and the
App would "look nice" automagically.

I am tempted to just use my BROADCAST_NMEA200_INFO define
in both programs and move on.


## Installed and ran NMEA-Simulator and OpenSkipper

**NMEA-Simulator** is all about SENDING fake NMEA information.
It has rudimentary listening capabilities, a good device list,
and perhaps most importantly, ENUMERATES THE DEVICES ON THE NET
in a way that makes sense to me (as opposed to ActisenseReader app).


**OpenSkipper** seems to be a (good) general purpose UI for
displaying NMEA information.
It works with actisense protocol over a serial port.
It has an interesting XML driven approach to displaying panels
(modeless dialogs) in an old-school windows application, but
as interesting, once I figured out to turn the "localhost only"
checkbox off which then caused Win10 to ask me if I wanted to
grant the app the ability to open a port on my machine, it
HAS A WEBSERVER which, I hope, makes use of the same xml definitions
as used by the native EXE file.

This would be interesting, if I learn about it and continue to like it
for my internet of things library myIOT.

But in any case, I am having a hard time because the message I picked
(130316 Environmental Temperatures) is not represented in the OpenSkipper
default UI and I don't want to delve into how you build the UI with
Visual Studio, and so I think I'm gonna change the sensor to transmit

- **127250:Vessel Heading** - in **PGN** Explorer as identifier **Heading**
  and the **Parameter** Explorer also as identifier **Heading**.
- **128250:Speed** - specifically the 1st param, the Speed Over Water
  which is represented in the OpenSkipper **PGN** Explorer as the *identifier*
  **Speed Water Referenced** which gets turned into the OpenSkipper
  **Parameter** explorer as **Boat_kts** via *hook[0]*
- **128267:Water Depth** as PGN Explorer **Depth** (under keel) and
  **Parameter** **Depth**

By using these, instead of the 130316 temperature PGN, I *should*
see the dials and values change in OpenSkipper.


### Continuing

I thought that the system would handle actisense requests to
the Monitor if I merely wrote all messages to the bus.
However, although the Sensor was talking to NMEA_Simulator
well (regarding ISO requests for Product Info, TransmitPGN
list, etc), nothing was coming from the Monitor.

99 is the address of the Monitor.
So I changed the Monitor's handleActisense() method to

- put the message on the buss if dest==255 or dest!=99
- explicitly respond to PGN requests if dest=255 or dest==99

Now the Monitor appears as a "better" device in NMEA-Simulator,
showing up automatically when I start it, with all Product
PGN lists, and other information visible.

However, although they show up in OpenSkipper, neither
device has product information. Starting OpenSkipper causes
the Monitor to reboot, prolly when it opens COM6, so it
shows up in the device list. I have to reboot the Sensor
for it to send an addtess claim and be noticed by OpenSkipper.

After modifying the Sensor to broadcastNMEA2000Info()
upon startup, the product info shows up in OpenSkipper,
although the text fields appear padded with unprintable
characters.

I'm really not much closer to understanding how this whole
system works.  I can get my head around the fact that
I have to explicitly handle actisense messages, and I like
the way the NMEA_Simluator automatically now shows "nice"
devices, WITHOUT me having to handle requests explicitly
in the Sensor.

Next I think I revisit parseMessages() and see how perhaps
I might *better* handle "self" actisense messages in the Monitor.


### No Good Solution

In the end, to get the Monitor to show up in any apps on
the laptop, I currently have to explicitly respond to
actisense messages to myself.  There is no way to "feed"
these into the CANBUS receive queue which is, probably for
very good reasons, hardwired to actual CANBUS messages.


### Next Steps 237

There are a couple of things that I still need to mess with.

- (a) Move the actisense stream to Serial2 so that I can keep Serial
  open as my own port.
- (b) Try using interrupts on the mpc2515
- (c) Mess around with the deviceList in prep for AGS devices
  i.e. the generator.
  
Fortunately, there do not seem to be any stray calls
to Serial.print() etc in the NMEA2000 library, so I
think (a) will work.


