/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MineApp.c
 *
 * Description:
 *	  This is the main source module for the MineHunt game.
 *
 * History:
 *		October 8, 1995	Created by Vitaly Kruglikov
 *
 *****************************************************************************/

#include <PalmOS.h>

#include "Mine.h"
#include "MineRsc.h"

/***********************************************************************
 *
 *	Entry Points
 *
 ***********************************************************************/


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/

#define numPieceBitmaps		6			// number of different bitmaps used to represent game pieces

#define pieceBitmapWidth		15
#define pieceBitmapHeight		15
#define pieceBitmapXSpacing	-1		// negative indicates overlap
#define pieceBitmapYSpacing	-1		// negative indicates overlap

#define pieceFontID			boldFont

// Number of moves the game board is shuffled
#define numShuffleMoves		700


// Mines left display bounds and font
#define mineCountLeft		74
#define mineCountTop			1
#define mineCountWidth		20
#define mineCountHeight		10
#define mineCountFontID		boldFont


/***********************************************************************
 *
 *	Internal Structures
 *
 ***********************************************************************/


/***********************************************************************
 *
 *	Private global variables
 *
 ***********************************************************************/
static GameType		Game;
static MinePrefType	MinePref;

static WinHandle		OffscreenBoardWinH = 0;
static WinHandle		OffscreenBitmapWinH = 0;
static Int16			PieceBitmapTable[lastSquareGraphic] =
		{
		CoveredSquareBitmap,
		MarkedUnknownSquareBitmap,
		MarkedMineSquareBitmap,
		MineSquareBitmap,
		NotMineSquareBitmap,
		EmptySquareBitmap
		};

static UInt16 SoundAmp;		// default sound amplitude


/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
static UInt16 StartApplication (void);
static void StopApplication (void);
static Boolean MainFormDoCommand (UInt16 command);
static void MainFormInit (FormPtr frm);
static Boolean MainFormHandleEvent (EventPtr event);
static Boolean CurrentFormHandleEvent (EventPtr event);
static Boolean ChangePreferences(void);
static Boolean PromptToStartNewGame(void);


static void		DrawGameBoard(void);
static void		UpdateMinesLeftDisplay(void);
static Boolean	HandlePenDown(Int16 penX, Int16 penY, Boolean marking, Boolean * inBoundsP);
static void		DeadlyMove(void);
static void		Victory(void);
static Boolean	CheckIfWon(void);
static Boolean	MapPenPosition(Int16 penX, Int16 penY, PieceCoordType* coordP);
static void		DrawPiece(WinHandle dstWinH, Int16 row, Int16 col);
static void		DrawBitmap(WinHandle winHandle, Int16 resID, Int16 x, Int16 y);
static void		GetPieceRectangle(Int16 row, Int16 col, RectanglePtr rP);
static void		NewGameBoard(UInt32 moves);
static void		SaveGameBoard(void);
static void		LoadGameBoard(void);


