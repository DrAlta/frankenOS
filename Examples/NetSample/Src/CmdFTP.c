/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CmdFTP.c
 *
 * Description:
 * This module provides the "ftp" command to the NetSample application. 
 * This is a minimalistic FTP implementation that supports ftp'ing of a 
 * Pilot database (either resource or record) to and from an FTP server. 
 *
 * WARNING: This module does NOT support transferring ANY OTHER TYPES OF FILES
 *  other than Pilot databases. 
 *
 * ANOTHER WARNING!! This module is not guaranteed to be compatible in the
 *  future if the format of Pilot Databases changes!!! Notice that it includes
 *  <DataPrv.h> which is a NON_PORTABLE header file. Future versions of PalmOS
 *  might change the definitions in this header. 
 *
 * The supported commands are:
 *  get, put, dir, cd, pwd
 *
 *****************************************************************************/

// Socket Equates
#include <sys_socket.h>

// WARNING!!! This module is NOT GUARANTEED to be compatible with future
//  versions of the PalmOS because it uses definitions in the non-portable
//  header file <DataPrv.h>
//#define	NON_PORTABLE
/*#include <DataPrv.h>
 * This include is a problem with Palm OS 3.5, but we can access the same data
 * structures and routines through the DataMgr api.
 */

// Palm headers
#include <PalmOS.h>

// StdIO header
#include "AppStdIO.h"

// Application headers
#include "NetSample.h"

// Constants
#define	kReplyTimeout			AppNetTimeout



/***********************************************************************
 * Globals
 *
 ***********************************************************************/
static Boolean	FTPQuit = false;
static int		FTPCtlSock = -1;

// This is a global used for forming FTP commands. We only make this
//  a global so that we can cut down on stack space usage.
static Char		FTPCommand[32];


/***********************************************************************
 *
 * FUNCTION:   CmdDivider()
 *
 * DESCRIPTION: This command exists solely to print out a dividing line
 *		when the user does a help all.
 *
 *	CALLED BY:	 
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdDivider(int argc, char * argv[])
{
	// Check for short help
	if (argc > 1 && !StrCompare(argv[1], "?")) 
		printf("\n-- FTP --\n");
}

/***********************************************************************
 *
 * FUNCTION:    PrvDmReadN
 *
 * DESCRIPTION: High level convenience function that reads N bytes from
 *		a socket into a database record
 *	
 *		This code was modified from the readn() routine from Richard Stevens' 
 *		book 'Unix Network  Programming'. 
 *
 * CALLED BY:	 Applications.
 *
 * PARAMETERS:  fd			- socket refnum
 *					 ptr			- buffer pointer
 *					 nbytes		- number of bytes to read
 *
 * RETURNED:    # of bytes read, or <0 if error
 *
 ***********************************************************************/
static Int32		PrvDmReadN(NetSocketRef sock, UInt8 * recP, UInt32 offset, UInt32 nbytes)
{
	UInt32	nleft;
	Int16 	nread;
	UInt16	chunk;

	nleft = nbytes;
	while (nleft > 0) {
		if (nleft > 0x7000) chunk = 0x7000;
		else chunk = nleft;
		
		nread = NetLibDmReceive(AppNetRefnum, sock, recP, offset, chunk, 
						0, 0, 0, AppNetTimeout, &errno);

		if (nread < 0)
			return(nread);		/* error, return < 0 */
		else if (nread == 0)
			break;			/* EOF */

		nleft -= nread;
		offset += nread;
	}
	return(nbytes - nleft);		/* return >= 0 */
}

/***********************************************************************
 *
 * FUNCTION:   FTPDatabaseRead()
 *
 * DESCRIPTION: This routine reads a .PRC from the FTP server and 
 *		installs into the Pilot as a database.
 *
 * PARAMETERS:
 *		cardNo   - card number of database
 *		nameP		- name of database to send
 *		sock		- socket refNum of socket to send to
 *
 *	CALLED BY: FTPPut 
 *
 * RETURNED:  0 if no err
 *
 * HISTORY:	
 *				BLT	10/26/99		Removed this modules dependency on DataPrv.
 *										The comments
 *
 ***********************************************************************/
