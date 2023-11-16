/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Expense.h
 *
 * Description:
 *	  This file defines the structures and routines of the Expense 
 *   Application
 *
 * History:
 *		August 22, 1996	Created by Art Lamb
 *
 *****************************************************************************/

#include "ExpAttendees.h"
#include "ExpCurrency.h"
#include "ExpDB.h"
#include "ExpRsc.h"
#include "ExpLookup.h"


/***********************************************************************
 *
 * Constants
 *
 ***********************************************************************/
// Feature number of the sorted array of expense types
#define expFeatureNum					0
#define expRevFeatureNum				1

#define noExpenseType					255


// Update codes, used to determine how the views should be redrawn.

// ListView Update codes
#define updateRedrawAll					0x00
#define updateItemDelete				0x01
#define updateItemMove					0x02
#define updateItemChanged				0x04
#define updateCategoryChanged			0x08
#define updateGoTo						0x10
#define updateDisplayOptsChanged		0x20
#define updateCustomCurrencyChanged	0x40
#define updateFontChanged				0x80

// Details Dialog update codes.
#define updateAttendees					0x01

// Maximun number of currencies that appear in the currency popup list.
#define maxCurrenciesDisplayed		7

// wrdListRscType Resources
#define idCurrencies						1900
#define idPaymentTypes					1901


/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/
extern DmOpenRef			ExpenseDB;									// expense database
extern UInt16				CurrentRecord;								// record being edited
extern Boolean				ItemSelected;								// true if a list view item is selected
extern FontID				NoteFont;

extern UInt8				Currencies [maxCurrenciesDisplayed];

extern UInt8				CurrentCustomCurrency;					

extern UInt16				HomeCountry;

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
extern void * GetObjectPtr (UInt16 objectID);
extern void	SetDBAttrBits(DmOpenRef dbP, UInt16 attrBits);