/***********************************************************************
 *
 * FUNCTION:     NewGameBoard
 *
 * DESCRIPTION:	Shuffle the game board.
 *
 * PARAMETERS:	moves		-- number of moves to shuffle
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void NewGameBoard(UInt32 moves)
{
	UInt16		randRow, randCol;
	UInt16		i, j;
	GamePieceType	tempPiece;
	UInt16		numMines;
	// Mapping from difficulty level to the number of mines
	Int16		difficultyToNumMinesMapping[] =	{
															10,		// difficultyEasy
															20,		// difficultyIntermediate
															30,		// difficultyMoreDifficult
															60			// difficultyImpossible
															};
	
	// Initialize the board
	Game.restored = false;
	Game.numRows = maxRows;
	Game.numCol = maxColumns;
	Game.numMines = Game.minesLeft = difficultyToNumMinesMapping[MinePref.difficulty];
	Game.state = gameInProgress;
	Game.origin.x = 2;
	Game.origin.y = 17;
	
	numMines = Game.numMines;
	
	for ( i=0; i < Game.numRows; i++ )
		for ( j=0; j < Game.numCol; j++ )
			{
			Game.piece[i][j].mined = numMines ? (numMines--,1) : 0;
			Game.piece[i][j].state = covered;
			Game.piece[i][j].neighbours = 0;
			}
	
	// Shuffle the pieces
	do	{
		for ( i=0; i < Game.numRows; i++ )
			for ( j=0; j < Game.numCol; j++ )
				{
				randRow = (UInt16)SysRandom( 0 ) % Game.numRows;
				randCol = (UInt16)SysRandom( 0 ) % Game.numCol;
				tempPiece = Game.piece[i][j];
				Game.piece[i][j] = Game.piece[randRow][randCol];
				Game.piece[randRow][randCol] = tempPiece;
				if ( !(--moves) )
					goto Done;
				}
		}
	while ( true );
	
Done:

	// Count the neighbours
	for ( i=0; i < Game.numRows; i++ )
		for ( j=0; j < Game.numCol; j++ )
			{
			if ( !Game.piece[i][j].mined )
				continue;
			
			// Take care of the 3 neighbours to the left
			if ( j > 0 )
				{
				Game.piece[i][j-1].neighbours += 1;
				if ( i > 0 )
					Game.piece[i-1][j-1].neighbours += 1;
				if ( i < (Game.numRows - 1) )
					Game.piece[i+1][j-1].neighbours += 1;
				}
				
			// Take care of the 3 neighbours to the right
			if ( j < (Game.numCol - 1) )
				{
				Game.piece[i][j+1].neighbours += 1;
				if ( i > 0 )
					Game.piece[i-1][j+1].neighbours += 1;
				if ( i < (Game.numRows - 1) )
					Game.piece[i+1][j+1].neighbours += 1;
				}
			
			// Take care of the neighbour above
			if ( i > 0 )
				Game.piece[i-1][j].neighbours += 1;
			
			// Take care of the neighbour below
			if ( i < (Game.numRows - 1) )
				Game.piece[i+1][j].neighbours += 1;
				
			}	// for ( j=0; j < Game.numCol; j++ )


	return;
}


/***********************************************************************
 *
 * FUNCTION:     LoadGameBoard
 *
 * DESCRIPTION:	Load saved game from app preferences.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void LoadGameBoard(void)
{
	Boolean	found;
	
	// Try loading a saved game board - if that fails, generate a new one
	found = PrefGetAppPreferencesV10 (mineAppCreator, mineAppVersionNum,
			&MinePref, sizeof(MinePref) );
	if ( found && MinePref.signature == minePrefSignature && MinePref.game.state == gameInProgress)
		{
		Game = MinePref.game;
		Game.restored = true;
		}
	else
		{
		MinePref.difficulty = defaultDifficulty;
		NewGameBoard( numShuffleMoves );
		}
}



/***********************************************************************
 *
 * FUNCTION:     SaveGameBoard
 *
 * DESCRIPTION:	Save game in app preferences.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void SaveGameBoard(void)
{
	MinePref.signature = minePrefSignature;
	MinePref.game = Game;

	// Save our preferences to the Preferences database
	PrefSetAppPreferencesV10( mineAppCreator, mineAppVersionNum, &MinePref,
			sizeof(MinePref) );
}


/***********************************************************************
 *
 * FUNCTION:     HandlePenDown
 *
 * DESCRIPTION:	Handles a pen down;
 *
 * PARAMETERS:	penX			-- display-relative x position
 *					penY			-- display-relative y position
 *					marking		-- if true, pen down on game pieces is
 *									   interpreted as marking
 *					inBoundsP	-- if pen landed in game board bounds, *inBoundsP
 *										will be set to true, otherwise to false
 *
 * RETURNED:	true if handled; false if not
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static Boolean HandlePenDown(Int16 penX, Int16 penY, Boolean marking, Boolean * inBoundsP)
{
	PieceCoordType		coord;
	UInt16					i, j;
	GamePieceType		piece, *pieceP;
	Int16					savedMinesLeft;
	Boolean				deadlyMove = false;
	Boolean				victory = false;

	
	// Map pen position to game board row and column
	*inBoundsP = MapPenPosition( penX, penY, &coord );

	
	// If not in game board bounds or game is over, bail out
	if ( !(*inBoundsP) || Game.state != gameInProgress )
		return( false );
	
	// Save mines left value for update optimization
	savedMinesLeft = Game.minesLeft;
	
	piece = Game.piece[coord.row][coord.col];
	pieceP = &Game.piece[coord.row][coord.col];

	// Nothing to do if we're already uncovered		
	if ( piece.state == uncovered )
		return( true );
	
	// If marking
	if ( marking )
		{
		if ( piece.state == covered )
			{
			pieceP->state = markedMine;
			Game.minesLeft--;
			}
		else
		if ( piece.state == markedMine )
			{
			pieceP->state = markedUnknown;
			Game.minesLeft++;
			}
		else
		if ( piece.state == markedUnknown )
			pieceP->state = covered;
		}
	
	// Otherwise, if marked as a mine
	else if ( pieceP->state == markedMine )
		{
		return( true );				// protect from accidental explosion
		}
	
	// Otherwise, uncover the piece
	else
		{
		pieceP->state = uncovered;
		
		if ( pieceP->mined )
			deadlyMove = true;
		
		// If piece is not mined and has no mined neighbours, automatically
		// uncover any neighbouring pieces which are not mined and
		// which are neighbours of other uncovered pieces without
		// mined neighbours. (there must be a better way to explain this,
		// but everything in its own time).
		// 
		if ( pieceP->neighbours == 0 && !pieceP->mined )
			{
			Boolean somethingMarked;
			// Clear marks
			for ( i=0; i < Game.numRows; i++ )
				for ( j=0; j < Game.numCol; j++ )
					{
					Game.piece[i][j].mark = 0;
					}
			
			pieceP->mark = 1;
			
			do	{
				somethingMarked = false;
				for ( i=0; i < Game.numRows; i++ )
					for ( j=0; j < Game.numCol; j++ )
						{
						pieceP = &Game.piece[i][j];
						if ( pieceP->mark || pieceP->mined ||
								pieceP->state == uncovered ) 
							continue;

						// Look at the 3 neighbours to the left
						if ( j > 0 )
							{
							if ( Game.piece[i][j-1].mark &&
									!Game.piece[i][j-1].neighbours )
								goto Redraw;
							if ( i > 0 && Game.piece[i-1][j-1].mark &&
									!Game.piece[i-1][j-1].neighbours )
								goto Redraw;
							if ( i < (Game.numRows - 1) && Game.piece[i+1][j-1].mark &&
									!Game.piece[i+1][j-1].neighbours )
								goto Redraw;
							}
							
						// Take care of the 3 neighbours to the right
						if ( j < (Game.numCol - 1) )
							{
							if ( Game.piece[i][j+1].mark &&
									!Game.piece[i][j+1].neighbours )
								goto Redraw;
							if ( i > 0 && Game.piece[i-1][j+1].mark &&
									!Game.piece[i-1][j+1].neighbours )
								goto Redraw;
							if ( i < (Game.numRows - 1) && Game.piece[i+1][j+1].mark &&
									!Game.piece[i+1][j+1].neighbours )
								goto Redraw;
							}
						
						// Take care of the neighbour above
						if ( i > 0 && Game.piece[i-1][j].mark &&
									!Game.piece[i-1][j].neighbours )
							goto Redraw;
						
						// Take care of the neighbour below
						if ( i < (Game.numRows - 1) && Game.piece[i+1][j].mark &&
									!Game.piece[i+1][j].neighbours )
							goto Redraw;

						continue;
						
Redraw:
						pieceP->mark = 1;
						if ( pieceP->state == markedMine )	// If it was marked as a mine, it was wrong!
							Game.minesLeft++;						// Bump minesLeft counter before we uncover!
						pieceP->state = uncovered;
						DrawPiece( 0, i, j );
						somethingMarked = true;
						
						}	//
				}
			while( somethingMarked );
			
			} // If piece is not mined and has no mined neighbours...
			
		} // Otherwise, uncover the piece
	
	
	// Check if we died or won
	if ( deadlyMove )
		{
		Game.state = gameDead;
		Game.deadCoord = coord;
		}
	else if ( (victory = CheckIfWon()) != false )
		Game.state = gameWon;
		
	// Draw the piece which was tapped
	DrawPiece( 0, coord.row, coord.col );
	
	// Update mines left display
	if ( Game.minesLeft != savedMinesLeft )
		UpdateMinesLeftDisplay();

	
	// Check for end of game
	if ( deadlyMove )
		DeadlyMove();
	else if ( victory )
		Victory();

	return( true );
}


/***********************************************************************
 *
 * FUNCTION:     DeadlyMove
 *
 * DESCRIPTION:	Uncovers all mined pieces which are not marked mined
 *						and puts up the "end of game" dialog.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *			roger	1/9/97	Obey the system game sound setting
 *
 ***********************************************************************/