static Err FTPDatabaseRead(UInt16 cardNo, char * nameP, int sock)
{
	char*					bufferP = 0;

	DmOpenRef			dbP = 0;
/*	DatabaseHdrType	hdr;	*/
	LocalID				hdr;		// We'll use the chunk ID of the db
	LocalID				dbID=0;

	Int16					i;
	char					typeStr[8];
	char					crStr[8];
	Err					err;
	Boolean				preserveDates = true;

	RsrcEntryType		resEntry, nextResEntry;
	UInt32				resSize;
	MemHandle			newResH;
	
	UInt8 *				dstP;

	MemHandle			newRecH;
/*	RecordEntryType	recEntry, nextRecEntry; */
	LocalID				recEntry, nextRecEntry;
	UInt32				recSize;

	LocalID				appInfoID;
	LocalID				sortInfoID;
	LocalID				recordID;

	UInt16				index = 0xFFFF;
	UInt16				attr;
	UInt32				id;

	UInt8 *				bufP;
	const int			bufSize = 0x400;
	Int16					bytes;
	UInt32				offset = 0;
	
	UInt32				totalBytes = 0;
	UInt32				ticks;
	UInt16				filler;

/* Use these to get db info instead of directly accessing DatabaseHdrType. */
	UInt16 * 			attributesP;
	UInt16 * 			versionP;
	UInt32 * 			crDateP;
	UInt32 *				modDateP;
	UInt32 * 			bckUpDateP;
	UInt32 *				modNumP;
	LocalID * 			appInfoIDP;
	LocalID * 			sortInfoIDP;
	UInt32 * 			typeP;
	UInt32 * 			creatorP;				

/* Get the record info from the db header. */
	DmOpenRef 			dbR;
//	UInt16 				index;
	UInt16 * 			attrP;
	UInt32 * 			uniqueIDP;
	LocalID* 			chunkIDP;
	
	dbID = DmFindDatabase(cardNo, nameP);
	nextRecEntry = DmFindDatabase(cardNo, nameP);

/* This call will indirectly get the DatabaseHdr info. */
	DmDatabaseInfo(cardNo, dbID, c, attributesP, versionP, crDateP,
					modDateP, bckUpDateP, modNumP, appInfoIDP,
					sortInfoIDP, typeP, creatorP);
				
	hdr = DmFindDatabase(cardNo, nameP);

	DmRecordInfo(hdr, index, attrP, uniqueIDP, chunkIDP);
	
	//------------------------------------------------------------
	// Create temp buffer
	//------------------------------------------------------------
	bufP = MemPtrNew(bufSize);
	if (!bufP) {err = memErrNotEnoughSpace; goto Exit;}
				
	//------------------------------------------------------------
	// Read in the header
	//------------------------------------------------------------
	ticks = TimGetTicks();
	DmDatabaseSize(cardNo, dbID, 0, &recSize, 0);	// get the total size, including overhead.
/*	recSize = sizeof(hdr) - sizeof(hdr.recordList.firstEntry); */
	if (NetUReadN(sock, (UInt8 *)&hdr, recSize)  != recSize)  goto ExitBadDatabase;
//	if (NetUReadN(sock, (UInt8 *)&hdr, recSize)  != recSize)  goto ExitBadDatabase;
	totalBytes += recSize;

	//------------------------------------------------------------
	// Create The database 
	//------------------------------------------------------------
	MemMove(typeStr, &typeP, 4);
/*	MemMove(typeStr, &hdr.type, 4); */
	typeStr[4] = 0;
	MemMove(crStr, &creatorP, 4);
/*	MemMove(crStr, &hdr.creator, 4); */
	crStr[4] = 0;
	
	printf("\nCreating Database on card %d\n name: %s\n type %s, creator %s\n",
			cardNo, nameP, typeStr, crStr);
/*			cardNo, hdr.name, typeStr, crStr); */

	// Delete the old one, if it exists
//	dbID = DmFindDatabase(cardNo, nameP);
/*	dbID = DmFindDatabase(cardNo, (Char *)hdr.name); */
	if (dbID) 
		DmDeleteDatabase(cardNo, dbID);
	err = DmCreateDatabase(cardNo, nameP, creatorP, typeP, 
				attributesP & dmHdrAttrResDB);
/*	err = DmCreateDatabase(cardNo, (Char *)hdr.name, hdr.creator, hdr.type, 
 *				hdr.attributes & dmHdrAttrResDB);
 */
 
	if (err) goto Exit;



	//------------------------------------------------------------
	// Set the database info 
	//------------------------------------------------------------
	dbID = DmFindDatabase(cardNo, nameP);
/*	dbID = DmFindDatabase(cardNo, (char*)hdr.name); */
	if (preserveDates) {
		err = DmSetDatabaseInfo(cardNo, dbID, 0,
					&attributesP, &versionP, &crDateP,
					&modDateP, &bckUpDateP,
					&modNumP, 0,
					0, &typeP,
					&creatorP);
		}
	else {
		err = DmSetDatabaseInfo(cardNo, dbID, 0,
					&attributesP, &versionP, 0,
					0, 0,
					&modNumP, 0,
					0, &typeP,
					&creatorP);
		}
	if (err) goto Exit;



	//------------------------------------------------------------
	// If it's a resource database, Add the Resources
	//------------------------------------------------------------
	if (attributesP & dmHdrAttrResDB) {
/*	if (hdr.attributes & dmHdrAttrResDB) { */
		dbP = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
		if (!dbP) {err = DmGetLastErr(); goto Exit;}
		
		// Get the "next" resource entry
		if (DmNumRecords(&hdr)) {
/*		if (hdr.recordList.numRecords) { */
			if (NetUReadN(sock, (UInt8 *)&nextResEntry, sizeof(nextResEntry)) != sizeof(nextResEntry))
				goto ExitBadDatabase;
			totalBytes += sizeof(nextResEntry);
			}
				

		// Loop through each of the entries and add them to the database
		for (i=0; i<DmNumRecords(&hdr); i++) {
/*		for (i=0; i<hdr.recordList.numRecords; i++) { */
			resEntry = nextResEntry;
		
			// Get the resource entry
			if (i < DmNumRecords(&hdr)-1) {
/*			if (i < hdr.recordList.numRecords-1) { */
				if (NetUReadN(sock, (UInt8 *)&nextResEntry, sizeof(nextResEntry)) != sizeof(nextResEntry))
					goto ExitBadDatabase;
				totalBytes += sizeof(nextResEntry);
				}
				
			// Calculate the size of the resource
			if (i < DmNumRecords(&hdr) - 1) 
/*			if (i < hdr.recordList.numRecords - 1) */
				resSize = (UInt32)nextResEntry.localChunkID - (UInt32)resEntry.localChunkID;
			else
				resSize = 1;										// This gets fixed up later.
			
			// Allocate space for the new resource
			newResH = (MemHandle)DmNewResource(dbP, resEntry.type, resEntry.id, resSize);
			if (!newResH) {err = DmGetLastErr(); goto Exit;}
	
			// Done with it for now
			DmReleaseResource(newResH);
			}
			
			
			
		// Read the filler byte which is in .PRC files due to an historical "oversight"
		NetUReadN(sock, (UInt8 *)&filler, sizeof(filler));
		totalBytes += sizeof(filler);
			
		// Now, go back and write the data into each resource
		for (i=0; i<c; i++) {
/*		for (i=0; i<hdr.recordList.numRecords; i++) { */
			
			// Print progress
			StdPutS(".");
				
			// Get the new resource handle
			newResH = (MemHandle)DmGetResourceIndex(dbP, i);
			if (!newResH) {err = DmGetLastErr(); goto Exit;}
			
			// Write the data to it
			if (i < DmNumRecords(&hdr) - 1) {
/*			if (i < hdr.recordList.numRecords - 1) { */
				dstP = (UInt8 *)MemHandleLock(newResH);
				resSize = MemHandleSize(newResH);
				if (PrvDmReadN(sock, dstP, 0, resSize) != resSize)
					goto ExitBadDatabase;
				totalBytes += resSize;
				MemHandleUnlock(newResH);
				}
				
				
			// The last one is tricky because we don't know the file size, hence the resource
			//   size. We'll read in 1K chunks until there ain't no more.
			else {
				resSize = offset = 0;
				while (1) {
					bytes = NetUReadN(sock, bufP, bufSize);
					if (bytes <= 0) break;
					totalBytes += bytes;

					resSize += bytes;
					newResH = DmResizeResource(newResH, resSize);
					if (!newResH) {err = DmGetLastErr(); break;}
					
					dstP = (UInt8 *)MemHandleLock(newResH);
					DmWrite(dstP, offset, bufP, bytes);
					offset += bytes;
					MemHandleUnlock(newResH);
					}
				if (bytes < 0) {err = errno; goto Exit;}
				}
	
			// Done with it for now
			DmReleaseResource(newResH);
			}
		}
		
		
	//------------------------------------------------------------
	// If it's a Record database, Add the Records
	//------------------------------------------------------------
	else {
		dbP = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
		if (!dbP) {err = DmGetLastErr(); goto Exit;}
		

		// This is 0 till we read it in.
		nextRecEntry.localChunkID = 0;
		
		
		// If there's an appInfo, add that. NOTE: This logic assumes there is at least
		//  1 record in the database because it calculates the size of the appInfo from
		//  the offset of the first record. 
		if (appInfoIDP) {
/*		if (hdr.appInfoID) { */
			if (!DmNumRecords(&hdr)) 
/*			if (!hdr.recordList.numRecords) */
				goto ExitBadDatabase;
				
			if (sortInfoIDP) 
/*			if (hdr.sortInfoID) */
				recSize = (UInt32)sortInfoIDP - (UInt32)appInfoIDP;
			else {
				if (NetUReadN(sock, (UInt8 *)&nextRecEntry, sizeof(nextRecEntry)) != 
										sizeof(nextRecEntry))
					goto ExitBadDatabase;
				totalBytes += sizeof(nextRecEntry);
				recSize = (UInt32)chunkIDP - (UInt32)appInfoIDP;
/*				recSize = (UInt32)nextRecEntry.localChunkID - (UInt32)hdr.appInfoID; */
				}
			
			// Allocate the chunk
			newRecH = (MemHandle)DmNewHandle(dbP, recSize);
			if (!newRecH) {err = DmGetLastErr(); goto Exit;}
				
			// Store it in the header
			appInfoID = MemHandleToLocalID(newRecH);
			DmSetDatabaseInfo(cardNo, dbID, 0,
				0, 0, 0,
				0, 0,
				0, &appInfoID,
				0, 0, 
				0);
			}
			
			
			
		// If there's a sortInfo, add that. NOTE: This logic assumes there is at least
		//  1 record in the database because it calculates the size of the appInfo from
		//  the offset of the first record. 
		if (sortInfoIDP) {
			if (!DmNumRecords(&hdr)) 
				goto ExitBadDatabase;
				
			if (NetUReadN(sock, (UInt8 *)&nextRecEntry, sizeof(nextRecEntry)) 
							!= sizeof(nextRecEntry))
				goto ExitBadDatabase;
			totalBytes += sizeof(nextRecEntry);
			recSize = (UInt32)chunkIDP - (UInt32)sortInfoIDP;
/*			recSize = (UInt32)nextRecEntry.localChunkID - (UInt32)hdr.sortInfoID; */
			
			// Allocate the chunk
			newRecH = (MemHandle)DmNewHandle(dbP, recSize);
			if (!newRecH) {err = DmGetLastErr(); goto Exit;}
				
			// Store it in the header
			sortInfoID = MemHandleToLocalID(newRecH);
			DmSetDatabaseInfo(cardNo, dbID, 0,
				0, 0, 0,
				0, 0,
				0, 0,
				&sortInfoID, 0, 
				0);
			}
			
		
		
		// Add Each of the records.
		if (!chunkIDP && DmNumRecords(&hdr)) {
/*		if (!nextRecEntry.localChunkID && hdr.recordList.numRecords) { */
			if (NetUReadN(sock, (UInt8 *)&nextRecEntry, sizeof(nextRecEntry)) != sizeof(nextRecEntry))
				goto ExitBadDatabase;
			totalBytes += sizeof(nextRecEntry);
			}

			
		for (i=0; i<DmNumRecords(&hdr); i++) {
/*		for (i=0; i<hdr.recordList.numRecords; i++) { */
			index = i;
			
			// Read in the next entry
			recEntry = nextRecEntry;
			if (i < DmNumRecords(&hdr)-1) {
/*			if (i < hdr.recordList.numRecords-1) { */
				if (NetUReadN(sock, (UInt8 *)&nextRecEntry, sizeof(nextRecEntry)) != sizeof(nextRecEntry))
					goto ExitBadDatabase;
				totalBytes += sizeof(nextRecEntry);
				}
			
			
			// Calculate the size of the record
			if (i < DmNumRecords(&hdr) - 1) 
				recSize = (UInt32)nextRecEntry.localChunkID - (UInt32)recEntry.localChunkID;
/*			if (i < hdr.recordList.numRecords - 1) 
 *				recSize = (UInt32)nextRecEntry.localChunkID - (UInt32)recEntry.localChunkID;
 */
			else
				recSize = 1;							// will get fixed later...
			
			// If the record is 0 size, just attach a nil handle
			if (!recSize) {
				DmAttachRecord(dbP, &index, 0, 0);
				}
			
			// Allocate the new record if it has non-zero size
			else {
				newRecH = (MemHandle)DmNewRecord(dbP, &index, recSize);
				if (!newRecH) {err = DmGetLastErr(); goto Exit;}
				}
					
			// Set the attributes
			attr = recEntry.attributes;
			id = recEntry.uniqueID[0];
			id = (id << 8) | recEntry.uniqueID[1];
			id = (id << 8) | recEntry.uniqueID[2];
			DmSetRecordInfo(dbP, index, &attr, &id);
	
			// Done with it
			DmReleaseRecord(dbP, index, false);		// vmk 10/17/95 preserve dirty flag
			}
			
			
		// Read the filler byte which is in .PRC files due to an historical "oversight"
		NetUReadN(sock, (UInt8 *)&filler, sizeof(filler));
		totalBytes += sizeof(filler);
			
		//--------------------------------------------------------------------------------
		// Now, go back and put in the contents of each record
		//--------------------------------------------------------------------------------
		// First the app info
		if (hdr.appInfoID) {
			newRecH = MemLocalIDToGlobal(appInfoID, cardNo);
			dstP = MemHandleLock(newRecH);
			recSize = MemHandleSize(newRecH);
			if (PrvDmReadN(sock, dstP, 0, recSize) != recSize)
			MemHandleUnlock(newRecH);
			totalBytes += recSize;
			}
		
		// Next the sort info
		if (hdr.sortInfoID) {
			newRecH = MemLocalIDToGlobal(sortInfoID, cardNo);
			dstP = MemHandleLock(newRecH);
			recSize = MemHandleSize(newRecH);
			if (PrvDmReadN(sock, dstP, 0, recSize) != recSize)
			MemHandleUnlock(newRecH);
			totalBytes += recSize;
			}
		
		// Next each of the records
		for (i=0; i<hdr.recordList.numRecords; i++) {
			index = i;
			
			// Get the record info
			if ((i & 0x0F) == 0) StdPutS(".");

			
			// If the record is marked as deleted, temporarily un-delete it so
			//  that we can get the handle to it's data, if any
			DmRecordInfo(dbP, index, &attr, 0, &recordID);
			if ((attr & dmRecAttrDelete) && recordID) {
				attr &= ~dmRecAttrDelete;
				DmSetRecordInfo(dbP, index, &attr, 0);
				newRecH = (MemHandle)DmQueryRecord(dbP, index);
				attr |= dmRecAttrDelete;
				DmSetRecordInfo(dbP, index, &attr, 0);
				}
			else
				newRecH = (MemHandle)DmQueryRecord(dbP, index);

			if (!newRecH) continue;
			
			
			// Read it in
			if (i < hdr.recordList.numRecords-1) {
				recSize = MemHandleSize(newRecH);
				dstP = MemHandleLock(newRecH);
				if (PrvDmReadN(sock, dstP, 0, recSize) != recSize)
					goto ExitBadDatabase;
				totalBytes += recSize;
				MemHandleUnlock(newRecH);
				}
				
			// The last one is tricky because we don't know the file size, hence the record size.
			else {
				recSize = offset = 0;
				while (1) {
					bytes = NetUReadN(sock, bufP, bufSize);
					if (bytes <= 0) break;
					totalBytes += bytes;

					recSize += bytes;
					newRecH = DmResizeRecord(dbP, index, recSize);
					if (!newRecH) {err = DmGetLastErr(); break;}
					
					dstP = (UInt8 *)MemHandleLock(newRecH);
					DmWrite(dstP, offset, bufP, bytes);
					offset += bytes;
					MemHandleUnlock(newRecH);
					}
				if (bytes < 0) {err = errno; goto Exit;}
				}
			
			}
			
		}
		
		
	goto Exit;
	
ExitBadDatabase:
	printf("\nError: invalid format Pilot database\n");

	
Exit:
	ticks = TimGetTicks() - ticks;
	if (dbP) DmCloseDatabase(dbP);

	if (!err && preserveDates) {
		DmSetDatabaseInfo(cardNo, dbID, 0,
					&hdr.attributes, &hdr.version, &hdr.creationDate,
					&hdr.modificationDate, &hdr.lastBackupDate,
					&hdr.modificationNumber, 0,
					0, &hdr.type,
					&hdr.creator);
		}

	if (bufP) MemPtrFree(bufP);
	if (err) {
		printf("Get error: %s", appErrString(err));
		if (dbID) 
			DmDeleteDatabase(cardNo, dbID);
		}
	else printf("\n%ld bytes in %ld sec.s (%ld bytes/sec)", 
			totalBytes, ticks/sysTicksPerSecond, totalBytes * sysTicksPerSecond / ticks);
	return err;
}




