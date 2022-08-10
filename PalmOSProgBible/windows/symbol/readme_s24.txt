Readme file for Symbol Palm Terminal Spectrum 24 (S24) Wireless Lan SDK 
components

CONTENTS:

1.0 Purpose of this release
2.0 Directory structure of the Symbol SDK
3.0 About the files in each subdirectory
4.0 Special notes for this release

======================================================================
1.0 Purpose of this release

This release of the SPT SDK is provided for use with the SPT-1740 
Terminals. It contains all the necessary shared libraries, header files and 
sample code to enable applications to be developed to communicate via an S24
network.

The driver necessary to support the Spectrum24(r) network is already included on 
the ROM image of your SPT-1740.  The primary interface to the Spectrum24(r) 
driver is via the PalmOS NetLib API functions.   NetLib is a wrapper around the 
IP stack and is used by SPT-1740 applications to establish connections, send and 
receive data.  The NetLib API functions are very similar to the Berkley Sockets 
API used by many Unix systems.  For a detailed description of NetLib refer the 
standard PalmOS programming documentation.  No additional SDK support is 
required to enable applications to communicate via Spectrum24(r).

The SPT-1740 ROM image also contains the Spectrum24(r) Driver Extensions shared 
library.  This shared library contains all of the public functions implemented 
in the low-level driver software. Using these functions, an application can 
control some of the SPT 1740's radio behavior and gather information about the 
SPT 1740's radio settings. However, applications are not required to call any of 
the functions in this library. By default, the SPT 1740 initializes and connects 
to the Spectrum24 network based on the preferences set in the SPT 1740 Network 
(Spectrum24) Preference Panel. These extension functions let the developer 
configure such items in the SPT 1740 radio as preferred access points and power 
usage algorithm. They also allow the developer to read items like the SPT 1740's 
driver version number, access point association table, and the SPT 1740's MAC 
layer access point connection association status (which is the SPT 1740's 
current radio connection status with an access point).

Details of the Spectrum24(r) Driver Extensions can be found in the "SPT-1740 
Spectrum24(r) Driver Extensions Library Developer's Guide" (S24API.PDF),  
included in the Document subdirectory of this SDK.  


======================================================================
2.0  Directory structure of the Symbol MSR SDK

Symbol SDK Support	        	 -  The main Symbol Symbol Palm Terminal SDK 
directory
Symbol SDK Support\Document  		 -  Documentation in PDF format
Symbol SDK Support\Include    		 -  S24 Driver Extensions header files
Symbol SDK Support\prc	      		 -  Shared Library executable 	
						
Symbol SDK Support\lib			 -  Linkable interface library for S24 
Driver extensions
Symbol SDK Support\Samples\S24Samples	 -  Sample S24 applications 


======================================================================
3.0 About the files in each sub directory

3.1 Symbol SDK Support\Document
The "SPT-1740 Spectrum24 Driver Extensions Library Developer's Guide " 
(S24API.pdf), provides
Documentation in PDF format for use when developing applications that interface 
with the S24 driver.


3.2 Symbol SDK Support\Samples\S24 Samples
Source code for the S24 Sample applications.  This sub-directory contains demos 
and source code for the Client, SPT-1740, and Host portions for three separate 
S24 sample programs.  For more details about these samples refer to the 
documentation contained in the spt1740_ReadMe.htm file.



======================================================================
4.0 Special notes for this release

4.1 Users should be able to generate the sample code for the demos
using Metrowerks CodeWarrior, all necessary project files have been
included.

======================================================================