static void DeadlyMove(void)
{
	UInt16		i, j;
	//RectangleType	rect;
	GamePieceType*	pieceP;
	SndCommandType sndCmd;
	
	#if 0		// CASTRATION
	// Invert the game for visual effect
	rect.topLeft.x = Game.origin.x;
	rect.topLeft.y = Game.origin.y;
	rect.extent.x =  (Game.numCol * pieceBitmapWidth) +
			((Game.numCol) * pieceBitmapXSpacing);
	rect.extent.y =  (Game.numRows * pieceBitmapHeight) +
			((Game.numRows) * pieceBitmapYSpacing);
	WinInvertRectangle ( &rect, 0/*cornerDiam*/ );
	#endif
	
	// Play error sound
	sndCmd.cmd = sndCmdFreqDurationAmp;
	sndCmd.param1 = 500;
	sndCmd.param2 = 70;
	sndCmd.param3 = SoundAmp;
	SndDoCmd( 0, &sndCmd, true);

	// Uncover all mined pieces which are not marked as mined
	for ( i=0; i < Game.numRows; i++ )
		for ( j=0; j < Game.numCol; j++ )
			{
			pieceP = &Game.piece[i][j];
			if ( pieceP->mined && pieceP->state != markedMine )
				pieceP->state = uncovered;
			}
	DrawGameBoard();
	// Display the "end of game dialog"
	//FrmAlert( GameLostAlert );
}