/***********************************************************************
 *
 * FUNCTION:   FTPDatabaseWrite()
 *
 * DESCRIPTION: This routine takes a database from the Pilot and writes it
 *		out, in .PRC format, to a socket. It is used by the FTPPut command
 *
 * PARAMETERS:
 *		cardNo   - card number of database
 *		nameP		- name of database to send
 *		sock		- socket refNum of socket to send to
 *
 *	CALLED BY: FTPPut 
 *
 * RETURNED:  0 if no err
 *
 ***********************************************************************/
static Err FTPDatabaseWrite(Int16 cardNo, char * nameP, int sock)
{
	LocalID				dbID;
	UInt16					attributes, version;
	UInt32					crDate, modDate, bckUpDate;
	UInt32					type, creator;
	UInt32					modNum;
	LocalID				appInfoID, sortInfoID, recordID;
	DmOpenRef			dbP = 0;

	Err					err = 0;
	UInt16					numRecords;
	UInt32					size;
	UInt32					offset;
	Int16					i;
	DatabaseHdrType	hdr;
	UInt8 *				srcP;

	MemHandle				resH;
	LocalID				resChunkID;
	UInt32					resType;
	Int16					resID;
	RsrcEntryType 		resEntry;
	
	RecordEntryType	recEntry;
	UInt16					attr;
	UInt32					uniqueID;
	MemHandle				srcH;
	MemHandle				appInfoH=0, sortInfoH=0;
	UInt32					appInfoSize=0, sortInfoSize=0;

	UInt32					totalBytes = 0;
	UInt32					ticks;
	UInt16					filler = 0;
	
	//------------------------------------------------------------
	// Locate the Database and see what kind of database it is
	//------------------------------------------------------------
	dbID = DmFindDatabase(cardNo, nameP);
	if (!dbID) {err = DmGetLastErr(); goto Exit;}
	
	err = DmDatabaseInfo(cardNo, dbID, 0,
				&attributes, &version, &crDate,
				&modDate, &bckUpDate,
				&modNum, &appInfoID,
				&sortInfoID, &type,
				&creator);
	if (err) goto Exit;
	
	dbP = DmOpenDatabase(cardNo, dbID, dmModeReadWrite);
	if (!dbP) {err = DmGetLastErr(); goto Exit;}
	


	//------------------------------------------------------------
	// Write out a resource database
	//------------------------------------------------------------
	if (attributes & dmHdrAttrResDB) {
		// Figure out size of header
		numRecords = DmNumRecords(dbP);
		size = sizeof(DatabaseHdrType) + numRecords * sizeof(RsrcEntryType);
		
		// Fill in header
		strcpy((char*)hdr.name, nameP);
		hdr.attributes = attributes;
		hdr.version = version;
		hdr.creationDate = crDate;
		hdr.modificationDate = modDate;
		hdr.lastBackupDate = bckUpDate;
		hdr.modificationNumber = modNum;
		hdr.appInfoID = 0;
		hdr.sortInfoID = 0;
		hdr.type = type;
		hdr.creator = creator;
		hdr.recordList.nextRecordListID = 0;
		hdr.recordList.numRecords = numRecords;
	
	
		// Write out the header
		ticks = TimGetTicks();
		NetUWriteN(sock, (UInt8 *)&hdr, sizeof(hdr) - sizeof(hdr.recordList.firstEntry));
		totalBytes += sizeof(hdr) - sizeof(hdr.recordList.firstEntry);
		
	
		// Fill in the info on each resource into the header
		offset = size;
		for (i=0; i<numRecords; i++)  {
			DmResourceInfo(dbP, i, &resType, &resID, 0);
			resH = (MemHandle)DmGetResourceIndex(dbP, i);
			if (resH)  {
				size = MemHandleSize(resH);
				DmReleaseResource(resH);
				}
			else
				size = 0;
				
			resEntry.type = resType;
			resEntry.id = resID;
			resEntry.localChunkID = offset;
			
			// Write out the entry
			NetUWriteN(sock, (UInt8 *)&resEntry, sizeof(resEntry));
			totalBytes += sizeof(resEntry);
			
			offset += size;
			}
	
	
		// Write out the extra WORD that is currently in all .PRC files due to
		//  an historical "oversight".
		NetUWriteN(sock, (UInt8 *)&filler, sizeof(filler));
		totalBytes += sizeof(filler);
		
	
	
		// Write out each resource
		for (i=0; i<numRecords; i++) {
			
			StdPutS(".");

			DmResourceInfo(dbP, i, &resType, &resID, &resChunkID);
			if (resChunkID) {
				resH = (MemHandle)DmGetResourceIndex(dbP, i);
				if (!resH) {
					printf("Error getting resource");
					err = -1;
					goto Exit;
					}
				size = MemHandleSize(resH);
					
				srcP = MemHandleLock(resH);
				NetUWriteN(sock, srcP, size);
				totalBytes += size;
				MemPtrUnlock(srcP);
				DmReleaseResource(resH);
				}
			}
	
			
		} // Resource database
		
		




	//------------------------------------------------------------
	// Write out a records database
	//------------------------------------------------------------
	else {
		numRecords = DmNumRecords(dbP);
		size = sizeof(DatabaseHdrType) + numRecords * sizeof(RecordEntryType);
		
		// Fill in header
		strcpy((char*)hdr.name, nameP);
		hdr.attributes = attributes;
		hdr.version = version;
		hdr.creationDate = crDate;
		hdr.modificationDate = modDate;
		hdr.lastBackupDate = bckUpDate;
		hdr.modificationNumber = modNum;
		hdr.appInfoID = 0;
		hdr.sortInfoID = 0;
		hdr.type = type;
		hdr.creator = creator;
	
		hdr.recordList.nextRecordListID = 0;
		hdr.recordList.numRecords = numRecords;
			

		// Get the size of the appInfo and sort Info if they exist
		offset = size;
		if (appInfoID) {
			hdr.appInfoID = offset;
			appInfoH = (MemHandle)MemLocalIDToGlobal(appInfoID, cardNo);
			if (!appInfoH) {
				printf("Error getting appInfo");
				err = -1;
				goto Exit;
				}
			appInfoSize = MemHandleSize(appInfoH);
			offset += appInfoSize;
			}
			
		if (sortInfoID) {
			hdr.sortInfoID = offset;
			sortInfoH = (MemHandle)MemLocalIDToGlobal(sortInfoID, cardNo);
			if (!sortInfoH) {
				printf("Error getting sortInfo");
				err = -1;
				goto Exit;
				}
			sortInfoSize = MemHandleSize(sortInfoH);
			offset += sortInfoSize;
			}			
	
	
		// Write out the header
		ticks = TimGetTicks();
		NetUWriteN(sock, (UInt8 *)&hdr, sizeof(hdr) - sizeof(hdr.recordList.firstEntry));
		totalBytes += sizeof(hdr) - sizeof(hdr.recordList.firstEntry);
		
	
		// Fill in the info on each record into the header
		for (i=0; i<numRecords; i++)  {
			
			err = DmRecordInfo(dbP, i, &attr, &uniqueID, &recordID);
			if (err) {
				printf("\n## Error getting record info for record #%d", i);
				err = -1;
				goto Exit;
				}
			
			// If the record is marked as deleted, temporarily un-delete it so
			//  that we can get the handle to it's data, if any
			if ((attr & dmRecAttrDelete) && recordID) {
				attr &= ~dmRecAttrDelete;
				DmSetRecordInfo(dbP, i, &attr, 0);
				srcH = (MemHandle)DmQueryRecord(dbP, i);
				attr |= dmRecAttrDelete;
				DmSetRecordInfo(dbP, i, &attr, 0);
				}
			else
				srcH = (MemHandle)DmQueryRecord(dbP, i);

			// Collect the record entry info
			recEntry.localChunkID = offset;
			recEntry.attributes = attr;
			recEntry.uniqueID[0] = (uniqueID >> 16) & 0x00FF;	// vmk 10/16/95 fixed: && 0x00FF --> & 0x00FF
			recEntry.uniqueID[1] = (uniqueID >> 8) & 0x00FF;
			recEntry.uniqueID[2] = uniqueID  & 0x00FF;
			
			// Write it out
			NetUWriteN(sock, (UInt8 *)&recEntry, sizeof(recEntry));
			totalBytes += sizeof(recEntry);

			if (srcH)
				offset += MemHandleSize(srcH);
			}
	
	
	
		// Write out the extra WORD that is currently in all .PRC files due to
		//  an historical "oversight".
		NetUWriteN(sock, (UInt8 *)&filler, sizeof(filler));
		totalBytes += sizeof(filler);
		

		// Write out the appInfo followed by sortInfo, if they exist
		if (appInfoID && appInfoSize) {
			srcP = MemHandleLock(appInfoH);
			NetUWriteN(sock, srcP, appInfoSize);
			totalBytes += appInfoSize;
			MemPtrUnlock(srcP);
			}
			
		if (sortInfoID && sortInfoSize) {
			srcP = MemHandleLock(sortInfoH);
			NetUWriteN(sock, srcP, sortInfoSize);
			totalBytes += sortInfoSize;
			MemPtrUnlock(srcP);
			}
	
	
	
		// Write out each record
		for (i=0; i<numRecords; i++) {
			
			if ((i & 0x0F) == 0) StdPutS(".");
			err = DmRecordInfo(dbP, i, &attr, &uniqueID, &recordID);
			if (err) {
				printf("Error getting record");
				err = -1;
				goto Exit;
				}
				
			// If the record is marked as deleted, temporarily un-delete it so
			//  that we can get the handle to it's data, if any
			if ((attr & dmRecAttrDelete) && recordID) {
				attr &= ~dmRecAttrDelete;
				DmSetRecordInfo(dbP, i, &attr, 0);
				srcH = (MemHandle)DmQueryRecord(dbP, i);
				attr |= dmRecAttrDelete;
				DmSetRecordInfo(dbP, i, &attr, 0);
				}
			else
				srcH = (MemHandle)DmQueryRecord(dbP, i);

				
			if (srcH) {
				size = MemHandleSize(srcH);
				srcP = MemHandleLock(srcH);
				NetUWriteN(sock, srcP, size);
				totalBytes += size;
				MemPtrUnlock(srcP);
				}
			}
	
			
		}
		
	
Exit:
	ticks = TimGetTicks() - ticks;
	if (err) printf("\nPut error: %s", appErrString(err));
	else 		printf("\n%ld bytes in %ld sec.s (%ld bytes/sec)", 
			totalBytes, ticks/sysTicksPerSecond, totalBytes * sysTicksPerSecond / ticks);
	
	if (dbP)
		DmCloseDatabase(dbP);
		
	return err;
}




