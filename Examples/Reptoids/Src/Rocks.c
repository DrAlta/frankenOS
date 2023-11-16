/***********************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * The code here to Reptoids, subject to two restrictions, may be freely 
 * copied and used. The first restriction is that the copyright notices
 * and restrictions that appear in the code and in the game must remain 
 * with the code and remain visible in the game.  My copyright
 * notices should appear wherever your copyright notices appear.  The 
 * second restriction is that these files may not be used to produce
 * a program to be accompanied with or contained in hardware for sale.  
 * Those wishing to do so should contact me.
 *
 * File: Rocks.c
 *
 * History:
 *	April 24, 1996	Created by Roger Flores
 *


11/11/96 version 1.0

The model below is used for animated games.  The idea is to draw the game
state at regular time periods.  To this end the first thing done when a
time period begins is the game state is drawn.  Anything that isn't a 
draw operation is performed after this.  Next, changes while the time 
period elaspses are made to the game state.  Buttons depressed by the 
user are polled and stored for those ships which are controlled by them.
All the objects are moved one game state forward.  Collisions causes objects
to be removed and sparks to generally be added for display.  All objects
marked for removal are then removed.  Everything left is then rendered
to a display buffer for quick, near constant time display at the next 
game period.  Sounds are played last because they have such a variable
time.


The flow of a time period T:

appStartEvent
	
	GameStatus.status = gameInitializing	// suspend updates

frmOpenEvent	

	GameStart
		GameInitLevel
	GameStateDraw

nT

	GameStateDraw();				// Draw the state renedered in the screen buffer
	GameStateElapse();			// Poll user input, moved objects, handle collisions
	GameStatePrepareDraw();		// Render the game state
	GamePlaySounds();				// Play the most important sound requested.
	variable time

nT + T



This code is based off of the SubHunt code which is based on the HardBall
code.  These are much simpler examples (maybe a quarter this size) and they
demonstrate the essentials for a Pilot based game.  This code is released
with two intentions.  First, having browsed so many game programming books
in stores and purchasing a few, I feel they really lack good code.  They 
generally seem so hard coded to specific cases.  This code should easily
run in different situations which leads me to the second intention.  I 
really hope this code can be used as the basis for better games which I
don't have the time to write.  Specifically I'd love to see a Time Pilot
or Bosconian type of game.  The basics of those games (animation, collision,
user input, sound, object storage) are here.  Go for it!


For comments, code suggestions and enhancements, email me at roger@palm.com.
Don't email me for feature requests.  I'm essentially done with this version
of the game but I will maintain it and answer good questions.


History

This game was originally named Rocks but I found
destroying rocks a bit, ah, unchallenging.  I enjoy killing aliens much
more!  So I switched the focus from rocks to aliens.  I added alien waves
for pure killing pleasure and I added bases to them to give me something 
tougher to pound on.  Finally I added the end level so that people could
say they "finished" the game.  I realize that people don't play these
games forever and I think it's better to have a good point after which
players can leave a game feeling satisfied.

With the focus on aliens, "Rocks" no longer was right.  After many failed
names I decided I needed an "official" name for the alien race.  That
caused me to check the alt.alien.visitors FAQ and now you know the whole
story!


General Overview

Graphics are generally all graphic bitmaps which are drawn to an
offscreen memory buffer.  The exception are sparks used for explosions
which are a single pixel each.  Positioning and motion is described
as integers with the lower four bits treated as fractional values.
No floating point or high level math operations are used by this code.
Coordinates like this are said to be in game space.  Objects have 
their top, left, bottom, and right edges stored for quicker collision
detection.  Object motion is described by a simple vector.  Screen
drawing truncates (not round) the fractional value of the coordinates.


Double Buffered Graphics

As mentioned all graphics are drawn to an offscreen memory buffer.
Specfically they are drawn with an OR operator.  Masks are not used.
As graphics are drawn to the buffer the extent of the buffer used is
recorded so the entire frame doesn't need to be drawn.  This does
save time.  The buffer is copied to the screen directly over the
existing data.  This eliminates flicker.  For the Pilot hardware
there doesn't seem to be a need to sync to the screen refreshing.
The screen routines consume about 75% of the program time.


Collision Detection

Collsion detection is based on spatial subdivision and bounding boxes.
Rocks can have a large number of object on screen at the same time.
Comparing every object to each other is O(n^2) which is too slow.
Instead a spatial subdivision is used to divided up the game space into
boxes called sectors.  When an object is checked for collisions the sector
it occupies is determined, and then collisions are checked against all
objects in the same sector.  If there isn't a collision and the object
also covers other sectors, or can touch an object in other sectors, then
those sectors are checked for colliding objects too.  

For collisions within a sector a bounding box is used for the object.
The corners are rounded a bit so all objects used must have rounded
corners.

The last thing to mention is that rocks are added before all other
objects and collsions against other objects in the same sector are
not checked for.  This eliminates many comparisons.


Sound Managment

Sound is from the simple single tone square wave sound generator.
The routine GameRequestSound is called whenever the code wants a 
sound to reflect something in the game state.  Only one sound 
can be played at once.  If the game sound being requested is of
a higher priority than the sound currently playing, the new sound
replaces the old one.  Otherwise the requested sound is ignored.
Every game period the requested game sound is played.  The game 
should generally play only a brief bit each game period.  Otherwise
the game will often start the next game period late.  This gives 
the game a "sluggish" feel.  To still keep the sounds longer enough
to be heard, sounds have a repetition count.  

Improvements to the sound code obviously would be a way to play a 
sequence of different sounds for each sound type.  Another improvement
would be to allow high priority short sounds to play, and then resume
a prior lower priority longer sound where it would be without the
interruption.


Last Comments

This code has been around a long time and is no longer fresh in my
head.  Being a part time project I've only worked on it now and then
and I've noticed some poor things slip in because of the breaks in
time.  In general I feel pretty good about the code.  I tried to get
all the bugs out for the users.  As I mentioned, if you find bugs or 
have code suggestions please notify me.



The code was edited with tabs set to four spaces.

The code style in GNU's because that what's generally used for the Pilot.

 **********************************************************************/

#include <PalmOS.h>
#include	<DLServer.h>				//	Needed for DlkGetSyncInfo

#include "RocksRsc.h"

/***********************************************************************
 *
 *	Entry Points
 *
 ***********************************************************************/

#define appFileCreator				'RPTD'
#define appPrefID						0
#define appSavedGameID				0
#define appPrefVersion				4
#define appSavedGameVersion		4

#define version20						0x02000000


/***********************************************************************
 *
 *	Internal Structures
 *
 ***********************************************************************/

#define firstLevelPlayed			0		// 0
#define numberOfPlayerLives		5		// 5


// Put the game into an auto play type of mode.  It's 
// great at finding bugs and is repeatable. 
//#define OPTION_DETERMINISTIC_PLAY
//#define DETERMINISTIC_SEED		23456

// Make the game play without waiting at the end of the game cycle.
// The purpose is to speed up OPTION_DETERMINISTIC_PLAY runs.
//#define OPTION_NO_DELAY


// List of key bindings
#define rotateLeftKey		keyBitHard1		// polled every game period
#define rotateRightKey		keyBitHard2		// polled every game period
#define shootKey				keyBitHard3		// polled every game period
#define thrustKey				keyBitHard4		// polled every game period
#define warpKey				keyBitPageUp	// polled every game period
#define keysAllowedMask		(rotateLeftKey | rotateRightKey | shootKey | thrustKey | warpKey)
#define restartGameChar		pageDownChr
#define rotateLeftKeyRepeatDelay		2		// polled every game period
#define rotateRightKeyRepeatDelay	2		// polled every game period
#define shootKeyRepeatDelay			4		// polled every game period
#define thrustKeyRepeatDelay			2		// polled every game period

// List of bitmaps
#define OverlayBmp								1000
#define shipPlayerDegree0Bitmap				0
#define shipPlayerDegree22Bitmap				1
#define shipPlayerDegree45Bitmap				2
#define shipPlayerDegree67Bitmap				3
#define shipPlayerDegree90Bitmap				4
#define shipPlayerDegree112Bitmap			5
#define shipPlayerDegree135Bitmap			6
#define shipPlayerDegree157Bitmap			7
#define shipPlayerDegree180Bitmap			8
#define shipPlayerDegree202Bitmap			9
#define shipPlayerDegree225Bitmap			10
#define shipPlayerDegree247Bitmap			11
#define shipPlayerDegree270Bitmap			12
#define shipPlayerDegree292Bitmap			13
#define shipPlayerDegree315Bitmap			14
#define shipPlayerDegree337Bitmap			15
#define shipEarthBitmap							16
#define alienShipHomeWorldBitmap				17
#define alienShipBaseBitmap					18
#define alienShipLargeBitmap					19
#define alienShipSmallBitmap					20
#define alienShipAceBitmap						21
#define rockSmallBitmap							22
#define rockMediumBitmap						23
#define rockLargeBitmap							24
#define shotNormalBitmap						25
#define shotPlasmaBitmap1						26
#define shotPlasmaBitmap2						27
#define bonusExtraShotsBitmap					28
#define bonusLongShotsBitmap					29
#define bonusRetroRocketsBitmap				30
#define bonusScoreBitmap						31
#define bonusArmorBitmap						32
#define bonusBombBitmap							33
#define gaugeLifeBitmap							34
#define gaugeArmorBitmap						35
#define gaugeArmorNoneBitmap					36
#define gaugeArmorFullBitmap					37
#define bitmapTypeCount							38


#define firstShipBitmap				shipPlayerDegree0Bitmap
#define lastShipBitmap				shipPlayerDegree337Bitmap
#define firstShotBitmap				shotNormalBitmap
#define firstRockBitmap				rockSmallBitmap
#define firstAlienBitmap			alienShipHomeWorldBitmap
#define firstBonusBitmap			bonusExtraShotsBitmap


// Object types
#define shipPlayer					0
#define shipEarth						1		// unused - was for extended ending sequence
#define shipAlienHomeWorld			2
#define shipAlienBase				3
#define shipAlienLarge				4
#define shipAlienSmall				5
#define shipAlienAce					6
#define shotNormal					7
#define shotPlasma					8
#define rockSmall						9
#define rockMedium					10
#define rockLarge						11
#define bonusExtraShots				12		// add two shots to the player
#define bonusLongShots				13		// shots travel farther
#define bonusRetroRockets			14		// a retro rocket stops the player
#define bonusScore					15		// add extra points
#define bonusArmor					16		// add extra armor
#define bonusBomb						17		// explode everything owner by other players
#define objectTypeCount				18

#define firstAlienShip				shipAlienHomeWorld
#define lastAlienShip				shipAlienAce
#define firstRockType				rockSmall
#define firstShotType				shotNormal
#define firstBonusType				bonusExtraShots
#define lastBonusType				bonusBomb

// Game space has four bits of fraction value for smoother movement rates.
#define ScreenToGame(p)				((p) * 16)
#define GameToScreen(p)				((p) / 16)
#define biggestObject				ScreenToGame(16)	// excepts planets	
#define screenWidth					160
#define screenHeight					145
#define borderAroundScreen			16		// should be as big as the biggest object.
#define gameWidth						ScreenToGame(borderAroundScreen + screenWidth)
#define gameHeight					ScreenToGame(borderAroundScreen + screenHeight)


// Board settings
#define screenTopLeftXOffset		0
#define screenTopLeftYOffset		15
#define boardWidth					gameWidth
#define boardHeight					gameHeight


// Sectors
#define sectorWidth					ScreenToGame(32)
#define sectorHeight					ScreenToGame(32)
#define sectorsHorizontally		(gameWidth / sectorWidth + 1)
#define sectorsVertically			(gameHeight / sectorHeight + 1)
#define sectorCount					(sectorsHorizontally * sectorsVertically)


// Lives gauge position
#define liveGaugeX					53
#define liveGaugeSeparator			2
#define liveGaugeY					4
#define livesDisplayable			5
#define lifeWidth						7
#define lifeHeight					7

// Score gauge position
#define scoreGaugeX					131
#define scoreGaugeY					2
#define scoreGaugeDigitsMax		5
#define scoreGaugeCharWidth		5			// width of numbers in the bold font
#define scoreGaugeWidth				(maxScoreDigits * scoreCharWidth - 1)
#define scoreGaugeFont				boldFont

// Armor gauge
#define armorGaugeX					88
#define armorGaugeY					1
#define armorGaugeFont				stdFont


#pragma mark Defines

// Shot settings
#define shotWidth						ScreenToGame(2)
#define shotHeight					ScreenToGame(2)
#define shotPlasmaWidth				ScreenToGame(5)
#define shotPlasmaHeight			ScreenToGame(5)
#define shotsMax						12			// increasing this allows aliens more shots - too hard!
#define shotDuration					20
#define shotLongDuration			40
#define shotPlasmaDuration			100
#define shotAimedVariation			(shipPlayerWidth * 3)
#define chanceToAimPoorly			20
#define chanceForPlasmas			10

// Ship settings
#define shipPlayerWidth				ScreenToGame(12)
#define shipPlayerHeight			ScreenToGame(12)
#define shipsMax							15
#define shipMovement						2		// faster than a shot
#define periodsToSinkOneNotch			40
#define shipCompletelySunkAmount		1
#define periodsToWaitForAnotherShip	50
#define periodsToEnterWarp				2
#define minimumPeriodsSpentInWarp	20
#define randomPeriodsSpentInWarp		80
#define periodsToExitWarp				15
#define warpEffectDuration				periodsToExitWarp
#define sparkDurationAfterShipExplosion	16
#define chanceToHaveMaxSpeedOutOfWarp		50
#define chanceToCloneOutOfWarp				110	// 1 in 9 chance
#define invalidUniqueShipID					0xffff

// Alien settings
#define shipAlienHomeWorldWidth		ScreenToGame(51)
#define shipAlienHomeWorldHeight		ScreenToGame(51)
#define shipAlienHomeWorldSpeed		0
#define shipAlienHomeWorldArmor		64
#define shipAlienHomeWorldShots		0
#define shipAlienBaseWidth				ScreenToGame(27)
#define shipAlienBaseHeight			ScreenToGame(22)
#define shipAlienBaseSpeed				0
#define shipAlienBaseArmor				27
#define shipAlienBaseShots				1
#define baseStartPositions				4
#define shipAlienLargeWidth			ScreenToGame(21)
#define shipAlienLargeHeight			ScreenToGame(7)
#define shipAlienLargeSpeed			4
#define shipAlienSmallWidth			ScreenToGame(13)
#define shipAlienSmallHeight			ScreenToGame(6)
#define shipAlienSmallSpeed			7
#define shipAlienAceSpeed				9
#define alienMovement					2				// faster than a shot
#define chanceForNewAlien				4				// percent per period
#define chanceForAceAlien				200 //75		// perthousand per small alien
#define levelsForAnAlienWave			5
#define maxAlienShipArmor				6
#define minAliensToDefendBases		2
#define chanceToStopFleeing			100			// perthousand per period
#define fleeSpeedIncrease				6

// Ship positioning
#define startPositionInset	12
#define startPositionCases 4		// We only have 4 possible places even if there are 
#define shipStartPositions 4		// more players.
#define safetyDistanceAroundShip	ScreenToGame(16)

// Rock settings
#define rockSmallWidth				ScreenToGame(8)
#define rockSmallHeight				ScreenToGame(8)
#define rockMediumWidth				ScreenToGame(12)
#define rockMediumHeight			ScreenToGame(12)
#define rockLargeWidth				ScreenToGame(16)
#define rockLargeHeight				ScreenToGame(16)
#define rocksMax								30
#define largeRocksFromExplodedPlanet	12

// Bonus settings
#define bonusWidth					ScreenToGame(12)
#define bonusHeight					ScreenToGame(12)
#define bonusesMax					3
#define bonusDuration				(255 / 2)	// half of max value
#define lowBottomScore				100
#define highBottomScore				250

// Spark settings
#define sparksMax									(40)
#define sparkDurationAfterCollision			20
#define sparkDurationFromObject				20
#define sparkDurationAfterRockExplosion	20

// Score settings
#define scoresMax						5
#define scoreDigitsMax				5
#define scoreDuration				45
#define scoreFont						stdFont


// Level Message settings
#define LevelMessageFont		largeFont
#define levelMessageDuration	40

// This controls how often an object is moved.  Remember that game coordinates
// have four bits of fraction.  So, a rock moving at a speed of four moves
// one pixel every four game periods.
#define speedMax					ScreenToGame(8)
#define shotSpeed					(24)
#define rockSpeed					4

// Player settings
#define playersMax				4
#define player0					0
#define player1					1
#define noPlayer					0xff
#define alienPlayer				0xfe
#define scoreForAnotherShip	10000
#define shipsInFormationMax	4

// List of all the degrees
#define degrees0			0
#define degrees22			1
#define degrees45			2
#define degrees67			3
#define degrees90			4
#define degrees112		5
#define degrees135		6
#define degrees157		7
#define degrees180		8
#define degrees202		9
#define degrees225		10
#define degrees247		11
#define degrees270		12
#define degrees292		13
#define degrees315		14
#define degrees337		15
#define degrees360		16
#define degreesMax		16


#if EMULATION_LEVEL != EMULATION_NONE
#define advanceTimeInterval	3				// the emulator is slow, run as fast as possible
#else
#define advanceTimeInterval	6
#endif

// Various time intervals
#define levelOverTimeInterval		(35)			// (sysTicksPerSecond / 4 * 5)
#define gameEndingTimeInterval	(80)			// time for game to continue after last ship gone
#define gameOverTimeInterval		(2 * 60)		// time to pause after game over and before high scores
#define pauseLengthBeforeResumingSavedGame	(3 * 60)

// Levels at which features are possible
#define alienWave1Level							5
#define alienWave2Level							10
#define alienWave3Level							15
#define alienWave4Level							20
#define alienHomeWorldLevel					25

// A hero is a winner who lived through the planet explosion and returned
// alive to earth.  These defines are used for skits and for the high scores
// dialog.
#define heroLevel									(alienHomeWorldLevel + 1)
#define winnerLevel								(alienHomeWorldLevel + 4)

#define firstLevelForAliens						2
#define firstLevelSmallAliens						(alienWave1Level + 1)
#define firstLevelShootSometimesAccurately	(alienWave2Level + 1)
#define firstLevelTurnSlow							alienWave1Level
#define firstLevelTurnRightAngles				alienWave2Level
#define firstLevelTurnRandomly					alienWave3Level
#define firstLevelArmoredAliens					(alienWave2Level + 3)
#define firstLevelWarpAliens						(alienWave4Level + 1)
#define firstLevelAceAliens						(alienWave3Level + 1)

// High Scores settings
#define highScoreFont				stdFont
#define firstHighScoreY				28
#define highScoreHeight				12
#define highScoreNameColumnX		17
#define highScoreScoreColumnX		114		// Right aligned
#define highScoreLevelColumnX		153		// Right aligned
#define nameLengthMax				15
#define highScoreMax					9


// Report settings
#define clearMessageX				30
#define clearMessageY				80
#define reportFont					largeFont


// Threshold for the system launcher to not always save bytes.  This
// is only to work around a system bug.
#define saveBitsThreshold 4000				

#pragma mark Structures


// List of sounds possible
typedef enum 
	{
	noSound, 
	releaseShot, 
	explosion,
	rockLargeExplosion,
	rockMediumExplosion,
	rockSmallExplosion,
	shipHit,
	shipExplosion, 
	bonusAwarded,
	newHighScore,
	bonusShip,
	soundTypeCount
	} SoundType;

typedef struct
	{
	UInt8 priority;
	UInt8 periods;
	Int32 frequency;
	UInt16 duration;
	} SoundInfo;

typedef struct
	{
	UInt16 initDelay;
	UInt16 period;
	UInt16 doubleTapDelay;
	Boolean queueAhead;
	} KeyRateType;


enum gameProgress 
	{
	gameResuming, 			// don't draw or change the game state.  Do resume the save game.
	gameInitializing, 	// don't draw or change the game state
	waitingForAliens, 	// advance and draw the game state (allows ship to move)
	gameInMotion, 	 		// advance and draw the game state
	levelOver, 	 			// pause, start new level, and go to waitingForBall
	gameWon,					// Special controlled subset of gameInMotion to perform a skit 
	gameOver,				// don't draw or change the game state, pause before high score check
	checkHighScores		// check for high score, get name if high score.
	};

typedef enum  
	{
	collisionCooperative,	// Multiple players don't kill each other
	collisionCompetitive	// Multiple players kill each other
	} collisionRules;

typedef UInt16 ShipUniqueIDType;	// ID to recognize a ship even if moved in the ship list

typedef struct 
	{
	Int16					x;
	Int16					y;
	} GamePoint;

typedef GamePoint GameVector;
	
typedef struct 
	{
	Int16					left;
	Int16					top;
	Int16					right;
	Int16					bottom;
	} GameLocation;
	
typedef struct ObjectType
	{
	GameLocation		location;
	GameVector			motion;
	Boolean				usable;
	Boolean				changed;
	UInt8					type;
	struct ObjectType	*nextObjectInSector;
	} ObjectType;

typedef enum
	{
	ShootRandomly,
	ShootSometimesAccurately,		// shoots close object accurately
	ShootSometimesPlasmas,
	ShootAtNearestShip,
	GunnerCount
	} GunnerType;

typedef enum
	{
	TurnNever,
	TurnSlow,
	TurnRightAngles,
	TurnRandomly,
	NavigatorCount
	} NavigatorType;

typedef struct
	{
	Boolean				rotateLeft :1;
	Boolean				rotateRight :1;
	Boolean				thrusterOn :1;
	Boolean				appearSafely :1;		// appear when an object won't hit the ship
	Boolean				finishedSkit :1;		// set when ship disappears
	} PlayerShipAttributes;
		
typedef struct
	{
	GunnerType			gunner: 2;
	NavigatorType		navigator: 2;
	Boolean				fleeing :1;
	unsigned int		speed: 5;
	} AlienShipAttributes;
		
typedef struct
	{
	Boolean				shieldOn :1;
	Boolean				enteringWarp :1;
	Boolean				inWarp :1;
	Boolean				exitingWarp :1;
	Boolean				longShots :1;
	Boolean				retroRockets :1;


	// Players and aliens have some different attributes
	union 
		{
		PlayerShipAttributes	player;
		AlienShipAttributes alien;
		} type;
	} ShipStatus;

typedef struct 
	{
	ObjectType			object;
	UInt8					owner;
	ShipUniqueIDType	uniqueShipID;			// Used to identify this ship
	Int8					heading;
	UInt8					periodsToWait;
	UInt8					shotsAvailable;
	UInt8					warpsAvailable;
	UInt8					armorAvailable;
	ShipStatus			status;
	UInt16				score;
	} ShipType;

typedef struct 
	{
	ObjectType			object;
	UInt8					ownerPlayerNumber;	// Player who fired shot
	ShipUniqueIDType	ownerUniqueShipID;	// ship who fired shot
	UInt8					duration;				// how long the object lasts (shots)
	} ShotType;

typedef struct 
	{
	ObjectType			object;
	} RockType;

typedef struct 
	{
	ObjectType			object;
	UInt8					duration;
	} BonusType;

typedef struct
	{
	GamePoint			location;
	GameVector			motion;
	UInt8					duration;
	} SparkType;

typedef struct
	{
	GameLocation		location;
	UInt8					duration;
	Char					digits[scoreDigitsMax + 1];
	UInt8					digitCount;
	} ScoreType;

typedef struct 
	{
	// These are initialized in GameStart.
	UInt8						shotCount;			// shots 
	UInt8						shotsNotUsable;
	UInt8						shipCount;			// ships 
	UInt8						alienCount;			// ships 
	UInt8						baseCount;			// ships 
	ShipUniqueIDType		nextUniqueShipID;
	UInt8						shipsNotUsable;
	UInt8						rockCount;			// rocks 
	UInt8						rocksNotUsable;
	UInt8						bonusesCount;		// bonuses
	UInt8						bonusesNotUsable;
	UInt8						sparkCount;			// sparks 
	UInt8						scoreCount;			// scores
	ShipType					ship[shipsMax];
	ShotType					shot[shotsMax];
	RockType					rock[rocksMax];
	BonusType				bonus[bonusesMax];
	SparkType				spark[sparksMax];
	ScoreType				score[scoresMax];
	} WorldState;

typedef struct 
	{
	UInt32					playerInput;		// the keys the user is pressing
	Int32						score;
	Boolean					scoreChanged;
	Int32						scoreToAwardBonusShip;
	UInt8						livesRemaining;	// number of lives left before the game ends
	Boolean					livesChanged;
	Boolean					armorChanged;
	UInt8						shipsExpectedToOwn;
	UInt8						shipsOwned;
	UInt8						shipFormationCount;
	ShipUniqueIDType		shipFormation[shipsInFormationMax];
	UInt8						periodsUntilAnotherShip;
	} PlayerType;


typedef struct
	{
	UInt32					period;				// period this status is set for
	Boolean					periodsToWait;		// time ships spends in warp
	} WarpEnterStatus;

typedef struct
	{
	UInt32					period;				// period this status is set for
	Boolean					highSpeed;			// ships move quickly (out of control!)
	Boolean					cloneShips;			// all ships should clone
	UInt8						heading;				// the heading of all ships
	} WarpExitStatus;

// This structure is used to coordinate multiple ships dropping out of 
//  warp at the same time.  The goal is for ships on the same team to
//  be in a formation after warpping.  Otherwise they are too hard to control.
typedef struct
	{
	WarpEnterStatus		enter;
	WarpExitStatus			exit;
	} WarpStatus;

typedef struct {
	enum gameProgress		status;
	UInt32					periodCounter;		// time when next period occurs
	UInt32					nextPeriodTime;	// time when next period occurs
	Boolean					paused;				// indicates that time should not pass
	UInt32					pausedTime;			// Used to 
	UInt32					startTime;			// Time since starting Rocks
	
	WinHandle				screenBufferH;		// screenBuffer
	AbsRectType				screenBoundsOld;
	AbsRectType				screenBoundsNew;
	
	UInt8						rocksToSend;		// large rocks to send this level
	UInt8						aliensToSend;		// aliens to send this level
	UInt8						playerCount;
	
	Boolean					bombExplode;		// Explode a bomb
	UInt8						bombOwner;			// Player released the bomb
	WarpStatus				warp;					// State of warp space
	
	PlayerType				stats[playersMax];	// number of lives left before the game ends
	UInt8						playerUsingScreen;// the player who's stats are displayed
	collisionRules			collisionStatus;
	
	UInt8						level;				// controls the rocks
	Int16						periodsTillNextLevel;	// time between levels
	WorldState				objects;					// world to be drawn
	SoundType				soundToMake;		// one sound can be made per game period
	Int8						soundPeriodsRemaining;	// times to repeat the sound
	Boolean					lowestHighScorePassed;	// User beat the lowest high score
	Boolean					highestHighScorePassed;	// User beat the highest high score
} GameStatusType;


typedef struct 
	{
	UInt8						rotateLeftDelayInPeriods;	// periods until repeat key
	UInt8						rotateRightDelayInPeriods;	// periods until repeat key
	UInt8						thrustDelayInPeriods;	// periods until repeat key
	UInt8						shootDelayInPeriods;	// periods until repeat key
	} ConsoleType;

typedef struct
	{
	char						name[nameLengthMax + 1];
	Int32						score;
	Int16						level;
	} SavedScore;

typedef struct
	{
	SavedScore				highScore[highScoreMax];
	UInt8						lastHighScore;
	UInt32					accumulatedTime;	// Total time spent by player playing Rocks
	} RocksPreferenceType;



/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/

#pragma mark Data

static GameStatusType		GameStatus;
static MemHandle				ObjectBitmapHandles[bitmapTypeCount];
static BitmapPtr				ObjectBitmapPtr[bitmapTypeCount];
static WinHandle				ObjectWindowHandles[bitmapTypeCount];
		
static enum gameProgress	SavedGameStatus;

static ConsoleType			Console;

static Boolean					redrawWhenReturningToGameWindow = false;

// Level Message
static Char LevelMessageText[32];
static UInt8 LevelMessageLength;
static AbsRectType LevelMessageBounds;
static Int16 LevelMessageWidth;
static Int16 LevelMessageHeight;
static UInt8 LevelMessageDuration;

