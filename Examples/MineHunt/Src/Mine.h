/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Mine.h
 *
 * Description:
 *		This is the MineHunt application's header file.
 *
 * History:
 *		October 8, 1995	Created by Vitaly Kruglikov
 *
 *****************************************************************************/

#ifndef __MINE_H__
#define __MINE_H__

#include <PalmTypes.h>


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define mineAppVersionNum		0x0101
#define mineAppCreator			'mine'

#define version20					0x02000000


#define maxRows				10
#define maxColumns			11


/***********************************************************************
 *
 *	Internal Structures
 *
 ***********************************************************************/
typedef struct PieceCoordType {
	Int16		row;							// 0-based
	Int16		col;							// 0-based
	} PieceCoordType;


// States of a game piece
enum {
	covered = 0,
	markedMine = 1,
	markedUnknown = 2,
	uncovered = 3
	};
	
typedef struct GamePieceType {
	unsigned mined : 1;				// if set, there is a mine at this position
	unsigned state : 2;				// covered, uncovered, markedMine, markedUnknown
	unsigned neighbours: 4;			// number of neighbours
	unsigned mark : 1;				// used for computing which pieces to uncover
											// automatically
	} GamePieceType;


// States of the game
typedef enum GameStateType {
	gameInProgress,
	gameWon,
	gameDead
	} GameStateType;

typedef struct GameType {
	Boolean			restored;		// if true, the game was restored from preferences
	PointType		origin;			// board origin (x,y)
	UInt8				numRows;			// number of rows
	UInt8				numCol;			// number of columns
	UInt8				numMines;		// number of mines
	Int16				minesLeft;		// number of mines left to uncover
	GameStateType	state;			// game state
	PieceCoordType	deadCoord;		// coordinate of piece which killed the game
	GamePieceType	piece[maxRows][maxColumns];	// game pieces
	} GameType;

typedef enum SquareGraphicType {
	coveredSquareGraphic = 0,
	markedUnknownSquareGraphic,
	markedMineSquareGraphic,
	mineSquareGraphic,
	notMineSquareGraphic,
	emptySquareGraphic,
	lastSquareGraphic
	} SquareGraphicType;

//
// App preferences structures
//

// Game difficulty
typedef enum DifficultyType {
	difficultyEasy = 0,				// must start at zero for mapping tables to work correctly
	difficultyIntermediate,
	difficultyMoreDifficult,
	difficultyImpossible
	} DifficultyType;

#define defaultDifficulty		difficultyIntermediate;
	
// Preferences resource type
typedef struct MinePrefType {
	UInt32				signature;		// signature for preferences resource validation
	GameType			game;				// saved game info
	DifficultyType	difficulty;		// difficulty level
	} MinePrefType;
	
#define	minePrefSignature	'MiNe'

#endif	// __MINE_H__