/***********************************************************************
 *
 * FUNCTION:   FTPGetReply 
 *
 * DESCRIPTION:Waits for a reply message from the FTP control socket. 
 *		Parses the reply for the result number and prints text portion
 *		of reply to stdout.
 *
 *		Returns result number in *resultP
 *
 *	CALLED BY:	Various FTP command process routines
 *
 * PARAMETERS:
 *			sock		- socket refNum of socket connected to Telnet server
 *			resultP 	- pointer to storage for result.
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static int FTPGetReply(NetSocketRef sock,  UIntPtr resultP)
{
	UInt32				endTime;
	
	const 	int 	bufSize = 0x00FF;
	UInt8 *			inBufP = 0, outBufP = 0;
	
	UInt8 *			bP;
	Byte				c;
	Int16				cc = 0;
	UInt16				outChars;
	Boolean			gotCR;
	
	Err				err;
	
	
	// Allocate temp buffers so we don't eat up stack space
	err = memErrNotEnoughSpace;
	inBufP = MemPtrNew(bufSize+1);
	if (!inBufP) goto Exit;
	outBufP = MemPtrNew(bufSize+1);
	if (!outBufP) goto Exit;
	

	// Calculate timeout ticks
	endTime = TimGetTicks() + kReplyTimeout;
	
	
	// Print <> around response from server
	printf("\n#");

	
	//--------------------------------------------------------------------------------------
	// Loop til last line of reply is received.
	//--------------------------------------------------------------------------------------
	while (1) {
	
	
		// Calculate timeout ticks for each line of the reply
		endTime = TimGetTicks() + kReplyTimeout;

		//--------------------------------------------------------------------------------------
		// Wait for a reply line. If a multiline reply is sent, the first line contains
		// a hyphen instead of a space after the 3 digit reply code.
		//--------------------------------------------------------------------------------------
		gotCR = false;
		outChars = 0;
		while(TimGetTicks() < endTime) {
		
			// Read as much as possible from the socket into our input buffer.
			if (!cc) {
				cc = read (sock, inBufP, bufSize);
				if (cc == 0) {
					printf("\nConnection closed.");
					err = -1;
					goto Exit;
					}
				if (cc < 0) {
					printf("\nSocket read error: %s", appErrString(errno));
					err = -1;
					goto Exit;
					}
				bP = inBufP;
				}
			
			// Copy input chars into output buf, looking for end of line and skipping <cr>'s
			while(cc) {
				c = *bP++;
				if (c == '\r') gotCR = true;
				else {
					if (outChars < bufSize) outBufP[outChars++] = c;
					if (c == '\n' && gotCR) {cc--; break; }
					}
				cc--;
				}
			if (c == '\n' && gotCR) break;
			}
			
			
		//--------------------------------------------------------------------------------------
		// Terminate and print the output buffer
		//--------------------------------------------------------------------------------------
		outBufP[outChars] = 0;
		StdPutS((char *)outBufP);
	
	
		//--------------------------------------------------------------------------------------
		// See if this line has the final result in it
		//--------------------------------------------------------------------------------------
		if (outChars >= 4) {
			*resultP = atoi((char *)outBufP);
			if (*resultP && outBufP[3] == ' ') {
				err = 0;
				goto Exit;
				}
			}
			
		}
		
		
	// Timed out
	printf(">\nGetReply timeout.");
	err = -1;
	
Exit:
	if (inBufP) MemPtrFree(inBufP);
	if (outBufP) MemPtrFree(outBufP);
	return err;
}



/******************************************************************************
 * FUNCTION:   FTPHelp
 *
 * DESCRIPTION: Handles the 'help' command
 *
 *	CALLED BY:	FTPExecCommand
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPHelp(UInt16 argc, char * argv[])
{
	printf("\ndir        # directory listing");
	printf("\nget <file> # get file from server");
	printf("\nput <file> # send file to server");
	printf("\ncd  <path> # change current directory");
	printf("\npwd <path> # print current directory.");

}



/******************************************************************************
 * FUNCTION:   FTPDataAcceptSocket
 *
 * DESCRIPTION: Opens up a data socket for FTP and sends the PORT command
 *		to the server to tell it the socket address and port number
 *
 *	CALLED BY:	Various FTP commands
 *
 * PARAMETERS: sock - socket refNum of FTP control socket. 
 *
 * RETURNED:  socket refNum of data socket, or -1 if error.
 *
 *****************************************************************************/
