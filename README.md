# Tracerclient
 Read Data from Epever Tracer Solar Charge Controller and send to Tracerserver and also send to a Display MT50 if connected.

## Getting Started
Needed is a RS485 Dongle for the Solar charge controller and optional one for the Display MT50.

## Features
Tested with Tracer BN 3215 but probably works with other Tracer that supports the Display MT50 (Modbus protocol) also, but I can't test it.
The Display use a undocumented Function (0x43) and I can only store the whole Header info (don't know what it mean)
This Program is split into a Client part (tracerclient) and a Server part (tracerserver).
The Client is connected to the Tracer Solarchargecontroller and receives all the Registers like the Display MT50,
and forward it to the Server.
Optional The Display MT50 can be connected to the Client or Server or both. The Display can be far away from the controller when connected to the Server.
Client and Server can be controlled via a TCP Connection to read out Registers Override Registers and set user defined Values.
I replace some Values with the Victron BMV to show real Values like SOC, Battery Current, ... on the Display.

### Installing
To Compile Client and server

```
make all
```

Included is also a displaytest and tracertest to test communication.
Modify tracertest.c:

```
#define TRACERDEV "/dev/ttyXRUSB0"
```

displaytest.c:

```
#define DISPLAYDEV "/dev/ttyUSB0"
```

and compile:

```
make tracertest
```

```
make displaytest
```

### Usage
To Get Usage Info:

```
./tracerclient --help
```

```
./tracerserver --help
```

In the script directory there are some scripts I use to Control the Client/Server.

To Control Client/server via TCP use a Program like nc:

```
nc [IP] [port]
help -> Print Usage
```

The DeviceID of the Tracer is hardcoded to 1, and the tracerclient/server also react only to DeviceID 1 from the Display.
That why it could take a moment for the Display to connect, because the Display try every deviceid from 1 to 200 when connected to power.

The Client sends Data to the Server in Intervals defined in tracerclient.c:

```
//Define Timeouts
#define TRACER_STATISTIC_TIMEOUT 600 //Timeout to receive statistic Data from Charge-controller
#define TRACER_REALTIME_TIMEOUT 0    //Timeout to receive real-time Data from Charge-controller
#define SERVER_STATISTIC_TIMEOUT 600 //Timeout to send statistic Data to Server
#define SERVER_REALTIME_TIMEOUT 60   //Timeout to send real-time Data to Server
```


## License

This project is licensed under the GNU GENERAL PUBLIC LICENSE - see the LICENSE file for details
Included in this Project is the threadwrapper licensed under the CPOL - see the LICENSE.htm and source files in the threadwrapper directory for details about the LICENSE and the Author.