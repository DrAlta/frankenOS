                        CF Library Demo Readme

Introduction:

The CF Library Demo app is intended to provide a simple demonstration of
the use of the CF Library to access a CF ATA or I/O card, and as a simple
debug tool for developers intending to write non-FAT CF applications.
The program implements a simple text prompt and a series of low-level
commands for powering the card, modifying the slot interface, and listing
important addresses. The app is not particularly robust, and is meant to
serve primarily as an illustration of the use of the library.


Purpose of the CF Library:

The CF library is provided for developers wishing to access CF+ I/O cards.
It does not support any filesystem, and is not intended to be used in
conjunction with the Ffs Library. The CF library implements a very
simple abstraction of the CF hardware interface, so that developers do not
need to be worried about hardware details. The CF serial plug-in is an
example of an application/driver that uses the CF Library API.


Using the Demo App:

The complete CodeWarrior project for the CF Demo app is provided, along
the compiled PRC file. The app, CfDemo.prc, and the library itself,
CfLib.prc, should be installed on the TRGPro device. They may run in RAM or
copied to flash, if desired. The Demo app communicates with the user over
the HotSync port; it must be placed in a cable connected to a PC running
some kind of terminal program, such as ProComm or HyperTerm. The settings
are 57600 bps, N-8-1, no handshaking. When the Demo app runs, it will put
a form with a single "Done" button on the TRGPro, and produce a ">"
prompt over the serial port. Type "?" for a list of commands.


The Demo App source code:

The Demo app source code is supplied as a starting point for CF Library
application developers. The key files to examine are Starter.c, CfLib.h,
cmd.c, and notify.h.

        Starter.c -- loads, opens, and closes the CF library.

        CfLib.h -- header file for the Cf library. Lists all library calls,
                data structures, and constants.

        cmd.c -- implements the various CF commands. All the CF Library
                calls are made in this file.

        notify.h -- header file for CF-specific event notification.

The library and demo app use new data types that are not defined in the pre-
OS 3.3 header files. Compilation with old headers may require defining one
or more of the following types:

                UInt8  --> Byte
                Int8   --> char
                UInt16 --> Word
                Int16  --> Int
                UInt32 --> DWord
                Int32  --> long

In addition, the card insertion/removal notification requires OS 3.3 headers.
To compile with 3.0 headers, the notification specific calls and includes
must be commented out.


Documentation:

In addition to this Readme and the source code itself, the CF Library
SDK Reference is provided. It describes the use of the library and serves
as a reference to the library calls.