static int FTPDataAcceptSocket()
{
	int				acceptSock;
	struct			sockaddr_in address;
	struct			sockaddr_in ctlAddress;
	int				namelen;
	UInt16				result;
	
	
	
	// Open a socket
	acceptSock = socket(AF_INET, SOCK_STREAM, 0);
	if (acceptSock < 0) goto ExitErr;
	
	// Bind it to a local address
	bzero((char*)&address, sizeof(address));
	address.sin_family = AF_INET;
	if ( bind (acceptSock, (struct sockaddr *)&address, sizeof(address)) < 0) goto ExitErr;
	
	// Set the listen queue size
	if ( listen (acceptSock, 1) < 0) goto ExitErr;
	
	
	// Get our local address of the data socket and use the port number from that
	namelen = sizeof(address);
	if ( getsockname(acceptSock, (struct sockaddr*)&address, &namelen) < 0) goto ExitErr;
	
	// Get our local address for the control socket. We have to use the IP address from
	//  this socket since the data socket isn't connected yet.
	namelen = sizeof(ctlAddress);
	if ( getsockname(FTPCtlSock, (struct sockaddr*)&ctlAddress, &namelen) < 0) goto ExitErr;
	
	
	// Send the port command
	StrPrintF(FTPCommand, "PORT %d,%d,%d,%d,%d,%d\r\n", 
		ctlAddress.sin_addr.S_un.S_un_b.s_b1,
		ctlAddress.sin_addr.S_un.S_un_b.s_b2,
		ctlAddress.sin_addr.S_un.S_un_b.s_b3,
		ctlAddress.sin_addr.S_un.S_un_b.s_b4,
		address.sin_port >> 8,
		address.sin_port & 0x00FF);
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
	
	
	// Get response
	if (FTPGetReply(FTPCtlSock, &result)) goto ExitErr;
	if (result >= 200 && result <= 299) return acceptSock;
	

ExitErr:
	if (acceptSock  >= 0) close(acceptSock);
	printf("\nError creating data connection: %s", appErrString(errno));
	return -1;
}




