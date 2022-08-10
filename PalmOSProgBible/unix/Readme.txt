The CD-ROM attached to this book contains a variety of useful tools, sample code, and documentation to help you develop applications for the Palm OS. Many of the tools included are programs that I use for my own Palm OS development work, and together, they form a formidable toolkit for Palm OS programming.

Besides PRC-Tools and various utilities that go with them, I have attempted to collect as wide a variety of alternative development tools as I can find. My hope is that the CD-ROM will serve not only as a toolkit but also as an opportunity to sample different development environments until you find one that suits your needs as a Palm OS developer.

A Word About Shareware
Many of the programs included on the CD-ROM are shareware. The shareware software distribution model allows you to install a program from a shareware collection CD-ROM, or download the program from the Internet, and use the application on a "try before you buy" basis. If you find the application useful, you are obligated to pay a registration fee to the author of the program.

There is a common misconception that shareware is inferior to commercial software. On the contrary, many shareware development tools are written by individuals or small companies who are highly motivated to provide quality programs and support for the development community. Quite often, shareware applications have better support than their commercial equivalents. Also, most shareware tends to be less expensive than commercial software, because the overhead spent on of fancy packaging and marketing is not required for an application that may be downloaded from somebody's Web site.

Shareware is not free, however. Somebody put a lot of hard work into creating a shareware application, and if you choose to use that program, its author deserves compensation for the effort required to write a good piece of software. The shareware applications included on this CD-ROM are not registered; as much as I appreciate your purchasing this book, buying the book does not register these applications. Each shareware application on the CD-ROM includes clear instructions about how to go about registering that program with its author. If you find that you cannot live without one of the shareware applications on this disk, I heartily recommend that you register it to encourage its author to continue developing and supporting useful software.

A Word About Free Software
Many of the tools on the CD-ROM are distributed as free software under the GNU General Public License (GNU GPL), which is included on the disk in the file GNU.txt. The GNU General Public License was developed by the Free Software Foundation as a means to promote the development and distribution of free software.
In this context, "free software" does not merely refer to software that you do not have to pay any money to use. Software that is merely given away may be thought of as "free, as in beer," whereas free software under the GNU GPL should be thought of as "free, as in speech." Free software is a concept that embodies what the user of the software is permitted to do with it. Specifically, the user of a free software program should have the following rights:

*	Freedom to run the program, for any purpose
*	Freedom to study how the program works, and adapt it to one's own needs
*	Freedom to redistribute copies of the software
*	Freedom to improve the program and release those improvements to the public, so that         the entire community benefits

The GNU GPL protects these rights through the concept of copyleft, which is a rule governing distribution of free software. Copyleft states that anyone who distributes free software, with or without changes, must pass along the freedom to copy and change the software. In this way, nobody can download a copy of the source for a free software project and convert it to proprietary software.

Because studying how software works and improving that software both require the software's source code, access to source code is a necessary condition for free software. Access to the source also means that many programmers can contribute to improving a program, adding their own improvements and releasing them to the public, comfortable in the knowledge that nobody will be able to steal their code for use in a proprietary application. As the GNU C++ compiler and GNU/Linux operating system projects have amply demonstrated, this style of "distributed development" allows for creation of very feature-rich and stable software.

Recently, there has been an "open source software" movement, based on the very successful model of allowing many programmers access to an application's source code, so they can all offer improvements to the program. As with free software, many very good pieces of software have come out of the open source movement. Unfortunately, the licensing schemes used by a number of open source projects place restrictions on the source code that interfere with the freedoms allowed by copylefted free software. The people at the Free Software Foundation prefer the term "free software" to "open source software", because the "free" in "free software" implies the freedoms and liberties associated with software protected by the GNU GPL.

The entire GNU PRC-Tools development toolkit, as well as other very useful tools included on the CD-ROM, is distributed under the GNU GPL. These programs are the work of dozens of talented programmers, working together to produce some of the finest Palm OS development software available. In many cases, the GNU tools are easier to use and more powerful than their commercial counterparts. Best of all, these tools are free software, so you can get under the hood and see what makes them tick, or modify them to suit your own development needs.

The Contents of the CD-ROM
Here are some highlights of what you will find on the CD-ROM:

