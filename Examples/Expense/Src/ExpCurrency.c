/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpCurrency.c
 *
 * Description:
 *	  This file contains routines the manage currencies.
 *
 * History:
 *		August 27, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#include <PalmOS.h>
#include <CharAttr.h>

#include "Expense.h"

/***********************************************************************
 *
 *	Internal Structutes
 *
 ***********************************************************************/
typedef struct {
	Char * 	countryName;
	UInt16	currencyID;
} CountrySortType;



/***********************************************************************
 *
 * FUNCTION:    CurrencyGetCountryName
 *
 * DESCRIPTION: This routine returns the country name for the specified 
 *              currency.
 *
 * PARAMETERS:  currency - currency
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/96	Initial Revision
 *			grant	3/25/99	added support for third group of currencies
 *
 ***********************************************************************/
static void CurrencyGetCountryName (UInt16 currency, Char * name)
{
	UInt16							index;
	ExpenseAppInfoPtr 			appInfoP;
	CountryPreferencesType *	resP;

	if (currency >= countryFirst && currency <= countryLast)
		{
  		resP = MemHandleLock (DmGetResource (sysResTCountries, sysResIDCountries));
		StrCopy (name, resP[currency].countryName);
		MemPtrUnlock (resP);
		}

	else if (currency >= currencyCustomUser1 && currency <= currencyCustomUser4)
		{
		appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
		index = currency - currencyCustomUser1;
		StrCopy (name, appInfoP->currencies[index].country);
		MemPtrUnlock (appInfoP);
		}
	
	else if (currency >= currencyCustomAppFirst && currency <= currencyCustomAppLast)
		{
		SysStringByIndex (currencyNameStrListID, currency - currencyCustomAppFirst, name, 
			maxCurrencyNameLen - 1);
		}
		
	else
		{
		*name = 0;
		}
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyGetDecimalSeperator
 *
 * DESCRIPTION: This routine returns the decimal seperator character 
 *              for the specified currency.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/30/96	Initial Revision
 *			trm	7/22/97	Feture request to have number format changed to user defined.
 *
 ***********************************************************************/
Char CurrencyGetDecimalSeperator (UInt16 currency)
{
	Char thousandSeparator;
	Char decimalSeparator;

	if (currency >= countryFirst && currency <= countryLast)
		{
		LocGetNumberSeparators ((NumberFormatType) PrefGetPreference (prefNumberFormat),
			&thousandSeparator, &decimalSeparator);
		}
	else
		ErrNonFatalDisplay("Bad currency");

	return (decimalSeparator);
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyGetDecimalPlaces
 *
 * DESCRIPTION: This routine returns the decimal positions 
 *              for the specified currency.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/11/96	Initial Revision
 *
 ***********************************************************************/
UInt16 CurrencyGetDecimalPlaces (UInt16 currency)
{
	Char decimalPlaces;
	CountryPreferencesType *	resP;


	if (currency >= countryFirst && currency <= countryLast)
		{
  		resP = MemHandleLock (DmGetResource (sysResTCountries, sysResIDCountries));
		
		decimalPlaces = resP[currency].currencyDecimalPlaces;
		
		MemPtrUnlock (resP);
		}
		
	else
		decimalPlaces = 2;

	return (decimalPlaces);
}


/***********************************************************************
 *
 * FUNCTION:    CountryCompare
 *
 * DESCRIPTION: Case blind comparision of two country names. This routine is 
 *              used as a callback routine when sorting the the country list.
 *
 * PARAMETERS:  c1 - a country name (null-terminated)
 *              c2 - a country name (null-terminated)
 *
 * RETURNED:    if c1 > c2  - a positive integer
 *              if c1 = c2  - zero
 *              if c1 < c2  - a negative integer
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/96	Initial Revision
 *
 ***********************************************************************/
static Int16 CountryCompare (CountrySortType *c1, CountrySortType *c2, Int32 /*other*/)
{
	return StrCaselessCompare (c1->countryName, c2->countryName);
}


/***********************************************************************
 *
 * FUNCTION:    CountrySelect
 *
 * DESCRIPTION: This routine creates a list of currency symbols.
 *  
 * PARAMETERS:  listP        - list in which to place the categories
 *              currencyName - name of the current currency
 *					 name         - name of the country (returned)
 *
 * RETURNED:    true if a defferent cunntry is selected
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *			grant	3/25/99	added support for third group of currencies
 *
 ***********************************************************************/
static Boolean CountrySelect (ListPtr lst, UInt8 * currencyP, Char * name)

{
	Int16								i;
	Int16								listIndex;
	Int16								selection;
	Int16								newSelection;
	Boolean							changed = false;
	Char *							str;
	Char *							noneStrP;
	Char *							listItems [countryCount + maxUserCustomCurrencies + currencyCustomAppCount + 1];
	CountrySortType				countries [countryCount + maxUserCustomCurrencies + currencyCustomAppCount + 1];
	CurrencyInfoPtr 				currencies;
	ExpenseAppInfoPtr 			appInfoP;
	CountryPreferencesType *	countryPreferencesP;
	MemHandle							currNamesH;
	Char *							currNamesP;
			

	// Get the system resource that contains the country name.
  	countryPreferencesP = MemHandleLock (DmGetResource (sysResTCountries, sysResIDCountries));

	// Get the block the contains the custom currencies.
	appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
	currencies = appInfoP->currencies;

	// Build a sorted list of country names.
	listIndex = 0;
	// Add the basic supported countries
	for (i = 0; i < countryCount; i++)
		{
		countries[listIndex].countryName = countryPreferencesP[i].countryName;
		countries[listIndex].currencyID = i;
		listIndex++;
		}

	// Add the user's custom currencies
	for (i = 0; i < maxUserCustomCurrencies; i++)
		{
		if (*currencies [i].country)
			{
			countries[listIndex].countryName = currencies[i].country;
			countries[listIndex].currencyID = currencyCustomUser1 + i;
			listIndex++;
			}
		}
	
	// Add expense app currencies
	currNamesH = DmGetResource(sysResTErrStrings, currencyNameStrListID);
	ErrNonFatalDisplayIf(!currNamesH, "missing currency name resource");
	currNamesP = (Char *) MemHandleLock(currNamesH);
	currNamesP += StrLen(currNamesP) + sizeOf7BitChar(nullChr) + sizeof(UInt16);		// skip prefix string & number of entries

	for (i = 0; i < currencyCustomAppCount; i++)
		{
		ErrNonFatalDisplayIf(listIndex >= (countryCount + maxUserCustomCurrencies + currencyCustomAppCount), "too many currencies");
		countries[listIndex].countryName = currNamesP;						// save pointer to this one
		currNamesP += StrLen(currNamesP) + sizeOf7BitChar(nullChr);		// go to the next one
		countries[listIndex].currencyID = currencyCustomAppFirst + i;
		listIndex++;
		}
	
	// Sort the list
	SysQSort (countries, listIndex, sizeof (CountrySortType), (CmpFuncPtr)CountryCompare, 0);
		
	
	// End "None" choice to the end of the list.
	noneStrP = MemHandleLock (DmGetResource (strRsc, noneStrID));
	countries[listIndex].countryName = noneStrP;
	countries[listIndex].currencyID = currencyNone;
	listIndex++;


	// Initial the choices, selection and height of the list.
	selection = noListSelection;
	for (i = 0; i < listIndex; i++)
		{
		listItems[i] = countries[i].countryName;
		if (*currencyP == countries[i].currencyID)
			selection = i;
		}

	LstSetListChoices (lst, listItems, listIndex);
	LstSetSelection (lst, selection);
	if (selection != noListSelection)
		LstMakeItemVisible  (lst, selection);
	else
		LstMakeItemVisible  (lst, 0);

	// Display the country list.
	newSelection = LstPopupList (lst);

	// If a new country was selected then return the county name and id.
	 if (newSelection != -1 && newSelection != selection)
		{
		*currencyP = countries[newSelection].currencyID;

		// Make a copy of the currency name, it's going to freed.
		if (name)
			{
			str = LstGetSelectionText (lst, newSelection);
			StrCopy (name, str);
			}
		changed = true;		
		}


	MemPtrUnlock (noneStrP);
	MemPtrUnlock (appInfoP);
	MemPtrUnlock (countryPreferencesP);
	MemHandleUnlock (currNamesH);
	
	return (changed);
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyGetSymbol
 *
 * DESCRIPTION: This routine returns the symbol of the specified 
 *              currency.
 *
 * PARAMETERS:  currency - Currency to select
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/28/96	Initial Revision
 *			grant	3/25/99	added support for third group of currencies
 *
 ***********************************************************************/
void CurrencyGetSymbol (UInt16 currency, Char * symbol)
{
	UInt16							index;
	ExpenseAppInfoPtr 			appInfoP;
	CountryPreferencesType *	resP;
			

	if (currency >= countryFirst && currency <= countryLast)
		{
  		resP = MemHandleLock (DmGetResource (sysResTCountries, sysResIDCountries));
		if (currency == HomeCountry)
			StrCopy (symbol, resP[currency].currencySymbol);
		else
			StrCopy (symbol, resP[currency].uniqueCurrencySymbol);
		MemPtrUnlock (resP);
		}

	else if (currency >= currencyCustomUser1 && currency <= currencyCustomUser4)
		{
		appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
		index = currency - currencyCustomUser1;
		StrCopy (symbol, appInfoP->currencies[index].symbol);
		MemPtrUnlock (appInfoP);
		}
	
	else if (currency >= currencyCustomAppFirst && currency <= currencyCustomAppLast)
		{
		SysStringByIndex (currencySymbolStrListID, currency - currencyCustomAppFirst, symbol, 
			maxCurrencySymbolLen);
		}
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyCreateList
 *
 * DESCRIPTION: This routine creates a list of currency symbols.
 *
 * PARAMETERS:  listP - list in which to place the categories
 *					 currentCurrency - Currency to select
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *			grant	3/25/99	added support for third group of currencies
 *
 ***********************************************************************/
static void CurrencyCreateList (ListPtr listP, UInt16 currentCurrency)
{
	Int16								count;
	Int16								listIndex;
	Int16								selection;
	UInt16							i;
	UInt16							size;
	Char *							ptr;
	Char *							items;
	Char *							editingStr;
	Char *							listItems [maxCurrenciesDisplayed + 1];
	Char ** 							itemsPtr;
	CurrencyInfoPtr 				currencies;
	ExpenseAppInfoPtr 			appInfoP;
	CountryPreferencesType *	countryPrefP;
	MemHandle							currNamesH;
	Char *							srcP;
	Char *							currNames [currencyCustomAppCount];
			

	appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
	currencies = appInfoP->currencies;

  	countryPrefP = MemHandleLock (DmGetResource (sysResTCountries, sysResIDCountries));

	// Initialize the list of expense currencies
	currNamesH = DmGetResource(sysResTErrStrings, currencySymbolStrListID);
	ErrNonFatalDisplayIf(currNamesH == NULL, "missing currency symbol resource");
	srcP = (Char *) MemHandleLock(currNamesH);
	srcP += StrLen(srcP) + sizeOf7BitChar(nullChr) + sizeof(UInt16);		// skip prefix string & number of entries
	for (i = 0; i < currencyCustomAppCount; i++)
		{
		currNames [i] = srcP;										// save pointer to i'th string
		srcP += StrLen(srcP) + sizeOf7BitChar(nullChr);		// bump string pointer
		}

	// For each Currency not empty, copy it's string to the list's
	// item list.
	listIndex = 0;
	for (i = 0; i < maxCurrenciesDisplayed; i++)
		{
		if (Currencies[i] >= countryFirst && Currencies[i] <= countryLast)
			{
			if (Currencies[i] == HomeCountry)
				listItems[listIndex++] = countryPrefP[Currencies[i]].currencySymbol;
			else
				listItems[listIndex++] = countryPrefP[Currencies[i]].uniqueCurrencySymbol;
			}

		else if (Currencies[i] >= currencyCustomUser1 && Currencies[i] <= currencyCustomUser4)
			{
			listItems[listIndex++] = currencies [Currencies[i] - currencyCustomUser1].symbol;
			}
			
		else if (Currencies[i] >= currencyCustomAppFirst && Currencies[i] <= currencyCustomAppLast)
			{
			listItems[listIndex++] = currNames[Currencies[i] - currencyCustomAppFirst];
			}
		}
		
		
	// Add "edit currencies" item to the end of the list.
	editingStr = MemHandleLock(DmGetResource(strRsc, editingCurenciesStrID));
	listItems[listIndex++] = editingStr;
	

	// Allocate a block to hold the items in the list.
	if (listIndex > 0)
		{
		size = (listIndex) * sizeof (Char *);
		itemsPtr = MemPtrNew (size);

		// Allocate a block to hold the country names.
		size = 0;
		for (i = 0; i < listIndex; i++)
			size += StrLen (listItems[i]) + 1;
		items = MemPtrNew (size);


		// Copy the country names into the list.
		ptr = items;
		for (i = 0; i < listIndex; i++)
			{
			itemsPtr[i] = ptr;
			StrCopy (ptr, listItems[i]);
			ptr += StrLen (listItems[i]) + 1;
			}
		}
	else
		{
		itemsPtr = NULL;
		items = NULL;
		}
	
	LstSetListChoices (listP, itemsPtr, listIndex);
	LstSetHeight (listP, listIndex);


	// The the selection of the list to the passed curency.
	selection = noListSelection;
	count = 0;
	for (i = 0; i < maxCurrenciesDisplayed; i++)
		{
		if (Currencies[i] != currencyNone)
			{
			if (Currencies[i] == currentCurrency)
				{
				selection = count;
				break;
				}
			else
				count++;
			}
		}
	LstSetSelection (listP, selection);
	

	MemPtrUnlock (editingStr);
	MemPtrUnlock (appInfoP);
	MemPtrUnlock (countryPrefP);
	MemHandleUnlock (currNamesH);
}



/***********************************************************************
 *
 * FUNCTION:    CurrencySelect
 *
 * DESCRIPTION: This routine process the selection and editting of
 *              Currencies. 
 *
 * PARAMETERS:  frm          - form the contains the currency popup list
 *              lst          - pointer to the popup list
 *              currencyP    - 
 *              currencyName - name of the current currency
 *
 * RETURNED:    This routine returns true if any of the following conditions
 *					 are true:
 *					 	- the current currency is renamed
 *					 	- the current currency is deleted
 *					 	- the current currency is merged with another currency
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/95	Initial Revision
 *
 ***********************************************************************/
Boolean CurrencySelect (ListPtr lst, UInt16 * currencyP, Char * currencyName)

{
	Char *		str;
	Int16			i;
	Int16			count;
	Int16			newSelection;
	Int16			curSelection;
	Boolean		currencyEdited = false;
	UInt16 			currency;
	
	currency = *currencyP;
	
	// If the currency trigger is part of the title, then the "all" item
	// should be in the list.
	CurrencyCreateList (lst, currency);

	// Make sure the list is entirely visible.
	// zzz Need a LstGetPosition function.
	LstSetPosition (lst, lst->bounds.topLeft.x, lst->bounds.topLeft.y);

	curSelection = LstGetSelection (lst);
	newSelection = LstPopupList (lst);


	// Was the  "edit Currencies ..." item selected?
	if (newSelection == LstGetNumberOfItems (lst) - 1)
		{
		// DOLATER - This architecture currently won't work. The routine wants
		// to return true if the user edits a currency, but at this point
		// the Currency Select form isn't displayed, so we don't know if
		// anything's been changed or not. Better would be some special
		// event generated by the Currency Select form which triggers this
		// reformatting.
		
		// We'll treat moving a currency like selecting a diffent currency. 
		FrmPopupForm (CurrencySelectDialog);
		}

	// Was a new currency selected?
	else if ((newSelection != curSelection) && (newSelection != -1))
		{
		// Make a copy of the currency name, it's going to freed.
		if (currencyName)
			{
			str = LstGetSelectionText (lst, newSelection);
			StrCopy (currencyName, str);
			}
		
		// Get the currency id that corresponds to the list item selected.
		count = 0;
		for (i = 0; i < maxCurrenciesDisplayed; i++)
			{
			if (Currencies[i] != currencyNone)
				{
				if (count == newSelection)
					{
					currency = Currencies[i];
					break;
					}
				else
					count++;
				}
			}
		}


	// Free the memory allocated for the list choices.
	if (lst->itemsText[0])
		MemPtrFree (lst->itemsText[0]);

	if (lst->itemsText)
		MemPtrFree (lst->itemsText);


	*currencyP = currency;
	return (currencyEdited);
}


/***********************************************************************
 *
 * FUNCTION:    CurrencySelectCurrency
 *
 * DESCRIPTION: This routine select a currency that will be displayed
 *              in the currency sysmbol popup list.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/96	Initial Revision
 *
 ***********************************************************************/
static void CurrencySelectCountry (UInt16 ctlID, UInt8 * currencies)
{
	UInt16 index;
	ListPtr lst;
	Char * label;
	ControlPtr ctl;

	switch (ctlID)
		{
		case CurrencySelectCurr1Trigger: index = 0;		break;
		case CurrencySelectCurr2Trigger: index = 1;		break;
		case CurrencySelectCurr3Trigger: index = 2;		break;
		case CurrencySelectCurr4Trigger: index = 3;		break;
		case CurrencySelectCurr5Trigger: index = 4;		break;
		}

	lst = GetObjectPtr (CurrencySelectCountryList);
	ctl = GetObjectPtr (ctlID);
	label = (Char *)CtlGetLabel (ctl);	// OK to cast, we call CtlSetLabel

	if (CountrySelect (lst, &currencies[index], label))
		{
		CtlSetLabel (ctl, label);
		}
}


/***********************************************************************
 *
 * FUNCTION:    CurrencySelectInit
 *
 * DESCRIPTION: This routine initializes the Currency Properties
 *              Dialog Box.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/96	Initial Revision
 *
 ***********************************************************************/
static void CurrencySelectInit (void)
{
	Int16 i;
	UInt16 id;
	Char * label;
	Char * rscP;
	ControlPtr ctl;
			
	for (i = 0; i < 5; i++)
		{
		switch (i)
			{
			case 0:	id  = CurrencySelectCurr1Trigger;		break;
			case 1:	id  = CurrencySelectCurr2Trigger;		break;
			case 2:	id  = CurrencySelectCurr3Trigger;		break;
			case 3:	id  = CurrencySelectCurr4Trigger;		break;
			case 4:	id  = CurrencySelectCurr5Trigger;		break;
			}

		ctl = GetObjectPtr (id);
		label = (Char *)CtlGetLabel (ctl);	// ok to cast, we call CtlSetLabel
		CurrencyGetCountryName (Currencies [i], label);
		
		if (*label)
			CtlSetLabel (ctl, label);
		else
			{
			rscP = MemHandleLock (DmGetResource (strRsc, noneStrID));
			StrCopy (label, rscP);
			CtlSetLabel (ctl, label);
			MemPtrUnlock (rscP);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    CurrencySelectHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Currency
 *              Selection Dialog Box.  This dialog is used to specify
 *              which currency symbol appear to the currency popup
 *              list.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/96	Initial Revision
 *
 ***********************************************************************/
Boolean CurrencySelectHandleEvent (EventType * event)
{
	static UInt8	currencies [maxCurrenciesDisplayed];
	
	FormPtr frm;
	Boolean handled = false;

	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case CurrencySelectOkButton:
				MemMove (Currencies, currencies, sizeof(Currencies));
				FrmReturnToForm (0);
				handled = true;
				break;

			case CurrencySelectCancelButton:
				FrmReturnToForm (0);
				handled = true;
				break;

			case CurrencySelectCurr1Trigger:
			case CurrencySelectCurr2Trigger:
			case CurrencySelectCurr3Trigger:
			case CurrencySelectCurr4Trigger:
			case CurrencySelectCurr5Trigger:
				CurrencySelectCountry (event->data.ctlSelect.controlID, currencies);
				break;				
			}
		}

	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		CurrencySelectInit ();
		FrmDrawForm (frm);
		// samk - sizeof(currencies) is *NOT* a variable; this is smallcased currencies!
		MemMove (currencies, Currencies, sizeof (currencies));
		handled = true;
		}

	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyPropApply
 *
 * DESCRIPTION: This routine applies the changes made in the CurrencyPropences
 *					 Dialog.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *
 ***********************************************************************/
static void CurrencyPropApply (void)
{
	Int16 i;
	UInt32 offset;
	Char * ptr;
	Char * textP;
	FieldPtr fld;
	CurrencyInfoPtr currency;
	ExpenseAppInfoPtr appInfoP;
			
	appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
	currency = &appInfoP->currencies[CurrentCustomCurrency];
	
	// Set the country, symbol, and exchange rate fields to the 
	// values in the application info block.
	for (i = 0; i < 2; i++)
		{
		switch (i)
			{
			case 0:
				fld = GetObjectPtr (CurrencyPropCountryField);
				ptr = currency->country;
				break;
				
			case 1:
				fld = GetObjectPtr (CurrencyPropSymbolField);
				ptr = currency->symbol;
				break;
			}


		if (FldDirty (fld))
			{
			textP = FldGetTextPtr (fld);
			offset = ptr - (Char *) appInfoP;
			DmWrite (appInfoP, offset, textP, StrLen(textP)+1);
			}
		}

	MemPtrUnlock (appInfoP);	


	// If the custom currency appears in the list of popup currencies
	// an its name is blank then remove it from the list.
	fld = GetObjectPtr (CurrencyPropCountryField);
	textP = FldGetTextPtr (fld);
	if (textP != NULL && ! *textP)
		{
		for (i = 0; i < maxUserCustomCurrencies; i++)
			{
			if (Currencies[i] == (CurrentCustomCurrency + currencyCustomUser1))
				Currencies[i] = currencyNone;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyPropInit
 *
 * DESCRIPTION: This routine initializes the Currency Properties
 *              Dialog Box.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *
 ***********************************************************************/
static void CurrencyPropInit (void)
{
	Int16 i;
	UInt16 len;
	Char * ptr;
	Char * textP;
	MemHandle textH;
	FieldPtr fld;
	CurrencyInfoPtr currency;
	ExpenseAppInfoPtr appInfoP;
			
	appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
	currency = &appInfoP->currencies[CurrentCustomCurrency];
	
	// Set the country, symbol, and exchange rate fields to the 
	// values in the application info block.
	for (i = 0; i < 2; i++)
		{
		switch (i)
			{
			case 0:
				fld = GetObjectPtr (CurrencyPropCountryField);
				ptr = currency->country;
				break;
				
			case 1:
				fld = GetObjectPtr (CurrencyPropSymbolField);
				ptr = currency->symbol;
				break;
			}

		if (*ptr)
			{
			len = StrLen (ptr) + 1;
			textH = MemHandleNew (len);
			textP = MemHandleLock (textH);
			MemMove (textP, ptr, len);
			FldSetTextHandle (fld, textH);
			MemHandleUnlock (textH);
			}
		}

	MemPtrUnlock (appInfoP);	
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyPropHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Currency
 *              Properties Dialog Box.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *
 ***********************************************************************/
Boolean CurrencyPropHandleEvent (EventType * event)
{
	FormPtr frm;
	Boolean handled = false;

	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case CurrencyPropOkButton:
				CurrencyPropApply ();
				FrmGotoForm (CurrencyDialog);
				handled = true;
				break;

			case CurrencyPropCancelButton:
				FrmGotoForm (CurrencyDialog);
				handled = true;
				break;
				
			}
		}

	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		CurrencyPropInit ();
		FrmDrawForm (frm);

#if 0
		// DOLATER - figure out if this is a Japanese-specific bug fix or should
		// be rolled into Palm OS 3.5.
		FrmSetFocus (frm, FrmGetObjectIndex (frm, CurrencyPropCountryField));
			{
			FieldPtr fld = GetObjectPtr (CurrencyPropCountryField);
			if (fld != NULL)
				{
				FldGrabFocus (fld);
				FldSetSelection (fld, 0, FldGetTextLength (fld));
				}
			}
#endif

		handled = true;
		}

	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyInit
 *
 * DESCRIPTION: This routine initializes the Currency Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *
 ***********************************************************************/
static void CurrencyInit (void)
{
	Int16 i;
	UInt16 len;
	Char * rscP;
	Char * label;
	ControlPtr ctl;
	CurrencyInfoPtr currencies;
	ExpenseAppInfoPtr appInfoP;
			
	appInfoP = MemHandleLock (ExpenseGetAppInfo (ExpenseDB));
	currencies = appInfoP->currencies;

	// Set the labels the the currency selectors
	for (i = 0 ; i < maxUserCustomCurrencies; i++)
		{
		ctl = GetObjectPtr (CurrencyCountry1Selector + i);
		label = (Char *)CtlGetLabel (ctl);	// ok to cast, we call CtlSetLabel

		if (currencies[i].country[0] != 0)
			StrCopy (label, currencies[i].country);

		// If the name is blank display "Country 1" or "Country 2" or ...
		else
			{
			rscP = MemHandleLock (DmGetResource (strRsc, countryStrID));
			len = StrLen (rscP);
			if (len > maxCurrencyNameLen - 2) 
				len = maxCurrencyNameLen - 2;
			StrNCopy (label, rscP, len);
			label [len++] = i + '1';
			label [len] = 0;
			MemPtrUnlock (rscP);
			}

		CtlSetLabel (ctl, label);
		}

	MemPtrUnlock (appInfoP);	
}


/***********************************************************************
 *
 * FUNCTION:    CurrencyHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Custom
 *              Currencies Dialog Box
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/27/96	Initial Revision
 *
 ***********************************************************************/
Boolean CurrencyHandleEvent (EventType * event)
{
	FormPtr frm;
	Boolean handled = false;

	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case CurrencyOkButton:
				FrmReturnToForm (0);
				FrmUpdateForm (0, updateCustomCurrencyChanged);
				handled = true;
				break;

			case CurrencyCancelButton:
				FrmReturnToForm (0);
				handled = true;
				break;
				
			case CurrencyCountry1Selector:
			case CurrencyCountry2Selector:
			case CurrencyCountry3Selector:
			case CurrencyCountry4Selector:
				CurrentCustomCurrency = event->data.ctlSelect.controlID - 
					CurrencyCountry1Selector;
				FrmGotoForm (CurrencyPropDialog);
				break;
			}
		}

	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		CurrencyInit ();
		FrmDrawForm (frm);
		handled = true;
		}

	return (handled);
}