/******************************************************************************
 * FUNCTION:   FTPDir
 *
 * DESCRIPTION: Handles the 'dir' command
 *
 *	CALLED BY:	FTPExecCommand
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPDir(UInt16 argc, char * argv[])
{
	int				acceptSock = -1;
	int				dataSock = -1;
	
	UInt16				reply;
	struct			sockaddr_in address;
	int				addrLen;
	const int		bufSize = 0x0100;
	UInt8 *			bufP = 0;
	int				cc;
	UInt16				i, j;
	
	
	// Create a data port to get the results
	acceptSock = FTPDataAcceptSocket();
	if (acceptSock < 0) return;
	
	
	// Allocate our buffer
	bufP = MemPtrNew(bufSize);
	if (!bufP) {errno = memErrNotEnoughSpace; goto Exit;}
	
	// Send the dir command
	if (argc > 1)
		StrPrintF(FTPCommand, "LIST \"%s\"\r\n", argv[1]);
	else
		StrPrintF(FTPCommand, "LIST\r\n");
		
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
		
		
	// Get Reply from the LIST command
	if (FTPGetReply(FTPCtlSock, &reply)) goto Exit;
	if (reply >= 400) goto Exit;
	
	
	// Accept a connection on our data port
	addrLen = sizeof(address);
	if ((dataSock = accept (acceptSock, &address, &addrLen)) < 0) goto Exit;
	
	
	// Read the directory listing, throwing out carriage returns.
	while (1) {
		cc = NetUReadN(dataSock, bufP, bufSize-1);
		if (cc <= 0) break;
		
		bufP[cc] = 0;
		
		// Get rid of carriage returns in the output
		for (i=0,j=0; i<=cc; i++) {
			if (bufP[i] == '\r') continue;
			bufP[j++] = bufP[i];
			}
			
		printf("%s", bufP);
		}
	if (cc<0) printf("\nError: %s", appErrString(errno));
		

	// Get Transer complete message
	FTPGetReply(FTPCtlSock, &reply);
	
	
Exit:
	if (errno) printf("\nError: %s", appErrString(errno));
	if (acceptSock >= 0) close(acceptSock);	
	if (dataSock >= 0) close(dataSock);	
}



/******************************************************************************
 * FUNCTION:   FTPGet
 *
 * DESCRIPTION: Handles the 'get' command
 *
 *	CALLED BY:	FTPExecCommand
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPGet(UInt16 argc, char * argv[])
{
	int				acceptSock = -1;
	int				dataSock = -1;
	
	UInt16				reply;
	struct			sockaddr_in address;
	int				addrLen;
	
	
	// Syntax check
	if (argc < 2) {
		printf("\nSyntax: %s <filename>", argv[0]);
		return;
		}
	
	// Send the type command to put us into binary mode
	StrPrintF(FTPCommand, "TYPE I\r\n");
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
	if (FTPGetReply(FTPCtlSock, &reply)) goto Exit;
	if (reply >= 400) goto Exit;


	// Create a data port to get the results
	acceptSock = FTPDataAcceptSocket();
	if (acceptSock < 0) return;
	
	
	// Send the RETR command
	StrPrintF(FTPCommand, "RETR %s\r\n", argv[1]);
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
		
		
	// Get Reply from the RETR command
	if (FTPGetReply(FTPCtlSock, &reply)) goto Exit;
	if (reply >= 400) goto Exit;
	
	
	// Accept a connection on our data port
	addrLen = sizeof(address);
	if ((dataSock = accept (acceptSock, &address, &addrLen)) < 0) goto Exit;
	
	
	// Read the file 
	errno = FTPDatabaseRead(0, argv[1], dataSock);
		

	// Get Transer complete message
	FTPGetReply(FTPCtlSock, &reply);
	
	
Exit:
	if (errno) printf("\nError: %s", appErrString(errno));
	if (acceptSock >= 0) close(acceptSock);	
	if (dataSock >= 0) close(dataSock);	
}


/******************************************************************************
 * FUNCTION:   FTPPut
 *
 * DESCRIPTION: Handles the 'put' command
 *
 *	CALLED BY:	FTPExecCommand
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPPut(UInt16 argc, char * argv[])
{
	int				acceptSock = -1;
	int				dataSock = -1;
	
	UInt16				reply;
	struct			sockaddr_in address;
	int				addrLen;
	
	
	// Syntax check
	if (argc < 2) {
		printf("\nSyntax: %s <filename>", argv[0]);
		return;
		}
	
	// Send the type command to put us into binary mode
	StrPrintF(FTPCommand, "TYPE I\r\n");
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
	if (FTPGetReply(FTPCtlSock, &reply)) goto Exit;
	if (reply >= 400) goto Exit;


	// Create a data port to send the file to
	acceptSock = FTPDataAcceptSocket();
	if (acceptSock < 0) return;
	
	
	// Send the STOR command
	StrPrintF(FTPCommand, "STOR %s\r\n", argv[1]);
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
		
		
	// Get Reply from the STOR command
	if (FTPGetReply(FTPCtlSock, &reply)) goto Exit;
	if (reply >= 400) goto Exit;
	
	
	// Accept a connection on our data port
	addrLen = sizeof(address);
	if ((dataSock = accept (acceptSock, &address, &addrLen)) < 0) goto Exit;
	
	
	// Send the database
	errno = FTPDatabaseWrite(0, argv[1], dataSock);
	if (errno) goto Exit;
	close(dataSock);
	close(acceptSock);
	dataSock = acceptSock = -1;
	
		
	// Get Transer complete message
	FTPGetReply(FTPCtlSock, &reply);
	
	
Exit:
	if (errno) printf("\nError: %s", appErrString(errno));
	if (acceptSock >= 0) close(acceptSock);	
	if (dataSock >= 0) close(dataSock);		 
}




/******************************************************************************
 * FUNCTION:   FTPPwd
 *
 * DESCRIPTION: Handles the 'pwd' command
 *
 *	CALLED BY:	FTPExecCommand
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPPwd(UInt16 argc, char * argv[])
{
	UInt16				reply;
	
	
	// Send the pwd command
	StrPrintF(FTPCommand, "PWD\r\n");
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
		
	// Get Reply
	if (FTPGetReply(FTPCtlSock, &reply)) goto ExitErr;
	return;
	
ExitErr:
	printf("\nError: %s", appErrString(errno));
}


/******************************************************************************
 * FUNCTION:   FTPCd
 *
 * DESCRIPTION: Handles the 'cd' command
 *
 *	CALLED BY:	FTPExecCommand
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPCd(UInt16 argc, char * argv[])
{
	UInt16				reply;
	
	
	// Validate arguments
	if (argc < 2) {
		printf("\nSyntax: %s <path>", argv[0]);
		return;
		}
	
	// Send the pwd command
	StrPrintF(FTPCommand, "CWD %s\r\n", argv[1]);
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
		
	// Get Reply
	if (FTPGetReply(FTPCtlSock, &reply)) goto ExitErr;
	return;
	
ExitErr:
	printf("\nError: %s", appErrString(errno));
}




/******************************************************************************
 * FUNCTION:   FTPExecCommand
 *
 * DESCRIPTION: Separates an FTP command line string into argv's and
 *		then calls the appropriate FTP routine to process it.
 *
 *	CALLED BY:	CmdFTP
 *
 * PARAMETERS:
 *			cmdParamP	- pointer to command line
 *
 * RETURNED:   void
 *
 *****************************************************************************/