static UInt16 SoundAmp;		// default sound amplitude


static Int16					MovementX[degreesMax] = 
	{
	2, 2, 2, 1, 0, -1, -2, -2, -2, -2, -2, -1, 0, 1, 2, 2
	};
static Int16					MovementY[degreesMax] = 
	{
	0, -1, -2, -2, -2, -2, -2, -1, 0, 1, 2, 2, 2, 2, 2, 1
	};

// These are the chances of a ship cloning when returning from warp.
// The index is the count of object onscreen.  The chance for when
// more objects are onscreen is determined by an equation.  The odds are
// never better than one in three.
static Int16 cloneChances[4] = 
	{
	64, 32, 16, 8
	};

static SoundInfo  Sound[soundTypeCount] = 
	{
		{0,	0,		0, 	0},			// no sound
		{0,	1,		2330,	8},			// release shot
		{40,	4,		180,	30},			// explosion
		{40,	7,		130,	30},			// rockLargeExplosion
		{40,	5,		130,	30},			// rockMediumExplosion
		{40,	3,		130,	30},			// rockSmallExplosion
		{50,	2,		170,	30},			// Ship hit
		{60,	8,		210,	20},			// Ship exploding
		{70,	2,		550,	50},			// Bonus awarded to player
		{90,	8,		2500,	50},			// high score passed
		{90,	6,		3740,	20},			// bonus ship awarded
	};

// The original values for key rates.		
KeyRateType KeyRate;

// This is really only used for shots and rocks.
static Int16 ObjectScore[objectTypeCount] = 
	{
	350,		// shipPlayer
	10000,	// shipEarth
	10000,	// shipAlienHomeworld
	1000,		// shipAlienBase
	75,		// shipAlienLarge
	150,		// shipAlienSmall
	150,		// shipAlienAce
	5,			// shotNormal
	20,		// shotPlasma
	20,		// rockSmall
	10,		// rockMedium
	5,			// rockLarge
	0,			// explosionRock
	};

// This tables lists where ships can appear on the screen.  Importantly, it handles
// placing ships during multiple players.
static GamePoint ShipPositionToStart[startPositionCases][shipStartPositions] = 
	{
		// One player
		{
			// Center of the Screen
			{ScreenToGame((screenWidth - GameToScreen(shipPlayerWidth)) / 2 + borderAroundScreen), 
			 ScreenToGame((screenHeight - GameToScreen(shipPlayerHeight)) / 2 + borderAroundScreen)},
			{0,0},
			{0,0},
			{0,0}
		},
		// Two players
		{
			// Top left
			{ScreenToGame(startPositionInset + borderAroundScreen), 
			 ScreenToGame(startPositionInset + borderAroundScreen)},
			// Bottom Right
			{ScreenToGame(screenWidth - GameToScreen(shipPlayerWidth) - startPositionInset + borderAroundScreen), 
			 ScreenToGame(screenHeight - GameToScreen(shipPlayerHeight) - startPositionInset + borderAroundScreen)},
			{0,0},
			{0,0}
		},
		// Three players
		{
			// Top middle
			{ScreenToGame((screenWidth - GameToScreen(shipPlayerWidth)) / 2 + borderAroundScreen), 
			 ScreenToGame(startPositionInset + borderAroundScreen)},
			// Bottom Right
			{ScreenToGame(screenWidth - GameToScreen(shipPlayerWidth) - startPositionInset + borderAroundScreen), 
			 ScreenToGame(screenHeight - GameToScreen(shipPlayerHeight) - startPositionInset + borderAroundScreen)},
			// Bottom left
			{ScreenToGame(startPositionInset + borderAroundScreen), 
			 ScreenToGame(screenHeight - GameToScreen(shipPlayerHeight) - startPositionInset + borderAroundScreen)},
			{0,0}
		},
		// Four players
		{
			// Top left
			{ScreenToGame(startPositionInset + borderAroundScreen), 
			 ScreenToGame(startPositionInset + borderAroundScreen)},
			// Top right
			{ScreenToGame(screenWidth - GameToScreen(shipPlayerWidth) - startPositionInset + borderAroundScreen), 
			 ScreenToGame(startPositionInset + borderAroundScreen)},
			// Bottom Right
			{ScreenToGame(screenWidth - GameToScreen(shipPlayerWidth) - startPositionInset + borderAroundScreen), 
			 ScreenToGame(screenHeight - GameToScreen(shipPlayerHeight) - startPositionInset + borderAroundScreen)},
			// Bottom left
			{ScreenToGame(startPositionInset + borderAroundScreen), 
			 ScreenToGame(screenHeight - GameToScreen(shipPlayerHeight) - startPositionInset + borderAroundScreen)}
		},
	};


// This tables lists where alien bases can appear on the screen.
static GamePoint BasePositionToStart[baseStartPositions] = 
	{
		// Left middle
		{ScreenToGame(startPositionInset + borderAroundScreen), 
		 ScreenToGame((screenHeight - GameToScreen(shipAlienBaseHeight)) / 2 + borderAroundScreen)},
		// Right middle
		{ScreenToGame(screenWidth - startPositionInset + borderAroundScreen) - shipAlienBaseHeight, 
		 ScreenToGame((screenHeight - GameToScreen(shipAlienBaseHeight)) / 2 + borderAroundScreen)},
		// Top middle
		{ScreenToGame((screenWidth - GameToScreen(shipAlienBaseWidth)) / 2 + borderAroundScreen), 
		 ScreenToGame(startPositionInset + borderAroundScreen)},
		// Bottom middle
		{ScreenToGame((screenWidth - GameToScreen(shipAlienBaseWidth)) / 2 + borderAroundScreen), 
		 ScreenToGame(screenWidth - startPositionInset + borderAroundScreen) - shipAlienBaseHeight},
	};


static ObjectType *Sector[sectorCount];

// This is the pattern covering the screen when a mega bomb activates.
static const CustomPatternType BombPattern = 
	{
	0x88, 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55
	};

// The following global variables are saved to a state file.

// Scores
RocksPreferenceType			Prefs;


/***********************************************************************
 *
 *	Macros
 *
 ***********************************************************************/
#pragma mark Macros

#define noItemSelection			-1

//#define IsAlienWaveLevel(l)		(((l) - 2 + 1) % levelsForAnAlienWave == 0)
#define IsAlienWaveLevel(l)		((l) == alienWave1Level || \
											 (l) == alienWave2Level || \
											 (l) == alienWave3Level || \
											 (l) == alienWave4Level || \
											 (l) == alienHomeWorldLevel)
#define IsAlienHomeWorldLevel(l)	((l)  == alienHomeWorldLevel)

// Bitmaps
#define GetShipBitmap(t, heading)	((t) < firstAlienShip ? firstShipBitmap + heading \
													: firstAlienBitmap + (t) - firstAlienShip)
#define GetShotBitmap(t)			((t) + firstShotBitmap - firstShotType)
#define GetRockBitmap(t)			((t) + firstRockBitmap - firstRockType)
#define GetBonusBitmap(t)			((t) + firstBonusBitmap - firstBonusType)


#define ShipIsOwnedByHuman(s)		(OwnerIsHuman((s)->owner))
#define PlayerIsHuman(p)			((p) < playersMax)
#define OwnerIsHuman(o)				((o) < playersMax)
#define OwnerIsAlien(o)				((o) == alienOwner)
#define IsRock(o) 					((o)->type >= rockSmall && (o)->type <= rockLarge)
#define IsShot(o) 					((o)->type >= shotNormal && (o)->type <= shotPlasma)
#define IsShip(o) 					((o)->type <= lastAlienShip)
#define IsAlienShip(o)				((o)->type >= firstAlienShip && (o)->type <= lastAlienShip)
#define IsPlanet(o)					((o)->type <= shipAlienHomeWorld && (o)->type >= shipEarth)
#define IsBase(o)						((o)->type >= shipAlienHomeWorld && (o)->type <= shipAlienBase)
#define IsBonus(o) 					((o)->type >= firstBonusType && (o)->type <= lastBonusType)
#define IsDestructible(o)			(!(IsShip(o) && ((((ShipType *) (o))->status.shieldOn) || (((ShipType *) (o))->armorAvailable))))
#define CausesDamage(o)				((o)->type < firstBonusType)

#define LastRock						(&GameStatus.objects.rock[GameStatus.objects.rockCount] - 1)
#define LastSpark						(&GameStatus.objects.spark[GameStatus.objects.sparkCount] - 1)
#define LastShip						(&GameStatus.objects.ship[GameStatus.objects.shipCount] - 1)
#define LastShot						(&GameStatus.objects.shot[GameStatus.objects.shotCount] - 1)
#define LastScore						(&GameStatus.objects.score[GameStatus.objects.scoreCount] - 1)
#define LastBonus						(&GameStatus.objects.bonus[GameStatus.objects.bonusesCount] - 1)


#define RandN(N)						((((Int32) SysRandom (0)) * (N)) / ((Int32) sysRandomMax + 1))
#define AbsoluteValue(n)			( ((n) >= 0) ? (n) : (-(n)))

#define ObjectCenterX(o)			(((o)->location.left + (o)->location.right) / 2)
#define ObjectCenterY(o)			(((o)->location.top + (o)->location.bottom) / 2)

#define TimeToChangeHeading(s)	(((s)->duration & 0x07) == 4)

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
static void GameStart ();
static Boolean SectorAddObject (ObjectType *object, Boolean detectOnly);
static void HighScoresAddScore (Char * name, Int32 score, Int16 level, 
	Boolean dontAddIfExists);
static void HighScoresCheckScore (Int32 score);
static void GameDrawLivesGauge (Int16 livesRemaining);
static void InfoDisplay (void);
static Boolean CheckForCollisionWithObject (ObjectType *object1, 
	ObjectType *object2, Boolean detectOnly);
static void GameUnmaskKeys ();


/***********************************************************************
 *
 * FUNCTION:     TimeUntillNextPeriod
 *
 * DESCRIPTION:  Return the time until the next world advance.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/24/95	Initial Revision
 *
 ***********************************************************************/
static Int32 TimeUntillNextPeriod (void)
{
	Int32 timeRemaining;
	
	
	if (GameStatus.status == gameInitializing || 
		GameStatus.status == gameResuming || 
		GameStatus.status == checkHighScores ||
		GameStatus.paused)
		return evtWaitForever;
		
#ifdef OPTION_NO_DELAY
	return 0;
#endif
		
	timeRemaining = GameStatus.nextPeriodTime - TimGetTicks();
	if (timeRemaining < 0)
		timeRemaining = 0;
		
	return timeRemaining;
}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine opens the application's resource file and
 *               database.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/6/95	Initial Revision
 *			roger	1/23/96	Use new 2.0 api calls.
 *			jmp	10/24/99	Moved the bringing up of the Help dialog until
 *								AFTER the original DrawWindow has been
 *								restored.  Side-effect:  Help dismisses a dialog
 *								that ends up making the DrawWindow invalide because
 *								Reptoids hasn't brought up its initial Form yet.
 *								Perhaps this should be reworked?
 *			jmp	12/16/99	When resuming from the initialization state,
 *								make sure we stay in the initialization state.
 *
 ***********************************************************************/
static UInt16 StartApplication (void)
{
	int i;
	WinHandle oldDrawWinH;
	UInt16 error;
	RectangleType bounds;
	UInt16 prefsSize;
	Boolean bringupHelp = false;

	// Get SoundAmp for the sound code.  If game sounds are desired use
	// the default sound else set the volume to zero to turn it off.
	if (PrefGetPreference(prefGameSoundLevelV20) != slOff)
		SndGetDefaultVolume(NULL, NULL, &SoundAmp);
	else
		SoundAmp = 0;

	// Get the key repeat rate for when we want to restore it after the game.
	KeyRates(false, &KeyRate.initDelay, &KeyRate.period, &KeyRate.doubleTapDelay, 
		&KeyRate.queueAhead);

	// Keep the Object graphics locked because they are frequently used
	oldDrawWinH = WinGetDrawWindow();
	for (i = 0; i < bitmapTypeCount; i++)
		{
		ObjectBitmapHandles[i] = DmGetResource( bitmapRsc, firstObjectBmp + i);
		ObjectBitmapPtr[i] = MemHandleLock(ObjectBitmapHandles[i]);
		
		// It is actually faster and more versatile to store the graphics
		// as window images.  It is faster because WinDrawBitmap contructs a
		// window from the bitmap on the fly before drawing.  It is more
		// versatile because when the window is copied to the screen a 
		// screen copy mode like scrCopyNot can be used.  This makes
		// images masks possible.
		// We can do this as long as their is enough memory free in the dynamic
		// ram.  We don't do this to large images.
		if (i == 999)			// don't skip any bitmaps we use
			{
			ObjectWindowHandles[i] = 0;
			}
		else
			{
			ObjectWindowHandles[i] = WinCreateOffscreenWindow(
				ObjectBitmapPtr[i]->width, ObjectBitmapPtr[i]->height,
				screenFormat, &error);
			ErrFatalDisplayIf(error, "Error loading images");
			WinSetDrawWindow(ObjectWindowHandles[i]);
			WinDrawBitmap(ObjectBitmapPtr[i], 0, 0);
			}
		
		}

	// Initialize the console data
	Console.rotateLeftDelayInPeriods = 0;
	Console.rotateRightDelayInPeriods = 0;
	Console.thrustDelayInPeriods = 0;
	Console.shootDelayInPeriods = 0;

	// Restore the saved preferences.  These contain the high scores and
	// other details like how much time has been spent playing the game.
	prefsSize = sizeof (RocksPreferenceType);
	if (PrefGetAppPreferences (appFileCreator, appPrefID, &Prefs, &prefsSize, 
		true) == noPreferenceFound)
		{
		// There aren't any preferences
		
		// Clear the high scores.
		for (i = 0; i < highScoreMax; i++)
			{
			Prefs.highScore[i].name[0] = '\0';
			Prefs.highScore[i].score = 0;
			Prefs.highScore[i].level = 1;
			}
		
		// Add Best Score
//		HighScoresAddScore ("Mr. P", 	70445, 26, true);

		// No last high score
		Prefs.lastHighScore = highScoreMax;
		
		// No time has been recorded.
		Prefs.accumulatedTime = 0;
		
		// Now remember that we want to bring up the about box and instructions.
		// We don't bring up the about box now because it's dismissal changes
		// the drawing environment, and we're still creating offscreen windows
		// and such.
		bringupHelp = true;
		}	

	// Restore a saved game.  Games are kept in the unsaved preference database.
	prefsSize = sizeof (GameStatus);
	if (PrefGetAppPreferences (appFileCreator, appSavedGameID, &GameStatus, 
		&prefsSize, false) == noPreferenceFound)
		{
		// Initialize this now so that the GetNextEvent wait time is set properly.	
		GameStatus.status = gameInitializing;		// don't draw yet!
		}
	else
		{
		if (GameStatus.status != checkHighScores)
			{
			SavedGameStatus = GameStatus.status;
			
			// Don't draw yet unless we're resuming in the initialization state.
			if (GameStatus.status != gameInitializing)
				GameStatus.status = gameResuming;
			}
		}

	// Now set up game info specific to this session.

	// Setup the screenBuffer.  Clear it for use.
	GameStatus.screenBufferH = WinCreateOffscreenWindow (screenWidth, screenHeight, 
		screenFormat, &error);
	WinSetDrawWindow(GameStatus.screenBufferH);
	bounds.topLeft.x = 0;
	bounds.topLeft.y = 0;
	bounds.extent.x = screenWidth;
	bounds.extent.y = screenHeight;
	WinEraseRectangle(&bounds, 0);

	WinSetDrawWindow(oldDrawWinH);

	// Set up one player
	GameStatus.playerCount = 1;
//	GameStatus.playerCount = 4;
	GameStatus.collisionStatus = collisionCooperative;

	// Record the start time of this game session.
	GameStatus.startTime = TimGetTicks();

	// Initialize the random number generator
	SysRandom (GameStatus.startTime);
#ifdef DETERMINISTIC_SEED
	#ifdef OPTION_DETERMINISTIC_PLAY
		SysRandom (DETERMINISTIC_SEED);			// test code
	#endif
		SysRandom (DETERMINISTIC_SEED);			// test code
#endif

#if EMULATION_LEVEL == EMULATION_NONE
	// Now display the about box and instructions.  This appears only
	// the first time the program is run.
	if (bringupHelp)
		{
		InfoDisplay();
		FrmHelp (InstructionsStr);
		}
#endif
	
	return 0;		// no error
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: This routine opens the application's resource file and
 *              database, and posts an application enter event.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/6/95	Initial Revision
 *			roger	1/23/96	Use new 2.0 api calls.
 *			jmp	10/24/99	Call FrmCloseAllForms() to eliminate "memory leak" error that
 *								comes up on debug ROMs.
 *
 ***********************************************************************/
static void StopApplication (void)
{
	int i;

	// Delete the screenBuffer
	WinDeleteWindow(GameStatus.screenBufferH, false);

	// Unlock and release the locked bitmaps
	for (i = 0; i < bitmapTypeCount; i++)
		{
		MemPtrUnlock(ObjectBitmapPtr[i]);
		DmReleaseResource(ObjectBitmapHandles[i]);
		
		if (ObjectWindowHandles[i]) 
			WinDeleteWindow(ObjectWindowHandles[i], false);
		}

	// Update the time accounting.
	Prefs.accumulatedTime += (TimGetTicks() - GameStatus.startTime);
	
	// If we are saving a game resuming (it hasn't started playing yet)
	// then preserve the game status.
	if (GameStatus.status == gameResuming)
		{
		GameStatus.status = SavedGameStatus;
		}
	
	// Save state/prefs.
	PrefSetAppPreferences (appFileCreator, appPrefID, appPrefVersion, 
		&Prefs, sizeof (Prefs), true);

	PrefSetAppPreferences (appFileCreator, appSavedGameID, appSavedGameVersion, 
		&GameStatus, sizeof (GameStatus), false);

	// Restore the keys. 
	GameUnmaskKeys();
	
	// Close all the open forms.
	FrmCloseAllForms ();
}


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version meets your
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
 ***********************************************************************/
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we have at least the minimum required version of the ROM or later.
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
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		
		return sysErrRomIncompatible;
		}

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/6/95	Initial Revision
 *
 ***********************************************************************/
static MemPtr GetObjectPtr (UInt16 objectID)
{
	FormPtr frm;
	MemPtr obj;
	
	frm = FrmGetActiveForm ();
	obj = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID));

	return obj;
}


/***********************************************************************
 *
 * FUNCTION:     GameMaskKeys
 *
 * DESCRIPTION:  Mask the keys to reduce keyDownEvents from being sent.
 * This saves time.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/25/96	Initial Revision
 *
 ***********************************************************************/
static void GameMaskKeys ()
{
	UInt16 initDelay;
	UInt16 period;
	Boolean queueAhead;
	
	// Set the keys we poll to not generate events.  This saves cpu cycles.
	KeySetMask(	~keysAllowedMask );
	return;
	
	// Also set the key repeat rate low to avoid constantly checking them.
	initDelay = slowestKeyDelayRate;
	period = slowestKeyPeriodRate;
	queueAhead = false;
	KeyRates(true, &initDelay, &period, &period, &queueAhead);
}


/***********************************************************************
 *
 * FUNCTION:     GameUnmaskKeys
 *
 * DESCRIPTION:  Unmask the keys.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/25/96	Initial Revision
 *
 ***********************************************************************/
static void GameUnmaskKeys ()
{
	// Set the keys we poll to not generate events.  This saves cpu cycles.
	KeySetMask(keyBitsAll);
	
	// Also set the key repeat rate low to avoid constantly checking them.
	KeyRates(true, &KeyRate.initDelay, &KeyRate.period, &KeyRate.doubleTapDelay, 
		&KeyRate.queueAhead);
}


/***********************************************************************
 *
 * FUNCTION:		DrawBitmap
 *
 * DESCRIPTION:	Get and draw a bitmap at a specified location
 *
 * PARAMETERS:	resID		-- bitmap resource id
 *					x, y		-- bitmap origin relative to current window
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	10/9/95	Initial Revision
 *
 ***********************************************************************/
static void DrawBitmap(Int16 resID, Int16 x, Int16 y)
{
	MemHandle	resH;
	BitmapPtr	resP;


	resH = DmGetResource( bitmapRsc, resID );
	ErrFatalDisplayIf( !resH, "Missing bitmap" );
	resP = MemHandleLock(resH);
	WinDrawBitmap (resP, x, y);
	MemPtrUnlock(resP);
	DmReleaseResource( resH );
}


/***********************************************************************
 *
 * FUNCTION:		ScreenBoundsIncludeArea
 *
 * DESCRIPTION:	Be sure the screen covers a specific area, extending 
 * the screen bounds if neccessary
 *
 * PARAMETERS:		left, top, right, bottom - the bounds of the area to 
 * appear in the screen
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/
static void ScreenBoundsIncludeArea(Int16 left, Int16 top, Int16 right, Int16 bottom)
{
	// Update the bounds of the area drawn to in the screenBuffer
	if (left < GameStatus.screenBoundsNew.left)
		GameStatus.screenBoundsNew.left = left;
		
	if (top < GameStatus.screenBoundsNew.top)
		GameStatus.screenBoundsNew.top = top;
	
	if (right > GameStatus.screenBoundsNew.right)
		{
		GameStatus.screenBoundsNew.right = right;
		}

	if (bottom > GameStatus.screenBoundsNew.bottom)
		{
		GameStatus.screenBoundsNew.bottom = bottom;
		}
}


/***********************************************************************
 *
 * FUNCTION:		DrawObject
 *
 * DESCRIPTION:	Draw an object at a specified location and mode
 *
 * PARAMETERS:	bitmapNumber -- bitmap number
 *					x, y		-- bitmap origin relative to current window
 *					mode		-- transfer mode (scrANDNOT for masks)
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/29/96	Initial Revision
 *
 ***********************************************************************/
static void DrawObject(Int16 bitmapNumber, Int16 x, Int16 y, WinDrawOperation mode)
{
	RectangleType srcR;
	Int16	screenX;
	Int16	screenY;


	ErrFatalDisplayIf (ObjectWindowHandles[bitmapNumber] == 0, "Unhandled object image");
	
	
	// Map from game to screen coordinates
	screenX = (x >> 4) - borderAroundScreen;
	screenY = (y >> 4) - borderAroundScreen;
	
	// Copy the entire source window.
	MemMove (&srcR, &(ObjectWindowHandles[bitmapNumber]->windowBounds), sizeof(RectangleType));

	// Copy the source window (contains the image to draw) to the draw window.
	WinCopyRectangle(ObjectWindowHandles[bitmapNumber], 0, &srcR, 
		screenX, screenY, mode);
	
	
	// Update the bounds of the area drawn to in the screenBuffer
	ScreenBoundsIncludeArea(screenX, screenY, 
		screenX + ObjectWindowHandles[bitmapNumber]->windowBounds.extent.x - 1,
		screenY + ObjectWindowHandles[bitmapNumber]->windowBounds.extent.y - 1);
}


/***********************************************************************
 *
 * FUNCTION:		DrawPoint
 *
 * DESCRIPTION:	Draw a point at a specified location and mode
 *
 * PARAMETERS:	x, y		-- bitmap origin relative to current window
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/29/96	Initial Revision
 *
 ***********************************************************************/
static void DrawPoint(Int16 x, Int16 y)
{
	Int16	screenX;
	Int16	screenY;


	// Map from game to screen coordinates
	screenX = (x >> 4) - borderAroundScreen;
	screenY = (y >> 4) - borderAroundScreen;
	
	// Draw the point to the draw window.
	WinDrawLine(screenX, screenY, screenX, screenY);
	
	
	// Update the bounds of the area drawn to in the screenBuffer
	ScreenBoundsIncludeArea(screenX, screenY, screenX, screenY);

}


/***********************************************************************
 *
 * FUNCTION:     GameRequestSound
 *
 * DESCRIPTION:  Play a game sound.
 *
 * PARAMETERS:   sound - sound to play
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/30/96	Initial Revision
 *
 ***********************************************************************/
static void GameRequestSound (SoundType sound)
{
	if (Sound[sound].priority >= Sound[GameStatus.soundToMake].priority)
		{
		GameStatus.soundToMake = sound;
		GameStatus.soundPeriodsRemaining = Sound[sound].periods;
		}
}


/***********************************************************************
 *
 * FUNCTION:     GameRemoveSound
 *
 * DESCRIPTION:  Remove a game sound.
 *
 * PARAMETERS:   sound - sound to remove if playing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/10/96	Initial Revision
 *
 ***********************************************************************/
static void GameRemoveSound (SoundType sound)
{
	if (GameStatus.soundToMake == sound)
		{
		GameStatus.soundToMake = noSound;
		GameStatus.soundPeriodsRemaining = Sound[noSound].periods;
		}
}


/***********************************************************************
 *
 * FUNCTION:     IncreaseScore
 *
 * DESCRIPTION:  Increase the score by some amount
 *
 * PARAMETERS:   score - the amount to add to the score.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/20/96	Initial Revision
 *
 ***********************************************************************/
