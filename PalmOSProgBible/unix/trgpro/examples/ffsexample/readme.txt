                        Ffs Library Demo Readme

Introduction:

The Ffs Library Demo app is intended to provide a simple demonstration of
the use of the Ffs Library to access a FAT-formatted CF storage card.
The program implements a simple text prompt and a series of MS-DOS type
commands for file listing, copying, deleting, etc. The app is not
particularly robust, and the commands are quite simple compared to their
DOS counterparts. It is meant to serve merely as an illustration of the
use of the library.


Using the Demo App:

The complete CodeWarrior project for the Ffs Demo app is provided, along
the compiled PRC file. The app, FfsDemo.prc, and the library itself,
FfsLib.prc, should be installed on the TRGPro device. They may run in RAM or
copied to flash, if desired. The Demo app communicates with the user over
the HotSync port; it must be placed in a cable connected to a PC running
some kind of terminal program, such as ProComm or HyperTerm. The settings
are 57600 bps, N-8-1, no handshaking. When the Demo app runs, it will put
a form with a single "Done" button on the TRGPro, and produce an "A:\>"
prompt over the serial port. Type "?" for a list of commands.


The Demo App source code:

The Demo app source code is supplied as a starting point for Ffs Library
application developers. The key files to examine are Starter.c, FfsLib.h,
cmd.c, and notify.h.

        Starter.c -- loads, opens, and closes the Ffs library.

        FfsLib.h -- header file for the Ffs library. Lists all library calls,
                data structures, and constants.

        cmd.c -- implements the various FAT commands. All the Ffs Library
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

In addition to this Readme and the source code itself, the Ffs Library
SDK Reference is provided. It describes the use of the library and serves
as a reference to the library calls.