/***********************************************************************
 *
 * FUNCTION:     Victory
 *
 * DESCRIPTION:	Displays the "victory" dialog box.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void Victory(void)
{
	
	// Display the "end of game dialog"
	FrmAlert( GameWonAlert );
}


/***********************************************************************
 *
 * FUNCTION:     CheckIfWon
 *
 * DESCRIPTION:	Check if the game has been won.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	true if won.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static Boolean CheckIfWon(void)
{
	UInt16			i, j;
	GamePieceType*	pieceP;
	
	
	// If we find a mined piece which is not marked as "mined" or
	// any other piece which is not uncovered, the game is not won
	for ( i=0; i < Game.numRows; i++ )
		for ( j=0; j < Game.numCol; j++ )
			{
			pieceP = &Game.piece[i][j];
			if ( (pieceP->mined && pieceP->state != markedMine) ||
					(!pieceP->mined && pieceP->state != uncovered) )
				return( false );
			}
	
	return( true );
}

/***********************************************************************
 *
 * FUNCTION:     MapPenPosition
 *
 * DESCRIPTION:	Map a screen-relative pen position to a game piece
 *						coordinate;
 *
 * PARAMETERS:	penX		-- display-relative x position
 *					penY		-- display-relative y position
 *					coordP	-- MemPtr to store for coordinate
 *
 * RETURNED:	true if the pen went down on a valid game piece; false if not
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static Boolean MapPenPosition(Int16 penX, Int16 penY, PieceCoordType* coordP)
{
	Int16		x;
	Int16		y;
	RectangleType	rect;
	
	// Map display relative coordinates to window-relative
	x = (Int16)penX;
	y = (Int16)penY;
	WinDisplayToWindowPt( &x, &y );
	
	rect.topLeft.x = Game.origin.x - pieceBitmapXSpacing;
	rect.topLeft.y = Game.origin.y - pieceBitmapYSpacing;
	rect.extent.x = (Game.numCol * pieceBitmapWidth) +
			((Game.numCol+1) * pieceBitmapXSpacing);
	rect.extent.y = (Game.numRows * pieceBitmapHeight) +
			((Game.numRows+1) * pieceBitmapYSpacing);
	
	if ( !RctPtInRectangle(x, y, &rect) )
		return( false );
	
	// Convert to board-relative coordinates
	x -= Game.origin.x;
	y -= Game.origin.y;
	
	if ( x < 0 )
		{
		ErrDisplay( "board x is negative" );
		x = 0;
		}
	if ( y < 0 )
		{
		ErrDisplay( "board y is negative" );
		y = 0;
		}
	
	// Get the piece position
	coordP->col = x / (pieceBitmapWidth + pieceBitmapXSpacing);
	coordP->row = y / (pieceBitmapHeight + pieceBitmapYSpacing);
	if ( coordP->col > (Game.numCol - 1) )
		{
		ErrDisplay( "column out of bounds" );
		coordP->col = Game.numCol - 1;
		}
	if ( coordP->row > (Game.numRows - 1) )
		{
		ErrDisplay( "row out of bounds" );
		coordP->row = Game.numRows - 1;
		}
	
	return( true );
}


/***********************************************************************
 *
 * FUNCTION:     DrawPiece
 *
 *		DOLATER... add real graphics and optimize routine for size
 *
 * DESCRIPTION:	Draw a game piece
 *
 * PARAMETERS:	dstWinH	-- destination window MemHandle (0 for current window)
 *					row		-- piece row (0-based)
 *					col		-- piece column (0-based)
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void DrawPiece(WinHandle dstWinH, Int16 row, Int16 col)
{
	RectangleType	rect;
	FontID			oldFontID;
	GamePieceType	piece;
	Int16				bitmapID = 0;
	Int16				bitmapIndex = 0;

	// Get bounds of the game piece
	GetPieceRectangle( row, col, &rect );
		
	//
	// Draw the new piece
	//
	piece = Game.piece[row][col];

	// Determine which bitmap to use
	switch ( piece.state )
		{
		case covered:
			bitmapIndex = coveredSquareGraphic;
			break;
		case markedUnknown:
			bitmapIndex = markedUnknownSquareGraphic;
			break;
		case markedMine:
			if ( Game.state != gameDead || piece.mined )
				bitmapIndex = markedMineSquareGraphic;
			else
				bitmapIndex = notMineSquareGraphic;
			break;
		case uncovered:
			bitmapIndex = (piece.mined ? mineSquareGraphic : emptySquareGraphic);
			break;
		default:
			ErrDisplay( "unhandled game piece state" );
			break;
		}
	bitmapID = PieceBitmapTable[bitmapIndex];
	
	// Draw the bitmap

	// Optimization
	if ( OffscreenBitmapWinH )
		{
		RectangleType	srcRect;
		srcRect.topLeft.x = pieceBitmapWidth * bitmapIndex;
		srcRect.topLeft.y = 0;
		srcRect.extent.x = pieceBitmapWidth;
		srcRect.extent.y = pieceBitmapHeight;
		WinCopyRectangle ( OffscreenBitmapWinH/*srcWin*/, dstWinH/*dstWin*/, &srcRect,
						rect.topLeft.x, rect.topLeft.y, winPaint );
		}
	else
		DrawBitmap( dstWinH, bitmapID, rect.topLeft.x, rect.topLeft.y );
	
	// If this is an un-mined uncovered square, draw the neighbour count
	if ( bitmapID == EmptySquareBitmap && piece.neighbours )
		{
		Char		text[32];
		UInt16		textLen;
		Int16		textHeight, textWidth;
		Int16		x, y;
		WinHandle currDrawWindow;
		
		// Set a new draw window, saving the current one
		if ( dstWinH )
			currDrawWindow = WinSetDrawWindow ( dstWinH );
			
		oldFontID = FntSetFont( pieceFontID );			// change font
		StrIToA( text, piece.neighbours );
		textLen = StrLen( text );
		textHeight = FntLineHeight();
		textWidth = FntCharsWidth( text, textLen );
		x = rect.topLeft.x + ((rect.extent.x - textWidth) / 2);
		y = rect.topLeft.y + ((rect.extent.y - textHeight) /2 );
		WinDrawChars( text, textLen, x, y);
		FntSetFont( oldFontID );							// restore original font
		
		// Restore the original draw window
		if ( dstWinH )
			WinSetDrawWindow ( currDrawWindow );
		}

}