*  Electronic version of Palm OS Programming Bible. The complete (and searchable) text of    this book in Adobe's Portable Document Format (PDF), readable with the Adobe Acrobat    Reader (also included).
*  Sample applications and source code. Examples from the text of this book, as well as    several complete sample applications and their source code.
*  SDKs. Software development kits for third-party Palm OS devices.
*  Development Tools. Many different tools for developing Palm OS software, including    complete development systems and helpful utilities.

Running the CD-ROM
The CD-ROM contains material for two different operating systems; at the root level of the CD-ROM are two folders: /unix and \windows. Each of these folders contains all the operating system-specific material for the appropriate OS, including the sample code from this book formatted properly for each operating system.

Sample applications and source code
Within both of the /unix and \windows directories is a samples directory. In the samples directory you will find sample applications and source code from Palm OS Programming Bible. Samples and sources are arranged in subdirectories of samples by chapter; for example, the directory \windows\samples\ch11 contains samples from Chapter 11, formatted for Windows. Librarian has its own directory, samples\librarian, as does Librarian's conduit application, located in samples\libconduit.

The CD-ROM programs
Many of the applications and tools in this section are described elsewhere in this book, but the list that follows contains brief descriptions of each of them, along with where you may find them on the CD-ROM. All of these programs are also available on the Web, and URLs for the appropriate Web sites are also listed. Because development tools tend to evolve much faster than I or the fine people at IDG Books Worldwide can cram them onto a CD-ROM, you should check the Web sites listed in this section for newer, improved versions of these tools.

Acrobat Reader (Adobe)
The electronic version of this book, as well as most of the SDK documentation from Palm and third-party Palm OS developers, is in Adobe Portable Document Format (PDF). Adobe's Acrobat Reader is a free PDF reader, available for Windows, Mac OS, and several flavors of Unix.
Acrobat Reader is in the /unix/acrobat or \windows\acrobat directories. It is also available on the Web at http://www.adobe.com/acrobat/.

Compact Applications Solution Language (CASL) demo
CASL is a cross-platform development system for creating Palm OS and Windows CE applications in the Windows environment. A runtime library installed on the handheld allows an application built by CASL to run the appropriate operating system. This demo version should give you a good idea of what CASL development is like.
CASL is in the \windows\casl directory. It is also available on the Web at http://www.caslsoft.com.

GNU PRC-Tools 2.0
A complete compiler chain for building Palm OS applications, the GNU PRC-Tools are free software under the GNU General Public License. The PRC-Tools are distributed as binaries for both Windows and GNU/Linux, and source is available for compilation on other Unix flavors. Best of all, the 2.0 version of the PRC-Tools is officially supported by Palm. You will also need PilRC (also included on the CD-ROM) to compile resources.
The PRC-Tools are in the /unix/prctools and \windows\prctools directories. The /unix/prctools directory also contains an src directory, where the source code is kept separate from the RPM binaries in the /unix/prctools directory. The GNU PRC-Tools are also available on the Web at http://www.palmos.com/dev/tech/tools/.

HotPaw Basic demo
Formerly cbasPad Pro, HotPaw Basic allows you to write and execute small Basic programs, directly on the handheld. HotPaw Basic is quite versatile, including features like forms creation and access to several popular handheld database formats, such as JFile Pro and HandDBase. The demo version limits you to running up to four programs. HotPaw Basic requires MathLib (also included on the CD-ROM) in order to run.
The HotPaw Basic demo is in the /unix/hotpaw and \windows\hotpaw directories. It is also available on the Web at http://www.hotpaw.com/rhn/hotpaw/.

Kyocera pdQ Software Developer's Kit
The Kyocera pdQ (formerly Qualcomm pdQ) SDK provides documentation and support for programming the special phone-related features of the pdQ smartphone. The SDK and sample applications are available in both zipped and StuffIt formats.
The Kyocera pdQ SDK is in the \windows\pdq directory. It is also available on the Web at http://www.kyocera-wireless.com/pdq/devzone.html.

LispME (Fred Bayer Informatics)
LispME is an onboard Scheme interpreter that runs entirely on the handheld. The author, Fred Bayer, provides the application and its source as free software under the GNU GPL.
LispME is in the /unix/lispme and \windows\lispme directories. It is also available on the Web at http://www.lispme.de/lispme/index.html.