static void FTPExecCommand(char * cmdParamP)
{
	const int	maxArgc=5;
	char *		argv[maxArgc+1];
	int			argc;
	Boolean		done = false;
	char *		cmdBufP = 0;
	char *		cmdP;

	// if null string, return
	if (!cmdParamP[0]) return;

	
	// Make a copy of the command since we'll need to write
	// 0's between the words and it might be from read-only space
	cmdP = cmdBufP = MemPtrNew(StrLen(cmdParamP) + 1);
	if (!cmdP) {
		printf("\nOut of memory\n");
		goto Exit;
		}
	MemMove(cmdP, cmdParamP, StrLen(cmdParamP) + 1);

	// Separate the cmd line into arguments
	argc = 0;
	argv[0] = 0;
	for (argc=0; !done && argc<=maxArgc; argc++) {
	
		// Skip leading spaces
		while((*cmdP == ' ' || *cmdP == '\t') && (*cmdP != 0))
			cmdP++;
			
		// Break out on null
		if (*cmdP == 0) break;
		
		// Get pointer to command
		argv[argc] = cmdP;
		if (*cmdP == 0) break;
		
		// Find the end of a quoted argument
		if (*cmdP == '"') {
			cmdP++;
			argv[argc] = cmdP;
			while(*cmdP) {
				if (*cmdP == '"') {*cmdP = 0; break;}
				if (*cmdP == 0) {done = true; break;}
				cmdP++;
				}
			cmdP++;
			}
			
		// Find the end of an unquoted argument
		else {
			while(1) {
				if (*cmdP == 0) {done = true; break;}
				if ((*cmdP == ' ') || (*cmdP == '\t')) {
					*cmdP = 0; 
					break;
					}
				cmdP++;
				}
			cmdP++;
			}
		}
	
	// Return if no arguments
	if (!argc) goto Exit;
		

	//--------------------------------------------------------------------
	// Call the appropriate routine
	//--------------------------------------------------------------------
	if (!StrCompare(argv[0], "help"))
		FTPHelp(argc, argv);
		
	else if (!StrCompare(argv[0], "quit"))
		FTPQuit = true;
		
	else if (!StrCompare(argv[0], "dir"))
		FTPDir(argc, argv);
	
	else if (!StrCompare(argv[0], "pwd"))
		FTPPwd(argc, argv);
	
	else if (!StrCompare(argv[0], "cd"))
		FTPCd(argc, argv);
	
	else if (!StrCompare(argv[0], "get"))
		FTPGet(argc, argv);
	
	else if (!StrCompare(argv[0], "put"))
		FTPPut(argc, argv);
	
	// Not found
	else
		printf("Unknown command: %s\n", argv[0]);
	
Exit:
	if (cmdBufP) MemPtrFree(cmdBufP);
			
}