/***********************************************************************
 *
 * FUNCTION:	DrawBitmap
 *
 * DESCRIPTION:	Convenience routine to draw a bitmap at specified location
 *
 * PARAMETERS:	winHandle	-- MemHandle of window to draw to (0 for current window)
 *					resID			-- bitmap resource id
 *					x, y			-- bitmap origin relative to current window
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/9/95	Initial Revision
 *
 ***********************************************************************/
static void DrawBitmap(WinHandle winHandle, Int16 resID, Int16 x, Int16 y)
{
	MemHandle		resH;
	BitmapPtr	resP;
	WinHandle currDrawWindow;
	
	// If passed a non-null window MemHandle, set it up as the draw window, saving
	// the current draw window
	if ( winHandle )
		currDrawWindow = WinSetDrawWindow( winHandle );
		
	resH = DmGetResource( bitmapRsc, resID );
	ErrFatalDisplayIf( !resH, "no bitmap" );
	resP = MemHandleLock(resH);
	WinDrawBitmap (resP, x, y);
	MemPtrUnlock(resP);
	DmReleaseResource( resH );
	
	// Restore the current draw window
	if ( winHandle )
		WinSetDrawWindow( currDrawWindow );
}


/***********************************************************************
 *
 * FUNCTION:     GetPieceRectangle
 *
 * DESCRIPTION:	Get the rectangle coordinates of a game piece
 *
 * PARAMETERS:	row		-- piece row (0-based)
 *					col		-- piece column (0-based)
 *					rP			-- MemPtr to rectangle structure
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/11/95	Initial Version
 *
 ***********************************************************************/