Pendragon Forms 14-day Evaluation Version
Pendragon Forms allows rapid development of data collection applications for the Palm OS, which can synchronize with Microsoft Access and ODBC data sources on the desktop. This trial version gives you two weeks to evaluate Pendragon Forms on a Windows system with Microsoft Access 97 or later installed.
The Pendragon Forms trial is in \windows\pendragon. It is also available on the Web at http://www.pendragonsoftware.com/forms.html.

PocketC Compiler 3.5 (OrbWorks)
The PocketC system uses a slightly enhanced C syntax to allow you to write applets that run under the PocketC runtime. There are compilers for PocketC that run both on the handheld (allowing development on the handheld itself) and in a Windows environment (the PocketC Desktop Edition). The runtime module is free for anyone to download and use to run PocketC applets, and the compilers are shareware. Numerous PocketC applets are already in existence, and using them is a popular alternative to regular Palm OS development.

PocketC Compiler for Palm OS is in /unix/pocketc and \windows\pocketc. The Desktop Edition is in \windows\pocketc\desktop. Both versions are also available on the Web at http://www.orbworks.com.

Quartus Forth Evaluation Version
Quartus Forth is an onboard ISO/ANSI Standard Forth optimizing native-code compiler for the Palm OS, allowing you to create freestanding applications on the handheld itself. The evaluation version cannot compile stand-alone applications, requiring a runtime library to run, but all of the other features of Quartus Forth are available to try in the evaluation. Quartus Forth has an impressive array of features, some of which are hard to find in a good compiler running on a desktop system.
The Quartus Forth Evaluation Version is in /unix/qforth and \windows\qforth. It is also available on the Web at http://www.quartus.net.

Satellite Forms Trial (Puma Technology)
Satellite Forms is a very easy-to-use forms-based rapid application development system for creating Palm OS programs. The program features easy "drag-and-drop" forms creation on Windows, along with a Visual Basic-like syntax for adding smarts to the forms composing an application. Satellite Forms comes in both Standard and Enterprise versions, offering a good set of features for small or large projects. The trial version allows you to compile and load more than 20 sample applications, which should give you a good idea how well Satellite Forms will work for regular development.
The Satellite Forms trial is in \windows\satforms. It is also available on the Web at http://www.pumatech.com/satforms_fam.html.

TRGPro Development Kit
The TRGPro development kit contains documentation, header files, and sample applications for making use of the extra features available on the TRGPro handheld, including its Compact Flash slot and enhanced sound capabilities. TRG recommends and supports Metrowerks' CodeWarrior for Palm Computing Platform for development of TRGPro-specific features in a Palm OS application, and the sample applications in the development kit are CodeWarrior projects, but the libraries for these added functions are distributed as Palm OS shared libraries, so it should be possible (with a little work) to develop for TRGPro extensions using the GNU PRC-Tools.
The TRGPro Development Kit is in /unix/trgpro and \windows\trgpro. It is also available on the Web at http://www.trgpro.com/developer/developer.html.

Waba SDK (Wabasoft)
The Waba Virtual Machine (WabaVM) is a virtual machine for the Palm OS and Windows CE. Wabasoft provides a complete SDK, including documentation and tools for programming in "Waba," a Java-like programming language. Because of the way the Waba was designed, you can use Java development tools to write Waba applications, which will also run as Java applications or applets. The Java Class Libraries will not run on the WabaVM, though. The WabaVM is an interesting idea because it allows platform-independent development of applications for different handheld operating systems. Best of all, Waba is free software under the GNU GPL, available for Windows, GNU/Linux, and Solaris.
The Waba SDK is in /unix/waba and \windows\waba. It is also available on the Web at http://www.wabasoft.com.

WinZip evaluation version (Nico Mak Computing)
WinZip is a popular archive creation and extraction tool for the Windows operating system, allowing easy manipulation of compressed zip archives. Many of the applications and tools on the CD-ROM are distributed as compressed zip archives. WinZip is shareware; the evaluation version is not crippled in any way, but it is such a mind-bogglingly useful tool that you will probably want to register it, anyhow.
WinZip is in \windows\winzip. It is also available on the Web at http://www.winzip.com.