/***********************************************************************
 *
 * FUNCTION:   CmdFTP()
 *
 * DESCRIPTION: FTP Client
 *
 *	CALLED BY:	AppProcessCommand in response to the "ftp" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdFTP(int argc, char * argv[])
{
	char *		hostname = "localhost";
	char *		service = "ftp";
	Int16			port = 0;
	UInt16			result;
	char *		promptP=0;
	UInt32			oldAppNetTimeout;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;

	// Assume err
	FTPCtlSock = -1;
	FTPQuit = false;
	
	// Increase the timeout
	oldAppNetTimeout = AppNetTimeout;
	if (AppNetTimeout < 180*sysTicksPerSecond) 
		AppNetTimeout = 180*sysTicksPerSecond;


	// Get the host name and service name, if any
	if (argc > 1) 	hostname = argv[1];
	if (argc > 2) {
		service = argv[2];
		port = atoi(service);
		if (port > 0) service = 0;
		}
	
	
	// Test the new trace call
	NetLibTracePrintF(AppNetRefnum, "%s...", "about to connect");
	
	
	// Open up a TCP connection to the FTP control port.
	FTPCtlSock = NetUTCPOpen(hostname, service, port);
	if (FTPCtlSock < 0) {
		printf("\nError connecting to %s: %s", hostname, appErrString(errno));
		goto Exit;
		}
	else
		printf("\nConnected to %s.\n", hostname);
		
	// Test the new trace call
	NetLibTracePrintF(AppNetRefnum, "Connect: %s", hostname);
	NetLibTracePutS(AppNetRefnum, "Connected");
	

	// Get the connect message from the server
	if (FTPGetReply(FTPCtlSock, &result)) goto Exit;
	
	
	// Allocate space for the prompt string
	promptP = MemPtrNew(0x100);
	if (!promptP) {printf("\nOut of Memory"); goto Exit;}
	
	
	//-----------------------------------------------------------------
	// Ask for user name
	//-----------------------------------------------------------------
	printf("\nName: ");
	if (StdGetS(promptP, true) < 0) goto Exit;
	StrPrintF(FTPCommand, "USER %s\r\n", promptP);
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
	
	// Get response
	if (FTPGetReply(FTPCtlSock, &result)) goto Exit;
	
	
	
	//-----------------------------------------------------------------
	// If necessary, ask for password
	//-----------------------------------------------------------------
	if (result >= 300 && result <= 399)  {
		// Ask for password
		printf("\nPassword: ");
		if (StdGetS(promptP, false) < 0) goto Exit;
		StrPrintF(FTPCommand, "PASS %s\r\n", promptP);
		NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
		
		// Get response
		if (FTPGetReply(FTPCtlSock, &result)) goto Exit;
		if (result < 200 || result > 299) goto Exit;
		}
		
	
	
	// Print help string
	printf("\nType 'help' to get list of commands\n");
	
	
	//-----------------------------------------------------------------
	// Enter loop to process user commands now
	//-----------------------------------------------------------------
	while (!FTPQuit) {
		
		// Get prompt from user
		printf("\nftp> ");
		if (StdGetS(promptP, true) < 0) goto Exit;
		
		FTPExecCommand(promptP);
		}


	//-----------------------------------------------------------------
	// Send the Quit command
	//-----------------------------------------------------------------
	StrPrintF(FTPCommand, "QUIT\r\n");
	NetUWriteN(FTPCtlSock, (UInt8 *)FTPCommand, StrLen(FTPCommand));
	
	// Get response
	FTPGetReply(FTPCtlSock, &result);
	
	


	
	//==================================================================
	// Exit now.
	//======================================================================
Exit:
	// Restore timeout
	AppNetTimeout = oldAppNetTimeout;

	// Release prompt string ptr
	if (promptP) MemPtrFree(promptP);

	// Close the connection, if open
	if (FTPCtlSock >= 0) close(FTPCtlSock);
	printf("\nExited FTP.\n");
	return;
	
	
	
ShortHelp:
	printf("%s\t\t\t# FTP Client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nFTP Client");
	printf("\nSyntax: %s <server> [<port>]", argv[0]);
	printf("\n");
}




/***********************************************************************
 *
 * FUNCTION:   	CmdFTPInstall
 *
 * DESCRIPTION: 	Installs the commands from this module into the
 *		master command table used by AppProcessCommand.
 *
 *	CALLED BY:		AppStart 
 *
 * RETURNED:    	void
 *
 ***********************************************************************/
void CmdFTPInstall(void)
{
	AppAddCommand("_", CmdDivider);

#if INCLUDE_FTP
	AppAddCommand("ftp", CmdFTP);
#endif

}