static void GetPieceRectangle(Int16 row, Int16 col, RectanglePtr rP)
{
	
	ErrFatalDisplayIf( row >= Game.numRows, "bad row" );
	ErrFatalDisplayIf( col >= Game.numCol, "bad column" );
	ErrFatalDisplayIf( !rP, "bad param" );
	
	rP->topLeft.x = Game.origin.x +
			(pieceBitmapWidth + pieceBitmapXSpacing) * col;
	rP->topLeft.y = Game.origin.y +
			(pieceBitmapHeight + pieceBitmapYSpacing) * row;
	rP->extent.x = pieceBitmapWidth;
	rP->extent.y = pieceBitmapHeight;
}


/***********************************************************************
 *
 * FUNCTION:     DrawGameBoard
 *
 * DESCRIPTION:	Draw the game board.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void DrawGameBoard(void)
{
	Int16				i, j;
	PointType		savedOrigin;
	
	UpdateMinesLeftDisplay();

	//
	// Draw the game pieces -- If the off-screen board window exists,
	// the drawing will happen on the off-screen window, in which case we
	// will copy it to the on-screen baord window(appearance optimization).
	// If, on the other hand, the off-screen window was not created, the drawing
	// will take place directly on the on-screen window 
	//
	
	// For off-screen drawing, set board origin to (0,0), saving the old origin for later restoration
	if ( OffscreenBoardWinH )
		{
		savedOrigin = Game.origin;
		Game.origin.x = Game.origin.y = 0;
		}

	for ( i=0; i < Game.numRows; i++ )
		for ( j=0; j < Game.numCol; j++ )
			DrawPiece( OffscreenBoardWinH, i, j );
	
	// Check if image needs to be copied from off-screen window to the active window
	if ( OffscreenBoardWinH )
		{
		RectangleType	srcRect;
		
		// Restore the game's real origin
		Game.origin = savedOrigin;
		
		srcRect.topLeft.x = 0;
		srcRect.topLeft.y = 0;
		srcRect.extent.x =  (Game.numCol * pieceBitmapWidth) +
				((Game.numCol) * pieceBitmapXSpacing) - pieceBitmapXSpacing;
		srcRect.extent.y =  (Game.numRows * pieceBitmapHeight) +
				((Game.numRows) * pieceBitmapYSpacing) - pieceBitmapYSpacing;
		WinCopyRectangle ( OffscreenBoardWinH/*srcWin*/, 0/*dstWin*/, &srcRect,
						Game.origin.x, Game.origin.y, winPaint );
		}
}


/***********************************************************************
 *
 * FUNCTION:     UpdateMinesLeftDisplay
 *
 * DESCRIPTION:	Update the number of mines left display.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	4/16/96	Initial Version
 *
 ***********************************************************************/
static void UpdateMinesLeftDisplay(void)
{
	Char		text[32];
	Char *	cP;
	UInt16		textLen;
	//Int16		textHeight, textWidth;
	Int16		x, y;
	FontID			oldFontID;
	RectangleType	r;
	Int16		absMinesLeft;


	// Erase the old display, first
	r.topLeft.x = mineCountLeft;
	r.topLeft.y = mineCountTop;
	r.extent.x = mineCountWidth;
	r.extent.y = mineCountHeight;
	WinEraseRectangle( &r, 0 );
	
	oldFontID = FntSetFont( mineCountFontID );			// change font
	
	cP = text;
	absMinesLeft = Game.minesLeft;
	if ( absMinesLeft < 0 )
		{
		absMinesLeft = -absMinesLeft;
		*cP = '-';
		cP++;
		}
	StrIToA( cP, absMinesLeft );
	textLen = StrLen( text );
	
	#if 0
	textHeight = FntLineHeight();
	textWidth = FntCharsWidth( text, textLen );
	x = rect.topLeft.x + ((rect.extent.x - textWidth) / 2);
	y = rect.topLeft.y + ((rect.extent.y - textHeight) /2 );
	#endif
	
	x = mineCountLeft;
	y = mineCountTop;
	WinDrawChars( text, textLen, x, y);
	
	FntSetFont( oldFontID );							// restore original font
}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:	Initialize application.
 *
 *						Initialize random number generator
 *						Create and initialize offscreen window for board drawing optimization
 *						Load board from preferences, or generate a new board
 *
 * PARAMETERS:   none
 *
 * RETURNED:     0 on success
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static UInt16 StartApplication (void)
{
	UInt16	error;

	// Initialize the random number seed;
	SysRandom( TimGetSeconds() );

	LoadGameBoard();
	
	// Initialize offscreen bitmap window once
	// (used for drawing optimization - this significantly speeds up mass redraws
	// such as during during initial draw, board restoraration, and screen update)
	if ( !OffscreenBitmapWinH )
		{
		Int16	i;
		OffscreenBitmapWinH = WinCreateOffscreenWindow (
				pieceBitmapWidth * lastSquareGraphic,
				pieceBitmapHeight,
				screenFormat, &error );
		if ( OffscreenBitmapWinH )
			{
			for ( i=0; i < lastSquareGraphic; i++ )
				{
				DrawBitmap( OffscreenBitmapWinH, PieceBitmapTable[i], (pieceBitmapWidth * i)/*x*/,
						0/*y*/ );
				}
			}
		}
	
	if ( !OffscreenBoardWinH )
		{
		OffscreenBoardWinH = WinCreateOffscreenWindow (
				(Game.numCol * pieceBitmapWidth) + ((Game.numCol) * pieceBitmapXSpacing) + 1,
				(Game.numRows * pieceBitmapHeight) + ((Game.numRows) * pieceBitmapYSpacing) + 1,
				screenFormat, &error );
		}
	

	return( 0 );
}


