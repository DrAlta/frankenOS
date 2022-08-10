Readme file for Symbol Palm Terminal Printing SDK

CONTENTS:

1.0 Purpose of this release
2.0 Directory structure of the Symbol Scanner SDK
3.0 About the files in each subdirectory
4.0 Special notes for this release
5.0 Testing with the sample code

======================================================================
1.0 Purpose of this release

This release of the Symbol Printer SDK is provided for use with 
version 3.0 of the Palm OS. It contains files and sample application
code to print using the SPT or Palm III terminal.  The files contained
in this SDK release are subject to change.

======================================================================
2.0  Directory structure of the Symbol Printer SDK

Symbol SDK Support 		-  The main Symbol ScanManager directory
Symbol SDK Support\Document	-  Documentation in PDF format
Symbol SDK Support\Lib		-  Printer library file 
Symbol SDK Support\Include	-  Printer header files
Symbol SDK Support\prc		-  Supporting prc and pdb files
Symbol SDK Support\Sample	-  Source for the printer application

======================================================================
3.0 About the printer files in each sub directory

3.1 Symbol SDK Support\Document
The "Printer Software Manual for the Palm Computing Platform" provides
documentation in PDF format for use when developing applications 
containing printing for the Symbol Palm Terminal (SPT) or Palm III.  

3.2 Symbol SDK Support\Lib
PtStatic.lib is the shared library that must be linked with the 
printing application. 

3.3 Symbol SDK Support\prc
 
Two pdb files are used for printing. These two files, Printcap.pdb and 
ptDynLib.pdb, must be HotSynced onto the SPT, along with 
the user application or PrintSample.prc (See sample subdirectory).

If the file, printcap.pdb, does not contain an entry for your printer,
contact Symbol or the printer manufacturer for updates to the printcap
file.  Updates will also be posted on the Symbol website:

	www.symbol.com/palm

3.4 Symbol SDK Support\Sample
Source code is provided for the printer sample application. The source 
for PrintSample is provided as a example of how to use the
printer API.  It is supplied as a prototype for developers, so that 
they can easily create applications that require printing using the 
Metrowerks Codewarrior IDE.  It is expected that portions of this sample 
code will be used in developer applications.  See Chapter 3 of the manual
for a description of sample code and the sample application.

======================================================================
4.0 Special notes for this release

4.1 Users should be able to generate the sample code for the demo
using Metrowerks CodeWarrior.

======================================================================
5.0 Testing with the sample code

If the developer wants to test with the sample code that is provided, 
here's the installation procedure:

	1. HotSync the following 3 files onto the unit:
			
			PrintSample.prc - sample application  
			ptDynLib.pdb	- printer dynamic library
			printcap.pdb	- printcap data file that
					  contains descriptions of
					  each supported printer

	2. Tap on the ptSample icon
     	3. Select a Transport and a Printer
	4. Then select the Command from the lower left hand corner.  
	   (See chapter 3 for full details.) For example:

	     	a. Connect to the printer with a serial cable, if the 
		   transport selected is serial.  For Ir, just align 
		   the unit with the Ir port of the printer.
		b. Open the library
		c. Select Form
		d. Select Data to print
		e. Close the library

