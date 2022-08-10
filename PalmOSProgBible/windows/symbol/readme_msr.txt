Readme file for Symbol Palm Terminal Magnetic Stripe Reader (MSR) SDK components

CONTENTS:

1.0 Purpose of this release
2.0 Directory structure of the Symbol Scanner SDK
3.0 About the files in each subdirectory
4.0 Special notes for this release

======================================================================
1.0 Purpose of this release

This release of the Symbol MSR SDK is provided for use with the SPT-17xx 
Terminal equipped the MSR 3000 Magnetic Stripe Reader attachment.
It contains all the necessary shared libraries, header files and sample 
code to enable applications to be developed which interface with the 
MSR 3000.

======================================================================
2.0  Directory structure of the Symbol MSR SDK

Symbol SDK Support	        -  The main Symbol Symbol Palm Terminal SDK directory
Symbol SDK Support\Document  	-  Documentation in PDF format
Symbol SDK Support\Include    	-  MSR header files
Symbol SDK Support\prc	      	-  MSR Manager Shared Library executable 							
Symbol SDK Support\Samples\	    -  Source code for sample MSR applications 
Symbol SDK Support\Host Tools	-  Development Tools for use on the  Host development System

======================================================================
3.0 About the MSR files in each sub directory

3.1 Symbol SDK Support\Document
The "MSR 3000 System Software Manual" (msrssm.pdf), provides
Documentation in PDF format for use when developing applications 
that interface with the MSR 3000. 

3.2 Symbol SDK Support\prc
Contains the MSR Manager Shared Library (MsrMgrLib.prc)

3.3 Symbol SDK Support\Samples\MSR Sample
Source code for the MSR Sample application, which is a very simple application that
illustrates the basic method of utilizing the MSR API functions.
This source code is explained in detail in the MSR System Software Manual found
in the Document subdirectory.

3.4 Symbol SDK Support\Samples\MSR Demo	
Source code for MSR Demo application, which illustrates all of the MSR API 
functions.  This application is a good reference for developers needing to take
advantage of any of the advanced features of the MSR 3000.   

3.5 Symbol SDK Support\Host Tools\MSR Configurator
Installation files for the MSR Configurator tool.  The MSR configurator is a 
Development tool used to assist developers in configuring the advanced
features of the MSR 3000.  For more information of the operation of the MSR
Configurator refer to the MSR 3000 System Software Manual.

To install the MSR configurator Tool execute the Setup.exe program found
in this sub-directory and follow the installation prompts.

======================================================================
4.0 Special notes for this release

4.1 Users should be able to generate the sample code for the demos
using Metrowerks CodeWarrior, all necesary project files have been
included.

======================================================================