/***********************************************************************
 *
 * FUNCTION:	StopApplication
 *
 * DESCRIPTION:	Save the current state of the application, close all
 *						forms, and deletes the offscreen window.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void StopApplication (void)
{
	SaveGameBoard();
	FrmCloseAllForms ();
	
	// Free the offscreen window
	if ( OffscreenBitmapWinH )
		{
		WinDeleteWindow( OffscreenBitmapWinH, false/*eraseIt*/ );
		OffscreenBitmapWinH = 0;
		}

	// Free the offscreen window
	if ( OffscreenBoardWinH )
		{
		WinDeleteWindow( OffscreenBoardWinH, false/*eraseIt*/ );
		OffscreenBoardWinH = 0;
		}

}



/***********************************************************************
 *
 * FUNCTION:	ChangePreferences
 *
 * DESCRIPTION:	Display the preferences dialog and save the new settings to
 *						be applied to the next game.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	true if the user changed preferences
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	5/13/96	Initial Version
 *
 ***********************************************************************/
static Boolean ChangePreferences(void)
{
	UInt16		buttonHit;
	FormPtr	prefFormP;
	UInt16		controlID;
	Boolean	changed = false;
	
	// Create the form
	prefFormP = FrmInitForm( PrefView );
		
	// Initialize difficulty level selection
	switch( MinePref.difficulty )
		{
		case difficultyEasy:
			controlID = PrefViewDifficultyEasyPBN;
			break;

		case difficultyIntermediate:
			controlID = PrefViewDifficultyIntermediatePBN;
			break;

		case difficultyMoreDifficult:
			controlID = PrefViewDifficultyMoreDifficultPBN;
			break;

		case difficultyImpossible:
			controlID = PrefViewDifficultyImpossiblePBN;
			break;

		default:
			controlID = PrefViewDifficultyEasyPBN;
			break;
		}
	FrmSetControlGroupSelection ( prefFormP, PrefViewDifficultyGroup, controlID );
	
	// Put up the form
	buttonHit = FrmDoDialog( prefFormP );
	
	// Collect new setting
	if ( buttonHit == PrefViewOkButton )
		{
		controlID = FrmGetObjectId( prefFormP,
				FrmGetControlGroupSelection(prefFormP, PrefViewDifficultyGroup) );
		switch ( controlID )
			{
			case PrefViewDifficultyEasyPBN:
				MinePref.difficulty = difficultyEasy;
				break;
				
			case PrefViewDifficultyIntermediatePBN:
				MinePref.difficulty = difficultyIntermediate;
				break;
				
			case PrefViewDifficultyMoreDifficultPBN:
				MinePref.difficulty = difficultyMoreDifficult;
				break;
				
			case PrefViewDifficultyImpossiblePBN:
				MinePref.difficulty = difficultyImpossible;
				break;
			}

		changed = true;													// user hit OK
		}
	
	// Delete the form
	FrmDeleteForm( prefFormP );
		
	return( changed );
}


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *                             
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/15/96	Initial Revision
 *
 ***********************************************************************/
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < 0x02000000)
				{
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return (sysErrRomIncompatible);
		}

	return (0);
}


/***********************************************************************
 *
 * FUNCTION:	PromptToStartNewGame
 *
 * DESCRIPTION:	Prompt the user to start a new game and start it if so.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	true if new game started
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	5/13/96	Initial Version
 *
 ***********************************************************************/
