/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpCurrency.h
 *
 * Description:
 *	  This file defines the structures and routines to support managing
 *   curency information.
 *
 * History:
 *		August 27, 1996	Created by Art Lamb
 *
 *****************************************************************************/

extern Boolean CurrencyHandleEvent (EventType * event);

extern Boolean CurrencyPropHandleEvent (EventType * event);

extern Boolean CurrencySelect (ListPtr lst, UInt16 * currencyP, Char * currencyName);

extern void CurrencyGetSymbol (UInt16 currency, Char * symbol);

extern Boolean CurrencySelectHandleEvent (EventType * event);

extern Char CurrencyGetDecimalSeperator (UInt16 currency);

extern UInt16 CurrencyGetDecimalPlaces (UInt16 currency);