static void IncreaseScore (Int32 score, UInt8 playerNum)
{
	if (PlayerIsHuman(playerNum))
		{
		GameStatus.stats[playerNum].score += score;
		GameStatus.stats[playerNum].scoreChanged = true;
		
		if (playerNum == GameStatus.playerUsingScreen)
			{
			// Beep if the user is setting a new high score. Don't beep if not
			// all the high scores were already set (it's annoying).
			if (!GameStatus.lowestHighScorePassed &&
				GameStatus.stats[playerNum].score > Prefs.highScore[highScoreMax - 1].score &&
				Prefs.highScore[highScoreMax - 1].score > 0)
				{
				GameStatus.lowestHighScorePassed = true;
				if (GameStatus.stats[playerNum].score > 0)
					GameRequestSound (newHighScore);
				}
		
		
			// Beep if the user is setting the highest score.
			if (!GameStatus.highestHighScorePassed &&
				GameStatus.stats[playerNum].score > Prefs.highScore[0].score &&
				Prefs.highScore[0].score > 0)
				{
				GameStatus.highestHighScorePassed = true;
				if (GameStatus.stats[playerNum].score > 0)
					GameRequestSound (newHighScore);
				}
			}
		
		
		// Beep if the user is awarded an extra ship.
		if (GameStatus.stats[playerNum].score >= GameStatus.stats[playerNum].scoreToAwardBonusShip)
			{
			GameStatus.stats[playerNum].scoreToAwardBonusShip += scoreForAnotherShip;
			
			// Add a ship to those remaining and update the lives gauge.
			GameStatus.stats[playerNum].livesRemaining++;
			GameStatus.stats[playerNum].livesChanged = true;
			
			if (playerNum == GameStatus.playerUsingScreen)
				{
				GameRequestSound (bonusShip);
				}
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:     GetVisibleGameSpace
 *
 * DESCRIPTION:  Get the visible portion of the game space.
 *
 * PARAMETERS:   location - pointer to location
 *
 * RETURNED:     location set
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/10/96	Initial Revision
 *
 ***********************************************************************/
static void GetVisibleGameSpace (GameLocation *location)
{
	location->left = ScreenToGame(borderAroundScreen);
	location->right = gameWidth;
	location->top = ScreenToGame(borderAroundScreen);
	location->bottom = gameHeight;
}


/***********************************************************************
 *
 * FUNCTION:     LocationWrap
 *
 * DESCRIPTION:  Wrap the object to the game space.
 *
 * PARAMETERS:   location - pointer to location
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/
static void LocationWrap (GameLocation *location)
{
	// Wrap the coordinates if neccessary.
	if (location->left < 0)
		{
		location->left += gameWidth;
		location->right += gameWidth;
		}
	else if (location->left >= gameWidth)
		{
		location->left -= gameWidth;
		location->right -= gameWidth;
		}

	if (location->top < 0)
		{
		location->top += gameHeight;
		location->bottom += gameHeight;
		}
	else if (location->top >= gameHeight)
		{
		location->top -= gameHeight;
		location->bottom -= gameHeight;
		}
}


/***********************************************************************
 *
 * FUNCTION:     ObjectCompletelyVisible
 *
 * DESCRIPTION:  Check if an object lies completely the visible portion 
 * of the game space.
 *
 * PARAMETERS:   location - pointer to location
 *
 * RETURNED:     true if the location is entirely visible
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/14/96	Initial Revision
 *
 ***********************************************************************/
static Boolean ObjectCompletelyVisible (GameLocation *location)
{
	GameLocation screenLocation;
	
	
	GetVisibleGameSpace(&screenLocation);
	
	if (location->top < screenLocation.top)
		return false;
	
	if (location->bottom > screenLocation.bottom)
		return false;
	
	if (location->left < screenLocation.left)
		return false;
	
	if (location->right > screenLocation.right)
		return false;
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     ObjectMoveToVisibleGameSpace
 *
 * DESCRIPTION:  Move an object to a completely the visible portion 
 * of the game space.
 *
 * PARAMETERS:   object - pointer to object to move
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/29/96	Initial Revision
 *
 ***********************************************************************/
static void ObjectMoveToVisibleGameSpace (ObjectType *object, Boolean avoidEdges)
{
	GameLocation visibleGameSpace;
	Int16 width;
	Int16 height;
	
	
	// Pick a new location for the object.  Leave some room
	// around the edges of the screen so that if the player gets hit
	// he sees what hit them.  Also, preserve the width of the ship
	GetVisibleGameSpace(&visibleGameSpace);
	width = object->location.right - object->location.left;		// save the ship width
	height = object->location.bottom - object->location.top;		// save the ship height

	if (avoidEdges)
		{
		// Reduce the area to pick from
		visibleGameSpace.left += ScreenToGame(borderAroundScreen) / 2;
		visibleGameSpace.top += ScreenToGame(borderAroundScreen) / 2;
		visibleGameSpace.right -= ScreenToGame(borderAroundScreen) / 2;
		visibleGameSpace.bottom -= ScreenToGame(borderAroundScreen) / 2;
		}

	// Pick a location
	object->location.left = RandN(visibleGameSpace.right - width - 
		visibleGameSpace.left) + visibleGameSpace.left;
	object->location.top = RandN(visibleGameSpace.bottom - height - 
		visibleGameSpace.top) + visibleGameSpace.top;
	object->location.right = object->location.left + width;
	object->location.bottom = object->location.top + height;
}


/***********************************************************************
 *
 * FUNCTION:     SectorReset
 * DESCRIPTION:  Reset the sector list to have no objects.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/7/96	Initial Revision
 *
 ***********************************************************************/

static void SectorReset (void)
{
	MemSet(Sector, sizeof(Sector), 0);
}


/***********************************************************************
 *
 * FUNCTION:     GetSector
 *
 * DESCRIPTION:  Get the sector that a point is in.
 *
 * PARAMETERS:   x, y - point location
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/8/96	Initial Revision
 *
 ***********************************************************************/

static UInt8 GetSector(Int16 x, Int16 y)
{
	UInt8 result;


	ErrFatalDisplayIf(x > gameWidth + biggestObject ||
		 x < -ScreenToGame(0), "Object out of bounds");
	ErrFatalDisplayIf(y > gameHeight + biggestObject ||
		y < -ScreenToGame(0), "Object out of bounds");

	// For those points from the outside objects which are from portions 
	// technically wrapped and therefore not within the game space,
	// move them back in.  An example is the bottom half of a rock on
	// the bottom edge.  It lies past gameHeight.  Move it back to
	// gameHeight.
	if (x >= gameWidth)
		x = 0;

	
	if (y >= gameHeight)
		y = 0;

	
	result = (y / sectorHeight);
	result *= sectorsHorizontally;
	result += x / sectorWidth;
	
	return result;
}


/***********************************************************************
 *
 * FUNCTION:     ObjectRemoveFromSector
 *
 * DESCRIPTION:  Remove the object from the sector list
 *
 * PARAMETERS:   object - the object to remove from the sector list
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/10/96	Initial Revision
 *
 ***********************************************************************/
static void ObjectRemoveFromSector (ObjectType *object)
{
	UInt8 sectorMostlyIn;
	ObjectType **lastPointer;
	
	
	// Find which sector the center lies in
	sectorMostlyIn = GetSector(
		(object->location.left + object->location.right) / 2, 
		(object->location.top + object->location.bottom) / 2);
	lastPointer = &Sector[sectorMostlyIn];
	
	
	// Keep looking until the pointer doesn't point to something.
	while (*lastPointer != NULL)
		{
		if (*lastPointer == object)
			{
			*lastPointer = object->nextObjectInSector;
			break;
			}
		
		// Look at the next pointer
		lastPointer = &((*lastPointer)->nextObjectInSector);
		}
}


/***********************************************************************
 *
 * FUNCTION:     SparkAdd
 *
 * DESCRIPTION:  Add a spark to play
 *
 * PARAMETERS:   type - type of spark
 *					  side - the ship side over which the spark is dropped
 *
 * RETURNED:     true if a depth spark is sent
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/20/96	Initial Revision
 *
 ***********************************************************************/

static Boolean SparkAdd (Int16 x, Int16 y, Int16 motionX, Int16 motionY, UInt8 duration)
{
	SparkType *sparkP;
	
	
	// Check to make sure there is storage space for another spark.
	if (GameStatus.objects.sparkCount >= sparksMax)
		return false;


	GameStatus.objects.sparkCount++;
	sparkP = LastSpark;
	
	// Set the location of the spark.
	sparkP->location.x = x;
	sparkP->location.y = y;

	// Set the motion of the spark.
	sparkP->motion.x = motionX;
	sparkP->motion.y = motionY;

	sparkP->duration = duration;
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     SparkRemove
 *
 * DESCRIPTION:  Remove a spark from play.
 * remain.
 *
 * PARAMETERS:   sparkNumber - which spark to remove
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/20/96	Initial Revision
 *
 ***********************************************************************/

static void SparkRemove (SparkType *spark)
{
	SparkType *lastSpark;
	
	
	lastSpark = LastSpark;
	
	ErrFatalDisplayIf(spark > lastSpark, 
		"Removing a spark that doesn't exist");


	// Maintain the sparks ordered in depth ordered by shrinking the list
	// This only needs to be done if the item isn't the last one in the list.
	if (spark != lastSpark)
		{
		MemMove(spark, spark + 1, (lastSpark - spark) * sizeof(SparkType));
		}
	
	GameStatus.objects.sparkCount--;
}


/***********************************************************************
 *
 * FUNCTION:     SparkRemoveExpired
 *
 * DESCRIPTION:  Remove all expired sparks from play.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/20/96	Initial Revision
 *
 ***********************************************************************/

static void SparkRemoveExpired (void)
{
	SparkType *spark;
	SparkType *firstSpark;


	// Move the sparks
	spark = LastSpark;
	firstSpark = GameStatus.objects.spark;
	while (spark >= firstSpark)
		{
		// Remove unusable sparks
		if (spark->duration == 0)
			{
			SparkRemove(spark);
			}
		spark--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     SparkAddBetweenObjects
 *
 * DESCRIPTION:  Add sparks midway between two objects.  Sparks are 
 * affected by the motion of the objects.
 *
 * PARAMETERS:   object1 - pointer to the first object
 *					  object2 - pointer to the second object
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/14/96	Initial Revision
 *
 ***********************************************************************/

static void SparkAddBetweenObjects (ObjectType *object1, ObjectType *object2)
{
	Int16 centerX;			// midpoint between both objects
	Int16 centerY;			// midpoint between both objects
	Int16 sparkMotionX;
	Int16 sparkMotionY;
	Int16 sparkDistance;
	Int16 sparkDuration;
	Int16 sparkCount;


	// Determine some extra motion for the spark from the colliding objects.
	if (object1 != object2)
		{
		sparkMotionX = object1->motion.x + object2->motion.x;
		sparkMotionY = object1->motion.y + object2->motion.y;
		}
	
	
	// Add some sparks for the explosion
	sparkCount = 2 + RandN(4);
	sparkDistance = (ScreenToGame(12) * 2);
	centerX = (object1->location.left + object1->location.right + 
		object2->location.left + object2->location.right) / 4;
	centerY = (object1->location.top + object1->location.bottom + 
		object2->location.top + object2->location.bottom) / 4;
		

	while (sparkCount > 0)
		{
		// The sparks have different durations so that the explosion gradually
		// fades away.
		sparkDuration = sparkDurationAfterCollision + 
			RandN(sparkDurationAfterCollision / 2);
		SparkAdd(centerX, centerY, 
			(RandN(sparkDistance) - (sparkDistance / 2) + sparkMotionX) / sparkDuration,
			(RandN(sparkDistance) - (sparkDistance / 2) + sparkMotionY) / sparkDuration,
			sparkDuration);
		
		sparkCount--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     SparkAddFromObject
 *
 * DESCRIPTION:  Add one spark coming from an object.  The spark is not
 * affected by the motion of the object.
 *
 * PARAMETERS:   object - pointer to the object
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/21/96	Initial Revision
 *
 ***********************************************************************/

static void SparkAddFromObject (ObjectType *object)
{
	Int16 sparkDistance;
	Int16 sparkDuration;


	sparkDistance = (ScreenToGame(12) * 2);
		

	// The sparks have a random duration so that the spark doesn't appear
	// too synchronized with other sparks.
	sparkDuration = sparkDurationFromObject + 
		RandN(sparkDurationFromObject / 2);
	SparkAdd(
		object->location.left + RandN(object->location.right - object->location.left),
		object->location.top + RandN(object->location.bottom - object->location.top),
		(RandN(sparkDistance) - (sparkDistance / 2)) / sparkDuration,
		(RandN(sparkDistance) - (sparkDistance / 2)) / sparkDuration,
		sparkDuration);
		
}


/***********************************************************************
 *
 * FUNCTION:     ScoreAdd
 *
 * DESCRIPTION:  Add a score to play
 *
 * PARAMETERS:   x, y - location to center the score
 *					  value - the value to display
 *
 * RETURNED:     true if the score is displayed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/

static Boolean ScoreAdd (Int16 x, Int16 y, Int16 value)
{
	ScoreType *scoreP;
	Int16 scoreWidth;
	Int16 scoreHeight;
	
	
	// Check to make sure there is storage space for another score.
	if (GameStatus.objects.scoreCount >= scoresMax)
		return false;


	GameStatus.objects.scoreCount++;
	scoreP = LastScore;
	
	// Convert the value to a string
	StrIToA(scoreP->digits, value);
	scoreP->digitCount = StrLen(scoreP->digits);
	
	// Set the location of the score by centering the score around the passed x, y
	FntSetFont(scoreFont);
	scoreWidth = ScreenToGame(FntCharsWidth(scoreP->digits, scoreP->digitCount) - 1) - 1;
	scoreHeight = ScreenToGame(FntBaseLine()) - 1;
	scoreP->location.left = x - scoreWidth / 2;
	scoreP->location.top = y - scoreHeight / 2;
	scoreP->location.right = scoreP->location.left + scoreWidth;
	scoreP->location.bottom = scoreP->location.top + scoreHeight;

	scoreP->duration = scoreDuration;
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     ScoreRemove
 *
 * DESCRIPTION:  Remove a score from play.
 * remain.
 *
 * PARAMETERS:   score - which score to remove
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/

static void ScoreRemove (ScoreType *score)
{
	ScoreType *lastScore;
	
	
	lastScore = LastScore;
	
	ErrFatalDisplayIf(score > lastScore, 
		"Removing a score that doesn't exist");


	// Maintain the scores ordered in depth ordered by shrinking the list
	// This only needs to be done if the item isn't the last one in the list.
	if (score != lastScore)
		{
		MemMove(score, score + 1, (lastScore - score) * sizeof(ScoreType));
		}
	
	GameStatus.objects.scoreCount--;
}


/***********************************************************************
 *
 * FUNCTION:     ScoreRemoveExpired
 *
 * DESCRIPTION:  Remove all expired scores from play.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/

static void ScoreRemoveExpired (void)
{
	ScoreType *score;
	ScoreType *firstScore;


	// Move the scores
	score = LastScore;
	firstScore = GameStatus.objects.score;
	while (score >= firstScore)
		{
		// Remove unusable scores
		if (score->duration == 0)
			{
			ScoreRemove(score);
			}
		score--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     BonusAdd
 *
 * DESCRIPTION:  Add a bonus to play
 *
 * PARAMETERS:   x, y - location to center the bonus
 *					  value - the value to display
 *
 * RETURNED:     true if the bonus is displayed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/

static Boolean BonusAdd ()
{
	BonusType *bonusP;
	ShipType	*shipP;
	ShipType	*firstShipP;
	Boolean bonusTouchesShip;
	
	
	// Check to make sure there is storage space for another bonus.
	if (GameStatus.objects.bonusesCount >= bonusesMax)
		return false;


	GameStatus.objects.bonusesCount++;
	bonusP = LastBonus;
	
	// Pick a type of bonus.  Currently each bonus has an equal chance.
	bonusP->object.type = firstBonusType + RandN(lastBonusType - firstBonusType + 1);
	
	// Make bombs somewhat rarer.
	if (bonusP->object.type == bonusBomb &&
		RandN(2) == 1)
		{
		bonusP->object.type = bonusArmor;
		}
	
	
	bonusP->object.changed = true;
	bonusP->object.usable = true;

	do
		{
		// Set the location of the bonus to somewhere visible
		bonusP->object.location.left = 0;
		bonusP->object.location.top = 0;
		bonusP->object.location.right = bonusWidth;
		bonusP->object.location.bottom = bonusHeight;
		ObjectMoveToVisibleGameSpace(&bonusP->object, false);


		// Check the position of the bonus against all the ships.  If any ship
		// touches the bonus we find a different spot.  The idea is that users
		// should sometimes aren't aware why they've gained bonus powers.  It
		// gives a user more to worry about when they see an alien pick up a bonus.
		bonusTouchesShip = false;
		shipP = LastShip;
		firstShipP = GameStatus.objects.ship;
		while (shipP >= firstShipP)
			{
			if (CheckForCollisionWithObject((ObjectType *) bonusP, (ObjectType *) shipP, true))
				{
				bonusTouchesShip = true;
				break;
				}
			
			// Next ship
			shipP--;
			}
		
		} while (bonusTouchesShip);
	

	bonusP->duration = bonusDuration + RandN(bonusDuration);
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     BonusRemove
 *
 * DESCRIPTION:  Remove a bonus from play.
 * remain.
 *
 * PARAMETERS:   bonus - which bonus to remove
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/

static void BonusRemove (BonusType *bonus)
{
	BonusType *lastBonus;
	
	
	lastBonus = LastBonus;
	
	ErrFatalDisplayIf(bonus > lastBonus, 
		"Removing a bonus that doesn't exist");


	// Maintain the bonuses ordered in depth ordered by shrinking the list
	// This only needs to be done if the item isn't the last one in the list.
	if (bonus != lastBonus)
		{
		MemMove(bonus, bonus + 1, (lastBonus - bonus) * sizeof(BonusType));
		}
	
	GameStatus.objects.bonusesCount--;
}


/***********************************************************************
 *
 * FUNCTION:     BonusRemoveExpiredOrUnusable
 *
 * DESCRIPTION:  Remove all expired bonuses from play.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/

static void BonusRemoveExpiredOrUnusable (void)
{
	BonusType *bonus;
	BonusType *firstBonus;


	// Move the bonuses
	bonus = LastBonus;
	firstBonus = GameStatus.objects.bonus;
	while (bonus >= firstBonus)
		{
		// Remove unusable bonuses
		if (bonus->duration == 0 ||
			!bonus->object.usable)
			{
			BonusRemove(bonus);
			}
		bonus--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     BonusSetNotUsable
 *
 * DESCRIPTION:  Set a bonus unusable.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/1/96	Initial Revision
 *
 ***********************************************************************/

static void BonusSetNotUsable (BonusType *bonus)
{
	ErrFatalDisplayIf(!IsBonus(&bonus->object), "Bad object passed!");

	bonus->object.usable = false;
	bonus->object.changed = false;
}


/***********************************************************************
 *
 * FUNCTION:     RockAdd
 *
 * DESCRIPTION:  Add a rock to play
 *
 * PARAMETERS:   rockType - the type of rock to add
 *					  rockX, rockY - where to place the rock
 *					  motionX, motionY - where the rock should be headed.
 *
 * RETURNED:     true if a rock is placed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/2/96	Initial Revision
 *
 ***********************************************************************/

static Boolean RockAdd (UInt8 rockType, Int16 rockX, Int16 rockY, Int16 motionX, Int16 motionY)
{
	Int16 rockNum;
	Int16 rockWidth;
	Int16 rockHeight;
	
	
	// Remember that new rocks are ordered by depth.  A new rock is
	// therefore added at the end.
	if (GameStatus.objects.rockCount >= rocksMax)
		return false;
	
	
	// determine the width and height of the rock
	switch (rockType)
		{
		case rockSmall:
			rockWidth = rockSmallWidth;
			rockHeight = rockSmallHeight;
			break;
			
		case rockMedium:
			rockWidth = rockMediumWidth;
			rockHeight = rockMediumHeight;
			break;
			
		case rockLarge:
			rockWidth = rockLargeWidth;
			rockHeight = rockLargeHeight;
			break;
		}
		
		
	rockNum = GameStatus.objects.rockCount;
	
	
	// Set the rock up to draw
	GameStatus.objects.rock[rockNum].object.usable = true;
	GameStatus.objects.rock[rockNum].object.changed = true;
	GameStatus.objects.rock[rockNum].object.type = rockType;


	// Set the bounds of the rock
	GameStatus.objects.rock[rockNum].object.location.left = rockX;
	GameStatus.objects.rock[rockNum].object.location.top = rockY;
	GameStatus.objects.rock[rockNum].object.location.right = GameStatus.objects.rock[rockNum].object.location.left + 
		rockWidth;
	GameStatus.objects.rock[rockNum].object.location.bottom = GameStatus.objects.rock[rockNum].object.location.top + 
		rockHeight;
	

	// Set the rock in motion
	GameStatus.objects.rock[rockNum].object.motion.x = motionX;
	GameStatus.objects.rock[rockNum].object.motion.y = motionY;
	
	GameStatus.objects.rockCount++;
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     RockAddInitialRock
 *
 * DESCRIPTION:  Add a large rock if possible.  Don't add if it's close to
 * a ship.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/2/96	Initial Revision
 *
 ***********************************************************************/

static void RockAddInitialRock ()
{
	Int16 rockX;
	Int16 rockY;
	Int16 motionX;
	Int16 motionY;
	ShipType	*shipP;
	Int16 distance;
	Int16 heading;
	Int16 i;
	
	
	rockX = RandN(gameWidth);
	rockY = RandN(gameHeight);
	
	// Check the distance to all the ships
	for (i = shipsMax - 1; i >= 0; i--)
		{
		shipP = &GameStatus.objects.ship[i];
		
		// Draw the ship
		if (shipP->object.usable)
			{
			// Check if the X is too close
			distance = shipP->object.location.left - rockX;
			if (distance < 0)
				distance = -distance;
			
			if (distance < ScreenToGame(30))
				return;
			
			// Check if the Y is too close
			distance = shipP->object.location.top - rockY;
			if (distance < 0)
				distance = -distance;
			
			if (distance < ScreenToGame(30))
				return;
			}
		}


	
	// None of the distances was too close so add the rock.  Pick
	// where the rock is heading.
	heading = RandN(degreesMax);
	motionX = MovementX[heading] * 4;
	motionY = MovementY[heading] * 4;
	RockAdd(rockLarge, rockX, rockY, motionX, motionY);


	GameStatus.rocksToSend--;
}


/***********************************************************************
 *
 * FUNCTION:     RockRemove
 *
 * DESCRIPTION:  Remove a rock from play.
 * remain.
 *
 * PARAMETERS:   rockNumber - which rock to remove
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/10/96	Initial Revision
 *
 ***********************************************************************/

static void RockRemove (RockType *rock)
{
	RockType *lastRock;
	
	
	ErrFatalDisplayIf(!IsRock(&rock->object), "Bad object passed!");


	lastRock = &GameStatus.objects.rock[GameStatus.objects.rockCount - 1];
	
	ErrFatalDisplayIf(rock > lastRock, 
		"Removing a rock that doesn't exist");


	// Maintain the rocks ordered in depth ordered by shrinking the list
	// This only needs to be done if the item isn't the last one in the list.
	if (rock != lastRock)
		{
		MemMove(rock, rock + 1, (lastRock - rock) * sizeof(RockType));
		}
	
	// Wipe over the last rock since it was copied.
	lastRock->object.usable = false;
	lastRock->object.changed = false;
	
	GameStatus.objects.rockCount--;
}


/***********************************************************************
 *
 * FUNCTION:     RockRemoveUnusable
 *
 * DESCRIPTION:  Remove all unusable rocks from play.
 * This must be called after all sector operations are done and preferably
 * before the game state is drawn (to reduce the draw effor).
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/

static void RockRemoveUnusable (void)
{
	int i;
	RockType *rock;


	// Move the rocks
	i = GameStatus.objects.rocksNotUsable;
	rock = LastRock;
	while (i > 0)
		{
		ErrFatalDisplayIf(rock < &GameStatus.objects.rock[0], 
			"rocksNotUsable incorrect");

		// Remove unusable rocks
		if (!rock->object.usable)
			{
			RockRemove(rock);
			i--;
			}
		rock--;
		}
	
	
	// No rocks are now unusable.
	GameStatus.objects.rocksNotUsable = 0;
}	


/***********************************************************************
 *
 * FUNCTION:     RockSetNotUsable
 *
 * DESCRIPTION:  Set a rock unusable.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/

static void RockSetNotUsable (RockType *rock)
{
	ErrFatalDisplayIf(!IsRock(&rock->object), "Bad object passed!");


	rock->object.usable = false;
	rock->object.changed = false;
	
	GameStatus.objects.rocksNotUsable++;
}


/***********************************************************************
 *
 * FUNCTION:     RockExplode
 *
 * DESCRIPTION:  Explode the rock.  If the rock isn't the smallest rock
 * it releases two smaller fragments.  Sparks fly from the scene of the
 * explosion.  The exploded rock is set not usable.  If the destroyer
 * is the rock itself then assume the rock just exploded (probably from
 * a bomb).
 *
 * PARAMETERS:   rock - pointer to rock to explode
 *					  destroyer - pointer to destroyer
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/10/96	Initial Revision
 *
 ***********************************************************************/

static void RockExplode (RockType *rock, ObjectType *destroyer)
{
	Int16 centerX;			// Center of rock exploding
	Int16 centerY;			// Center of rock exploding
	Int16 sparkDistance;
	Int16 sparkDuration;
	Int16 sparkCount = 2;
	SoundType explosionSound;


	ErrFatalDisplayIf(!IsRock(&rock->object), "Bad object passed!");


	if (destroyer != &rock->object)
		{
		// We are going to combine the motion of the destroyer to that of the
		// rock.  Shots should only contribute a fraction of their motion because
		// there high speed would otherwise overwelm that of the rock's.
		if (IsShot(destroyer))
			{
			destroyer->motion.x /= 16;
			destroyer->motion.y /= 16;
			}
		
		// Add the motion of the destroyer to the rock
		rock->object.motion.x += destroyer->motion.x;
		rock->object.motion.y += destroyer->motion.y;
		}
	
	
	// Rocks which aren't already as small as can be are split into two 
	// smaller rocks which move ninety degrees from their parent's 
	// altered course.
	if (rock->object.type != rockSmall)
		{
		centerX = (rock->object.location.left + rock->object.location.right) / 2;
		centerY = (rock->object.location.top + rock->object.location.bottom) / 2;

		if (rock->object.type == rockLarge)
			{
			sparkCount = 5;

			RockAdd(rockMedium, 
				centerX - rockMediumWidth / 2, 
				centerY - rockMediumHeight / 2,
				(rock->object.motion.x - rock->object.motion.y) / 1,
				(rock->object.motion.y + rock->object.motion.x) / 1);
			RockAdd(rockMedium, 
				centerX - rockMediumWidth / 2, 
				centerY - rockMediumHeight / 2,
				(rock->object.motion.x + rock->object.motion.y) / 1,
				(rock->object.motion.y - rock->object.motion.x) / 1);
			
			explosionSound = rockLargeExplosion;
			}
		else if (rock->object.type == rockMedium)
			{
			sparkCount = 3;

			RockAdd(rockSmall, 
				centerX - rockSmallWidth / 2, 
				centerY - rockSmallHeight / 2,
				(rock->object.motion.x - rock->object.motion.y) / 1,
				(rock->object.motion.y + rock->object.motion.x) / 1);
			RockAdd(rockSmall, 
				centerX - rockSmallWidth / 2, 
				centerY - rockSmallHeight / 2,
				(rock->object.motion.x + rock->object.motion.y) / 1,
				(rock->object.motion.y - rock->object.motion.x) / 1);
			
			explosionSound = rockMediumExplosion;
			}
		
		// Allow a small rock to sometimes be released
		if (RandN(6) == 1)
			{
			sparkCount *= 2;
			
			RockAdd(rockSmall, 
				centerX - rockSmallWidth / 2, 
				centerY - rockSmallHeight / 2,
				(rock->object.motion.x + rock->object.motion.x) / 1,
				(rock->object.motion.y + rock->object.motion.x) / 1);
			}
		}
	else
		{
		explosionSound = rockSmallExplosion;
		}
		

	// Add some sparks for the explosion
	sparkCount += RandN(3);
	sparkDistance = ((rock->object.location.right - rock->object.location.left) * 4);
	centerX = (rock->object.location.left + rock->object.location.right + 
		destroyer->location.left + destroyer->location.right) / 4;
	centerY = (rock->object.location.top + rock->object.location.bottom + 
		destroyer->location.top + destroyer->location.bottom) / 4;
		

	while (sparkCount > 0)
		{
		// The sparks have different durations so that the explosion gradually
		// fades away.
		sparkDuration = sparkDurationAfterRockExplosion + 
			RandN(sparkDurationAfterRockExplosion / 2);
		SparkAdd(centerX, centerY, 
			(RandN(sparkDistance) - (sparkDistance / 2)) / sparkDuration,
			(RandN(sparkDistance) - (sparkDistance / 2)) / sparkDuration,
			sparkDuration);
		
		sparkCount--;
		}


	RockSetNotUsable (rock);
	GameRequestSound (explosionSound);
}


/***********************************************************************
 *
 * FUNCTION:     ShipGetNearestPlayer
 *
 * DESCRIPTION:  Find a player's ship nearest a given point
 *
 * PARAMETERS:   point - point to find nearest ship to
 *
 * RETURNED:     NULL if there isn't a player owned ship or 
 *					  a pointer to the nearest player owned ship.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/1/96	Initial Revision
 *
 ***********************************************************************/

static ShipType *ShipGetNearestPlayer (GamePoint *point)
{
	ShipType	*ship;
	ShipType	*firstShip;
	Int32 distanceX;
	Int32 distanceY;
	Int32 distance;
	Int32 nearestDistance = 0x7fffffff;
	ShipType	*nearestShip = NULL;

	
	ship = LastShip;
	firstShip = GameStatus.objects.ship;
	while (ship >= firstShip)
		{
		// Only pay attention to human controlled ships in normal space
		if (ShipIsOwnedByHuman(ship) &&
			!ship->status.inWarp &&
			!ship->status.exitingWarp &&
			!ship->status.type.player.appearSafely)
			{
			distanceX = point->x - ship->object.location.left;
			distanceY = point->y - ship->object.location.top;
			distance = distanceX * distanceX + distanceY * distanceY;
			if (distance < nearestDistance)
				{
				nearestDistance = distance;
				nearestShip = ship;
				}
			}
		
		ship--;
		}
	
	return nearestShip;
}


/***********************************************************************
 *
 * FUNCTION:     ShipFindByUniqueID
 *
 * DESCRIPTION:  Find a player's ship by it's unique ID
 *
 * PARAMETERS:   uniqueID - the ID to search for
 *
 * RETURNED:     NULL if there isn't a ship using the ID or 
 *					  a pointer to the ship found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/13/96	Initial Revision
 *
 ***********************************************************************/

static ShipType *ShipFindByUniqueID (ShipUniqueIDType uniqueID)
{
	ShipType	*ship;
	ShipType	*firstShip;

	
	ship = LastShip;
	firstShip = GameStatus.objects.ship;
	while (ship >= firstShip)
		{
		if (ship->uniqueShipID == uniqueID)
			return ship;
		
		ship--;
		}
	
	return NULL;
}


/***********************************************************************
 *
 * FUNCTION:     ShipInitUniqueID
 *
 * DESCRIPTION:  Initialize the unique ship ID code
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/7/96	Initial Revision
 *
 ***********************************************************************/

static void ShipInitUniqueID ()
{
	GameStatus.objects.nextUniqueShipID = 0;
}


/***********************************************************************
 *
 * FUNCTION:     ShipGetNewUniqueID
 *
 * DESCRIPTION:  Generate another unique ship ID
 *
 * The idea behind this routine is that works its way through a number
 * space large enough so that it never ends before a game ends.
 * ShipUniqueIDType must be larger than the number of ships generated
 * in a game.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     ShipUniqueIDType
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/7/96	Initial Revision
 *
 ***********************************************************************/

static ShipUniqueIDType ShipGetNewUniqueID ()
{
	return GameStatus.objects.nextUniqueShipID++;
}


/***********************************************************************
 *
 * FUNCTION:     ShipRemoveFromFormation
 *
 * DESCRIPTION:  Remove a ship from it's formation position and mark
 * the position as unused.
 *
 * PARAMETERS:   shipP - the ship to relocate
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/12/96	Initial Revision
 *
 ***********************************************************************/
	
static void ShipRemoveFromFormation (ShipType *shipP)
{
	PlayerType *playerP;
	ShipUniqueIDType *shipPositionP;
	ShipUniqueIDType *lastShipPositionP;
	UInt8 positionInFormation;
	
	
	playerP = &GameStatus.stats[shipP->owner];
	
	// We search forward since ship's are more likely to appear at the beginning
	// of the formation.
	shipPositionP = &playerP->shipFormation[0];
	lastShipPositionP = &playerP->shipFormation[shipsInFormationMax - 1];
	while (shipPositionP <= lastShipPositionP)
		{
		if (*shipPositionP == shipP->uniqueShipID)
			{
			// Remove the ship from the formation
			positionInFormation = shipPositionP - &playerP->shipFormation[0];
			playerP->shipFormation[positionInFormation] = invalidUniqueShipID;
			playerP->shipFormationCount--;
			
			break;
			}
		
		shipPositionP++;
		}
}


/***********************************************************************
 *
 * FUNCTION:     ShipPositionInFormation
 *
 * DESCRIPTION:  Move a ship into the next unoccupied formation position.
 * This doesn't actually record the ship as occupying the position but it
 * does move the ship
 *
 * PARAMETERS:   shipP - the ship to relocate
 *
 * RETURNED:     the position number
 *					  the ship is moved
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/12/96	Initial Revision
 *
 ***********************************************************************/
	
static UInt8 ShipPositionInFormation (ShipType *shipP)
{
	Int16 shipX;
	Int16 shipY;
	Int16 offset;
	Int16 shipWidth;
	Int16 shipHeight;
	PlayerType *playerP;
	ShipUniqueIDType *shipPositionP;
	ShipUniqueIDType *emptyShipPositionP;
	ShipUniqueIDType *lastShipPositionP;
	ShipType	*shipInFormationP;
	UInt8 positionInFormation;
	

	shipWidth = shipP->object.location.right - shipP->object.location.left;
	shipHeight = shipP->object.location.bottom - shipP->object.location.top;
	
	playerP = &GameStatus.stats[shipP->owner];
	
	lastShipPositionP = &playerP->shipFormation[shipsInFormationMax - 1];
	
	// Find the first empty formation position
	emptyShipPositionP = &playerP->shipFormation[0];
	while (emptyShipPositionP <= lastShipPositionP)
		{
		if (*emptyShipPositionP == invalidUniqueShipID)
			break;
		
		emptyShipPositionP++;
		}
	
	// Find the first ship in formation
	shipPositionP = &playerP->shipFormation[0];
	while (shipPositionP <= lastShipPositionP)
		{
		if (*shipPositionP != invalidUniqueShipID)
			break;
		
		shipPositionP++;
		}
	
	// Find the start of the formation.  If the first position is used
	// then use the start from that ship.  Else calculate where the formation
	// starts from the ship's position in the formation.
	shipInFormationP = ShipFindByUniqueID(*shipPositionP);
	shipX = shipInFormationP->object.location.left;
	shipY = shipInFormationP->object.location.top;
	if (shipPositionP != &playerP->shipFormation[0])
		{
		positionInFormation = shipPositionP - &playerP->shipFormation[0];
		
		// The ships are spaced 150% the diameter of the ship away from each other.
		offset = MovementX[(positionInFormation - 1) * 2];
		offset = (shipWidth * (offset + offset / 2)) / 2;		// offset = offset * 3 / 2;
		shipX -= offset;
		
		offset = MovementY[(positionInFormation - 1) * 2];
		offset = (shipHeight * (offset + offset / 2)) / 2;		// offset = offset * 3 / 2;
		shipY -= offset;
		}
	
	// Now calculate where the ship should be moved to.  Remember that the
	// first ship is in the center of the formation.
	if (emptyShipPositionP == &playerP->shipFormation[0])
		{
		positionInFormation = 0;
		}
	else
		{
		positionInFormation = emptyShipPositionP - &playerP->shipFormation[0];
		
		// The ships are spaced 150% the diameter of the ship away from each other.
		offset = MovementX[(positionInFormation - 1) * 2];
		offset = (shipWidth * (offset + offset / 2)) / 2;		// offset = offset * 3 / 2;
		shipX -= offset;
		
		offset = MovementY[(positionInFormation - 1) * 2];
		offset = (shipHeight * (offset + offset / 2)) / 2;		// offset = offset * 3 / 2;
		shipY -= offset;
		}

	// Now move the ship
	shipP->object.location.left = shipX;
	shipP->object.location.top = shipY;
	shipP->object.location.right = shipX + shipWidth;
	shipP->object.location.bottom = shipY + shipHeight;
	
	// Now make the ship move like the one in formation so that it remains there
	shipP->heading = shipInFormationP->heading;
	shipP->object.motion.x = shipInFormationP->object.motion.x;
	shipP->object.motion.y = shipInFormationP->object.motion.y;
	
	return positionInFormation;
}


/***********************************************************************
 *
 * FUNCTION:     ShipAppearSafely
 *
 * DESCRIPTION:  Place a ship on the screen if it's safe to do so.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/11/96	Initial Revision
 *
 ***********************************************************************/
	
static void ShipAppearSafely (ShipType	*shipP)
{
	Int16 position;
	Int16 shipWidth;
	Int16 shipHeight;
	Int16 positionCase;
	UInt8 formationPosition;
	PlayerType *playerP;
	
	
	playerP = &GameStatus.stats[shipP->owner];
	
	shipWidth = shipP->object.location.right - shipP->object.location.left;
	shipHeight = shipP->object.location.bottom - shipP->object.location.top;
	
	if (playerP->shipFormationCount == 0)
		{
		// Pick a random position to make the ship appear.  If the alien 
		// HomeWorld is present then place player ships in the corners.
		if (IsAlienHomeWorldLevel(GameStatus.level) && 
			GameStatus.objects.baseCount > 0)
			positionCase = startPositionCases - 1;
		else
			positionCase = GameStatus.playerCount - 1;
			
		position = RandN(positionCase + 1);
		
		// Now move the ship
		shipP->object.location.left = ShipPositionToStart[positionCase][position].x;
		shipP->object.location.top = ShipPositionToStart[positionCase][position].y;
		shipP->object.location.right = ShipPositionToStart[positionCase][position].x + shipWidth;
		shipP->object.location.bottom = ShipPositionToStart[positionCase][position].y + shipHeight;
		
		formationPosition = 0;
		}
	else
		{
		formationPosition = ShipPositionInFormation (shipP);
		}
	
	
	// Expand the bounds of the ship to included the area where
	// we don't want to find objects (because they would be close 
	// enough to hit the ship within too short of a time period).
	shipP->object.location.left -= safetyDistanceAroundShip;
	shipP->object.location.top -= safetyDistanceAroundShip;
	shipP->object.location.right += safetyDistanceAroundShip;
	shipP->object.location.bottom += safetyDistanceAroundShip;
	
	
	// If no object is detected within the bounds of the ship (including the
	// safetyDistanceAroundShip) then make the ship appear.
	if (SectorAddObject((ObjectType *) shipP, true))
		{
		// Remove safetyDistanceAroundShip from the ship's bounds.  Do this
		// before SectorAddObject to reduce the area checked (again!).
		shipP->object.location.left += safetyDistanceAroundShip;
		shipP->object.location.top += safetyDistanceAroundShip;
		shipP->object.location.right -= safetyDistanceAroundShip;
		shipP->object.location.bottom -= safetyDistanceAroundShip;
		
		SectorAddObject((ObjectType *) shipP, false);
		
		// Add the ship to the formation
		playerP->shipFormation[formationPosition] = shipP->uniqueShipID;
		playerP->shipFormationCount++;
		
		// Now remove 
		shipP->status.type.player.appearSafely = false;
		}
	else
		{
		// Restore the width so that it can be calculated the next time here.
		shipP->object.location.left = 0;
		shipP->object.location.top = 0;
		shipP->object.location.right = shipWidth;
		shipP->object.location.bottom = shipHeight;
		}
	
}


/***********************************************************************
 *
 * FUNCTION:     ShipAdd
 *
 * DESCRIPTION:  Place a new ship at the center of the screen
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/1/96	Initial Revision
 *
 ***********************************************************************/

static Boolean ShipAddPlayer (UInt8 playerNum)
{
	ShipType	*shipP;
	
	
	// Check to make sure there is storage space for another ship.
	if (GameStatus.objects.shipCount >= shipsMax)
		return false;


	shipP = &GameStatus.objects.ship[GameStatus.objects.shipCount++];
	
	shipP->object.type = shipPlayer;
	shipP->object.changed = true;
	shipP->object.usable = true;
	
	// Mark the ship for ShipAppearSafely to place it.  Just set the width
	// correctly.
	shipP->status.type.player.appearSafely = true;		// appear only when safe
	shipP->object.location.left = 0;
	shipP->object.location.top = 0;
	shipP->object.location.right = shipPlayerWidth - 1;
	shipP->object.location.bottom = shipPlayerHeight - 1;

	shipP->heading = RandN(degreesMax);	// degrees90;
	shipP->object.motion.x = 0;
	shipP->object.motion.y = 0;
	
	shipP->shotsAvailable = 4;
	shipP->armorAvailable = 0;
	shipP->status.type.player.finishedSkit = false;
	shipP->status.type.player.rotateLeft = false;
	shipP->status.type.player.rotateRight = false;
	shipP->status.type.player.thrusterOn = false;
	shipP->status.shieldOn = false;
	shipP->status.enteringWarp = false;
	shipP->status.inWarp = false;
	shipP->status.exitingWarp = false;
	shipP->status.longShots = false;
	shipP->status.retroRockets = false;
	
	
	shipP->periodsToWait = 0;

	shipP->owner = playerNum;
	shipP->uniqueShipID = ShipGetNewUniqueID();
	
	// Now record player stats changes
	GameStatus.stats[playerNum].shipsOwned++;
	GameStatus.stats[playerNum].periodsUntilAnotherShip = periodsToWaitForAnotherShip;
	
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     ShipRemove
 *
 * DESCRIPTION:  Remove a ship from play.
 * remain.
 *
 * PARAMETERS:   shipNumber - which ship to remove
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/2/96	Initial Revision
 *
 ***********************************************************************/

static void ShipRemove (ShipType *ship)
{
	ShipType *lastShip;
	
	
	ErrFatalDisplayIf(!IsShip(&ship->object), "Bad object passed!");


	// Record the fact that the player is down a ship so that they may
	// receive another one.
	if (ShipIsOwnedByHuman(ship))
		{
		GameStatus.stats[ship->owner].shipsOwned--;
		
		// Find the ship's position in the formation and mark it unused
		// assuming the ship wasn't in warp
		if (!ship->status.inWarp &&
			!ship->status.exitingWarp &&
			!ship->status.type.player.appearSafely)
			{
			ShipRemoveFromFormation(ship);
			}
		
		}
	else
		{
		// Keep track of the number aliens
		GameStatus.objects.alienCount--;
		if (IsBase(&ship->object))
			{
			GameStatus.objects.baseCount--;
			}
		}


	lastShip = &GameStatus.objects.ship[GameStatus.objects.shipCount - 1];
	
	ErrFatalDisplayIf(ship > lastShip, 
		"Removing a ship that doesn't exist");


	// Maintain the ships ordered in depth ordered by shrinking the list
	// This only needs to be done if the item isn't the last one in the list.
	if (ship != lastShip)
		{
		MemMove(ship, ship + 1, (lastShip - ship) * sizeof(ShipType));
		}
	
	// Wipe over the last ship since it was copied.
	lastShip->object.usable = false;
	lastShip->object.changed = false;
	
	GameStatus.objects.shipCount--;
}


/***********************************************************************
 *
 * FUNCTION:     ShipRemoveUnusable
 *
 * DESCRIPTION:  Remove all unusable ships from play.
 * This must be called after all sector operations are done and preferably
 * before the game state is drawn (to reduce the draw effor).
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/

static void ShipRemoveUnusable (void)
{
	int i;
	ShipType *ship;


	// Remove unusable ships
	ship = LastShip;
	i = GameStatus.objects.shipsNotUsable;
	while (i > 0)
		{
		ErrFatalDisplayIf(ship < &GameStatus.objects.ship[0], 
			"shipsNotUsable incorrect");

		// Remove unusable ships
		if (!ship->object.usable)
			{
			ShipRemove(ship);
			i--;
			}
		ship--;
		}
	
	
	// No ships are now unusable.
	GameStatus.objects.shipsNotUsable = 0;
}	


/***********************************************************************
 *
 * FUNCTION:     ShipSetNotUsable
 *
 * DESCRIPTION:  Set a ship unusable.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/

static void ShipSetNotUsable (ShipType *ship)
{
	ErrFatalDisplayIf(!IsShip(&ship->object), "Bad object passed!");

	ship->object.usable = false;
	ship->object.changed = false;
	
	GameStatus.objects.shipsNotUsable++;
}


/***********************************************************************
 *
 * FUNCTION:     PlanetExplode
 *
 * DESCRIPTION:  Explode the planet.  Send rocks out.  Ship explode
 * handles the the normal things.
 *
 * PARAMETERS:   ship - pointer to ship to explode
 *					  destroyer - pointer to destroyer
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/20/96	Initial Revision
 *
 ***********************************************************************/

static void PlanetExplode (ShipType *ship)
{
	Int16 centerX;			// Center of rock exploding
	Int16 centerY;			// Center of rock exploding
	Int16 rockDistance;
	Int16 rockCount;
	Int16 heading;
	Int16 motionX;
	Int16 motionY;


	ErrFatalDisplayIf(!IsPlanet(&ship->object), "Bad object passed!");

	
	// Add some rocks for the explosion
	centerX = ObjectCenterX(&ship->object);
	centerY = ObjectCenterY(&ship->object);
		

	// Add some large rocks
	rockCount = largeRocksFromExplodedPlanet;
	rockDistance = shipAlienHomeWorldWidth - rockLargeWidth;
	while (rockCount > 0)
		{
		heading = RandN(degreesMax);
		motionX = MovementX[heading] * 4;
		motionY = MovementY[heading] * 4;
		
		RockAdd(rockLarge, 
			centerX + (RandN(rockDistance) - (rockDistance / 2)), 
			centerY + (RandN(rockDistance) - (rockDistance / 2)), 
			motionX, motionY);
		
		rockCount--;
		}
	
	// With the home world destroyed stop sending in aliens.
	GameStatus.aliensToSend = 0;
	
//	GameRequestSound (shipExplosion);
}


/***********************************************************************
 *
 * FUNCTION:     ShipExplode
 *
 * DESCRIPTION:  Explode the ship.  Send sparks from the explosion site
 * which is the center of the ship (the engine room).  Set the ship not 
 * usable.  If the destoryer is the ship then consider the ship to have
 * exploded itself (or a bomb hit).
 *
 * PARAMETERS:   ship - pointer to ship to explode
 *					  destroyer - pointer to destroyer
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/7/96	Initial Revision
 *
 ***********************************************************************/

static void ShipExplode (ShipType *ship, ObjectType *destroyer)
{
	Int16 centerX;			// Center of rock exploding
	Int16 centerY;			// Center of rock exploding
	Int16 sparkDistance;
	Int16 sparkDuration;
	Int16 sparkCount;


	ErrFatalDisplayIf(!IsShip(&ship->object), "Bad object passed!");

			
	// When a planet explodes release lots of rocks.
	if (IsPlanet(&ship->object))
		{
		PlanetExplode(ship);
		
		// Use all free sparks to explode the planet
		sparkCount = sparksMax - GameStatus.objects.sparkCount;
		}
	else
		{
		sparkCount = 10 + RandN(10);
		}
	
	
	// Add some sparks for the explosion
	sparkDistance = ((ship->object.location.right - ship->object.location.left) * 4);
	centerX = ObjectCenterX(&ship->object);
	centerY = ObjectCenterY(&ship->object);
		

	// Calculate the combined motion because we make the sparks travel in
	// that direction.
	if (&ship->object != destroyer)
		{
		ship->object.motion.x = (ship->object.motion.x + destroyer->motion.x) / 2;
		ship->object.motion.y = (ship->object.motion.y + destroyer->motion.y) / 2;
		}
	
	while (sparkCount > 0)
		{
		sparkDuration = sparkDurationAfterShipExplosion + 
			RandN(sparkDurationAfterShipExplosion / 2);
		SparkAdd(centerX, centerY, 
			(RandN(sparkDistance) - (sparkDistance / 2)) / sparkDuration + ship->object.motion.x,
			(RandN(sparkDistance) - (sparkDistance / 2)) / sparkDuration + ship->object.motion.y,
			sparkDuration);
		
		sparkCount--;
		}


	ShipSetNotUsable (ship);
	GameRequestSound (shipExplosion);
}


/***********************************************************************
 *
 * FUNCTION:     ShipAwardBonus
 *
 * DESCRIPTION:  Award a bonus to a ship.
 * remain.
 *
 * PARAMETERS:   ship - pointer to ship to award a bonus
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/1/96	Initial Revision
 *
 ***********************************************************************/

static void ShipAwardBonus (ShipType *ship, BonusType *bonus)
{
	Int16 scoreIncrease;
	UInt8 bonusMultiplier = 0;
	
	
	// If something has been awarded before and can't be bestowed then
	// we award extra points.
	switch (bonus->object.type)
		{
		case bonusExtraShots:
			ship->shotsAvailable += 2;
			break;

		case bonusLongShots:
			if (ship->status.longShots)
				bonusMultiplier = 2;
			else
				ship->status.longShots = true;
			break;

		case bonusRetroRockets:
			if (ship->status.retroRockets)
				bonusMultiplier = 2;
			else
				ship->status.retroRockets = true;
			break;

		case bonusScore:
			bonusMultiplier = 1;
			break;

		case bonusArmor:
			ship->armorAvailable++;
			if (ShipIsOwnedByHuman(ship))
				GameStatus.stats[ship->owner].armorChanged = true;
			break;

		case bonusBomb:
			GameStatus.bombExplode = true;
			GameStatus.bombOwner = ship->owner;
			break;
		}
	
	
	// Are we to award points?
	if (bonusMultiplier > 0)
		{
		if (ShipIsOwnedByHuman(ship))
			{
			scoreIncrease = lowBottomScore + 
				RandN((highBottomScore - lowBottomScore) / 25) * 25;
			ScoreAdd(ObjectCenterX(&bonus->object), ObjectCenterY(&bonus->object), scoreIncrease);
			IncreaseScore(scoreIncrease, ship->owner);
			}
		}
	
	// Play a sound to notify the user of the bonus
	if (ShipIsOwnedByHuman(ship))
		{
		GameRequestSound (bonusAwarded);
		}
}


/***********************************************************************
 *
 * FUNCTION:     ShotPlace
 *
 * DESCRIPTION:  Place a shot just outside of the ship that added it.
 *
 * PARAMETERS:   shotP - shot to place
 *					  side - the ship side over which the shot is dropped
 *
 * RETURNED:     true if a shot is sent
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/12/96	Initial Revision
 *
 ***********************************************************************/

static void ShotPlace (ShotType *shotP, ShipType *shipP)
{
	Int16 distanceX;
	Int16 distanceY;
	Int16 width;
	Int16 height;
	
	
	ErrFatalDisplayIf(!IsShot(&shotP->object), "Bad object passed!");
	ErrFatalDisplayIf(!IsShip(&shipP->object), "Bad object passed!");


	// handle different sizes for the different shots
	if (shotP->object.type == shotPlasma)
		{
		width = shotPlasmaWidth;
		height = shotPlasmaHeight;
		}
	else
		{
		width = shotWidth;
		height = shotHeight;
		}
	
	
	// Calculate the amount to move past the ship horizontally
	if (shotP->object.motion.x != 0)
		{
		distanceX = shipP->object.location.right - shipP->object.location.left;
		distanceX = (distanceX / 2 + 2 * width) / shotP->object.motion.x;
		if (distanceX < 0)
			distanceX = -distanceX;
		}
	else
		distanceX = 0;
	
	// Calculate the amount to move past the ship vertically
	if (shotP->object.motion.y != 0)
		{
		distanceY = shipP->object.location.bottom - shipP->object.location.top;
		distanceY = (distanceY / 2 + 2 * height) / shotP->object.motion.y;
		if (distanceY < 0)
			distanceY = -distanceY;
		}
	else
		distanceY = 0;
	
	
	if ((distanceX < distanceY && distanceX != 0) ||
		distanceY == 0)
		{
		shotP->object.location.left += shotP->object.motion.x * distanceX;
		shotP->object.location.top += shotP->object.motion.y * distanceX;
		}
	else
		{
		shotP->object.location.left += shotP->object.motion.x * distanceY;
		shotP->object.location.top += shotP->object.motion.y * distanceY;
		}
	
	
	// Set the other sides of the shot
	shotP->object.location.right = shotP->object.location.left + width;
	shotP->object.location.bottom = shotP->object.location.top + height;
}


/***********************************************************************
 *
 * FUNCTION:     ShotHeadTowardsObject
 *
 * DESCRIPTION:  Change the motion of a shot to make it heads towards 
 *	an object.  Aimed shots should travel half normal speed to give the 
 * player more time to react.
 *
 * PARAMETERS:   shotP - shot to place
 *					  objectP - the object to head towards
 *
 * RETURNED:     true if a shot is sent
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/20/96	Initial Revision
 *
 ***********************************************************************/

static void ShotHeadTowardsObject (ShotType *shotP, ObjectType *objectP,
	Int16 *motionX, Int16 *motionY)
{
	Int16 localMotionX;
	Int16 localMotionY;
	Int16 distX;
	Int16 distY;
	Int16 variation;
	Int16 ratio;
	
	
	ErrFatalDisplayIf(objectP == NULL, "Bad object passed!");
	
	
	// Calculate a destination for the shot to head to.  The destination is
	// the object's center. Add to that a freak chance to aim poorly.  This
	// means sometimes ace aliens miss.  It also makes plasma have an occasional
	// tracking glitch.  Next, the further away the object, the more variation 
	// on the aim.  From this value we subtract the start to find a distance
	// to the destination picked.  Divide this amount of movement over a period
	// of turns so it takes more than one tick to reach the destination.
	// Note the calculation is performed slightly out of obvious order in
	// order to reuse some values.
	localMotionX = ObjectCenterX(objectP) - ObjectCenterX(&shotP->object);
	distX = AbsoluteValue(localMotionX);
	if (distX > gameWidth / 2)
		{
		// Travel offscreen after the object because that way is closer.
		distX = gameWidth - distX;			// the distance going the other way
		
		// Head the other direction than was calculated
		if (localMotionX >= 0)
			localMotionX = -distX;
		else
			localMotionX = distX;
		}
		
	localMotionY = ObjectCenterY(objectP) - ObjectCenterY(&shotP->object);
	distY = AbsoluteValue(localMotionY);
	if (distY > gameWidth / 2)
		{
		// Travel offscreen after the object because that way is closer.
		distY = gameWidth - distY;			// the distance going the other way
		
		// Head the other direction than was calculated
		if (localMotionY >= 0)
			localMotionY = -distY;
		else
			localMotionY = distY;
		}
		
	variation = max(distX, distY);
	variation /= 4;
	
	// This is the variation which increases with distance.
	localMotionX += RandN(variation) - variation / 2;
	localMotionY += RandN(variation) - variation / 2;
	
	// This is the variation caused by a freak shot.
	if (RandN(100) < chanceToAimPoorly)
		{
		localMotionX += RandN(shotAimedVariation) - shotAimedVariation / 2;
		localMotionY += RandN(shotAimedVariation) - shotAimedVariation / 2;
		}
	
	// Spread the motion over more game ticks.
	if (AbsoluteValue(localMotionX) >= AbsoluteValue(localMotionY))
		{
//		ratio = AbsoluteValue(localMotionX) / (shotSpeed / 2);
		ratio = AbsoluteValue(localMotionX) / shotSpeed;
		}
	else
		{
		ratio = AbsoluteValue(localMotionY) / shotSpeed;
		}
	localMotionX = localMotionX / ratio;
	localMotionY = localMotionY / ratio;
	
	// Slow down the shots so the player has time to react.  Note that
	// this also reduces the distance that the shots travel.
	*motionX = localMotionX - localMotionX / 4;
	*motionY = localMotionY - localMotionY / 4;
}


/***********************************************************************
 *
 * FUNCTION:     ShotAdd
 *
 * DESCRIPTION:  Add a shot to play
 *
 * PARAMETERS:   type - type of shot
 *					  side - the ship side over which the shot is dropped
 *
 * RETURNED:     true if a shot is sent
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/12/96	Initial Revision
 *
 ***********************************************************************/

static Boolean ShotAdd (UInt8 type, ShipType *shipP, Int16 motionX, Int16 motionY)
{
	Int16 shotNum;
	ShotType *shotP;
	Int16 shipWidth;
	Int16 shipHeight;
	
	
	ErrFatalDisplayIf(!IsShip(&shipP->object), "Bad object passed!");


	// Check to make sure there is storage space for another shot.
	if (GameStatus.objects.shotCount >= shotsMax)
		return false;


	// Does the ship have a shot available?
	if (shipP->shotsAvailable == 0)
		return false;
	shipP->shotsAvailable--;		// deduct the shot from the ship
	
	shotNum = GameStatus.objects.shotCount++;
	shotP = &GameStatus.objects.shot[shotNum];
	
	// Set the shot up to draw
	shotP->object.usable = true;
	shotP->object.changed = true;
	shotP->object.type = type;
	if (type == shotPlasma)
		shotP->duration = shotPlasmaDuration;
	else
		shotP->duration = shipP->status.longShots ? shotLongDuration : shotDuration;
	shotP->ownerPlayerNumber = shipP->owner;
	shotP->ownerUniqueShipID = shipP->uniqueShipID;
	

	// Set the heading.  It's motion is not affected by the ship's motion.
	shotP->object.motion.x = motionX;
	shotP->object.motion.y = motionY;
	
		
	// Set the bounds of the shot.  First locate in the center of the ship
	shipWidth = (shipP->object.location.right - shipP->object.location.left) / 2;
	shipHeight = (shipP->object.location.bottom - shipP->object.location.top) / 2;
	shotP->object.location.left = shipP->object.location.left + shipWidth - shotWidth / 2;
	shotP->object.location.top = shipP->object.location.top + shipHeight - shotHeight / 2;

	// Now place the shot just outside the ship's hull
	ShotPlace(shotP, shipP);
	
	
	GameRequestSound (releaseShot);
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     ShotRemove
 *
 * DESCRIPTION:  Remove a shot from play.
 * remain.
 *
 * PARAMETERS:   shotNumber - which shot to remove
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/2/96	Initial Revision
 *
 ***********************************************************************/

static void ShotRemove (ShotType *shot)
{
	ShotType *lastShot;
	ShipType *shipP;
	ShipType	*firstShipP;
	
	
	ErrFatalDisplayIf(!IsShot(&shot->object), "Bad object passed!");

	// Find the ship who fired this shot and let the ship have another 
	// shot available.
	shipP = LastShip;
	firstShipP = GameStatus.objects.ship;
	while (shipP >= firstShipP)
		{
		if (shipP->uniqueShipID == shot->ownerUniqueShipID)
			{
			shipP->shotsAvailable++;
			break;
			}
		
		shipP--;
		}


	lastShot = LastShot;
	
	ErrFatalDisplayIf(shot > lastShot, 
		"Removing a shot that doesn't exist");


	// Maintain the shots ordered in depth ordered by shrinking the list
	// This only needs to be done if the item isn't the last one in the list.
	if (shot != lastShot)
		{
		MemMove(shot, shot + 1, (lastShot - shot) * sizeof(ShotType));
		}
	
	// Wipe over the last shot since it was copied.
	lastShot->object.usable = false;
	lastShot->object.changed = false;
	
	GameStatus.objects.shotCount--;
}


/***********************************************************************
 *
 * FUNCTION:     ShotRemoveUnusable
 *
 * DESCRIPTION:  Remove all unusable shots from play.
 * This must be called after all sector operations are done and preferably
 * before the game state is drawn (to reduce the draw effor).
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/

static void ShotRemoveUnusable (void)
{
	int i;
	ShotType *shot;


	// Move the shots
	i = GameStatus.objects.shotsNotUsable;
	shot = LastShot;
	while (i > 0)
		{
		ErrFatalDisplayIf(shot < &GameStatus.objects.shot[0], 
			"shotsNotUsable incorrect");

		// Remove unusable shots
		if (!shot->object.usable)
			{
			ShotRemove(shot);
			i--;
			}
		shot--;
		}
	
	
	// No shots are now unusable.
	GameStatus.objects.shotsNotUsable = 0;
}	


/***********************************************************************
 *
 * FUNCTION:     ShotSetNotUsable
 *
 * DESCRIPTION:  Set a shot unusable.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/14/96	Initial Revision
 *
 ***********************************************************************/

static void ShotSetNotUsable (ShotType *shot)
{
	ErrFatalDisplayIf(!IsShot(&shot->object), "Bad object passed!");

	shot->object.usable = false;
	shot->object.changed = false;
	
	GameStatus.objects.shotsNotUsable++;
}


/***********************************************************************
 *
 * FUNCTION:     ShipInitAlienHomeWorld
 *
 * DESCRIPTION:  Initialize details specific to an alien HomeWorld.
 *
 * PARAMETERS:   shipP - pointer to the ship to initialize
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/11/96	Initial Revision
 *
 ***********************************************************************/

static void ShipInitAlienHomeWorld (ShipType *shipP)
{
	shipP->object.type = shipAlienHomeWorld;
	
	// The alien HomeWorld gets placed at the center of the screen.
	shipP->object.location.left = ScreenToGame((screenWidth - 
		GameToScreen(shipAlienHomeWorldWidth)) / 2 + borderAroundScreen);
	shipP->object.location.top = ScreenToGame((screenWidth - 
		GameToScreen(shipAlienHomeWorldHeight)) / 2 + borderAroundScreen);
	shipP->object.location.right = shipP->object.location.left + shipAlienHomeWorldWidth - 1;
	shipP->object.location.bottom = shipP->object.location.top + shipAlienHomeWorldHeight - 1;

	// Alien HomeWorlds are stationary
	shipP->heading = degrees90;
	shipP->status.type.alien.speed = shipAlienHomeWorldSpeed;
	shipP->object.motion.x = 0;
	shipP->object.motion.y = 0;
	
	// Various home world settings.
	shipP->shotsAvailable = shipAlienHomeWorldShots;
	shipP->armorAvailable = shipAlienHomeWorldArmor;
	shipP->status.type.alien.gunner = ShootSometimesPlasmas;
	shipP->status.type.alien.navigator = TurnNever;
	
}


/***********************************************************************
 *
 * FUNCTION:     ShipInitAlienBase
 *
 * DESCRIPTION:  Initialize details specific to an alien HomeWorld.
 *
 * PARAMETERS:   shipP - pointer to the ship to initialize
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/11/96	Initial Revision
 *
 ***********************************************************************/

static void ShipInitAlienBase (ShipType *shipP)
{
	Int16 baseCount = 0;
	ShipType	*shipIndexP;
	ShipType	*firstShipP;
	
	
	// We position the base by the number of bases that exist.  Because
	// this information is easily available we count the bases now.
	// This is reasonable because this routine is called six times during
	// a full game so it isn't worth keeping the information around.
	shipIndexP = LastShip;
	firstShipP = GameStatus.objects.ship;
	while (shipIndexP >= firstShipP)
		{
		if (shipIndexP->object.type == shipAlienBase)
			baseCount++;
		
		shipIndexP--;
		}
	
	
	shipP->object.type = shipAlienBase;
	
	// The alien base gets placed at different positions around the screen.
	shipP->object.location.left = BasePositionToStart[baseCount].x;
	shipP->object.location.top = BasePositionToStart[baseCount].y;
	shipP->object.location.right = shipP->object.location.left + shipAlienBaseWidth - 1;
	shipP->object.location.bottom = shipP->object.location.top + shipAlienBaseHeight - 1;

	// Alien bases are stationary
	shipP->heading = degrees90;
	if (IsAlienHomeWorldLevel(GameStatus.level))
		shipP->status.type.alien.speed = 0;
	else
		shipP->status.type.alien.speed = shipAlienBaseSpeed;
	shipP->object.motion.x = 0;
	shipP->object.motion.y = 0;
	
	// Various base settings.
	shipP->shotsAvailable = shipAlienBaseShots;
	shipP->armorAvailable = shipAlienBaseArmor;
	shipP->status.type.alien.gunner = ShootSometimesPlasmas;
	shipP->status.type.alien.navigator = TurnRandomly;
	
}


/***********************************************************************
 *
 * FUNCTION:     ShipInitAlienShip
 *
 * DESCRIPTION:  Initialize details specific to an alien ship.
 *
 * PARAMETERS:   shipP - pointer to the ship to initialize
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/11/96	Initial Revision
 *
 ***********************************************************************/

static void ShipInitAlienShip (ShipType *shipP)
{
	UInt16 shipWidth;
	UInt16 shipHeight;
	int n;
	
	
	if (GameStatus.level >= firstLevelSmallAliens &&
		RandN(2))
		{
		shipP->object.type = shipAlienSmall;
		shipWidth = shipAlienLargeWidth - 1;
		shipHeight = shipAlienLargeHeight - 1;
		shipP->status.type.alien.speed = shipAlienSmallSpeed;
		}
	else
		{
		shipP->object.type = shipAlienLarge;
		shipWidth = shipAlienLargeWidth - 1;
		shipHeight = shipAlienLargeHeight - 1;
		shipP->status.type.alien.speed = shipAlienLargeSpeed;
		}
	
	// Start the alien at either the top or left edge of the game so that it
	// starts offscreen and enters from one of the four sides depending on
	// it heading.
	if (RandN(2))
		{
		shipP->object.location.left = RandN(gameWidth);
		shipP->object.location.top = 0;
		}
	else
		{
		shipP->object.location.left = 0;
		shipP->object.location.top = RandN(gameHeight);
		}
	shipP->object.location.right = shipP->object.location.left + shipWidth;
	shipP->object.location.bottom = shipP->object.location.top + shipHeight;

	// Now move the ship in a direction
	// Pick a direction that isn't an a right angle to the screen so 
	// that the alien moves onscreen.
	do
		{
		shipP->heading = RandN(degreesMax);
		}
	while (shipP->heading % degrees90 == 0);
	
	shipP->object.motion.x = (MovementX[shipP->heading] * shipP->status.type.alien.speed);
	shipP->object.motion.y = (MovementY[shipP->heading] * shipP->status.type.alien.speed);
	
	
	// Aliens get to have at least one shot.
	shipP->shotsAvailable = 1;
	while (RandN(1000) < (250 + 10 * GameStatus.level))
		{
		shipP->shotsAvailable++;
		}
	
	// Aliens may have armor.
	shipP->armorAvailable = 0;
	if (GameStatus.level >= firstLevelArmoredAliens)
		{
		while (RandN(1000) < (30 * GameStatus.level / (shipP->armorAvailable + 1)) &&
			shipP->armorAvailable < maxAlienShipArmor)
			{
			shipP->armorAvailable++;
			}
		}
	
	
	// Sometimes we should make a small alien into an ace.
	if (shipP->object.type == shipAlienSmall &&
		GameStatus.level >= firstLevelAceAliens &&
		RandN(1000) < chanceForAceAlien)
		{
		shipP->object.type = shipAlienAce;
		shipP->armorAvailable++;			// at least one armor
		shipP->shotsAvailable += 2;		// at least three shots
		shipP->status.type.alien.gunner = ShootAtNearestShip;
		shipP->status.type.alien.navigator = TurnRightAngles;
		
		// takes effect at the next heading change
		shipP->status.type.alien.speed = shipAlienAceSpeed;
		
		return;
		}
	

	// Now pick a gunner based on what's allowed at the current level
	if (GameStatus.level >= firstLevelShootSometimesAccurately)
		shipP->status.type.alien.gunner = ShootSometimesAccurately;
	else 
		shipP->status.type.alien.gunner = ShootRandomly;
	
		
	// Now pick a navigator based on what's allowed at the current level
	if (GameStatus.level >= firstLevelTurnRandomly)
		n = TurnRandomly;
	else if (GameStatus.level >= firstLevelTurnRightAngles)
		n = TurnRightAngles;
	else if (GameStatus.level >= firstLevelTurnSlow)
		n = TurnSlow;
	else 
		n = TurnNever;
	shipP->status.type.alien.navigator = (NavigatorType) RandN(n + 1);
		
	
}


/***********************************************************************
 *
 * FUNCTION:     AlienAdd
 *
 * DESCRIPTION:  Add a alien to play
 *
 * PARAMETERS:   type - type of alien
 *					  side - the ship side over which the alien is dropped
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/12/96	Initial Revision
 *
 ***********************************************************************/

static void ShipAddAlien (UInt8 type)
{
	ShipType	*shipP;
	
	
	// Check to make sure there is storage space for another ship.
	if (GameStatus.objects.shipCount >= shipsMax)
		return;


	shipP = &GameStatus.objects.ship[GameStatus.objects.shipCount++];
	
	
	// Set data common to all alien ships.
	shipP->object.changed = true;
	shipP->object.usable = true;
	
	shipP->status.shieldOn = false;
	shipP->status.enteringWarp = false;
	shipP->status.inWarp = false;
	shipP->status.exitingWarp = false;
	shipP->status.longShots = false;
	shipP->status.retroRockets = false;
	shipP->status.longShots = false;
	shipP->status.type.alien.fleeing	= false;
	
	shipP->periodsToWait = 0;

	shipP->owner = alienPlayer;
	shipP->uniqueShipID = ShipGetNewUniqueID();
	
	
	// Set data specific according to the type of ship
	switch (type)
		{
		case shipAlienHomeWorld:
			ShipInitAlienHomeWorld(shipP);
			break;
		
		case shipAlienBase:
			ShipInitAlienBase(shipP);
			break;
		
		case shipAlienLarge:
			ShipInitAlienShip(shipP);
			break;
		
		}


	// Update the alien counters
	GameStatus.objects.alienCount++;
	if (GameStatus.aliensToSend > 0)
		GameStatus.aliensToSend--;
	if (IsBase(&shipP->object))
		GameStatus.objects.baseCount++;
}


/***********************************************************************
 *
 * FUNCTION:     AlienChangeDirection
 *
 * DESCRIPTION:  Change the direction of an alien ship.
 *
 * PARAMETERS:   shipP - the ship to change directions
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/12/96	Initial Revision
 *
 ***********************************************************************/
static void AlienChangeDirection (ShipType *shipP)
{
	ErrFatalDisplayIf(!IsAlienShip(&shipP->object), "Bad object passed!");
	

	switch (shipP->status.type.alien.navigator)
		{
		case TurnNever:
			break;
		
		case TurnSlow:
			if (RandN(2))
				shipP->heading += degrees22;
			else
				shipP->heading -= degrees22;
			break;
		
		case TurnRightAngles:
			if (RandN(2))
				shipP->heading += degrees90;
			else
				shipP->heading -= degrees90;
			break;
		
		case TurnRandomly:
			shipP->heading = RandN(degreesMax);
			break;
		}
	
	// Normalize the degrees to be between degrees0 and degrees337 inclusive
	if (shipP->heading < degrees0)
		shipP->heading += degreesMax;
	else if (shipP->heading >= degrees360)
		shipP->heading -= degreesMax;

	shipP->object.motion.x = (MovementX[shipP->heading] * shipP->status.type.alien.speed);
	shipP->object.motion.y = (MovementY[shipP->heading] * shipP->status.type.alien.speed);
}


/***********************************************************************
 *
 * FUNCTION:     AlienShoot
 *
 * DESCRIPTION:  Fire a shot
 *
 * PARAMETERS:   shipP - the ship to shoot
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/12/96	Initial Revision
 *
 ***********************************************************************/
static void AlienShoot (ShipType	*shipP, ObjectType *object)
{
	UInt8 heading;
	Int16 motionX;
	Int16 motionY;
	ShipType	*nearestShip;
	UInt8 type;

	
	ErrFatalDisplayIf(!IsAlienShip(&shipP->object), "Bad object passed!");


	// Aliens not entirely visible are not allowed to shoot!
	if (!ObjectCompletelyVisible(&shipP->object.location))
		return;
	
	
	// Aliens are not allowed to shoot if all but one shot is used.  This
	// allows a human player to shoot.  Otherwise during high levels the 
	// aliens use all the shots.
	if (GameStatus.objects.shotCount >= shotsMax - 1)
		return;
	
	
	type = shotNormal;
	switch (shipP->status.type.alien.gunner)
		{
		case ShootRandomly:
shootAnywhere:
			heading = RandN(degreesMax);
			motionX = MovementX[heading] * shotSpeed;
			motionY = MovementY[heading] * shotSpeed;
			break;
		
		case ShootSometimesPlasmas:
			if (RandN(100) > chanceForPlasmas)
				goto shootAnywhere;
			
			if (object == NULL)
				{
				nearestShip = ShipGetNearestPlayer((GamePoint *)&shipP->object.location);
				if (!nearestShip)
					goto shootAnywhere;
				
				object = &nearestShip->object;
				}

			type = shotPlasma;
			ShotHeadTowardsObject ((ShotType *) shipP, object, &motionX, &motionY);
			break;
			
		case ShootSometimesAccurately:
			// Aliens shooting accurately at players makes game play too hard.
			// Players can really dodge shots with the small screen and difficult
			// motion control.  Still, it's cool to watch an alien take out an
			// approaching rock so allow accurate shots against rocks.
			if (object == NULL || !IsRock(object))
				goto shootAnywhere;

			ShotHeadTowardsObject ((ShotType *) shipP, object, &motionX, &motionY);
			break;

		case ShootAtNearestShip:
			nearestShip = ShipGetNearestPlayer((GamePoint *)&shipP->object.location);
			if (!nearestShip)
				goto shootAnywhere;

			// Allow shipAlienAce to shoot plasmas.  The right way to do this is
			// to have a plasma attribute for ships.  I've not done so because
			// The status bitfield is at 16 bits.
			if (shipP->object.type == shipAlienAce &&
				RandN(100) <= chanceForPlasmas)
				{
				type = shotPlasma;
				}
			
			ShotHeadTowardsObject ((ShotType *) shipP, &nearestShip->object, &motionX, &motionY);
			break;
		
		}
	
	ShotAdd(type, shipP, motionX, motionY);
}


/***********************************************************************
 *
 * FUNCTION:     AlienAvoidObject
 *
 * DESCRIPTION:  Do something to avoid an object.
 *
 * PARAMETERS:   shipP - the ship to evaluate
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/14/96	Initial Revision
 *
 ***********************************************************************/
static void AlienAvoidObject (ShipType *shipP, ObjectType *objectP)
{
	
	
	ErrFatalDisplayIf(!IsAlienShip(&shipP->object), "Bad object passed!");


	if (shipP->warpsAvailable)
		{
		shipP->status.enteringWarp = true;
		shipP->periodsToWait = 0;
		shipP->warpsAvailable--;
		
		return;
		}
	
	if (shipP->shotsAvailable)
		{
		if (!IsShip(objectP) ||
			RandN(4) <= 1)
			{
			AlienShoot (shipP, shipP->object.nextObjectInSector);
		
			return;
			}
		}
	
	if (shipP->status.type.alien.speed > 0) // if (RandN(5) == 1)
		{
		// Flee by increasing speed and changing the direction.
		shipP->status.type.alien.speed += fleeSpeedIncrease;
		AlienChangeDirection(shipP);
		shipP->status.type.alien.fleeing	= true;
		return;
		}
}


/***********************************************************************
 *
 * FUNCTION:     AlienEvaluateWorth
 *
 * DESCRIPTION:  Calculate how much destroying the alien is worth
 * based on it's abilities
 *
 * PARAMETERS:   shipP - the ship to evaluate
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/
static Int16 AlienEvaluateWorth (ShipType *shipP)
{
	Int16 score;
	
	
	ErrFatalDisplayIf(!IsAlienShip(&shipP->object), "Bad object passed!");


	score = 75;
	
	score += (Int16) shipP->status.type.alien.navigator * 25;
	score += (Int16) shipP->status.type.alien.gunner * 75;
	score += (Int16) shipP->armorAvailable * 50;
	
	if (shipP->warpsAvailable)
		score += 50;
	

	if (shipP->object.type == shipAlienSmall)
		score *= 2;
	else if (shipP->object.type == shipAlienBase ||
		shipP->object.type == shipAlienHomeWorld)
		{
		score = ObjectScore[shipP->object.type];
		}
	
	
	return score;
}


/***********************************************************************
 *
 * FUNCTION:     AwardPointsForObject
 *
 * DESCRIPTION:  Increase a player's score by the points for destroying
 * an object.
 *
 * PARAMETERS:   score - the amount to add to the score.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/10/96	Initial Revision
 *
 ***********************************************************************/
static void AwardPointsForObject (ObjectType *object, UInt8 playerNum)
{
	Int16 scoreIncrease;
	
	
	if (PlayerIsHuman(playerNum))
		{
		scoreIncrease = ObjectScore[object->type];
		
		// Destroying a ship is different.  First the score increase depends on 
		// the value of the ship (which is based on it's abilities).  Second,
		// the score increase is displayed in the screen area for awhile.
		if (IsShip(object))
			{
			if (IsAlienShip(object))
				{
				scoreIncrease = AlienEvaluateWorth((ShipType *) object);
				}
			ScoreAdd(ObjectCenterX(object), ObjectCenterY(object), scoreIncrease);
			}
		}


		IncreaseScore(scoreIncrease, playerNum);
}


/***********************************************************************
 *
 * FUNCTION:     ObjectDestroy
 *
 * DESCRIPTION:  destroyer destorys destroyee
 *
 * PARAMETERS:   destroyee - the object being destroyed
 *					  destroyer - the object destroying the destroyee
 *
 * RETURNED:     true if the collision was destructive
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/10/96	Initial Revision
 *
 ***********************************************************************/
static Boolean ObjectDestroy (ObjectType *destroyee, ObjectType *destroyer)
{
	switch (destroyee->type)
		{
		case shipPlayer:
		case shipAlienHomeWorld:
		case shipAlienBase:
		case shipAlienLarge:
		case shipAlienSmall:
		case shipAlienAce:
			ObjectRemoveFromSector(destroyee);
			ShipExplode((ShipType *) destroyee, destroyer);
			break;
		
		case rockSmall:
		case rockMedium:
		case rockLarge:
			ObjectRemoveFromSector(destroyee);
			RockExplode((RockType *) destroyee, destroyer);
			break;
		
		case shotPlasma:
			SparkAddBetweenObjects(destroyee, destroyer);
			// fall thru
		case shotNormal:
			ObjectRemoveFromSector(destroyee);
			ShotSetNotUsable((ShotType *) destroyee);
			break;
		
		case bonusExtraShots:
		case bonusLongShots:
		case bonusRetroRockets:
		case bonusScore:
		case bonusArmor:
		case bonusBomb:
			ObjectRemoveFromSector(destroyee);
			BonusSetNotUsable((BonusType *) destroyee);
			break;
		}
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     CollideTwoObjects
 *
 * DESCRIPTION:  Check if object1 collides with object2.
 *
 * PARAMETERS:   object1, object2 - the objects to check for collision
 *
 * RETURNED:     true if the collision was destructive
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/9/96	Initial Revision
 *
 ***********************************************************************/
static Boolean CollideTwoObjects (ObjectType *object1, ObjectType *object2)
{
	Boolean object1Destructible;
	Boolean object2Destructible;
	UInt8 objectOwner1;
	UInt8 objectOwner2;
	ObjectType *temp;
	
	
	// If the objects are of the same type then the object later in the list
	// must be removed first!  This is due to the fact that the list of
	// objects of that type shrinks to fill in the void left by the removed
	// object which invalidates pointers to those objects after the void.
	if (object2 > object1)
		{
		temp =  object1;
		object1 = object2;
		object1 = temp;
		}
		
		
	// Determine if object2 has an owner to give credit for the kill
	if (IsShot(object2))
		objectOwner2 = ((ShotType *) object2)->ownerPlayerNumber;
	else if (IsShip(object2))
		objectOwner2 = ((ShipType *) object2)->owner;
	else
		objectOwner2 = noPlayer;
	
	
	// Determine if object1 has an owner to give credit for the kill
	if (IsShot(object1))
		objectOwner1 = ((ShotType *) object1)->ownerPlayerNumber;
	else if (IsShip(object1))
		objectOwner1 = ((ShipType *) object1)->owner;
	else
		objectOwner1 = noPlayer;
	
	
	// If the objects both belong to some player don't destroy them
	// because they're on the same team!
	if (objectOwner1 == objectOwner2 &&
		objectOwner1 != noPlayer)
		{
		return false;
		}
	
	
	// If two players collide and they're playing cooperatively then don't
	// destroy them.
	if (GameStatus.collisionStatus == collisionCooperative &&
		PlayerIsHuman(objectOwner1) &&
		PlayerIsHuman(objectOwner2))
		{
		return false;
		}
		
	
	// Planets don't destroy ships by contact - only shots.
	if ((IsPlanet(object1) && !IsShot(object2)) ||
		(IsPlanet(object2) && !IsShot(object1)))
		{
		return false;
		}
	
	
	if (CausesDamage(object2))
		{
		object1Destructible = IsDestructible(object1);
		if (!object1Destructible)
			{
			// Special code to reduce a ship's armor if there is any.
			if (IsShip(object1))
				{
				if (((ShipType *) object1)->armorAvailable)
					(((ShipType *) object1)->armorAvailable)--;
				
				// Visually indicate that something hit the ship.
				SparkAddBetweenObjects(object1, object2);
				
				// Play a sound indicating that the object was hit.
				GameRequestSound (shipHit);
				}
			}
		
		// Update the armor gauge because the ship is either going to have
		// less armor or no armor.
		if (IsShip(object1) &&
			ShipIsOwnedByHuman((ShipType *) object1))
			{
			GameStatus.stats[((ShipType *) object1)->owner].armorChanged = true;
			}
		}
	else
		object1Destructible = false;
		

		
	if (CausesDamage(object1))
		{
		object2Destructible = IsDestructible(object2);
		if (!object2Destructible)
			{
			// Reduce the armor if there is any.
			if (IsShip(object2))
				{
				if (((ShipType *) object2)->armorAvailable)
					(((ShipType *) object2)->armorAvailable)--;
				
				// Visually indicate that something hit the ship.
				SparkAddBetweenObjects(object2, object1);
				
				// Play a sound indicating that the object was hit.
				GameRequestSound (shipHit);
				}
			}
		
		// Update the armor gauge because the ship is either going to have
		// less armor or no armor.
		if (IsShip(object2) &&
			ShipIsOwnedByHuman((ShipType *) object2))
			{
			GameStatus.stats[((ShipType *) object2)->owner].armorChanged = true;
			}
		}
	else
		object2Destructible = false;
			


	// Handle the effects of touching the object
	if (OwnerIsHuman(objectOwner1) &&
		object2Destructible)
		{
		AwardPointsForObject(object2, objectOwner1);
		}
	
	
	// Handle the effects of touching the object
	if (object1Destructible)
		{
		// Grant the bonus to all ships regardless of their owner
		if (IsBonus(object1))
			{
			ShipAwardBonus((ShipType *)object2, (BonusType *)object1);
			}
		else if (OwnerIsHuman(objectOwner2))
			{
			AwardPointsForObject(object1, objectOwner2);
			}
		}
	
	
	// Now remove the objects destroyed
	
	// The second object is never a bonus because the second object comes 
	// from the sector list and bonuses are never entered into the sector list.
	if (object1Destructible)
		ObjectDestroy(object1, object2);

	if (object2Destructible && 
		!IsBonus(object1))
		ObjectDestroy(object2, object1);

	if (!object1Destructible && !object2Destructible)
		{
		// rebound the objects off each other (both have shields on or armor)
		
		return false;
		}
	
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:     CheckForCollisionWithObject
 *
 * DESCRIPTION:  Check if object1 collides with object2.
 *
 * PARAMETERS:   object1, object2 - the objects to check for collision
 *
 * RETURNED:     true if there was a destructive collision
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/9/96	Initial Revision
 *
 ***********************************************************************/
static Boolean CheckForCollisionWithObject (ObjectType *object1, 
	ObjectType *object2, Boolean detectOnly)
{
	Int16 minimumDistanceX;
	Int16 minimumDistanceY;
	Int16 distanceX;
	Int16 distanceY;
	
	
	ErrFatalDisplayIf(object1 == object2, "The object found itself in the sector list.");
	ErrFatalDisplayIf(!object1->usable, "Object not usable.");
	
	// The second object may have already been destroyed this turn.
	if (!object2->usable)
		return false;
	
	
	// minimumDistanceX is the sum of the radius of both objects
	minimumDistanceX = (object1->location.right - object1->location.left + 
		object2->location.right - object2->location.left) / 2;
	
	distanceX = (object1->location.right + object1->location.left - 
		(object2->location.right + object2->location.left)) / 2;

	// distance is always positive
	if (distanceX < 0)
		distanceX = -distanceX;

	// If horizontally close enough to touch check the vertically distance.
	if (distanceX < minimumDistanceX)
		{
		// minimumDistanceY is the sum of the radius of both objects
		minimumDistanceY = (object1->location.bottom - object1->location.top + 
			object2->location.bottom - object2->location.top) / 2;
		
		distanceY = (object1->location.bottom + object1->location.top - 
			(object2->location.bottom + object2->location.top)) / 2;
	
		// distance is always positive
		if (distanceY < 0)
			distanceY = -distanceY;
	

		// If vertically close enough to touch check the vertically distance.
		if (distanceY < minimumDistanceY)
			{
			// Peform more detailed check here
			
			// Until now we have performed only a simple bounding box collision
			// check.  Our graphics for objects typically don't have pixels in
			// the corner.  Ignore collisions that occurr in the extreme corners
			// to satisfy some player's desire to just barely squeak by!
			if ((distanceX + distanceY) < (minimumDistanceX + minimumDistanceY - ScreenToGame(4)))
				{
				// Consider the objects as colliding
				if (detectOnly)
					return true;
				else
					return CollideTwoObjects(object1, object2);
				}
			}
		}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:     CheckForCollisionWithSectorObjects
 *
 * DESCRIPTION:  Check if an object collides with one in a sector.
 *
 * PARAMETERS:   object - object to place in a sector
 *					  sectorNumber - the sector containing objects
 *
 * RETURNED:     true if there was a destructive collision
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/9/96	Initial Revision
 *
 ***********************************************************************/
static Boolean CheckForCollisionWithSectorObjects (ObjectType *object, 
	UInt8 sectorNumber, Boolean detectOnly)
{
	ObjectType *obj;
	
	
	// Rocks don't collide with each other and therefore don't need collision 
	// detection.
	if (IsRock(object))
		return false;
	
	
	// Bonuses need to only check for a ship (the last object type added 
	// to the sector list).
	if (IsBonus(object))
		{
		obj = Sector[sectorNumber];
		
		// Look through all objects in the sector as long as they are ships.
		// Remember that the ships were added last.
		while (obj != NULL &&
			IsShip(obj))
			{
			if (CheckForCollisionWithObject(object, obj, detectOnly))
				return true;
			
			obj = obj->nextObjectInSector;
			}
		
		// The bonus wasn't touched by a ship.
		return false;
		}


	// Look through all the objects in the sector and check for a collision
	// with each object found.
	for (obj = Sector[sectorNumber]; obj != NULL; obj = obj->nextObjectInSector)
		{
		if (CheckForCollisionWithObject(object, obj, detectOnly))
			return true;
		}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:     SectorAddObject
 *
 * DESCRIPTION:  Add an object to the sector it mostly resides in.
 *
 * PARAMETERS:   object - object to place in a sector
 *
 * RETURNED:     true if added or if no object detected.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/8/96	Initial Revision
 *
 ***********************************************************************/
static Boolean SectorAddObject (ObjectType *object, Boolean detectOnly)
{
	UInt8 sectorTopLeft;
	UInt8 sectorTopRight;
	UInt8 sectorBottomLeft;
	UInt8 sectorBottomRight;
	UInt8 sectorMostlyIn;
	Boolean destructiveCollision = false;
	GameLocation largestBounds;
	
	
	ErrFatalDisplayIf(!object->usable, "Unusable object");
	
	// Before adding an object to the sector list we check for collisions.
	// Although an object may occupy space in four sectors it is only stored
	// in the sector that it is mostly in.  Before being added, collisions are
	// checked for in each of the four sectors.  Objects which fit entirely
	// in a sector without a collision must check the case where an object
	// from another sector occupies part of the sector which the added object 
	// occupies.  To cover this case we treat the object to be added as if it
	// is the largest object possible.  This causes the corners to lie in any
	// sectors which might have objects which might touch the added object.
	// We must also handle the case where a colliding object is have in this 
	// sector and half in another, but mostly in the other.  To handle this we
	// extend the bounds out by half of the largest object again.

	largestBounds.left = (object->location.left + object->location.right - 
		biggestObject - biggestObject) / 2;
	largestBounds.top = (object->location.top + object->location.bottom - 
		biggestObject - biggestObject) / 2;
	largestBounds.right = largestBounds.left + biggestObject;
	largestBounds.bottom = largestBounds.top + biggestObject;
	LocationWrap(&largestBounds);
	
	
	sectorTopLeft = GetSector(largestBounds.left, largestBounds.top);
	sectorBottomRight = GetSector(largestBounds.right, largestBounds.bottom);
	
	if (sectorTopLeft == sectorBottomRight)
		{
		sectorMostlyIn = sectorTopLeft;
		destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorMostlyIn, detectOnly);
		}
	else
		{
		sectorBottomLeft = GetSector(largestBounds.left, largestBounds.bottom);
		if (sectorTopLeft == sectorBottomLeft)
			{
			// Find which sector the horizontal midpoint lies in
			sectorMostlyIn = GetSector(
				(largestBounds.left + largestBounds.right) / 2, 
				largestBounds.bottom);

			// Check each sector for a collision
			destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorMostlyIn, detectOnly);
			if (!destructiveCollision)
				destructiveCollision = CheckForCollisionWithSectorObjects(object, 
				(sectorMostlyIn != sectorTopLeft) ? sectorTopLeft : sectorBottomRight, detectOnly);
			}
		else if (sectorBottomLeft == sectorBottomRight)
			{
			// Find which sector the vertical midpoint lies in
			sectorMostlyIn = GetSector(
				largestBounds.left, 
				(largestBounds.top + largestBounds.bottom) / 2);

			// Check each sector for a collision
			destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorMostlyIn, detectOnly);
			if (!destructiveCollision)
				destructiveCollision = CheckForCollisionWithSectorObjects(object, 
				(sectorMostlyIn != sectorTopLeft) ? sectorTopLeft : sectorBottomRight, detectOnly);
			}
		// All four corners of the object are in a different sector
		else
			{
			sectorTopRight = GetSector(largestBounds.right, largestBounds.top);

			// Find which sector the center lies in
			sectorMostlyIn = GetSector(
				(largestBounds.left + largestBounds.right) / 2, 
				(largestBounds.top + largestBounds.bottom) / 2);


			// Check each sector for a collision
			destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorMostlyIn, detectOnly);
			if (!destructiveCollision && sectorMostlyIn != sectorTopLeft)
				destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorTopLeft, detectOnly);
			if (!destructiveCollision && sectorMostlyIn != sectorTopRight)
				destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorTopRight, detectOnly);
			if (!destructiveCollision && sectorMostlyIn != sectorBottomLeft)
				destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorBottomLeft, detectOnly);
			if (!destructiveCollision && sectorMostlyIn != sectorBottomRight)
				destructiveCollision = CheckForCollisionWithSectorObjects(object, sectorBottomRight, detectOnly);
			}
		}
	
	

	// Add the object to the sector if it wasn't destroyed during a collision
	// It seems faster to just let bonuses be entered into the sector list than
	// to filter them out.
	if (!destructiveCollision)
		{
		if (!detectOnly)
			{
			object->nextObjectInSector = Sector[sectorMostlyIn];
			Sector[sectorMostlyIn] = object;
			}
		return true;
		}

	return false;		// object not added

}


/***********************************************************************
 *
 * FUNCTION:     RockExplodeAll
 *
 * DESCRIPTION:  Explode all the rocks.
 *
 * PARAMETERS:   bombOwner - who released the bomb (and gets points)
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/27/96	Initial Revision
 *
 ***********************************************************************/
static void RockExplodeAll (UInt8 bombOwner)
{
	RockType *rock;
	RockType *firstRock;
	Boolean awardPoints;


	awardPoints = OwnerIsHuman(bombOwner);
	
	
	// Explode the rocks
	firstRock = GameStatus.objects.rock;
	rock = LastRock;
	while (rock >= firstRock)
		{
		if (rock->object.usable)
			{
			if (awardPoints)
				AwardPointsForObject(&rock->object, bombOwner);
			
			RockExplode(rock, &rock->object);
			}
		
		rock--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     ShipExplodeAll
 *
 * DESCRIPTION:  Explode all the ships.  The bomb releaser should not
 * be injured.  Ships with armor lose one chunk of armor.
 *
 * PARAMETERS:   bombOwner - who released the bomb (and gets points)
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/27/96	Initial Revision
 *
 ***********************************************************************/
static void ShipExplodeAll (UInt8 bombOwner)
{
	ShipType	*shipP;
	ShipType	*firstShipP;
	Boolean awardPoints;


	awardPoints = OwnerIsHuman(bombOwner);
	
	
	// Explode the ships.
	shipP = LastShip;
	firstShipP = GameStatus.objects.ship;
	while (shipP >= firstShipP)
		{
		if (shipP->owner != bombOwner &&
			IsDestructible(&shipP->object) &&
			shipP->object.usable)
			{
			if (shipP->armorAvailable)
				{
				shipP->armorAvailable--;
				
				// Update the armor gauge.
				if (ShipIsOwnedByHuman(shipP))
					{
					GameStatus.stats[shipP->owner].armorChanged = true;
					}
				
				SparkAddBetweenObjects(&shipP->object, &shipP->object);
				}
			else
				{
				AwardPointsForObject(&shipP->object, bombOwner);
				ShipExplode(shipP, &shipP->object);
				}
			
			
			// Update the armor gauge because the ship either has
			// less armor or no armor.
			if (ShipIsOwnedByHuman(shipP))
				{
				GameStatus.stats[shipP->owner].armorChanged = true;
				}
			}
		
		// Next ship
		shipP--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     ShotExplodeAll
 *
 * DESCRIPTION:  Explode all the shots.
 *
 * PARAMETERS:   bombOwner - who released the bomb (and gets points)
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/27/96	Initial Revision
 *
 ***********************************************************************/
static void ShotExplodeAll (UInt8 bombOwner)
{
	ShotType *shot;
	ShotType *firstShot;
	Boolean awardPoints;


	awardPoints = OwnerIsHuman(bombOwner);
	
	
	// Explode the shots
	firstShot = GameStatus.objects.shot;
	shot = LastShot;
	while (shot >= firstShot)
		{
		if (shot->object.usable)
			{
			if (awardPoints)
				AwardPointsForObject(&shot->object, bombOwner);
			
			ShotSetNotUsable(shot);
			}
		
		shot--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     BombExplode
 *
 * DESCRIPTION:  Explode the objects onscreen.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/27/96	Initial Revision
 *
 ***********************************************************************/

static void BombExplode ()
{
	RockExplodeAll (GameStatus.bombOwner);
	
	ShipExplodeAll (GameStatus.bombOwner);
	
	ShotExplodeAll (GameStatus.bombOwner);
}



/***********************************************************************
 *
 * FUNCTION:     GameDrawArmorGauge
 *
 * DESCRIPTION:  Draw the armor gauge.  The least armor remaining on an 
 * owned ship is drawn.  If no armor remains for a ship then no armor is 
 * displayed.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/23/96	Initial Revision
 *
 ***********************************************************************/

static void GameDrawArmorGauge (void)
{
	UInt8 leastArmor;
	UInt8 shipsToFind;
	UInt8 playerNum;
	ShipType	*ship;
	ShipType	*lastShip;
	Char digit;						// a digit to indicate how many shields exist
	FontID currFont;
	
	
	shipsToFind = GameStatus.stats[GameStatus.playerUsingScreen].shipsOwned;
	playerNum = GameStatus.playerUsingScreen;
	
	
	if (shipsToFind > 0)
		{
		leastArmor = 0xff;		// greatest amount
		
		// Find all ships that the player owns and find out the least amount
		// of armor available.
		ship = GameStatus.objects.ship;
		lastShip = LastShip;
		while (ship <= lastShip)
			{
			if (ship->owner == playerNum)
				{
				if (ship->armorAvailable < leastArmor)
					leastArmor = ship->armorAvailable;
				
				// Have all the player's ships been found?  If so stop looking.
				shipsToFind--;
				if (shipsToFind == 0)
					break;
				}
			
			ship++;
			}
		}
	else
		leastArmor = 0;


	if (leastArmor > 0)
		{
		// Draw armor where the last live is normally displayed.  The idea is
		// that if the user has enough lives for the armor gauge to overlap then
		// there not too concerned about not seeing a ship.  Also, if the player
		// has at least a ship in play with some armor then there not too interested
		// in how many ships they have remaining. 
		WinDrawBitmap (ObjectBitmapPtr[gaugeArmorBitmap], armorGaugeX, armorGaugeY);
		
		// The armor is only big enough to show a single digit.  For double
		// digits simply display a '+'.
		if (leastArmor < 10)
			{
			digit = '0' + leastArmor;
			
			currFont = FntSetFont(armorGaugeFont);
			WinDrawInvertedChars(&digit, 1, armorGaugeX + 3, armorGaugeY);
			FntSetFont(currFont);
			}
		else
			{
			WinDrawBitmap (ObjectBitmapPtr[gaugeArmorFullBitmap], armorGaugeX, armorGaugeY);
			}
		
		}
	else
		{
		// Erase where the armor was displayed since there is none now.
		WinDrawBitmap (ObjectBitmapPtr[gaugeArmorNoneBitmap], armorGaugeX, armorGaugeY);
		
		// Call GameDrawLivesGauge which may make use of the area.
		GameDrawLivesGauge (GameStatus.stats[playerNum].livesRemaining);
		}
}


/***********************************************************************
 *
 * FUNCTION:     GameDrawLivesGauge
 *
 * DESCRIPTION:  Draw the lives gauge.  Lives remaining are drawn.  Lives
 * no longer remaining are erased.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/10/96	Initial Revision
 *
 ***********************************************************************/

static void GameDrawLivesGauge (Int16 livesRemaining)
{
	Int16 i;
	RectangleType bounds;
	Int16 x;
	
	
	// Initial bounds incase it's used.  This removes the code from the loop.
	bounds.topLeft.y = liveGaugeY;
	bounds.extent.x = lifeWidth;
	bounds.extent.y = lifeHeight;
	

	// Draw some of the lives remaining	
	for (i = 0; i < livesDisplayable; i++)
		{
		x = liveGaugeX + i * (lifeWidth + liveGaugeSeparator);
		
		if (livesRemaining > i)
			{
			WinDrawBitmap (ObjectBitmapPtr[gaugeLifeBitmap], x, liveGaugeY);
			}
		else
			{
			bounds.topLeft.x = x;
			WinEraseRectangle(&bounds, 0);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:     GameDrawScoreGauge
 *
 * DESCRIPTION:  Draw the score gauge given a score to display.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/11/96	Initial Revision
 *
 ***********************************************************************/

static void GameDrawScoreGauge (Int32 score)
{
	char scoreText[scoreGaugeDigitsMax + 1];
	FontID currFont;
	Int16 scoreLength;


	currFont = FntSetFont(scoreGaugeFont);

	// Draw the score
	if (score > 0)
		StrIToA(scoreText, score);
	else
		{
		// First draw spaces over a prior score to blank it out.
		Char numSpace;
		ChrNumericSpace(&numSpace);
		MemSet(scoreText, scoreGaugeDigitsMax, numSpace);			// Write numeric spaces to remove old score
		WinDrawChars (scoreText, scoreGaugeDigitsMax, scoreGaugeX, scoreGaugeY);
		
		// Now set up to draw a score of zero.
		scoreText[0] = '0';
		scoreText[1] = '\0';
		}
	scoreLength = StrLen(scoreText);
	WinDrawChars (scoreText, scoreLength, 
		scoreGaugeX,
		scoreGaugeY);

	FntSetFont(currFont);
}


/***********************************************************************
 *
 * FUNCTION:     GameSetLevelMessage
 *
 * DESCRIPTION:  Set the level message.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/21/96	Initial Revision
 *
 ***********************************************************************/

static void GameSetLevelMessage (Int16 level)
{
	FontID currFont;
	
	
	// Set the level message
	if (level == alienHomeWorldLevel)
		{
		SysCopyStringResource(LevelMessageText, homeWorldMessageStr);
		}
	else if (IsAlienWaveLevel(level))
		{
		SysCopyStringResource(LevelMessageText, alienWaveMessageStr);
		}
	else
		{
		SysCopyStringResource(LevelMessageText, levelMessageStr);
		StrIToA(&LevelMessageText[StrLen(LevelMessageText)], level + 1);
		}

	
	// Based on the text set the remaining variables for positioning.
	LevelMessageLength = StrLen(LevelMessageText);
	currFont = FntSetFont(LevelMessageFont);
	LevelMessageWidth = FntCharsWidth(LevelMessageText, LevelMessageLength) - 1;
	LevelMessageHeight = FntLineHeight() - 1;
	LevelMessageBounds.left = (screenWidth - LevelMessageWidth) / 2;
	LevelMessageBounds.top = screenHeight / 4;
	LevelMessageBounds.right = LevelMessageBounds.left + LevelMessageWidth;
	LevelMessageBounds.bottom = LevelMessageBounds.top + LevelMessageHeight;
	LevelMessageDuration = levelMessageDuration;
	FntSetFont(currFont);
}


/***********************************************************************
 *
 * FUNCTION:     GameStateDraw
 *
 * DESCRIPTION:  Redraw the world.  Everything in the last world is 
 * is erased and redrawn
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/2/96	Initial Revision
 *
 ***********************************************************************/

static void GameStateDraw (void)
{
	RectangleType sourceBounds;
	Int16 offset;
	

	// Copy the screenBuffer to the screen.  Remember that the new bounds of
	// part of the screen used may not cover what was used before.  Simply
	// using them will not cause prior stuff to be erased.  Therefore we must
	// always copy the larger of the two bounds.
	sourceBounds.topLeft.x = min(GameStatus.screenBoundsOld.left, GameStatus.screenBoundsNew.left);
	sourceBounds.topLeft.x = max(sourceBounds.topLeft.x, 0);		// don't show stuff off screen
	sourceBounds.topLeft.y = min(GameStatus.screenBoundsOld.top, GameStatus.screenBoundsNew.top);
	sourceBounds.topLeft.y = max(sourceBounds.topLeft.y, 0);

	sourceBounds.extent.x = max(GameStatus.screenBoundsOld.right, GameStatus.screenBoundsNew.right) - 
		sourceBounds.topLeft.x + 1;
	sourceBounds.extent.y = max(GameStatus.screenBoundsOld.bottom, GameStatus.screenBoundsNew.bottom) - 
		sourceBounds.topLeft.y + 1;
	
	
	// Align the x coordinate with the nearest word to speed up the WinCopyRectangle.
	offset = sourceBounds.topLeft.x & 0x000F;
	if (offset)
		{
		sourceBounds.topLeft.x -= offset;
		sourceBounds.extent.x += offset;
		}
	
	// Push the extent out to fill the entire word.
	sourceBounds.extent.x = (sourceBounds.extent.x + 0x0F) & 0xFFF0;
		

	// Copy the screenBuffer if there is something	
	WinCopyRectangle (GameStatus.screenBufferH, 0, &sourceBounds, 
		screenTopLeftXOffset + sourceBounds.topLeft.x, 
		screenTopLeftYOffset + sourceBounds.topLeft.y, 
		winPaint);
		
	
	if (GameStatus.stats[GameStatus.playerUsingScreen].scoreChanged)
		{
		GameDrawScoreGauge (GameStatus.stats[GameStatus.playerUsingScreen].score);
		GameStatus.stats[GameStatus.playerUsingScreen].scoreChanged = false;
		}


	if (GameStatus.stats[GameStatus.playerUsingScreen].livesChanged ||
		GameStatus.stats[GameStatus.playerUsingScreen].armorChanged)
		{
		GameDrawLivesGauge (GameStatus.stats[GameStatus.playerUsingScreen].livesRemaining);
		GameStatus.stats[GameStatus.playerUsingScreen].livesChanged = false;
		
		GameDrawArmorGauge ();
		GameStatus.stats[GameStatus.playerUsingScreen].armorChanged = false;
		}
}


/***********************************************************************
 *
 * FUNCTION:     GameStateDrawEverything
 *
 * DESCRIPTION:  Redraw the world with the screen buffer and redraw
 * everything else as well.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/7/96	Initial Revision
 *
 ***********************************************************************/

static void GameStateDrawEverything (void)
{
	// While the settings haven't changed the visual display has.
	GameStatus.stats[GameStatus.playerUsingScreen].scoreChanged = true;
	GameStatus.stats[GameStatus.playerUsingScreen].livesChanged = true;
	GameStatus.stats[GameStatus.playerUsingScreen].armorChanged = true;
	
	GameStateDraw();
}


/***********************************************************************
 *
 * FUNCTION:     GameStatePrepareDraw
 *
 * DESCRIPTION:  Draw the game state to a buffer.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/23/96	Initial Revision
 *			jmp	10/25/99	Explicitly pass the address of BombPattern (new
 *								need for 3.5?).
 *
 ***********************************************************************/

static void GameStatePrepareDraw (void)
{
	ShipType *shipP;
	ShipType *firstShip;
	ShotType *shotP;
	ShotType *firstShot;
	RockType *rockP;
	RockType *firstRock;
	SparkType *sparkP;
	SparkType *firstSpark;
	ScoreType *scoreP;
	ScoreType *firstScore;
	BonusType *bonusP;
	BonusType *firstBonus;
	WinHandle currentDrawWindow;
	RectangleType bounds;
	Int16 screenX;
	Int16 screenY;
	FontID currFont;
	
	
	// Update the screenBounds
	MemMove (&GameStatus.screenBoundsOld, &GameStatus.screenBoundsNew, sizeof (AbsRectType));

	GameStatus.screenBoundsNew.left = screenWidth;
	GameStatus.screenBoundsNew.top = screenHeight;
	GameStatus.screenBoundsNew.right = 0;
	GameStatus.screenBoundsNew.bottom = 0;
	
	
	// Set the draw window to the screenBuffer so that all the draw operations
	// act on the screenBuffer which we draw later.
	currentDrawWindow = WinSetDrawWindow(GameStatus.screenBufferH);
	
	
	if (GameStatus.bombExplode)
		{
		// Draw an affect for the bomb exploding.  Fill the entire
		// screen with a pattern.
		bounds.topLeft.x = 0;
		bounds.topLeft.y = 0;
		bounds.extent.x = screenWidth;
		bounds.extent.y = screenHeight;
		WinSetPattern(&BombPattern);
		WinFillRectangle(&bounds, 0);
		
		// Set the bounds to the entire screen
		MemMove (&GameStatus.screenBoundsNew, &bounds, sizeof (AbsRectType));
	
	
		GameStatus.bombExplode = false;
		}
	else
		{
		// Clear the portion of screenBuffer used last time.
		bounds.topLeft.x = GameStatus.screenBoundsOld.left;
		bounds.topLeft.y = GameStatus.screenBoundsOld.top;
		bounds.extent.x = GameStatus.screenBoundsOld.right - GameStatus.screenBoundsOld.left + 1;
		bounds.extent.y = GameStatus.screenBoundsOld.bottom - GameStatus.screenBoundsOld.top + 1;
		WinEraseRectangle(&bounds, 0);
		}
	
	
	// Draw any level message
	if (LevelMessageLength > 0)
		{
		currFont = FntSetFont(LevelMessageFont);
		WinInvertChars(LevelMessageText, LevelMessageLength, 
			LevelMessageBounds.left, LevelMessageBounds.top);
		ScreenBoundsIncludeArea(LevelMessageBounds.left, LevelMessageBounds.top, 
			LevelMessageBounds.right, LevelMessageBounds.bottom);
		FntSetFont(currFont);
		}
	
	
	// Draw the scores
	scoreP = LastScore;
	firstScore = GameStatus.objects.score;
	while (scoreP >= firstScore)
		{
		// Draw the score
		screenX = GameToScreen(scoreP->location.left) - borderAroundScreen;
		screenY = GameToScreen(scoreP->location.top) - borderAroundScreen;
		WinInvertChars(scoreP->digits, scoreP->digitCount, 
			screenX, screenY);
		ScreenBoundsIncludeArea(screenX, screenY, 
			GameToScreen(scoreP->location.right) - borderAroundScreen,
			GameToScreen(scoreP->location.bottom) - borderAroundScreen);
		
		scoreP--;
		}
	
	
	// Draw the ships
	shipP = LastShip;
	firstShip = GameStatus.objects.ship;
	while (shipP >= firstShip)
		{
		// Draw the ship
		if (shipP->object.usable &&
			!shipP->status.inWarp &&
			!shipP->status.exitingWarp &&
			!(ShipIsOwnedByHuman(shipP) && 
				(shipP->status.type.player.appearSafely || 
				shipP->status.type.player.finishedSkit)))
			{
			DrawObject (GetShipBitmap(shipP->object.type, shipP->heading), 
				shipP->object.location.left, 
				shipP->object.location.top,
				winOverlay);
			
			
			// In multiplayer mode provide an identifier for player ships.
			if (GameStatus.playerCount > 1 &&
				ShipIsOwnedByHuman(shipP))
				{
				char ownerIndicator;
				
				ownerIndicator = '1' + shipP->owner;
				WinInvertChars(&ownerIndicator, 1, 
					GameToScreen(shipP->object.location.left) - borderAroundScreen + 3,
					GameToScreen(shipP->object.location.top) - borderAroundScreen + 1);
				}
			}
		
		shipP--;
		}


	// Draw the shots
	shotP = LastShot;
	firstShot = GameStatus.objects.shot;
	while (shotP >= firstShot)
		{
		// Draw the shot
		if (shotP->object.usable)
			{
			DrawObject (GetShotBitmap(shotP->object.type), 
				shotP->object.location.left, 
				shotP->object.location.top,
				winPaint);
			}
		
		shotP--;
		}
	
	
	// Draw the rocks
	rockP = LastRock;
	firstRock = GameStatus.objects.rock;
	while (rockP >= firstRock)
		{
		// Draw the rock
		if (rockP->object.usable)
			{
			DrawObject (GetRockBitmap(rockP->object.type), 
				rockP->object.location.left, 
				rockP->object.location.top,
				winOverlay);
			}
		
		rockP--;
		}
		
		
	// Draw the bonuses
	bonusP = LastBonus;
	firstBonus = GameStatus.objects.bonus;
	while (bonusP >= firstBonus)
		{
		// Draw the bonus
		if (bonusP->object.usable)
			{
			DrawObject (GetBonusBitmap(bonusP->object.type), 
				bonusP->object.location.left, 
				bonusP->object.location.top,
				winOverlay);
			}
		
		bonusP--;
		}
		
		
	// Draw the sparks
	sparkP = LastSpark;
	firstSpark = GameStatus.objects.spark;
	while (sparkP >= firstSpark)
		{
		// Draw the spark
		DrawPoint(sparkP->location.x, sparkP->location.y);
		sparkP--;
		}
	
	
	// Now restore the draw window	
	WinSetDrawWindow(currentDrawWindow);
}


/***********************************************************************
 *
 * FUNCTION:     GameInitLevel
 *
 * DESCRIPTION:  Set the data to start a new level
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/22/96	Initial Revision
 *
 ***********************************************************************/

static void GameInitLevel (void)
{
	Int16 i;
	ShipType *ship;
	
	
	if (GameStatus.level == heroLevel)
		{
		GameStatus.status = gameWon;

		// Ready all alive ships for the ending skit.
		ship = LastShip;
		while (ship >= &GameStatus.objects.ship[0])
			{
			// Turn on retroRockets to kill the ships' momentum.
			ship->status.retroRockets = true;
			
			ship--;
			}
		
		return;
		}
	
	
	GameSetLevelMessage(GameStatus.level);

	// Wipe the display clean (gets rid of the end of level report).
	GameStateDraw();
	

	GameStatus.status = gameInMotion;
	GameStatus.periodsTillNextLevel = gameEndingTimeInterval;
	

	// Reset the number of objects to destroy to complete the level
	if (IsAlienWaveLevel(GameStatus.level))
		{
		GameStatus.aliensToSend = GameStatus.level + 5;
		GameStatus.rocksToSend = 0;
		
		// Add the alien bases and HomeWorld.  The first wave doesn't
		// have an alien base. The number of alien bases increase with 
		// each alien wave until the alien homeworld which doesn't have
		// a base.  Four bases are too much for the homeworld level.  In
		// general if the player has made it that far I'd like for them to
		// suceed and four bases wasn't helping!
		if (IsAlienHomeWorldLevel(GameStatus.level))
			{
			ShipAddAlien(shipAlienHomeWorld);
			}
		else
			{
			i = GameStatus.level - alienWave1Level;
			while (i > 0)
				{
				ShipAddAlien(shipAlienBase);
				i -= levelsForAnAlienWave;
				}
			}
		
		}
	else
		{
		GameStatus.rocksToSend = GameStatus.level / 5 + 3;
		GameStatus.aliensToSend = 0;
		}
}


/***********************************************************************
 *
 * FUNCTION:     GameEndLevel
 *
 * DESCRIPTION:  Set the data to start a new level
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/22/96	Initial Revision
 *
 ***********************************************************************/

static void GameEndLevel (void)
{
	// Set the game status to advance to the next level
	GameStatus.status = levelOver;
	GameStatus.periodsTillNextLevel = levelOverTimeInterval;

}


/***********************************************************************
 *
 * FUNCTION:     GameStateAdvance
 *
 * DESCRIPTION:  Advance to the next level if time to do so.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/24/96	Initial Revision
 *
 ***********************************************************************/

static void GameStateAdvance (void)
{
	if (GameStatus.status == levelOver &&
		GameStatus.periodsTillNextLevel == 0)
		{
		GameStatus.level++;
		GameInitLevel();
		}
}


/***********************************************************************
 *
 * FUNCTION:     GameStart
 *
 * DESCRIPTION:  Initialize the game to start.  Nothing visual.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/1/96	Initial Revision
 *
 ***********************************************************************/
static void GameStart ()
{
	Int16 i;
	Int16 s;
	PlayerType *playerP;
	
	
	// Set the keys
	GameMaskKeys ();
	
	GameStatus.paused = false;
	GameStatus.pausedTime = 0;
	GameStatus.playerUsingScreen = player0;
	

	if (GameStatus.status != gameResuming)
		{
		GameStatus.periodCounter = 0;
		GameStatus.soundToMake = noSound;
		GameStatus.level = firstLevelPlayed;
		GameStatus.lowestHighScorePassed = false;
		GameStatus.highestHighScorePassed = false;

		GameStatus.bombExplode = false;
		
		// Clear all players
		for (i = 0; i < playersMax; i++)
			{
			playerP = &GameStatus.stats[i];
			
			playerP->score = 0;
			playerP->scoreChanged = true;
			playerP->scoreToAwardBonusShip = scoreForAnotherShip;
			playerP->shipsExpectedToOwn = 1;
			playerP->livesRemaining = numberOfPlayerLives * 
				playerP->shipsExpectedToOwn;
			playerP->livesChanged = true;
			playerP->armorChanged = true;
			playerP->shipsOwned = 0;
			playerP->periodsUntilAnotherShip = 0;
			
			// Clear the ship formations
			for (s = shipsInFormationMax - 1; s >= 0; s--)
				{
				playerP->shipFormation[s] = invalidUniqueShipID;
				}
			playerP->shipFormationCount = 0;
			}
			
		
		// Clear all ships
		for (i = 0; i < shipsMax; i++)
			{
			GameStatus.objects.ship[i].object.usable = false;
			GameStatus.objects.ship[i].object.changed = false;
			}
		GameStatus.objects.shipsNotUsable = 0;
		GameStatus.objects.shipCount = 0;
		GameStatus.objects.alienCount = 0;
		GameStatus.objects.baseCount = 0;
		ShipInitUniqueID ();
			
		
		// Clear all shots
		for (i = 0; i < shotsMax; i++)
			{
			GameStatus.objects.shot[i].object.usable = false;
			GameStatus.objects.shot[i].object.changed = false;
			}
		GameStatus.objects.shotsNotUsable = 0;
		GameStatus.objects.shotCount = 0;
		
		
		// Clear all rocks
		for (i = 0; i < rocksMax; i++)
			{
			GameStatus.objects.rock[i].object.usable = false;
			GameStatus.objects.rock[i].object.changed = false;
			}
		GameStatus.objects.rocksNotUsable = 0;
		GameStatus.objects.rockCount = 0;
		

		// Clear all bonuses
		for (i = 0; i < bonusesMax; i++)
			{
			GameStatus.objects.bonus[i].object.usable = false;
			GameStatus.objects.bonus[i].object.changed = false;
			}
		GameStatus.objects.bonusesNotUsable = 0;
		GameStatus.objects.bonusesCount = 0;
		

		// Clear all sparks
		GameStatus.objects.sparkCount = 0;
		
		
		// Clear all scores
		GameStatus.objects.scoreCount = 0;
		
		
		GameInitLevel ();


		GameStateAdvance();
		
		// Synchronize the periods to the timer.
		GameStatus.nextPeriodTime = TimGetTicks() + advanceTimeInterval;
		}
	else
		{
		GameStatus.status = SavedGameStatus;
		GameStatus.nextPeriodTime = TimGetTicks() + pauseLengthBeforeResumingSavedGame;
		
		// Force these to draw for resumed games.
		GameStatus.stats[GameStatus.playerUsingScreen].scoreChanged = true;
		GameStatus.stats[GameStatus.playerUsingScreen].livesChanged = true;
		GameStatus.stats[GameStatus.playerUsingScreen].armorChanged = true;
		
		GameSetLevelMessage (GameStatus.level);
		}
}




/***********************************************************************
 *
 * FUNCTION:     GameControlShips
 *
 * DESCRIPTION:  Control the ships to perform a skit.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/21/96	Initial Revision
 *
 ***********************************************************************/
static void GameControlShips ()
{
	ShipType *ship;
	Boolean skitDone;
	Int16 playerNum;


	// First, clear the input from all players from the last period.
	for (playerNum = GameStatus.playerCount - 1; playerNum >= 0; playerNum--)
		{
		GameStatus.stats[playerNum].playerInput = 0;
		}
	
	
	// If there are ships consider the skit done if none move.
	skitDone = GameStatus.objects.shipCount > 0;
	
	
	// First rotate the ships to point north. 
	// Second thrust until the ship is off the screen.
	ship = LastShip;
	while (ship >= &GameStatus.objects.ship[0])
		{
		if (!ship->status.type.player.finishedSkit)
			{
			if (!ship->status.type.player.appearSafely)
				{
				if (ship->heading == degrees90)
					{
					GameStatus.stats[ship->owner].playerInput = thrustKey;
					
					// Now release a trail of sparks as the ship
					// accellerates away because it looks cool.
					// Use SparkAddBetweenObjects because the sparks are
					// are affected by the ship's speed.
					SparkAddFromObject(&ship->object);
					}
				else if (ship->heading > degrees90 &&
					ship->heading < degrees270)
					{
					if (ship->periodsToWait == 0)
						{
						GameStatus.stats[ship->owner].playerInput = rotateRightKey;
						ship->periodsToWait = rotateRightKeyRepeatDelay * 2;
						}
					else
						ship->periodsToWait--;
					}
				else
					{
					if (ship->periodsToWait == 0)
						{
						GameStatus.stats[ship->owner].playerInput = rotateLeftKey;
						ship->periodsToWait = rotateLeftKeyRepeatDelay * 2;
						}
					else
						ship->periodsToWait--;
					}
				
				// If the ship has moved off the top of the screen or 
				// the object is moving so fast that at the next period the
				// ship's top will have moved off the top of the game space then
				// it is finished with the skit.
				if (ship->object.location.bottom < ScreenToGame(borderAroundScreen) ||
					(ship->object.location.top + ship->object.motion.y) < 0)
					{
					ship->status.type.player.finishedSkit = true;
					}
				else
					skitDone = false;		// Ship still moving
				}
			else
				skitDone = false;			// Ship still to appear
			}
		
		ship--;
		}
	
	// If the ships have all moved where they belong try to end the game.
	// This may be overriden if sparks remain to finish drawing.
	if (skitDone)
		GameStatus.status = gameOver;
}


/***********************************************************************
 *
 * FUNCTION:     ConsoleGetInput
 *
 * DESCRIPTION:  Get player input to rotate the ship, use the thrusters,
 * or fire a shot.  Account for repeat delays.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     the hard key state to use
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/7/96	Initial Revision
 *
 ***********************************************************************/
static UInt32 ConsoleGetInput()
{
	UInt32 hardKeyState;


	// Countdown the delay timers
	if (Console.rotateLeftDelayInPeriods > 0)
		Console.rotateLeftDelayInPeriods--;
	if (Console.rotateRightDelayInPeriods > 0)
		Console.rotateRightDelayInPeriods--;
	if (Console.thrustDelayInPeriods > 0)
		Console.thrustDelayInPeriods--;
	if (Console.shootDelayInPeriods > 0)
		Console.shootDelayInPeriods--;


	hardKeyState = KeyCurrentState() & keysAllowedMask;

	
	// If both rotate keys are pressed they cancel each other out.
	if ((hardKeyState & (rotateLeftKey | rotateRightKey)) == 
		(rotateLeftKey | rotateRightKey))
		{
		hardKeyState &= ~(rotateLeftKey | rotateRightKey);
		}
	

	// The rotateLeftKey
	if (hardKeyState & rotateLeftKey)
		{
		if (Console.rotateLeftDelayInPeriods == 0)
			{
			Console.rotateLeftDelayInPeriods = rotateLeftKeyRepeatDelay;
			}
		else
			{
			hardKeyState &= ~rotateLeftKey;
			}
		}

	// The rotateRightKey
	else if (hardKeyState & rotateRightKey)
		{
		if (Console.rotateRightDelayInPeriods == 0)
			{
			Console.rotateRightDelayInPeriods = rotateRightKeyRepeatDelay;
			}
		else
			{
			hardKeyState &= ~rotateRightKey;
			}
		}
		

	// The thrustKey
	if (hardKeyState & thrustKey)
		{
		if (Console.thrustDelayInPeriods == 0)
			{
			Console.thrustDelayInPeriods = thrustKeyRepeatDelay;
			}
		else
			{
			hardKeyState &= ~thrustKey;
			}
		}
		
	// The shootKey
	if (hardKeyState & shootKey)
		{
		if (Console.shootDelayInPeriods == 0)
			{
			Console.shootDelayInPeriods = shootKeyRepeatDelay;
			}
		else
			{
			hardKeyState &= ~shootKey;
			}
		}

	// The warpKey
	if (hardKeyState & warpKey)
		{
		}


	return hardKeyState;
}


/***********************************************************************
 *
 * FUNCTION:     ShipHandleInput
 *
 * DESCRIPTION:  Handle user input to rotate the ship, use the thrusters,
 * or fire a shot.
 *
 * PARAMETERS:   shipNumber - ship to be affected
 *					  hardKeyState - the input
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/22/96	Initial Revision
 *
 ***********************************************************************/
static void ShipHandleInput (ShipType *shipP, UInt32 hardKeyState)
{
	shipP->status.type.player.rotateLeft = false;
	shipP->status.type.player.rotateRight = false;
	shipP->status.type.player.thrusterOn = false;

	
	// Rotate the ships in a direction.  Remember that there may be multiple ships.
	// If one ship can't move then none can move.
	if (hardKeyState & rotateLeftKey && !(hardKeyState & rotateRightKey))
		{
		// Rotate the ship to the left
		if (shipP->object.usable)
			{
			shipP->status.type.player.rotateLeft = true;
			}
		}
	else if (hardKeyState & rotateRightKey && !(hardKeyState & rotateLeftKey))
		{
		// Rotate the ship to the right
		if (shipP->object.usable)
			{
			shipP->status.type.player.rotateRight = true;
			}
		}
	else 	// the ship moved neither direction
		{
		shipP->object.changed = false;
		}
		

	// Fire the thrusters if the key is depressed
	if (hardKeyState & thrustKey)
		{
		// Rotate the ship as much as possible to the left
		if (shipP->object.usable)
			{
			shipP->status.type.player.thrusterOn = true;
			}
		}
		
	// Shoot if the key is depressed
	if (hardKeyState & shootKey)
		{
		if (shipP->object.usable)
			{
			ShotAdd(shotNormal, shipP, 
				MovementX[shipP->heading] * shotSpeed,
				MovementY[shipP->heading] * shotSpeed);
			}
		}
		
	// Warp if the key is depressed
	if (hardKeyState & warpKey)
		{
		if (shipP->object.usable &&
			!shipP->status.enteringWarp &&
			!shipP->status.inWarp &&
			!shipP->status.exitingWarp)
			{
			shipP->status.enteringWarp = true;
			shipP->periodsToWait = periodsToEnterWarp;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:     ObjectMove
 *
 * DESCRIPTION:  Move the object according to it's motion.  Handling wrapping
 * around the game space.
 *
 * PARAMETERS:   objectP - pointer to object to move
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/2/96	Initial Revision
 *
 ***********************************************************************/
static void ObjectMove (ObjectType *objectP)
{
	// Move the ship
	objectP->location.left += objectP->motion.x;
	objectP->location.top += objectP->motion.y;
	objectP->location.right += objectP->motion.x;
	objectP->location.bottom += objectP->motion.y;

	LocationWrap(&objectP->location);
}


/***********************************************************************
 *
 * FUNCTION:     ShipDrawWarpEffects
 *
 * DESCRIPTION:  Draw the warping of the ship.
 *
 * PARAMETERS:   shipP - the ship to draw sparks for
 *					  entering - true if the ship should appear to enter warp
 *									 false if the ship should appear to leave warp
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/12/96	Initial Revision
 *
 ***********************************************************************/
static void ShipDrawWarpEffects (ShipType *shipP, Boolean entering)
{
	Int16 sparkCount;
	Int16 sparkX;
	Int16 sparkY;
	Int16 motionX;
	Int16 motionY;
	Int16 centerX;
	Int16 centerY;
	GameLocation visibleGameSpace;
	
	
	// Now adds sparks to show warping into normal space. The sparks 
	// all have the same duration so the warping appears to wink in 
	// all at once.
	sparkCount = 20;
	centerX = ObjectCenterX(&shipP->object);
	centerY = ObjectCenterY(&shipP->object);
	GetVisibleGameSpace(&visibleGameSpace);
	while (sparkCount > 0)
		{
		// The sparks have different durations so that the explosion gradually
		// fades away.
		sparkX = RandN(visibleGameSpace.right - visibleGameSpace.left) + 
			visibleGameSpace.left;
		sparkY = RandN(visibleGameSpace.bottom - visibleGameSpace.top) + 
				visibleGameSpace.top;
		
		if (entering)
			{
			motionX = (sparkX - centerX) / warpEffectDuration;
			motionY = (sparkY - centerY) / warpEffectDuration;
			SparkAdd(centerX, centerY, motionX, motionY, warpEffectDuration);
			}
		else
			{
			motionX = (centerX - sparkX) / warpEffectDuration;
			motionY = (centerY - sparkY) / warpEffectDuration;
			SparkAdd(sparkX, sparkY, motionX, motionY, warpEffectDuration);
			}
		
		sparkCount--;
		}
	
}


/***********************************************************************
 *
 * FUNCTION:     ShipHandleWarpEffects
 *
 * DESCRIPTION:  Handle any warp status bits.
 *
 * PARAMETERS:   shipP - the ship to check and handle
 *
 * RETURNED:     true if the ship is in a state where it should
 * not be updated by moving it or responding to user input.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/28/96	Initial Revision
 *
 ***********************************************************************/
static Boolean ShipHandleWarpEffects (ShipType *shipP)
{
	Int16 speed;
	UInt8 formationPosition;
	ShipType *cloneShipP;
	PlayerType *playerP;
	Int16 objectCount;
	Int16 cloneChance;
	
	
	// A ship in warp is not in normal space so other inputs have no
	// effect and are therefore ignored.  Both aliens and players may warp.
	if (shipP->status.inWarp)
		{
		if (shipP->periodsToWait > 0)
			shipP->periodsToWait--;
		if (shipP->periodsToWait == 0)
			{
			shipP->status.inWarp = false;
			shipP->status.exitingWarp = true;
			shipP->periodsToWait = periodsToExitWarp;
			
			// Pick a new location for the ship to warp to.  Leave some room
			// around the edges of the screen so that if the player gets hit
			// he sees what hit them.  Also, preserve the width of the ship
			ObjectMoveToVisibleGameSpace(&shipP->object, true);
			
			// Set the warp space for this period if not already done
			if (GameStatus.warp.exit.period != GameStatus.periodCounter)
				{
				GameStatus.warp.exit.period = GameStatus.periodCounter;
				
				GameStatus.warp.exit.heading = RandN(degreesMax);
				GameStatus.warp.exit.highSpeed = RandN(1000) < chanceToHaveMaxSpeedOutOfWarp;
				
				// To prevent clone abuses make it rare but with increasing odds the
				// more things are on the screen.  Also, don't allow cloning until
				// aliens appear.  They keep the player occupied and the game moving forward.
				if (GameStatus.level >= firstLevelForAliens)
					{
					objectCount = GameStatus.objects.rockCount + GameStatus.objects.alienCount;
					if (objectCount < 4)
						{
						// When the rock count is small the odds improve better than linearly.
						cloneChance = cloneChances[objectCount];
						}
					else if (objectCount < 18)
						{
						cloneChance = objectCount / -3 + 9;
						}
					else
						{
						// Never allow better than a one in three chance.
						cloneChance = 3;
						}
					
					GameStatus.warp.exit.cloneShips = (RandN(cloneChance) == 1);
					}
				
#ifdef OPTION_DETERMINISTIC_PLAY
				// Test clone ships more than normal
				if (RandN(2))
					GameStatus.warp.exit.cloneShips = true;
#endif
				}
			
			// Ships coming out of warp have a random direction and usually
			// no speed.  Sometimes though they come out at max speed!
			// Pick a random heading for the ship
			playerP = &GameStatus.stats[shipP->owner];
			if (playerP->shipFormationCount == 0)
				{
				shipP->heading = GameStatus.warp.exit.heading;
				if (GameStatus.warp.exit.highSpeed)
					speed = speedMax / 2;
				else
					speed = 0;
				shipP->object.motion.x = MovementX[shipP->heading] * speed;
				shipP->object.motion.y = MovementY[shipP->heading] * speed;
				
				formationPosition = 0;
				}
			else
				{
				ErrFatalDisplayIf(playerP->shipFormationCount + 1 > playerP->shipsOwned, "Bad formation");
				formationPosition = ShipPositionInFormation (shipP);
				}
			
			
			// Add the ship to the formation
			playerP->shipFormation[formationPosition] = shipP->uniqueShipID;
			playerP->shipFormationCount++;
			
			// Now adds sparks to show warping into normal space.
			ShipDrawWarpEffects(shipP, false);
			
			
			// Now that the ship is done returning to normal space clone it
			// if the effect is happening.
			if (GameStatus.warp.exit.cloneShips &&
				playerP->shipsOwned < shipsInFormationMax &&
				GameStatus.objects.shipCount < shipsMax)
				{
				cloneShipP = &GameStatus.objects.ship[GameStatus.objects.shipCount++];
				MemMove(cloneShipP, shipP, sizeof(ShipType));
				
				cloneShipP->uniqueShipID = ShipGetNewUniqueID();
				GameStatus.stats[cloneShipP->owner].shipsOwned++;
				
				// Add the ship to the formation
				formationPosition = ShipPositionInFormation (cloneShipP);
				playerP->shipFormation[formationPosition] = cloneShipP->uniqueShipID;
				playerP->shipFormationCount++;
				
				// Now adds sparks to show warping into normal space.
				ShipDrawWarpEffects(cloneShipP, false);
				}
			
			}
			
		return true;
		}
	
	
	// A ship in warp is not in normal space so other inputs have no
	// effect and are therefore ignored.  Both aliens and players may warp.
	// The only significance of this state is that it wait's for the sparks
	// to finish animating the exiting of warp.
	else if (shipP->status.exitingWarp)
		{
		if (shipP->periodsToWait > 0)
			shipP->periodsToWait--;
		if (shipP->periodsToWait == 0)
			{
			shipP->status.exitingWarp = false;
			}
			
		return true;
		}
	
	
	else
		{
		// A ship entering warp is still in normal space and so other 
		// inputs can be processed.  Both aliens and players may warp.
		// This is before the ship is in warp space.
		if (shipP->status.enteringWarp)
			{
			if (shipP->periodsToWait > 0)
				shipP->periodsToWait--;
			if (shipP->periodsToWait == 0)
				{
				shipP->status.enteringWarp = false;
				shipP->status.inWarp = true;
			
				// Set the warp space for this period if not already done
				if (GameStatus.warp.enter.period != GameStatus.periodCounter)
					{
					GameStatus.warp.enter.period = GameStatus.periodCounter;
				
					GameStatus.warp.enter.periodsToWait = minimumPeriodsSpentInWarp + RandN(randomPeriodsSpentInWarp);
					}

				shipP->periodsToWait = GameStatus.warp.enter.periodsToWait;
				
				// Remove the ship from the formation 
				ShipRemoveFromFormation(shipP);
				
				// Now adds sparks to show warping out of normal space.
				ShipDrawWarpEffects(shipP, true);
				}
			}
		}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:     ShipMoveAll
 *
 * DESCRIPTION:  Move all the aliens.  This routine also contains the logic
 * for aliens to change speed, direction, and levels.  It also has stuff
 * to move the alien to the surface and attack the ship.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/22/96	Initial Revision
 *
 ***********************************************************************/
static void ShipMoveAll ()
{
	ShipType	*shipP;
	ShipType	*firstShipP;
	ShipType	*firstShipToAppearSafelyP = NULL;	// ship to appear safely
	UInt8 owner;
	UInt8 shipsToAppearSafelyCount = 0;


	// Move the ships.
	shipP = LastShip;
	firstShipP = GameStatus.objects.ship;
	while (shipP >= firstShipP)
		{
		if (!ShipHandleWarpEffects(shipP))
			{
			// Player ships can rotate
			if (ShipIsOwnedByHuman(shipP))
				{
				// The ship isn't in play.  Try to use it.
				if (shipP->status.type.player.appearSafely)
					{
					shipsToAppearSafelyCount++;
					
					// Record the first such ship as a speed optimization.
					//  Generally there is only one such ship so this optimization
					//  handles most cases.
					if (firstShipToAppearSafelyP == NULL)
						firstShipToAppearSafelyP = shipP;
					
					shipP--;
					continue;
					}
				
				// Ships done with the skit aren't moved.
				if (shipP->status.type.player.finishedSkit)
					{
					shipP--;
					continue;
					}
				
				// First have the ship handle the player's input
				ShipHandleInput (shipP, GameStatus.stats[shipP->owner].playerInput);
				
				if (shipP->status.type.player.rotateLeft)
					{
					shipP->heading++;
					if (shipP->heading >= degreesMax)
						shipP->heading = degrees0;
					}
				else if (shipP->status.type.player.rotateRight)
					{
					shipP->heading--;
					if (shipP->heading < degrees0)
						shipP->heading = degreesMax - 1;
					}
				
				if (shipP->status.type.player.thrusterOn)
					{
					shipP->object.motion.x += (MovementX[shipP->heading] * 5) / 2;
					shipP->object.motion.y += (MovementY[shipP->heading] * 5) / 2;
					
					// This code attempts to stop the ship if the player is almost
					// stopped.
					if (AbsoluteValue(shipP->object.motion.x) < (MovementX[degrees0] * 5) / 2 &&
						AbsoluteValue(shipP->object.motion.y) < (MovementY[degrees90] * 5) / 2)
						{
						shipP->object.motion.x = 0;
						shipP->object.motion.y = 0;
						}
					
					
					// Limit the speed of ships to something that appears on screen.
					// Critically, the speed must be less than the span of the game world.
					if (shipP->object.motion.x > speedMax)
						shipP->object.motion.x = speedMax;
					else if (shipP->object.motion.x < -speedMax)
						shipP->object.motion.x = -speedMax;
					
					if (shipP->object.motion.y > speedMax)
						shipP->object.motion.y = speedMax;
					else if (shipP->object.motion.y < -speedMax)
						shipP->object.motion.y = -speedMax;
					}
				else if (shipP->status.retroRockets)
					{
					// Retro rockets reduce the ship's speed every turn until it is
					// stationary.
					if (shipP->object.motion.x > 0)
						shipP->object.motion.x--;
					else if (shipP->object.motion.x < 0)
						shipP->object.motion.x++;
					
					if (shipP->object.motion.y > 0)
						shipP->object.motion.y--;
					else if (shipP->object.motion.y < 0)
						shipP->object.motion.y++;
					}
				
				}
			else
				{
				// This ship is controlled by the computer
				
				// Change directions?
				if (RandN(1000) < 15)		// 20 slightly too often
					{
					AlienChangeDirection(shipP);
					}
					
				
				// Fire a shot?  Have an alien shoot more often if it has more
				// shots available.
				if (RandN(1000) < GameStatus.level * 2 * shipP->shotsAvailable)	// 30 is often
					{
					AlienShoot (shipP, NULL);
					}
				
				if (shipP->status.type.alien.fleeing &&
					RandN(1000) < chanceToStopFleeing)
					{
					shipP->status.type.alien.speed -= fleeSpeedIncrease;
					shipP->status.type.alien.fleeing	= false;
					}
				
				// Big ships (typically a base) should throw sparks when
				// damaged.
				if (IsPlanet(&shipP->object) && 
					shipP->armorAvailable < shipAlienHomeWorldArmor &&
					RandN(shipAlienHomeWorldArmor) < (shipAlienHomeWorldArmor - 
						shipP->armorAvailable))
					{
					SparkAddFromObject(&shipP->object);
					}
				
				if (shipP->object.type == shipAlienBase && 
					shipP->armorAvailable < shipAlienBaseArmor &&
					RandN(shipAlienBaseArmor * 2) < (shipAlienBaseArmor - 
						shipP->armorAvailable))
					{
					SparkAddFromObject(&shipP->object);
					}
				}
				
			ObjectMove((ObjectType *) shipP);
			SectorAddObject(&shipP->object, false);
			
			
			// Provide a error prone collision detection system for alien ships
			// by checking if there is anything dangerous in the sector that the
			// alien is mostly in
			if (shipP->object.nextObjectInSector != NULL &&
				!ShipIsOwnedByHuman(shipP) && 
				!shipP->status.type.alien.fleeing )
				{
				// Determine the object's owner
				if (IsShot(shipP->object.nextObjectInSector))
					owner = ((ShotType *) (shipP->object.nextObjectInSector))->ownerPlayerNumber;
				else if (IsShip(shipP->object.nextObjectInSector))
					owner = ((ShipType *) (shipP->object.nextObjectInSector))->owner;
				else
					owner = noPlayer;
					
				
				// If the object isn't on the same team as the alien ship then
				// do something about it.
				if (owner != alienPlayer)
					AlienAvoidObject(shipP, shipP->object.nextObjectInSector);
				}

			}
	
	
		// Next ship
		shipP--;
		}
	
	
	
	// Now handle any ships that needed to appear safely.
	// Without this code ships to appear safely can appear were alien
	// ships are because new ships are processed before existing ships
	// and without being processed the existing ships aren't entered into
	// the sector list and can't be detected for collisions.
	shipP = firstShipToAppearSafelyP;
	firstShipP = GameStatus.objects.ship;
	while (shipsToAppearSafelyCount > 0 &&
		shipP >= firstShipP)
		{
		if (!shipP->status.inWarp &&
			!shipP->status.exitingWarp)
			{
			// Only player ships appear safely.
			if (ShipIsOwnedByHuman(shipP) &&
				shipP->status.type.player.appearSafely)
				{
				shipsToAppearSafelyCount--;
				ShipAppearSafely(shipP);
				}
			}
			shipP--;
		}
				
}


/***********************************************************************
 *
 * FUNCTION:     ShotsMoveAll
 *
 * DESCRIPTION:  Move all the depth shots.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/22/96	Initial Revision
 *
 ***********************************************************************/
static void ShotsMoveAll ()
{
	ShotType *shotP;
	ShotType *firstShotP;
	ObjectType *target;
	Int8 heading;


	// Move the shots
	// Do this before adding a shot to insure that one just added can move
	// out of the way of the next one.
	shotP = LastShot;
	firstShotP = GameStatus.objects.shot;
	while (shotP >= firstShotP)
		{
		shotP->duration--;
		if (shotP->duration > 0)
			{
			if (shotP->object.type == shotPlasma &&
				TimeToChangeHeading(shotP))
				{
				target = (ObjectType *) ShipGetNearestPlayer((GamePoint *)&shotP->object.location);
				if (target != NULL)
					{
					ShotHeadTowardsObject (shotP, 
						target, 
						&shotP->object.motion.x, &shotP->object.motion.y);
					}
				else
					// Have the shot circle until a player appears.
					// Have the shot move randomly for now.
					{
					heading = RandN(degreesMax);
					shotP->object.motion.x = MovementX[heading] * shotSpeed;
					shotP->object.motion.y = MovementY[heading] * shotSpeed;
					}
				}
			ObjectMove(&shotP->object);
			SectorAddObject(&shotP->object, false);
			}
		else
			ShotSetNotUsable(shotP);
		
		shotP--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     RockMoveAll
 *
 * DESCRIPTION:  Move all the rocks.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/22/96	Initial Revision
 *
 ***********************************************************************/
static void RockMoveAll ()
{
	RockType *rock;
	RockType *firstRock;


	// Move the rocks
	firstRock = GameStatus.objects.rock;
	rock = LastRock;
	while (rock >= firstRock)
		{
		ObjectMove(&rock->object);
		SectorAddObject(&rock->object, false);
		
		rock--;
		}
	
	
	// It is possible for a rock to exist off screen and be angled so
	// that it will never travel somewhere visible.  Detect if the first
	// rock is offscreen and send it hurling toward the center of the
	// game space if so.  Most users will probably never notice this change
	// of direction other than that they'll never seem to get stuck in 
	// this condition.
	rock = GameStatus.objects.rock;
	if (rock->object.usable)
		{
		// Check for a rock traveling up or down the left border
		if (rock->object.location.right <= borderAroundScreen &&
			rock->object.motion.x == 0)
			{
			rock->object.motion.x = rock->object.motion.y;
			}
		// Check for a rock traveling across the top border
		else if (rock->object.location.bottom <= borderAroundScreen &&
			rock->object.motion.y == 0)
			{
			rock->object.motion.y = rock->object.motion.x;
			}
		}

}


/***********************************************************************
 *
 * FUNCTION:     SparksMoveAll
 *
 * DESCRIPTION:  Move all the sparks.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/20/96	Initial Revision
 *
 ***********************************************************************/
static void SparksMoveAll ()
{
	SparkType *sparkP;
	SparkType *firstSpark;


	// Move the sparks
	sparkP = LastSpark;
	firstSpark = GameStatus.objects.spark;
	while (sparkP >= firstSpark)
		{
		sparkP->duration--;
		sparkP->location.x += sparkP->motion.x;
		sparkP->location.y += sparkP->motion.y;
		sparkP--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     ScoresWaitAll
 *
 * DESCRIPTION:  Move all the scores.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/19/96	Initial Revision
 *
 ***********************************************************************/
static void ScoresWaitAll ()
{
	ScoreType *scoreP;
	ScoreType *firstScore;


	// Move the scores
	scoreP = LastScore;
	firstScore = GameStatus.objects.score;
	while (scoreP >= firstScore)
		{
		scoreP->duration--;

		scoreP--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     BonusesWaitAll
 *
 * DESCRIPTION:  Move all the bonuses.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/29/96	Initial Revision
 *
 ***********************************************************************/
static void BonusesWaitAll ()
{
	BonusType *bonusP;
	BonusType *firstBonus;


	// Move the bonuses
	bonusP = LastBonus;
	firstBonus = GameStatus.objects.bonus;
	while (bonusP >= firstBonus)
		{
		bonusP->duration--;
		SectorAddObject(&bonusP->object, false);

		bonusP--;
		}
}


/***********************************************************************
 *
 * FUNCTION:     PlayersAddShip
 *
 * DESCRIPTION:  Add ships for players that need one.  If no ships
 * are left the game is ended.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/21/96	Initial Revision
 *
 ***********************************************************************/
static void PlayersAddShip ()
{
	Boolean playersHaveNoMoreShips;
	Int16 playerNum;
	
	
	// See if any players need anymore ships.  Also check if the playersHaveNoMoreShips.
	playersHaveNoMoreShips = true;
	for (playerNum = GameStatus.playerCount - 1; playerNum >= 0; playerNum--)
		{
		if (GameStatus.stats[playerNum].livesRemaining > 0)
			{
			if (GameStatus.stats[playerNum].shipsOwned < 
				GameStatus.stats[playerNum].shipsExpectedToOwn &&
				GameStatus.stats[playerNum].livesRemaining > 0)
				{
				if (GameStatus.stats[playerNum].periodsUntilAnotherShip <= 0)
					{
					if (ShipAddPlayer(playerNum))
						{
#ifndef OPTION_DETERMINISTIC_PLAY
						GameStatus.stats[playerNum].livesRemaining--;
						GameStatus.stats[playerNum].livesChanged = true;
#endif
						}
					}
				else
					{
					GameStatus.stats[playerNum].periodsUntilAnotherShip--;
					}
				}

			// At least one player has a ship remaining
			playersHaveNoMoreShips = false;
			}
		else if (GameStatus.stats[playerNum].shipsOwned > 0)
			{
			// At least one player has a ship left in play
			playersHaveNoMoreShips = false;
			}
		}
	

	// If there are no more ships to play and we have waited a while then end 
	// the game.  We wait a while so that the players can see themselves die
	// and so that they lay off of the hard keys.	
	if (playersHaveNoMoreShips)
		{
		if (GameStatus.periodsTillNextLevel > 0)
			GameStatus.periodsTillNextLevel--;
		else
			GameStatus.status = gameOver;
		}	
}


/***********************************************************************
 *
 * FUNCTION:     RandomInput
 *
 * DESCRIPTION:  Return a made up set of input to control a ship.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     A mask of key statuses
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/3/97	Initial Revision
 *
 ***********************************************************************/
#ifdef OPTION_DETERMINISTIC_PLAY

static UInt32 RandomInput ()
{
	UInt32 result;
	
	
	result = RandN(0xff);	// test code
	if (1 > RandN(4))
		result |= thrustKey;
	else
		result &= ~thrustKey;
		
	if (1 > RandN(450))
		result |= warpKey;
	else
		result &= ~warpKey;
		
	return result;
}

#endif


/***********************************************************************
 *
 * FUNCTION:     GameStateElapse
 *
 * DESCRIPTION:  Increment the state of the game world.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/23/96	Initial Revision
 *
 ***********************************************************************/
static void GameStateElapse ()
{
	Int16 collisionCount = 0;
	Int32 currentTime;
	
	
	GameStatus.periodCounter++;

	// Don't advance past checking high scores.
	if (GameStatus.status == checkHighScores)
		return;
	
	// We have paused after the game.  Now check for a high score.
	if (GameStatus.status == gameOver)
		{
		// Check if the alien home world was destroyed.  If so set the
		// level to the winner level for the high score dialog.
		if (GameStatus.level == alienHomeWorldLevel &&
			GameStatus.objects.baseCount == 0)
			{
			GameStatus.level = winnerLevel;
			FrmHelp(winnerMessageStr);
			}
		
		// Display a message congratulating the player and finish the plot.
		if (GameStatus.level == heroLevel)
			FrmHelp(heroMessageStr);
		
		GameStatus.status = checkHighScores;
		HighScoresCheckScore(GameStatus.stats[GameStatus.playerUsingScreen].score);
		
		// Allow the hard keys to switch to another app now that the player is done.
		GameUnmaskKeys ();
		return;
		}
	
	// When all the rocks are clear advance to the next level.
	if (GameStatus.rocksToSend == 0 && 
		GameStatus.objects.rockCount == 0 && 
		GameStatus.aliensToSend == 0 && 
		(GameStatus.objects.alienCount == 0 || !IsAlienWaveLevel(GameStatus.level)) && 
		GameStatus.status == gameInMotion)
		{
		GameEndLevel();
		return;
		}
	
		
	// The time between the last advance and the next is constant
	GameStatus.nextPeriodTime = GameStatus.nextPeriodTime + advanceTimeInterval;
	
	// If a game has been slowed by a whole game period then resynch the period timer.
	currentTime = TimGetTicks();
	if ((Int32) GameStatus.nextPeriodTime - currentTime <= 0)
		GameStatus.nextPeriodTime = currentTime + advanceTimeInterval;
	
	
	// The players have won the game.  Perform end skit until done.
	if (GameStatus.status == gameWon)
		{
		PlayersAddShip ();
		
		GameControlShips ();
		
		SectorReset();		// Move the ships now.
		
		ShipMoveAll ();
		ShotsMoveAll ();
		SparksMoveAll ();
		
		ShotRemoveUnusable();
		SparkRemoveExpired();
		
		// Keep the game going until all the sparks have vanished.
		if (GameStatus.objects.sparkCount)
			GameStatus.status = gameWon;

		return;
		}
		
	
	// When a level is over there is a period of quite time during which
	// no aliens are sent.
	if (GameStatus.status == levelOver)
		{
		GameStatus.periodsTillNextLevel--;
		}
	else
		{
		// Add more aliens.  The higher the level the more likely an alien is added.
		if (IsAlienWaveLevel(GameStatus.level))
			{
			// This level is the alien wave level.  Add an alien if there are
			// aliens to send and there aren't already a bunch of aliens 
			// flying about.  Or send them if there is a base left but not
			// enough alien defenders.
			if ((GameStatus.objects.baseCount > 0 &&
					((GameStatus.objects.alienCount - GameStatus.objects.baseCount) < 
						minAliensToDefendBases)) ||
				((GameStatus.aliensToSend > 0) &&
					(GameStatus.objects.alienCount < 1 ||
					(GameStatus.objects.alienCount < (3 + GameStatus.level / (levelsForAnAlienWave * 2)) &&
						RandN(1000) < (GameStatus.level / 4 + 14)))))		// chanceForNewAlien * 2
				{
				ShipAddAlien(shipAlienLarge);
				}
			}
		else
			{
			// Add aliens if there aren't many rocks.  This motivates some users
			// to quickly finish the level.  Users who hang around for a bonus
			// will find that the aliens will destroy the remaining rocks.
			while (GameStatus.objects.rockCount < 5 &&
				GameStatus.level >= firstLevelForAliens &&
				RandN(1000) < (GameStatus.level / 6 + 4))		// chanceForNewAlien
				{
					ShipAddAlien(shipAlienLarge);
				}
			}
		
		
		// Add bonuses.  The higher the level the more likely a bonus is added.
		while (RandN(1000) < GameStatus.level / 2 &&		// chanceFor a bonus
			GameStatus.level >= firstLevelForAliens &&
			GameStatus.level < alienHomeWorldLevel)
			{
				BonusAdd();
			}
		
		// If the level message is showing reduce it's time up.
		if (LevelMessageLength > 0)
			{
			LevelMessageDuration--;
			if (LevelMessageDuration == 0)
				{
				LevelMessageLength = 0;
				}
			}
		}


	// Record the console player's input
	GameStatus.stats[GameStatus.playerUsingScreen].playerInput = ConsoleGetInput();
#ifdef OPTION_DETERMINISTIC_PLAY
	GameStatus.stats[GameStatus.playerUsingScreen].playerInput = RandomInput();

	GameStatus.stats[1].playerInput = RandomInput();
	GameStatus.stats[2].playerInput = RandomInput();
	GameStatus.stats[3].playerInput = RandomInput();

#endif


	PlayersAddShip ();
	
		
	SectorReset();
	
	if (GameStatus.rocksToSend > 0)
		RockAddInitialRock();
	
	// Rocks go first because they don't destroy each other when they touch
	// each other.  Therefore they don't need collision detection against each
	// other when entered into the sector list which speeds up their case.
	RockMoveAll ();

	// Shot go next because it's nice if a shot can take out a rock before we 
	// move the ship into the rock.
	ShotsMoveAll();

	// Move the ships now.
	ShipMoveAll ();


	// Move the sparks now.
	SparksMoveAll ();
	
	// The scores should wait.
	ScoresWaitAll();

	// The bonuses should wait.
	BonusesWaitAll();


	// Do this after the above move calls because it can set objects
	// not usable which the above don't check because of the speed impact.
	if (GameStatus.bombExplode)
		BombExplode();


	RockRemoveUnusable();
	ShotRemoveUnusable();
	ShipRemoveUnusable();
	SparkRemoveExpired();
	ScoreRemoveExpired();
	BonusRemoveExpiredOrUnusable();
}


/***********************************************************************
 *
 * FUNCTION:     GamePlaySounds
 *
 * DESCRIPTION:  Play a game sound.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/30/96	Initial Revision
 *
 ***********************************************************************/
static void GamePlaySounds ()
{
	SndCommandType		sndCmd;


	if (GameStatus.soundToMake != noSound)
		{
		sndCmd.cmd = sndCmdFreqDurationAmp;
		sndCmd.param1 = Sound[GameStatus.soundToMake].frequency;
		sndCmd.param2 = Sound[GameStatus.soundToMake].duration;
		sndCmd.param3 = SoundAmp;

		SndDoCmd( 0, &sndCmd, true/*noWait*/ );

		GameStatus.soundPeriodsRemaining--;
		if (GameStatus.soundPeriodsRemaining <= 0)
			GameStatus.soundToMake = noSound;
		
		}
}


/***********************************************************************
 *
 * FUNCTION:    InfoDisplay
 *
 * DESCRIPTION: Display the info dialog
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/20/96	Initial Revision
 *
 ***********************************************************************/
static void InfoDisplay (void)
{
	FormPtr curFormP;
	FormPtr formP;


	curFormP = FrmGetActiveForm ();
	formP = FrmInitForm (InfoDialog);
	FrmSetActiveForm (formP);
	FrmDoDialog (formP);
	FrmDeleteForm (formP);
	FrmSetActiveForm (curFormP);
}


/***********************************************************************
 *
 * FUNCTION:    HighScoresDisplay
 *
 * DESCRIPTION: Display the high score dialog
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/11/96	Initial Revision
 *
 ***********************************************************************/
static void HighScoresDisplay (void)
{
	FormPtr curFormP;
	FormPtr formP;
	MemHandle titleH;
	Char * titleP;
	char string[16];
	FontID currFont;
	int i;
	Int16 y;
	Char * winnerStringP;
	Char * heroStringP;


	curFormP = FrmGetActiveForm ();
	formP = FrmInitForm (HighScoresDialog);
	FrmSetActiveForm (formP);
	FrmDrawForm (formP);

	// Remember the font	
	currFont = FntSetFont(boldFont);
	
	
	// Draw the titles of the columns
	titleH = DmGetResource(strRsc, NameColumnStr);
	titleP = MemHandleLock(titleH);
	WinDrawChars(titleP, StrLen(titleP), highScoreNameColumnX,
		firstHighScoreY - highScoreHeight);
	MemPtrUnlock(titleP);	

	titleH = DmGetResource(strRsc, ScoreColumnStr);
	titleP = MemHandleLock(titleH);
	WinDrawChars(titleP, StrLen(titleP), highScoreScoreColumnX - 
		FntCharsWidth(titleP, StrLen(titleP)), firstHighScoreY - highScoreHeight);
	MemPtrUnlock(titleP);	

	titleH = DmGetResource(strRsc, LevelColumnStr);
	titleP = MemHandleLock(titleH);
	WinDrawChars(titleP, StrLen(titleP), highScoreLevelColumnX - 
		FntCharsWidth(titleP, StrLen(titleP)), firstHighScoreY - highScoreHeight);
	MemPtrUnlock(titleP);	


	WinDrawLine(highScoreNameColumnX, firstHighScoreY - 1, highScoreLevelColumnX, firstHighScoreY - 1);
	// Draw each high score in the right spot
	for (i = 0; i < highScoreMax && Prefs.highScore[i].score > 0; i++)
		{
		y = firstHighScoreY + i * highScoreHeight;

		// Differentiate the last high score by choosing a different font.
		if (i == Prefs.lastHighScore)
			FntSetFont(boldFont);
		else
			FntSetFont(highScoreFont);
		
		// Display the score number
		StrIToA(string, i + 1);
		StrCat(string, ". ");
		WinDrawChars(string, StrLen(string), highScoreNameColumnX - 
			FntCharsWidth(string, StrLen(string)), y);
		
		WinDrawChars(Prefs.highScore[i].name, StrLen(Prefs.highScore[i].name),
			highScoreNameColumnX, y);
		
		StrIToA(string, Prefs.highScore[i].score);
		WinDrawChars(string, StrLen(string), highScoreScoreColumnX - 
			FntCharsWidth(string, StrLen(string)), y);
		
		// The level is either a number or if they destroyed the homeworld
		// the level is a string.
		if (Prefs.highScore[i].level < heroLevel + 1)
			{
			StrIToA(string, Prefs.highScore[i].level);
			WinDrawChars(string, StrLen(string), highScoreLevelColumnX - 
				FntCharsWidth(string, StrLen(string)), y);
			}
		else if (Prefs.highScore[i].level == winnerLevel + 1)
			{
			winnerStringP = MemHandleLock(DmGetResource(strRsc, winnerStr));
			WinDrawChars(winnerStringP, StrLen(winnerStringP), highScoreLevelColumnX - 
				FntCharsWidth(winnerStringP, StrLen(winnerStringP)), y);
			MemPtrUnlock(winnerStringP);
			}
		else 
			{
			heroStringP = MemHandleLock(DmGetResource(strRsc, heroStr));
			WinDrawChars(heroStringP, StrLen(heroStringP), highScoreLevelColumnX - 
				FntCharsWidth(heroStringP, StrLen(heroStringP)), y);
			MemPtrUnlock(heroStringP);
			}
		}
	FntSetFont(currFont);

		
	FrmDoDialog (formP);
	FrmDeleteForm (formP);
	FrmSetActiveForm (curFormP);
}


/***********************************************************************
 *
 * FUNCTION:    HighScoresAddScore
 *
 * DESCRIPTION: Add the new score.
 *
 * PARAMETERS:  position - the position to add the score
 *					 name - name to add
 *					 score - score to add
 *					 level - level to add
 *					 dontAddIfExists - used when initializing scores
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/13/96	Initial Revision
 *			roger	3/19/96	Broke out the check and contratulations dialog
 *
 ***********************************************************************/
static void HighScoresAddScore (Char * name, Int32 score, Int16 level, 
	Boolean dontAddIfExists)
{
	Int16 position;
	
	
	// Find where the score belongs.  The new score looses any ties.
	position = highScoreMax;
	while (position > 0 &&
		score > Prefs.highScore[position - 1].score)
		{
		position--;
		}
	
	
	// Leave if the score doesn't make it into the high scores.
	if (position >= highScoreMax)
		return;
	
	if (dontAddIfExists &&
		position > 0 &&
		StrCompare(name, Prefs.highScore[position - 1].name) == 0 &&
		score == Prefs.highScore[position - 1].score &&
		level == Prefs.highScore[position - 1].level)
		return;
		
	// Move down the scores to make room for the new high score.
	MemMove(&Prefs.highScore[position + 1], &Prefs.highScore[position],
		(highScoreMax - 1 - position) * sizeof (SavedScore));
	
	
	Prefs.highScore[position].score = score;
	Prefs.highScore[position].level = level;
	StrCopy(Prefs.highScore[position].name, name);
	
	
	// Record this new score as the last one entered.
	Prefs.lastHighScore = position;
}


/***********************************************************************
 *
 * FUNCTION:    HighScoresCheckScore
 *
 * DESCRIPTION: Check if the current score is a high one and call
 *	HighScoresAddScore if so.
 *
 * PARAMETERS:  score - score to possibly add
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/13/96	Initial Revision
 *			roger	3/19/96	Broke into separate routine
 *			roger	4/23/96	Added a cancel button
 *			jmp	10/25/99	Don't use nameP unless it's locked; fixes
 *								bug #21539.
 *
 ***********************************************************************/
static void HighScoresCheckScore (Int32 score)
{
	int i;
	MemHandle nameH;
	Char * nameP;
	Char * firstSpaceP;
	FormPtr curFormP;
	FormPtr formP;
	UInt16 objIndex;
	UInt16 buttonHit;

	i = highScoreMax;
	while (i > 0 &&
		score > Prefs.highScore[i - 1].score)
		i--;

	// Leave if the score doesn't make it into the high scores.
	if (i >= highScoreMax)
		return;

	// Allocate a chunk for the user to edit.  The field in the dialog requires
	// the text to be in a chunk so it can be resized.
	nameH = MemHandleNew(dlkMaxUserNameLength + 1);
	nameP = MemHandleLock(nameH);

	// For the name, try and use the name last entered
	if (Prefs.lastHighScore != highScoreMax)
		{
		StrCopy(nameP, Prefs.highScore[Prefs.lastHighScore].name);
		}
	else
		{		
		StrCopy(nameP, "");
		
		// Try and use the user's name
		DlkGetSyncInfo(NULL, NULL, NULL, nameP, NULL, NULL);
		
		// Just use the first name
		firstSpaceP = StrChr(nameP, spaceChr);
		if (firstSpaceP)
		 	*firstSpaceP = '\0';
		
		// Truncate the string to insure it's not too long
		nameP[nameLengthMax] = '\0';
		}
	MemPtrUnlock(nameP);

	// Record this new score as the last one entered.
	Prefs.lastHighScore = i;

	// Now Display a dialog contragulating the user and ask for their name.
	curFormP = FrmGetActiveForm ();
	formP = FrmInitForm (NewHighScoresDialog);

	// Set the field to edit the name.
	objIndex = FrmGetObjectIndex (formP, NewHighScoresNameField);
	FldSetTextHandle(FrmGetObjectPtr (formP, objIndex), nameH);
	// Set the insertion point blinking in the only field
	FrmSetFocus(formP, objIndex);
	// Set Graffiti to be shifted.
	GrfSetState(false, false, true);
	
	// Allow the user to type in a name.  Wait until a button is pushed. OK is 
	// the default button so if the app is switched the high score is still entered.
	// The user must press cancel to not record the score.
	buttonHit = FrmDoDialog (formP);

	// Take the text handle from the field so the text isn't deleted when the form is.
	FldSetTextHandle(FrmGetObjectPtr (formP, objIndex), 0);
	
	FrmDeleteForm (formP);					// Deletes the field's new text.
	FrmSetActiveForm (curFormP);

	// Add the score unless the user removed the name.  If so they probably didn't
	// want the score entered so don't!
	nameP = MemHandleLock(nameH);
	if (buttonHit == NewHighScoresOKButton &&
		nameP[0] != '\0')	
		HighScoresAddScore(nameP, score, GameStatus.level + 1, false);

	// The name is now recorded and no longer needed.
	MemHandleFree(nameH);

	// Now display where the new high score is in relation to the others
	if (buttonHit == NewHighScoresOKButton)
		HighScoresDisplay();
}


/***********************************************************************
 *
 * FUNCTION:    MainViewDoCommand
 *
 * DESCRIPTION: Performs the menu command specified.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/30/96	Initial Revision
 *
 ***********************************************************************/
static Boolean MainViewDoCommand (UInt16 command)
{
	switch (command)
		{
		case BoardGameNewCmd:
			GameStart();
			GameStateDraw ();
			break;

		case AboutCmd:
			InfoDisplay();
			break;
					
		case BoardGameInstructionsCmd:
			FrmHelp (InstructionsStr);
			break;
					
		case BoardGameHighScoresCmd:
			HighScoresDisplay();
			break;
		}
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    MainViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Board View"
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/1/95	Initial Revision
 *			jmp	10/25/99	Made frmUpdateEvent work under the Palm OS 3.5
 *								environment (i.e., various items were missing,
 *								like the score & number of ships remaining, after
 *								things like the about box and help dialogs were
 *								dismissed).
 *			jmp	12/20/99	Prevent the command bar from interfering from
 *								game play by not allowing it to become available.
 *
 ***********************************************************************/
static Boolean MainViewHandleEvent (EventPtr event)
{
	FormPtr frm;
	Boolean handled = false;

	if (event->eType == nilEvent)
		{
		}

	else if (event->eType == keyDownEvent)
		{
		// time spent playing		(quick code at this point.)
		if (event->data.keyDown.chr == 't')
			{
			char timeString[timeStringLength + 5];
			UInt32 seconds;
			DateTimeType timeSpent;
			
			seconds = (Prefs.accumulatedTime + (TimGetTicks() - 
				GameStatus.startTime)) / SysTicksPerSecond();
			TimSecondsToDateTime(seconds, &timeSpent);
			TimeToAscii(timeSpent.hour, timeSpent.minute, tfColon24h, timeString);
			StrCat(timeString, ":");
			
			if (timeSpent.second < 10)
				StrCat(timeString, "0");
			StrIToA(&timeString[StrLen(timeString)], timeSpent.second);
			
			WinDrawChars (timeString, StrLen(timeString), 70, 130);
			}

		// Restart game using the page down key
		else if (event->data.keyDown.chr == restartGameChar)
			{
			if (GameStatus.status == checkHighScores)
				{
				GameStart();
				GameStateDraw ();
				}
			}
		return true;
		}

	else if (event->eType == menuEvent)
		{
		MainViewDoCommand (event->data.menu.itemID);
		return true;
		}

	else if (event->eType == frmCloseEvent)
		{
		}

	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();

		FrmDrawForm (frm);
		GameStart ();
		GameStatePrepareDraw();
		GameStateDraw ();

		handled = true;
		}

	else if (event->eType == frmUpdateEvent)
		{
		frm = FrmGetActiveForm ();

		FrmDrawForm (frm);
		GameStateDrawEverything();
		
		handled = true;
		}
		
	// Don't allow the command bar to come up as it interferes with game play.
	else if (event->eType == menuCmdBarOpenEvent)
		handled = true;
		
	return (handled);
}



/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/1/95	Initial Revision
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent (EventPtr event)
{
	UInt16 formId;
	FormPtr frm;

	if (event->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = event->data.frmLoad.formID;
		frm = FrmInitForm (formId);
		FrmSetActiveForm (frm);		
		
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
			{
			case MainView:
				FrmSetEventHandler (frm, MainViewHandleEvent);
				break;
		
			}
		return (true);
		}
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    EventLoop
 *
 * DESCRIPTION: This routine is the event loop for the aplication.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/1/95	Initial Revision
 *
 ***********************************************************************/
static void EventLoop (void)
{
	UInt16 error;
	EventType event;

	do
		{
		// Wait until the next game period.
		EvtGetEvent (&event, TimeUntillNextPeriod());
		
		
		// Detect exiting the game's window.  This must be checked for.  At this
		// point there probably exists another window which may cover part of
		// the MainView window.  Suppress drawing.  Otherwise drawing may draw
		// to part of the window covered by the new window.
		if (event.eType == winExitEvent)
			{
			if (event.data.winExit.exitWindow == (WinHandle) FrmGetFormPtr(MainView))
				{
				GameStatus.paused = true;
				GameStatus.pausedTime = TimGetTicks();
				}
			}

		// Detect entering the game's window.  Resume drawing to our window.
		else if (event.eType == winEnterEvent)
			{
			// In the current code, the menu doesn't remove itself when it receives
			// a winExitEvent.
			if (event.data.winEnter.enterWindow == (WinHandle) FrmGetFormPtr(MainView) &&
				event.data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ())
				{
				// Sometimes we can enter the game's window without knowing it was 
				// ever left.  In that case the pause time will not have been recorded.
				// Set the current period back to it's beginning
				if (!GameStatus.paused)
					{
					GameStatus.nextPeriodTime = TimGetTicks() + advanceTimeInterval;
					}
				else
					{
					// Unpause the game.  Account for time lost during pause
					GameStatus.paused = false;
					GameStatus.nextPeriodTime += (TimGetTicks() - GameStatus.pausedTime);
					
					// Fixup the time spent playing the game by changing the startTime.
					GameStatus.startTime += (TimGetTicks() - GameStatus.pausedTime);
					}
				
				
				// Redraw the game window.  This is normally triggered when the launcher
				// is activate during a low heap memory condition to work around a launcher
				// bug which causes it to fail to send a frmUpdateEvent when it doesn't
				// restore the bits underneath it.
				if (redrawWhenReturningToGameWindow)
					{
					GameStateDrawEverything();
					redrawWhenReturningToGameWindow = false;
					}
				}
			}


		// If it's time, go to the next time period
		else if (TimeUntillNextPeriod() == 0)
			{
			GameStateDraw();				// Draw the state renedered in the screen buffer
			GameStateAdvance();			// Advance the level if neccessary.
			GameStateElapse();			// Poll user input, moved objects, handle collisions
			// send state to clients here
			GameStatePrepareDraw();		// Render the game state
			GamePlaySounds();				// Play the most important sound requested.
			
			// Usually this code block is entered when EvtGetEvent has waited
			//  until the next period and returned a nilEvent.  For this case
			//  we should skip the event handlers which don't do anything with
			//  a nilEvent.
			if (event.eType == nilEvent)
				continue;
			}

		// Intercept the hard keys to prevent them from switching apps
		if (event.eType == keyDownEvent)
			{
			// Swallow events notifying us of key presses.  We poll instead.
			if (event.data.keyDown.chr >= hard1Chr &&
				event.data.keyDown.chr <= hard4Chr &&
				GameStatus.status != checkHighScores &&
				!(event.data.keyDown.modifiers & poweredOnKeyMask))
				{
				continue;
				}
			
			// Handle a launcher bug.  It can fail to send a frmUpdateEvent.
			else if (event.data.keyDown.chr >= launchChr)
				{
				UInt32 freeBytes;
				UInt32 maxChunk;
				
				
				MemHeapFreeBytes (0, &freeBytes, &maxChunk);
				if (freeBytes <= saveBitsThreshold)
					{
					redrawWhenReturningToGameWindow = true;
					}
				}

			}

		
		
		if (! SysHandleEvent (&event))
		
			if (! MenuHandleEvent (0, &event, &error))
			
				if (! ApplicationHandleEvent (&event))
	
					FrmDispatchEvent (&event); 
		
		}
	while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    RocksMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/22/96	Initial Revision
 *
 ***********************************************************************/
 
UInt32		PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;
	

	error = RomVersionCompatible (version20, launchFlags);
	if (error) return (error);


	if (cmd == sysAppLaunchCmdNormalLaunch)
		{
		error = StartApplication ();
	
	
		FrmGotoForm (MainView);
		
		if (! error)
			EventLoop ();
	
		StopApplication ();
		}
	
	return 0;
}