static Boolean PromptToStartNewGame(void)
{
	Boolean	newGame = false;
	
	// Display the new game prompt
	if ( FrmAlert(NewGameAlert) == 0 )
		{
		NewGameBoard( numShuffleMoves );
		DrawGameBoard();
		newGame = true;
		}
	
	return( newGame );
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    true if the command was handled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static Boolean MainFormDoCommand (UInt16 command)
{
	Boolean		handled = false;

	MenuEraseStatus (MenuGetActiveMenu());

	switch (command)
		{
#if 0		// removed to eliminate redundancy with New Game button
		case MainOptionsNewGameCmd:
			NewGameBoard( numShuffleMoves );
			DrawGameBoard();
			handled = true;
			break;
#endif			
		case MainOptionsHelpCmd:
			FrmHelp( GameTipsStr );
			handled = true;
			break;
			
		case MainOptionsPrefCmd:
			if ( ChangePreferences() )
				{
				PromptToStartNewGame();					// ask if the user wants to start a new game
				}
			handled = true;
			break;

		case MainOptionsAboutCmd:
			AbtShowAbout (mineAppCreator);
			handled = true;
			break;
		}
		
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: This routine initializes the "Main View"
 *
 * PARAMETERS:  frm  - a pointer to the MainForm form
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
static void MainFormInit (FormPtr /*frm*/)
{
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Main View"
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/9/95	Initial Revision
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent (EventPtr event)
{
	FormPtr frm;
	Boolean handled = false;


	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case MainFormNewGameButton:
				NewGameBoard( numShuffleMoves );
				DrawGameBoard();
				handled = true;
				break;
			
			default:
				break;
			}
		}

	else if ( event->eType == penDownEvent )
		{
		Boolean	inBounds;
				
		handled = HandlePenDown( event->screenX, event->screenY,
				((KeyCurrentState() & (keyBitPageUp | keyBitPageDown)) != 0), &inBounds );
		
		if ( !handled && inBounds )
			{
			// See if the user wants to start a new game
			PromptToStartNewGame();
			handled = true;
			}
		
		} // else if ( event->eType == penDownEvent )
	
				
	else if (event->eType == menuEvent)
		{
		return MainFormDoCommand (event->data.menu.itemID);
		}


	else if (event->eType == frmUpdateEvent)
		{
		FrmDrawForm (FrmGetActiveForm());
		DrawGameBoard();
		handled = true;
		}
	
		
	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm();
		MainFormInit (frm);
		FrmDrawForm (frm);
		DrawGameBoard();
		handled = true;
		}
	
		
	else if (event->eType == frmCloseEvent)
		{
		}

	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean AppHandleEvent( EventPtr eventP)
{
	UInt16 formId;
	FormPtr frmP;


	if (eventP->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
			{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			default:
				ErrNonFatalDisplay("Invalid Form Load Event");
				break;

			}
		return true;
		}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *		vmk	1/23/98	Restored the "else" to make sure that when up/down
 *							is held down, the event doesn't get passed to the system
 *		CSS	06/22/99	Standardized keyDownEvent handling
 *							(TxtCharIsHardKey, commandKeyMask, etc.)
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;


	do {
		EvtGetEvent(&event, evtWaitForever);
		
		
		// Swallow up/down arrow key chars to prevent the clicking sound
		// when the keys are being used as modifiers for marking cells
		if (	(event.eType == keyDownEvent)
			&&	(!TxtCharIsHardKey(	event.data.keyDown.modifiers,
											event.data.keyDown.chr))
			&&	(EvtKeydownIsVirtual(&event))
			&&	(	(event.data.keyDown.chr == vchrPageUp)
				||	(event.data.keyDown.chr == vchrPageDown)))
			{
			;		// Do nothing
			}
		
		else
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

		// Check the heaps after each event
		#if EMULATION_LEVEL != EMULATION_NONE
		MemHeapCheck(0);
		MemHeapCheck(1);
		#endif

	} while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    0
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/8/95	Initial Version
 *
 ***********************************************************************/
 
UInt32 PilotMain (UInt16 cmd, MemPtr /*cmdPBP*/, UInt16 launchFlags)
{
	UInt16 error;

	
	error = RomVersionCompatible (version20, launchFlags);
	if (error) return (error);


	if ( cmd == sysAppLaunchCmdNormalLaunch )
		{
		error = StartApplication ();
		if (error) return (error);

		FrmGotoForm (MainForm);

		AppEventLoop ();
		StopApplication ();
		}

	return (0);
}

