#include "dx_linux.h"
#include "kaskadeurrennfahrer.h"
#include "3D_Engine.h"
#include "Backdrop.h"
#include "Track.h"
#include "Car.h"
#include "Car_Behaviour.h"
#include "Opponent_Behaviour.h"
#include "wavefunctions.h"
#include "Atlas.h"
#include "version.h"
#include <unistd.h>

// constants, and global variables
#define STRING "%S"
#define DEFAULT_FRAME_GAP (6)  // 4 Used to limit frame rate.  Amiga StuntCarRacer uses value of 6 (called MIN.FRAMES)
#define HEIGHT_ABOVE_ROAD (100) // 100
#define FURTHEST_Z (131072.0f)

IDirect3DDevice9 pd3dDevice;
int screenH, screenW, screenX, screenY;
int viewH, viewW, viewX, viewY;
float screenScale = 1.;
GameModeType GameMode = TRACK_MENU;
const char *gameController;

// the following both are used for keyboard input
UINT keyPress = '\0';
uint32_t input = 0;
uint32_t lastInput = 0;
int track_number = 0; // 0 is first track

static IDirectSound8 *ds;
IDirectSoundBuffer8 *WreckSoundBuffer = NULL;
IDirectSoundBuffer8 *HitCarSoundBuffer = NULL;
IDirectSoundBuffer8 *GroundedSoundBuffer = NULL;
IDirectSoundBuffer8 *CreakSoundBuffer = NULL;
IDirectSoundBuffer8 *SmashSoundBuffer = NULL;
IDirectSoundBuffer8 *OffRoadSoundBuffer = NULL;
IDirectSoundBuffer8 *EngineSoundBuffers[8] = {NULL};

IDirect3DTexture9 *g_pAtlas = NULL;

static long frameGap = DEFAULT_FRAME_GAP;
static bool bFrameMoved = FALSE;

bool bShowStats = FALSE;
bool bNewGame = FALSE;
bool bPaused = FALSE;
bool bPlayerPaused = FALSE;
bool bOpponentPaused = FALSE;
long bTrackDrawMode = 0;
bool bOutsideView = FALSE;
long engineSoundPlaying = FALSE;
double gameStartTime, gameEndTime;

extern long TrackID;
extern long boostReserve, boostUnit, StandardBoost, SuperBoost;
extern long INITIALISE_PLAYER;
extern bool raceFinished, raceWon;
extern long lapNumber[];

// Gameplay variables
extern long damaged_limit;
extern long road_cushion_value;
extern long engine_power;
extern long boost_unit_value;
extern long opp_engine_power;

D3DXMATRIX matProj;
FLOAT fAspect = 256/128; //Seitenverhältnis Ingame

//-----------------------------------------------------------------------------
// Static variables
//-----------------------------------------------------------------------------
// Player 1 orientation
static long player1_x = 0,
			player1_y = 0,
			player1_z = 0;

static long player1_x_angle = (0 << 6),
			player1_y_angle = (0 << 6),
			player1_z_angle = (0 << 6);

// Opponent orientation
static long opponent_x = 0,
			opponent_y = 0,
			opponent_z = 0;

static float opponent_x_angle = 0.0f, opponent_y_angle = 0.0f, opponent_z_angle = 0.0f;

// Viewpoint 1 orientation
static long viewpoint1_x, viewpoint1_y, viewpoint1_z;
static long viewpoint1_x_angle, viewpoint1_y_angle, viewpoint1_z_angle;

// Target (lookat) point
static long target_x, target_y, target_z;

/**************************************************************************
  DSInit

  Description:
	Initialize all the DirectSound specific stuff
 **************************************************************************/

bool DSInit()
{
	HRESULT err;

	//
	//	First create a DirectSound object

	err = DirectSoundCreate8(NULL, &ds, NULL);
	if (err != DS_OK)
		return FALSE;

	return TRUE;
}

/**************************************************************************
  DSSetMode

	Initialises all DirectSound samples etc

 **************************************************************************/

bool DSSetMode()
{
	int i;

	// Amiga channels 1 and 2 are right side, channels 0 and 3 are left side

	if ((WreckSoundBuffer = MakeSoundBuffer(ds, L"WRECK")) == NULL)
		return FALSE;
	WreckSoundBuffer->SetPan(DSBPAN_RIGHT);
	WreckSoundBuffer->SetVolume(AmigaVolumeToDirectX(64));

	if ((HitCarSoundBuffer = MakeSoundBuffer(ds, L"HITCAR")) == NULL)
		return FALSE;
	HitCarSoundBuffer->SetFrequency(AMIGA_PAL_HZ / 238);
	HitCarSoundBuffer->SetPan(DSBPAN_RIGHT);
	HitCarSoundBuffer->SetVolume(AmigaVolumeToDirectX(56));

	if ((GroundedSoundBuffer = MakeSoundBuffer(ds, L"GROUNDED")) == NULL)
		return FALSE;
	GroundedSoundBuffer->SetFrequency(AMIGA_PAL_HZ / 400);
	GroundedSoundBuffer->SetPan(DSBPAN_RIGHT);

	if ((CreakSoundBuffer = MakeSoundBuffer(ds, L"CREAK")) == NULL)
		return FALSE;
	CreakSoundBuffer->SetFrequency(AMIGA_PAL_HZ / 238);
	CreakSoundBuffer->SetPan(DSBPAN_RIGHT);
	CreakSoundBuffer->SetVolume(AmigaVolumeToDirectX(64));

	if ((SmashSoundBuffer = MakeSoundBuffer(ds, L"SMASH")) == NULL)
		return FALSE;
	SmashSoundBuffer->SetFrequency(AMIGA_PAL_HZ / 280);
	SmashSoundBuffer->SetPan(DSBPAN_LEFT);
	SmashSoundBuffer->SetVolume(AmigaVolumeToDirectX(64));

	if ((OffRoadSoundBuffer = MakeSoundBuffer(ds, L"OFFROAD")) == NULL)
		return FALSE;
	OffRoadSoundBuffer->SetPan(DSBPAN_RIGHT);
	OffRoadSoundBuffer->SetVolume(AmigaVolumeToDirectX(64));

	if ((EngineSoundBuffers[0] = MakeSoundBuffer(ds, L"TICKOVER")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[1] = MakeSoundBuffer(ds, L"ENGINEPITCH2")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[2] = MakeSoundBuffer(ds, L"ENGINEPITCH3")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[3] = MakeSoundBuffer(ds, L"ENGINEPITCH4")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[4] = MakeSoundBuffer(ds, L"ENGINEPITCH5")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[5] = MakeSoundBuffer(ds, L"ENGINEPITCH6")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[6] = MakeSoundBuffer(ds, L"ENGINEPITCH7")) == NULL)
		return FALSE;
	if ((EngineSoundBuffers[7] = MakeSoundBuffer(ds, L"ENGINEPITCH8")) == NULL)
		return FALSE;

	for (i = 0; i < 8; i++)
	{
		EngineSoundBuffers[i]->SetPan(DSBPAN_CENTER);
		// Original Amiga volume was 48, but have reduced this for testing
		EngineSoundBuffers[i]->SetVolume(AmigaVolumeToDirectX(48 / 2));
	}

	return TRUE;
}

/**************************************************************************
  MixTerm
 **************************************************************************/

void MixTerm()
{
	// DirectSound
	if (WreckSoundBuffer)
		WreckSoundBuffer->Release(), WreckSoundBuffer = NULL;
	if (HitCarSoundBuffer)
		HitCarSoundBuffer->Release(), HitCarSoundBuffer = NULL;
	if (GroundedSoundBuffer)
		GroundedSoundBuffer->Release(), GroundedSoundBuffer = NULL;
	if (CreakSoundBuffer)
		CreakSoundBuffer->Release(), CreakSoundBuffer = NULL;
	if (SmashSoundBuffer)
		SmashSoundBuffer->Release(), SmashSoundBuffer = NULL;
	if (OffRoadSoundBuffer)
		OffRoadSoundBuffer->Release(), OffRoadSoundBuffer = NULL;

	for (int i = 0; i < 8; i++)
	{
		if (EngineSoundBuffers[i])
			EngineSoundBuffers[i]->Release(), EngineSoundBuffers[i] = NULL;
	}

	if (ds)
		ds->Release(), ds = NULL;
}

/*	======================================================================================= */
/*	Function:		InitialiseData															*/
/*																							*/
/*	Description:																			*/
/*	======================================================================================= */

static long InitialiseData(void)
{
	long success = FALSE;

	CreateSinCosTable();

	ConvertAmigaTrack(LITTLE_RAMP);

	// Seed the random-number generator with current time so that
	// the numbers will be different every time we run
	srand((unsigned)time(NULL));

	success = TRUE;

	return (success);
}

/*	======================================================================================= */
/*	Function:		FreeData																*/
/*																							*/
/*	Description:																			*/
/*	======================================================================================= */

static void FreeData(void)
{
	FreeCockpitVertexBuffer();
	FreeTrackData();
	MixTerm();
	// CloseAmigaRecording();
	return;
}

//--------------------------------------------------------------------------------------
// Colours
//--------------------------------------------------------------------------------------

#define NUM_PALETTE_ENTRIES (42 + 6)
// #define	PALETTE_COMPONENT_BITS	(8)	 // bits per colour r/g/b component

static PALETTEENTRY SCPalette[NUM_PALETTE_ENTRIES] =
	{
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00},

		// car colours 1
		{0x00, 0x00, 0x00},
		{0x88, 0x00, 0x22},
		{0xaa, 0x00, 0x33},
		{0xcc, 0x00, 0x44},
		{0xee, 0x00, 0x55},
		{0x22, 0x22, 0x33},
		{0x44, 0x44, 0x44},
		{0x33, 0x33, 0x33},

		// car colours 2
		{0x00, 0x00, 0x00},
		{0x22, 0x00, 0x88},
		{0x33, 0x00, 0xaa},
		{0x44, 0x00, 0xcc},
		{0x55, 0x00, 0xee},
		{0x22, 0x22, 0x33},
		{0x44, 0x44, 0x44},
		{0x33, 0x33, 0x33},

		// track colours (i.e. Stunt Car Racer car colours)
		{0x00, 0x00, 0x00},
		{0x99, 0x99, 0x77},
		{0xbb, 0xbb, 0x99},
		{0xff, 0xff, 0x00},
		{0x99, 0xbb, 0x33},
		{0x55, 0x77, 0x77},
		{0x55, 0xbb, 0xff},
		{0x55, 0x99, 0xff},
		{0x33, 0x55, 0x77},
		{0x55, 0x00, 0x00}, // 9
		{0x77, 0x33, 0x33}, // 10
		{0x99, 0x55, 0x55},
		{0xdd, 0x99, 0x99}, // 12
		{0x77, 0x77, 0x55},
		{0xbb, 0xbb, 0xbb},
		{0xff, 0xff, 0xff}
};

uint32_t SCRGB(long colour_index) // return full RGBA value
{
	return (RGB_MAKE(SCPalette[colour_index].peRed,
						  SCPalette[colour_index].peGreen,
						  SCPalette[colour_index].peBlue));
}

uint32_t Fill_Colour, Line_Colour;

void SetSolidColour(long colour_index)
{
	/*
		static uint32_t reducedSCPalette[NUM_PALETTE_ENTRIES];
		static long first_time = TRUE;

		// make all reduced palette values on first call
		if (first_time)
			{
			long i;
			for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
				{
				// reduce R/G/B to 5/8 of original
				reducedSCPalette[i] = D3DCOLOR_XRGB((5*SCPalette[i].peRed)/8,
											   (5*SCPalette[i].peGreen)/8,
											   (5*SCPalette[i].peBlue)/8);
				}

			first_time = FALSE;
			}

		Fill_Colour = reducedSCPalette[colour_index];
	*/
	Fill_Colour = SCRGB(colour_index);
}

void SetLineColour(long colour_index)
{
	Line_Colour = SCRGB(colour_index);
}

void SetTextureColour(long colour_index)
{
	Fill_Colour = SCRGB(colour_index);
}

TTF_Font *g_pFont;
/**************************************
/ CreateFonts
***************************************/
void CreateFonts()
{
	g_pFont = TTF_OpenFont("Font/8x8.ttf", 8);
	if (!g_pFont)
		cout << "Failed to load font: " << TTF_GetError() << endl;
	else
		cout << "Font created " << g_pFont << endl;
}

void LoadTextures()
{
	if (!g_pAtlas)
		g_pAtlas = new IDirect3DTexture9();
	g_pAtlas->LoadTexture("Bitmap/atlas.png");
	InitAtlasCoord();
	printf("Texture loaded\n");
}

void CreateBuffers(IDirect3DDevice9 *pd3dDevice)
{
	if (CreatePolygonVertexBuffer(pd3dDevice) != S_OK)
		printf("Error creating PolygonVertexBuffer\n");
	if (CreateTrackVertexBuffer(pd3dDevice) != S_OK)
		printf("Error creating TrackVertexBuffer\n");
	if (CreateShadowVertexBuffer(pd3dDevice) != S_OK)
		printf("Error creating ShadowVertexBuffer\n");
	if (CreateCarVertexBuffer(pd3dDevice) != S_OK)
		printf("Error creating CarVertexBuffer\n");
	if (CreateCockpitVertexBuffer(pd3dDevice) != S_OK)
		printf("Error creating CarVertexBuffer\n");
}

/*	======================================================================================= */
/*	Function:		CalcTrackMenuViewpoint													*/
/*																							*/
/*	Description:	*/
/*	======================================================================================= */

static void CalcTrackMenuViewpoint(void)
{
	static long circle_y_angle = 0;

	short sin, cos;
	long centre = (NUM_TRACK_CUBES * CUBE_SIZE) / 2;
	long radius = ((NUM_TRACK_CUBES - 2) * CUBE_SIZE) / PRECISION;

	// Target orientation - centre of world
	target_x = (NUM_TRACK_CUBES * CUBE_SIZE) / 2;
	target_y = 0;
	target_z = (NUM_TRACK_CUBES * CUBE_SIZE) / 2;

	// camera moves in a circle around the track
	if (!bPaused)
		circle_y_angle += 128;
	circle_y_angle &= (MAX_ANGLE - 1);

	GetSinCos(circle_y_angle, &sin, &cos);

	viewpoint1_x = centre + (sin * radius);
	viewpoint1_y = -CUBE_SIZE * 3;
	viewpoint1_z = centre + (cos * radius);

	LockViewpointToTarget(viewpoint1_x,
						  viewpoint1_y,
						  viewpoint1_z,
						  target_x,
						  target_y,
						  target_z,
						  &viewpoint1_x_angle,
						  &viewpoint1_y_angle);
	viewpoint1_z_angle = 0;
}

/*	======================================================================================= */
/*	Function:		CalcTrackPreviewViewpoint												*/
/*																							*/
/*	Description:	*/
/*	======================================================================================= */

#define NUM_PREVIEW_CAMERAS (9)

static void CalcTrackPreviewViewpoint(void)
{
	// Target orientation - opponent
	target_x = opponent_x,
	target_y = opponent_y,
	target_z = opponent_z;

#ifndef PREVIEW_METHOD1
	long centre = (NUM_TRACK_CUBES * CUBE_SIZE) / 2;

	viewpoint1_x = centre;

	if (TrackID == DRAW_BRIDGE)
		viewpoint1_y = opponent_y - (CUBE_SIZE * 5) / 2; // Draw Bridge requires a higher viewpoint
	else
		viewpoint1_y = opponent_y - CUBE_SIZE / 2;

	viewpoint1_z = centre;

	viewpoint1_x += (target_x - viewpoint1_x) / 2;
	viewpoint1_z += (target_z - viewpoint1_z) / 2;

	// lock viewpoint y angle to target
	LockViewpointToTarget(viewpoint1_x,
						  viewpoint1_y,
						  viewpoint1_z,
						  target_x,
						  target_y,
						  target_z,
						  &viewpoint1_x_angle,
						  &viewpoint1_y_angle);
#else
	// cameras - four at corners, four half way along, one at centre
	long camera_x[NUM_PREVIEW_CAMERAS] =
		{CUBE_SIZE,
		 CUBE_SIZE,
		 (NUM_TRACK_CUBES - 1) * CUBE_SIZE,
		 (NUM_TRACK_CUBES - 1) * CUBE_SIZE,
		 //
		 0,
		 (NUM_TRACK_CUBES / 2) * CUBE_SIZE,
		 (NUM_TRACK_CUBES)*CUBE_SIZE,
		 (NUM_TRACK_CUBES / 2) * CUBE_SIZE,
		 //
		 (NUM_TRACK_CUBES / 2) * CUBE_SIZE};
	long camera_z[NUM_PREVIEW_CAMERAS] =
		{CUBE_SIZE,
		 (NUM_TRACK_CUBES - 1) * CUBE_SIZE,
		 (NUM_TRACK_CUBES - 1) * CUBE_SIZE,
		 CUBE_SIZE,
		 //
		 (NUM_TRACK_CUBES / 2) * CUBE_SIZE,
		 (NUM_TRACK_CUBES)*CUBE_SIZE,
		 (NUM_TRACK_CUBES / 2) * CUBE_SIZE,
		 0,
		 //
		 (NUM_TRACK_CUBES / 2) * CUBE_SIZE};

	// calculate nearest camera
	long camera, distance, shortest_distance = 0, nearest = 0;
	double o, a;
	for (camera = 0; camera < NUM_PREVIEW_CAMERAS; camera++)
	{
		o = double(camera_x[camera] - target_x);
		a = double(camera_z[camera] - target_z);
		distance = (long)sqrt((o * o) + (a * a));

		if (camera == 0)
		{
			shortest_distance = distance;
			nearest = camera;
		}
		else if (distance < shortest_distance)
		{
			shortest_distance = distance;
			nearest = camera;
		}
	}

	viewpoint1_x = camera_x[nearest];
	viewpoint1_y = player1_y - CUBE_SIZE / 2;
	viewpoint1_z = camera_z[nearest];

	LockViewpointToTarget(viewpoint1_x,
						  viewpoint1_y,
						  viewpoint1_z,
						  target_x,
						  target_y,
						  target_z,
						  &viewpoint1_x_angle,
						  &viewpoint1_y_angle);
#endif

	viewpoint1_z_angle = 0;
}

/*	======================================================================================= */
/*	Function:		CalcGameViewpoint														*/
/*																							*/
/*	Description:	*/
/*	======================================================================================= */

static void CalcGameViewpoint(void)
{
	long x_offset, y_offset, z_offset;

	if (bOutsideView)
	{
		// set Viewpoint 1 to behind Player 1
		// 04/11/1998 - would probably need to do a final rotation (i.e. of the trig. coefficients)
		//			    to allow a viewpoint with e.g. a different X angle to that of the player.
		//				For the car this would mean the following rotations: Y,X,Z, Y,X,Z, X
		//				For the viewpoint this would mean the following rotations: Y,X,Z, X (possibly!)
		CalcYXZTrigCoefficients(player1_x_angle,
								player1_y_angle,
								player1_z_angle);

		// vector from centre of car
		x_offset = 0;
		y_offset = 0xc0;
		z_offset = 0x300;
		WorldOffset(&x_offset, &y_offset, &z_offset);
		viewpoint1_x = (player1_x - x_offset);
		viewpoint1_y = (player1_y - y_offset);
		viewpoint1_z = (player1_z - z_offset);

		viewpoint1_x_angle = player1_x_angle;
		// viewpoint1_x_angle = (player1_x_angle + (48<<6)) & (MAX_ANGLE-1);
		viewpoint1_y_angle = player1_y_angle;
		// viewpoint1_y_angle = (player1_y_angle - (64<<6)) & (MAX_ANGLE-1);
		viewpoint1_z_angle = player1_z_angle;
		// viewpoint1_x_angle = 0;
		// viewpoint1_z_angle = 0;
	}
	else
	{
		viewpoint1_x = player1_x;
		viewpoint1_y = player1_y - (HEIGHT_ABOVE_ROAD << LOG_PRECISION);
		//		viewpoint1_y = player1_y - (90 << LOG_PRECISION);
		viewpoint1_z = player1_z;

		viewpoint1_x_angle = player1_x_angle;
		if (viewpoint1_x_angle >= MAX_ANGLE)
			viewpoint1_x_angle -= MAX_ANGLE;
		viewpoint1_y_angle = player1_y_angle;
		viewpoint1_z_angle = player1_z_angle;
	}
}

//--------------------------------------------------------------------------------------
// Handle updates to the scene
//--------------------------------------------------------------------------------------
static D3DXMATRIX matWorldTrack, matWorldCar, matWorldOpponentsCar;

static void SetCarWorldTransform(void)
{
	D3DXMATRIX matRot, matTemp, matTrans;

	D3DXMatrixIdentity(&matRot);
	float xa = (((float)player1_x_angle * 2 * D3DX_PI) / 65536.0f);
	float ya = (((float)player1_y_angle * 2 * D3DX_PI) / 65536.0f);
	float za = (((float)player1_z_angle * 2 * D3DX_PI) / 65536.0f);
	// Produce and combine the rotation matrices
	D3DXMatrixRotationZ(&matTemp, za);
	D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
	D3DXMatrixRotationX(&matTemp, xa);
	D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
	D3DXMatrixRotationY(&matTemp, ya);
	D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
	// Produce the translation matrix
	// Position car slightly higher than wheel height (VCAR_HEIGHT/4) so wheels are fully visible
	D3DXMatrixTranslation(&matTrans, (float)(player1_x >> LOG_PRECISION), (float)(-player1_y >> LOG_PRECISION) + VCAR_HEIGHT / 3, (float)(player1_z >> LOG_PRECISION));
	// Combine the rotation and translation matrices to complete the world matrix
	D3DXMatrixMultiply(&matWorldCar, &matRot, &matTrans);
}

static void SetOpponentsCarWorldTransform(void)
{
	D3DXMATRIX matRot, matTemp, matTrans;

	D3DXMatrixIdentity(&matRot);
	//	float xa = (((float)opponent_x_angle * 2 * D3DX_PI) / 65536.0f);
	//	float ya = (((float)opponent_y_angle * 2 * D3DX_PI) / 65536.0f);
	//	float za = (((float)opponent_z_angle * 2 * D3DX_PI) / 65536.0f);
	// Produce and combine the rotation matrices
	D3DXMatrixRotationZ(&matTemp, opponent_z_angle);
	D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
	D3DXMatrixRotationX(&matTemp, opponent_x_angle);
	D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
	D3DXMatrixRotationY(&matTemp, opponent_y_angle);
	D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
	// Produce the translation matrix
	// Position car at wheel height (VCAR_HEIGHT/4)
	D3DXMatrixTranslation(&matTrans, (float)(opponent_x >> LOG_PRECISION), (float)(-opponent_y >> LOG_PRECISION) + VCAR_HEIGHT / 4, (float)(opponent_z >> LOG_PRECISION));
	// Combine the rotation and translation matrices to complete the world matrix
	D3DXMatrixMultiply(&matWorldOpponentsCar, &matRot, &matTrans);
}

static void StopEngineSound(void)
{
	if (engineSoundPlaying)
	{
		for (int i = 0; i < 8; i++)
			EngineSoundBuffers[i]->Stop();

		engineSoundPlaying = FALSE;
	}
}

void CALLBACK OnFrameMove(IDirect3DDevice9 *pd3dDevice, double fTime, float fElapsedTime, void *pUserContext)
{
	static D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	static long frameCount = 0;
	input = lastInput; // take copy of user input
	D3DXMATRIX matRot, matTemp, matTrans, matView;

	bFrameMoved = FALSE;
	//	VALUE3 = frameGap;

	if (GameMode == GAME_OVER)
	{
		StopEngineSound();
		return;
	}

	if (bPaused)
	{
		StopEngineSound();
	}

	if (TrackID == NO_TRACK)
		return;

	// Track preview and game mode run at reduced frame rate
	if ((GameMode == TRACK_PREVIEW) || (GameMode == GAME_IN_PROGRESS))
	{
		if (GameMode == GAME_IN_PROGRESS)
		{
			// Following function should run at 50Hz
			if (!bPaused)
				FramesWheelsEngine(EngineSoundBuffers);
		}

		if (frameCount > 0)
			--frameCount;

		if (frameCount == 0)
		{
			frameCount = frameGap;
			// DXUTPause( false, false );	//pausing doesn't work properly
		}
		else
		{
			// if (frameCount == frameGap-1) DXUTPause( true, true );	//pausing doesn't work properly
			return;
		}
	}
	else if (GameMode == TRACK_MENU)
	{
		// Stop engine sound if at track menu or if game has finished
		StopEngineSound();
	}

	if ((GameMode == GAME_IN_PROGRESS) && (keyPress == 'R'))
	{
		// point car in opposite direction
		player1_y_angle += _180_DEGREES;
		player1_y_angle &= (MAX_ANGLE - 1);
		INITIALISE_PLAYER = TRUE;
		keyPress = '\0';
	}

	if (!bPaused)
		MoveDrawBridge();

	// Car behaviour
	if ((GameMode == TRACK_PREVIEW) || (GameMode == GAME_IN_PROGRESS))
	{
		if (!bPaused)
		{
			if ((GameMode == GAME_IN_PROGRESS) && (!bPlayerPaused))
				CarBehaviour(input,
							 &player1_x,
							 &player1_y,
							 &player1_z,
							 &player1_x_angle,
							 &player1_y_angle,
							 &player1_z_angle);

			OpponentBehaviour(&opponent_x,
							  &opponent_y,
							  &opponent_z,
							  &opponent_x_angle,
							  &opponent_y_angle,
							  &opponent_z_angle,
							  bOpponentPaused);
		}

		LimitViewpointY(&player1_y);
	}

	if ((GameMode == TRACK_MENU) || (GameMode == TRACK_PREVIEW))
	{
		if (GameMode == TRACK_MENU)
			CalcTrackMenuViewpoint();
		else
		{
			CalcTrackPreviewViewpoint();

			// Set the car's world transform matrix
			SetOpponentsCarWorldTransform();
		}

		// Set Direct3D transforms, ready for OnFrameRender
		viewpoint1_x >>= LOG_PRECISION;
		// NOTE: viewpoint1_y must be preserved for use by DrawBackdrop
		viewpoint1_z >>= LOG_PRECISION;

		target_x >>= LOG_PRECISION;
		target_y = -target_y;
		target_y >>= LOG_PRECISION;
		target_z >>= LOG_PRECISION;

		// Set the track's world transform matrix
		D3DXMatrixIdentity(&matWorldTrack);

		//
		// Set the view transform matrix
		//
		// Set the eye point
		D3DXVECTOR3 vEyePt((float)viewpoint1_x, (float)(-viewpoint1_y >> LOG_PRECISION), (float)viewpoint1_z);
		// Set the lookat point
		D3DXVECTOR3 vLookatPt((float)target_x, (float)target_y, (float)target_z);
		D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
		pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
	}
	else if (GameMode == GAME_IN_PROGRESS)
	{
		CalcGameViewpoint();

		// Set Direct3D transforms, ready for OnFrameRender
		viewpoint1_x >>= LOG_PRECISION;
		// NOTE: viewpoint1_y must be preserved for use by DrawBackdrop
		viewpoint1_z >>= LOG_PRECISION;

		// Set the track's world transform matrix
		D3DXMatrixIdentity(&matWorldTrack);

		// Set the opponent's car world transform matrix
		/*
		// temp set opponent's position to same as player
		if ((opponent_x == 0) && (opponent_y == 0) && (opponent_z == 0))
		{
			opponent_x = player1_x;
			opponent_y = player1_y + (0xc00 * 256 * 4);	// Subtract amount above road, added by PositionCarAbovePiece()
			opponent_z = player1_z;
			opponent_x_angle = player1_x_angle;
			opponent_y_angle = player1_y_angle;
			opponent_z_angle = player1_z_angle;
		}
		*/
		SetOpponentsCarWorldTransform();

		if (bOutsideView)
		{
			// Set the car's world transform matrix
			SetCarWorldTransform();
		}

		//
		// Set the view transform matrix
		//
		// Produce the translation matrix
		D3DXMatrixTranslation(&matTrans, (float)-viewpoint1_x, (float)(viewpoint1_y >> LOG_PRECISION), (float)-viewpoint1_z);
		D3DXMatrixIdentity(&matRot);
		float xa = (((float)-(viewpoint1_x_angle) * 2 * D3DX_PI) / 65536.0f);
		float ya = (((float)-viewpoint1_y_angle * 2 * D3DX_PI) / 65536.0f);
		float za = (((float)-viewpoint1_z_angle * 2 * D3DX_PI) / 65536.0f);
		// Produce and combine the rotation matrices
		D3DXMatrixRotationY(&matTemp, ya + D3DX_PI);
		D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
		D3DXMatrixRotationX(&matTemp, -xa);
		D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
		D3DXMatrixRotationZ(&matTemp, -za);
		D3DXMatrixMultiply(&matRot, &matRot, &matTemp);
		// Combine the rotation and translation matrices to complete the world matrix
		D3DXMatrixMultiply(&matView, &matTrans, &matRot);
		D3DXMatrixScaling(&matTrans, +1, -1, +1);
		D3DXMatrixMultiply(&matView, &matView, &matTrans);
		pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
	}

	if (!bPaused)
		bFrameMoved = TRUE;
}

/*	======================================================================================= */
/*	Function:		HandleTrackMenu															*/
/*																							*/
/*	Description:	Output track menu text													*/
/*	======================================================================================= */
#define FIRSTMENU SDLK_1

static void HandleTrackMenu(CDXUTTextHelper &txtHelper)
{
	long i;
	UINT firstMenuOption, lastMenuOption;
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.SetInsertionPos(8, 10*8-4);
	txtHelper.DrawText(L"Choose track");
	if (gameController != nullptr) {
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
		txtHelper.DrawText(L" -(L1) +(R1)");
	}
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L":");
	txtHelper.SetInsertionPos(16, 11*8);
	for (i = 0, firstMenuOption = FIRSTMENU; i < NUM_TRACKS; i++)
	{
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
		txtHelper.DrawFormattedText(L"%d ", i + 1);
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
		txtHelper.DrawTextLine(GetTrackName(i));
	}
	lastMenuOption = i + FIRSTMENU - 1;

	txtHelper.SetInsertionPos(8, 20*8-4);
	txtHelper.DrawText(L"Current track: ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawFormattedTextLine(L"" STRING, (TrackID == NO_TRACK ? L"None" : GetTrackName(TrackID)));
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L"Press ");
	if (gameController != nullptr) {
		txtHelper.SetForegroundColor(D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));
		txtHelper.DrawText(L"(A)");
	} else {
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
		txtHelper.DrawText(L"S");
	}
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" to select");
	
	txtHelper.SetInsertionPos(8, 23*8);
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"Esc");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" to quit");

	if ( ((keyPress >= firstMenuOption) && (keyPress <= lastMenuOption)) )
	{
		track_number = keyPress - firstMenuOption; // start at 0

		if (!ConvertAmigaTrack(track_number))
		{
			MessageBox(NULL, "Failed to convert track", "Error", MB_OK); // temp
			return;
		}

		if (CreateTrackVertexBuffer(DXUTGetD3DDevice()) != S_OK)
		{
			MessageBox(NULL, "Failed to create track vertex buffer", "Error", MB_OK); // temp
			return;
		}

		keyPress = '\0';
	}

	if ( ((keyPress == SDLK_s)||(keyPress == SDLK_SPACE)) && (TrackID != NO_TRACK) )
	{
		keyPress = 0;
		bNewGame = TRUE; // Used here just to reset the opponent's car, which is then shown during the track preview
		GameMode = TRACK_PREVIEW;
		bPlayerPaused = bOpponentPaused = FALSE;
		keyPress = '\0';
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 32, fAspect*2, 0.5f, FURTHEST_Z);
		pd3dDevice.SetTransform(D3DTS_PROJECTION, &matProj);
	}

	return;
}

/*	======================================================================================= */
/*	Function:		HandleTrackPreview														*/
/*																							*/
/*	Description:	Output track preview text												*/
/*	======================================================================================= */

static void HandleTrackPreview(CDXUTTextHelper &txtHelper)
{
	// output instructions
	txtHelper.SetInsertionPos(8, 15*8);
	txtHelper.DrawFormattedText(L"Selected track: ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawFormattedTextLine(L"" STRING, (TrackID == NO_TRACK ? L"None" : GetTrackName(TrackID)));
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L"Press ");
	if (gameController != nullptr) {
		txtHelper.SetForegroundColor(D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));
		txtHelper.DrawText(L"(A)");
	} else {
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
		txtHelper.DrawText(L"S");
	}
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" to start game");

	txtHelper.SetInsertionPos(8, 18*8);
	txtHelper.DrawTextLine(L"Keyboard controls during game:");
#if defined(PANDORA) || defined(PYRA)
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"DPad");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" Steer, ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"(X)");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" Accel, ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"(B)");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" Brake");
#else
	txtHelper.SetInsertionPos(16, 19*8);
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"Arrow keys");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" Steer Accel Brake");
#endif
	if (gameController != nullptr) {
		txtHelper.SetForegroundColor(D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));
		txtHelper.DrawText(L"(A)");
	} else {
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
		txtHelper.DrawText(L"Space");
	}
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" Boost, ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"P");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" Pause, ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"O");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" Unpause");

	txtHelper.SetInsertionPos(8, 22*8);
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"F4");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" change skyline, ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"F9/F10");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" up/down fps");

	if (gameController != nullptr) {
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 0.5f, 0.0f, 1.0f));
		txtHelper.DrawText(L"(B)");
	} else {
		txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
		txtHelper.DrawText(L"M");
	}
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawText(L" return to track menu, ");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	txtHelper.DrawText(L"Esc");
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	txtHelper.DrawTextLine(L" to quit");

	if ((keyPress == SDLK_s)||(keyPress == SDLK_SPACE))
	{
		keyPress = 0;
		bNewGame = TRUE;
		GameMode = GAME_IN_PROGRESS;
		// initialise game data
		ResetLapData(OPPONENT);
		ResetLapData(PLAYER);
		gameStartTime = DXUTGetTime();
		gameEndTime = 0;
		boostReserve = StandardBoost;
		road_cushion_value = 0;
		engine_power = 240;
		boost_unit_value = 16;
		opp_engine_power = 236;
		boostUnit = 0;
		bPlayerPaused = bOpponentPaused = FALSE;
		keyPress = '\0';
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 8, fAspect, 0.5f, FURTHEST_Z);
		pd3dDevice.SetTransform(D3DTS_PROJECTION, &matProj);
	}

	return;
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for
// efficient text rendering.  Also render text specific to GameMode.
//--------------------------------------------------------------------------------------
extern long opponentsID;
extern WCHAR *opponentNames[];

GLuint g_pSprite = 0;
/**************************************
/ RenderText
***************************************/
void RenderText(unsigned int FPS)
{
	static CDXUTTextHelper txtHelper(g_pFont, g_pSprite, 8);
	txtHelper.Begin();
	txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));

	// Output statistics
	if (bShowStats)
	{
		const D3DSURFACE_DESC *pd3dsdBackBuffer = DXUTGetBackBufferSurfaceDesc();
		txtHelper.SetInsertionPos(0, 0);
		txtHelper.DrawFormattedTextLine(L"%3ifps %dx%d vsync:%d", FPS, pd3dsdBackBuffer->Width, pd3dsdBackBuffer->Height, SDL_GL_GetSwapInterval());
		// Output gamecontroller if connected
		if (gameController != nullptr)
			txtHelper.DrawFormattedTextLine(L"Gamepad: %s", gameController);
	}

	switch (GameMode)
	{
	case TRACK_MENU:
		HandleTrackMenu(txtHelper);
		txtHelper.End();
		break;

	case TRACK_PREVIEW:
		HandleTrackPreview(txtHelper);
		txtHelper.End();
		break;

	case GAME_IN_PROGRESS:
	case GAME_OVER:

		// Output lap number and boost reserve
		WCHAR lapText[2] = L" ";
		WCHAR lapBoost[3] = L"  ";
		if (lapNumber[PLAYER] > 0)
			StringCchPrintf(lapText, 2, L"%ld", lapNumber[PLAYER]);
		if (touching_road || lapNumber[PLAYER] > 0)
			StringCchPrintf(lapBoost, 3, L"%ld", boostReserve);
		txtHelper.SetForegroundColor(D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f)); // black
		txtHelper.SetInsertionPos(38, 178);
		txtHelper.DrawFormattedTextLine(L"L" STRING, lapText);
		txtHelper.SetInsertionPos(60, 178);
		txtHelper.DrawFormattedTextLine(L"B" STRING, lapBoost);

		// Output opponent distance
		txtHelper.SetInsertionPos(44, 188);
		long opponentsDistance = CalculateOpponentsDistance();
		if (opponentsDistance < 0)
			txtHelper.DrawFormattedTextLine(L"%+05d", opponentsDistance);
		else
			txtHelper.DrawFormattedTextLine(L" %04d", opponentsDistance);

		// Output PAUSED
		if (bPaused)
		{
			txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f)); // white
			txtHelper.SetInsertionPos(136, 32);
			txtHelper.DrawTextLine(L"PAUSED");
			break;
		}

		// Output opponent's name for four seconds at race start
		if (((DXUTGetTime() - gameStartTime) < 4.0) && (opponentsID != NO_OPPONENT))
		{
			txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f)); // white
			txtHelper.SetInsertionPos(124, 32);
			txtHelper.DrawTextLine(L"Opponent:");
			txtHelper.SetInsertionPos(112, 40 + 2);
			txtHelper.DrawTextLine(opponentNames[opponentsID]);
		}

		if (raceFinished)
		{
			double currentTime = DXUTGetTime(), diffTime;
			if (gameEndTime == 0.0)
				gameEndTime = currentTime;

			// Show race finished text for six seconds, then end the game
			diffTime = currentTime - gameEndTime;
			if (diffTime > 6.0)
			{
				GameMode = GAME_OVER;
			}

			if (GameMode == GAME_OVER)
			{
				txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f)); // white
				txtHelper.SetInsertionPos(140, 32);
				txtHelper.DrawTextLine(L"PRESS");
				txtHelper.SetInsertionPos(144, 40 + 2);
				txtHelper.DrawTextLine(L"FIRE");
			}
			else
			{
				long intTime = (long)diffTime;
				// Text flashes white, changing every half second
				if ((diffTime - (double)intTime) < 0.5)
				{
					txtHelper.SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f)); // white
					txtHelper.SetInsertionPos(144, 32);
					if (raceWon)
					{
						txtHelper.DrawTextLine(L"RACE");
						txtHelper.SetInsertionPos(148, 40 + 2);
						txtHelper.DrawTextLine(L"WON");
					}
					else
					{
						txtHelper.DrawTextLine(L"RACE"); // 4
						txtHelper.SetInsertionPos(144, 40 + 2);
						txtHelper.DrawTextLine(L"LOST"); // 4
					}
				}
			}
		}

		txtHelper.End();
		break;
	}
	//	VALUE2 = raceFinished ? 1 : 0;
	//	VALUE3 = (long)gameEndTime;
}

//--------------------------------------------------------------------------------------
// Render the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender(IDirect3DDevice9 *pd3dDevice, double fTime, float fElapsedTime, void *pUserContext, unsigned int FPS)
{
	//    // Clear the render target and the zbuffer
	//    V( pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0) );

	// Clear the zbuffer
	V(pd3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0));

	// Render the scene
	if (SUCCEEDED(pd3dDevice->BeginScene()))
	{
		// Disable Z buffer and polygon culling, ready for DrawBackdrop()
		pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		switch (GameMode)
		{
		case TRACK_MENU:
			// Clear Backdrop
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport( viewX*3 , viewY/2, viewW, viewH );
				// Draw Track
				pd3dDevice->SetTransform(D3DTS_WORLD, &matWorldTrack);
				DrawTrack(pd3dDevice);
			glViewport(screenX, screenY, screenW, screenH);
			// Draw Title
			DrawTitle(pd3dDevice);
			break;

		case TRACK_PREVIEW:
			// Clear Backdrop
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(screenX, screenY + (screenH/2), screenW, screenH/2);
				// Draw Horizon/Scenery
				DrawBackdrop(viewpoint1_y, viewpoint1_x_angle, viewpoint1_y_angle, viewpoint1_z_angle);
				// Draw Track
				pd3dDevice->SetTransform(D3DTS_WORLD, &matWorldTrack);
				DrawTrack(pd3dDevice);
				// Draw Opponent's Car
				pd3dDevice->SetTransform(D3DTS_WORLD, &matWorldOpponentsCar);
				DrawCar(pd3dDevice);
			glViewport(screenX, screenY, screenW, screenH);
			break;

		case GAME_IN_PROGRESS:
		case GAME_OVER:
			glViewport( viewX , viewY, viewW, viewH );
				// Draw Horizon/Scenery
				DrawBackdrop(viewpoint1_y, viewpoint1_x_angle, viewpoint1_y_angle, viewpoint1_z_angle);
				// Draw Track
				pd3dDevice->SetTransform(D3DTS_WORLD, &matWorldTrack);
				DrawTrack(pd3dDevice);
				// Draw Opponent's Car
				pd3dDevice->SetTransform(D3DTS_WORLD, &matWorldOpponentsCar);
				DrawCar(pd3dDevice);
			glViewport(screenX, screenY, screenW, screenH);

			if (bOutsideView)
			{
				// Draw Player1's Car
				pd3dDevice->SetTransform(D3DTS_WORLD, &matWorldCar);
				DrawCar(pd3dDevice);
			}
			else
			{
				// draw cockpit...
				DrawCockpit(pd3dDevice);
			}
			break;
		}

		if (GameMode == GAME_IN_PROGRESS)
		{
			DrawOtherGraphics();

			// jsr	display.speed.bar
			if (bFrameMoved)
				UpdateDamage();

			UpdateLapData();
			// jsr	display.opponents.distance
		}

		RenderText(FPS);

		// End the scene
		pd3dDevice->EndScene();
	}
}

void switch2track_menu()
{
	keyPress = input = lastInput = 0;
	if (GameMode != TRACK_MENU)
	{
		GameMode = TRACK_MENU;
		opponentsID = NO_OPPONENT;
		bPaused = FALSE;
		// reset all animated objects
		ResetDrawBridge();
	}
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 3, fAspect, 0.5f, FURTHEST_Z);
	pd3dDevice.SetTransform(D3DTS_PROJECTION, &matProj);
}

bool process_events()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{

		// open/close pluged devices
		case SDL_CONTROLLERDEVICEADDED:
			SDL_GameControllerOpen(0);
			gameController = SDL_GameControllerName(SDL_GameControllerOpen(0));
			cout << "Controller added: " << gameController << endl;
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			gameController = nullptr;
			SDL_GameControllerClose(0);
			cout << "Controller removed" << endl;
			break;

		// gamepad/joystick
		case SDL_CONTROLLERBUTTONUP:
			switch (event.cbutton.button)
			{
			case SDL_CONTROLLER_BUTTON_A:
				lastInput &= ~KEY_P1_BOOST;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				break;
			case SDL_CONTROLLER_BUTTON_X:
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				break;
			}
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			switch (event.cbutton.button)
			{
			case SDL_CONTROLLER_BUTTON_A:
				if (GameMode == GAME_OVER) {
					switch2track_menu();
					break;
				}
				lastInput |= KEY_P1_BOOST;
				keyPress = SDLK_s;
				break;
			case SDL_CONTROLLER_BUTTON_B:
			    switch2track_menu();
				break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				if (track_number != 0) {
					keyPress = track_number + FIRSTMENU - 1;
				}
				break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				if (track_number != NUM_TRACKS-1) {
					keyPress = track_number + FIRSTMENU + 1;
				}
				break;
			}
			break;
		case SDL_CONTROLLERAXISMOTION:
			if (event.caxis.axis == 1)
			{
				if (event.caxis.value < -1000)
					lastInput |= KEY_P1_ACCEL;
				else if (event.caxis.value > 1000)
					lastInput |= KEY_P1_BRAKE;
				else
				{
					lastInput &= ~KEY_P1_ACCEL;
					lastInput &= ~KEY_P1_BRAKE;
				}
			}
			else if (event.caxis.axis == 0)
			{
				if (event.caxis.value < -1000)
					lastInput |= KEY_P1_LEFT;
				else if (event.caxis.value > 1000)
					lastInput |= KEY_P1_RIGHT;
				else
				{
					lastInput &= ~KEY_P1_LEFT;
					lastInput &= ~KEY_P1_RIGHT;
				}
			}
			break;

		case SDL_KEYDOWN:
			keyPress = event.key.keysym.sym;
			// some special cases for French keyboards
			if ((event.key.keysym.mod & KMOD_SHIFT) == 0)
				switch (event.key.keysym.sym)
				{
				case SDLK_AMPERSAND:
					keyPress = SDLK_1;
					break;
				case 233:
					keyPress = SDLK_2;
					break;
				case SDLK_QUOTEDBL:
					keyPress = SDLK_3;
					break;
				case SDLK_QUOTE:
					keyPress = SDLK_4;
					break;
				case SDLK_LEFTPAREN:
					keyPress = SDLK_5;
					break;
				case SDLK_MINUS:
					keyPress = SDLK_6;
					break;
				case 232:
					keyPress = SDLK_7;
					break;
				case SDLK_UNDERSCORE:
					keyPress = SDLK_8;
					break;
				case 231:
					keyPress = SDLK_9;
					break;
				case 224:
					keyPress = SDLK_0;
					break;
				}

			switch (keyPress)
			{
			case SDLK_F2:
				++bTrackDrawMode;
				if (bTrackDrawMode > 1)
					bTrackDrawMode = 0;
				DXUTReset3DEnvironment();
				break;

			case SDLK_F4:
				NextSceneryType();
				break;

			case SDLK_F5:
				bShowStats = !bShowStats;
				break;

			case SDLK_F6:
				bPlayerPaused = !bPlayerPaused;
				break;

			case SDLK_F7:
				bOpponentPaused = !bOpponentPaused;
				break;

			case SDLK_F9:
				if (frameGap > 1)
					frameGap--;
				break;

			case SDLK_F10:
				frameGap++;
				break;

			case SDLK_m:
				switch2track_menu();
				break;

			case SDLK_o:
				bPaused = FALSE;
				break;

			case SDLK_p:
				if (!raceFinished && !raceWon && GameMode == GAME_IN_PROGRESS)
					bPaused = TRUE;
				break;

			// controls for Car Behaviour, Player 1
			case SDLK_LEFT:
				lastInput |= KEY_P1_LEFT;
				break;

			case SDLK_RIGHT:
				lastInput |= KEY_P1_RIGHT;
				break;

#if defined(PANDORA) || defined(PYRA)
			case SDLK_RCTRL:
#else
			case SDLK_SPACE:
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
#endif
				if (GameMode == GAME_OVER) {
					switch2track_menu();
					break;
				}
				lastInput |= KEY_P1_BOOST;
				break;

#if defined(PANDORA) || defined(PYRA)
			case SDLK_END:
#else
			case SDLK_DOWN:
#endif
				lastInput |= KEY_P1_BRAKE;
				break;

#if defined(PANDORA) || defined(PYRA)
			case SDLK_PAGEDOWN:
#else
			case SDLK_UP:
#endif
				lastInput |= KEY_P1_ACCEL;
				break;

			case SDLK_ESCAPE:
				return false;
			}
			break;

		case SDL_KEYUP:
			keyPress = 0;
			switch (event.key.keysym.sym)
			{
			// controls for Car Behaviour, Player 1
			case SDLK_LEFT:
				lastInput &= ~KEY_P1_LEFT;
				break;

			case SDLK_RIGHT:
				lastInput &= ~KEY_P1_RIGHT;
				break;

#if defined(PANDORA) || defined(PYRA)
			case SDLK_RCTRL:
#else
			case SDLK_SPACE:
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
#endif
				lastInput &= ~KEY_P1_BOOST;
				break;

#if defined(PANDORA) || defined(PYRA)
			case SDLK_END:
#else
			case SDLK_DOWN:
#endif
				lastInput &= ~KEY_P1_BRAKE;
				break;

#if defined(PANDORA) || defined(PYRA)
			case SDLK_PAGEDOWN:
#else
			case SDLK_UP:
#endif
				lastInput &= ~KEY_P1_ACCEL;
				break;
			}
			break;
		case SDL_QUIT:
			return false;
		}
	}
	return true;
}

bool GL_MSAA = FALSE;
int main(int argc, const char **argv)
{
	char maintitle[50] = {0};
	sprintf(maintitle, "KaskadeurRennfahrer v%d.%02d.%02d", V_MAJOR, V_MINOR, V_PATCH);
	printf("%s\n", maintitle);
	/*	// get executable folder and cd into it...
		// this is linux only, will not work on BSD or macOS
		char buf[500];
		ssize_t bufsized = readlink("/proc/self/exe", buf, sizeof(buf));
		if (bufsized > 0)
		{
			char *p = strrchr(buf, '/');
			if (*p)
			{
				*p = 0;
				chdir(buf);
				printf("chdir(\"%s\")\n", buf);
			}
		}
	*/

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		cout << "SDL_Init Error: " << SDL_GetError() << endl;
		exit(-1);
	}
	atexit(SDL_Quit);

	if (TTF_Init() != 0)
	{
		cout << "TTF_Init Error: " << TTF_GetError() << endl;
		exit(1);
	}

	// crude command line parameter reading
	int fullscreen = 0;
	int desktop = 0;
	int givehelp = 0;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-f"))
			fullscreen = 1;
		else if (!strcmp(argv[i], "--fullscreen"))
			fullscreen = 1;
		else if (!strcmp(argv[i], "-d"))
			desktop = 1;
		else if (!strcmp(argv[i], "--desktop"))
			desktop = 1;
		else if (!strcmp(argv[i], "-m"))
			GL_MSAA = TRUE;
		else if (!strcmp(argv[i], "--msaa"))
			GL_MSAA = TRUE;
		else
			givehelp = 1;
	}
	if (givehelp)
	{
		printf("parameters:\n -f | --fullscreen\n -d | --desktop\n -m | --msaa\n");
		exit(0);
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	SDL_GL_SetSwapInterval(1); //async-1, vsync 1, nosync 0

#if PANDORA
	int revision = 5;
	FILE *f = fopen("/etc/powervr-esrev", "r");
	if (f)
	{
		fscanf(f, "%d", &revision);
		fclose(f);
		printf("Pandora Model detected = %d\n", revision);
	}
	if (revision == 5 && GL_MSAA)
	{
		// only do MSAA for Gigahertz model
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
	}
#else
	if (GL_MSAA)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
#endif

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef PANDORA
	flags |= SDL_WINDOW_FULLSCREEN;
	screenW = 800;
	screenH = 480;
#elif CHIP
	flags |= SDL_WINDOW_FULLSCREEN;
	screenW = 480;
	screenH = 272;
#elif RPI
	//flags |= SDL_WINDOW_FULLSCREEN;
	screenW = 720;
	screenH = 480;
	cout << "versuche 720x480 framebuffer console" << endl;
#else
	if (desktop || fullscreen)
	{
		if (desktop)
		{
			screenW = 640;
			screenH = 400;
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			cout << "versuche desktop fullscreen" << endl;
		}
		else
		{
			screenW = 640;
			screenH = 400;
			flags |= SDL_WINDOW_FULLSCREEN;
			cout << "versuche 320x240 fullscreen" << endl;
		}
	}
	else
	{
		// double the orig. AtariST resolution, 320x200 -> 640x400 -> 1280x800
		screenW = 1280;
		screenH = 800;
		cout << "versuche 1280x800 desktop window" << endl;
	}
#endif
	SDL_Window *window = SDL_CreateWindow(maintitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, flags);
	if (window == nullptr && GL_MSAA)
	{
		cout << "SDL_CreateWindow Error (with GL_MSAA): " << SDL_GetError() << endl;
		// fallback to no MSAA
		GL_MSAA = FALSE;
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		window = SDL_CreateWindow(maintitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, flags);
		if (window == nullptr)
		{
			cout << "SDL_CreateWindow Error (without GL_MSAA): " << SDL_GetError() << endl;
			SDL_Quit();
			exit(-2);
		}
	}

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (context == NULL)
	{
		cout << "Couldn't create OpenGL context: " << SDL_GetError() << endl;
		SDL_Quit();
		exit(-3);
	}
	SDL_GetWindowSize(window, &screenW, &screenH);

	int r, g, b, depth, db, stencil, fsaa_buffer, fsaa_samples;
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &db);
	SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &fsaa_buffer);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &fsaa_samples);
	printf("\nActual SDL Video values: R %d, G %d, B %d, depth %d, double/stencil-buffer %d/%d, FSAA %d/%d\n",
		   r, g, b, depth, db, stencil, fsaa_buffer, fsaa_samples);

	SDL_SetWindowTitle(window, maintitle);

	int x, y, n;
	unsigned char *img = stbi_load("Bitmap/icon.png", &x, &y, &n, STBI_rgb_alpha);
	if (img)
	{
		SDL_Surface *icon = SDL_CreateRGBSurfaceWithFormatFrom(img, x, y, 16, x * 4, SDL_PIXELFORMAT_RGBA32);
		if (icon)
		{
			SDL_SetWindowIcon(window, icon);
			SDL_FreeSurface(icon);
		}
		free(img);
	}

	//orig. ATARI STe 320x200
	// automatic guess the scale
	if ((float)screenW / (float)320 < (float)screenH / (float)200)
		screenScale = (float)screenW / (float)320;
	else
		screenScale = (float)screenH / (float)200;

	cout << "Open Window: " << screenW << "x" << screenH << endl;
	cout << "Screenscale: " << screenScale << endl;

	if (flags & SDL_WINDOW_FULLSCREEN || flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
		SDL_ShowCursor(SDL_DISABLE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//Viewport
	// ATARI STe resolution 320x200
	screenX = (screenW - 320 * screenScale) / 2;
	screenY = (screenH - 200 * screenScale) / 2;
	screenW = 320 * screenScale;
	screenH = 200 * screenScale;

	//Viewport ingame
    // ATARI STe pos x32 y56 w256xh128
	viewX = 32 * screenScale + screenX;
	viewY = 56 * screenScale + screenY;
	viewW = 256 * screenScale;
	viewH = 128 * screenScale;
	
	glViewport(screenX, screenY, screenW, screenH);
	cout << "Viewport:        " << screenX << "-" << screenX+screenW << " " << screenY << "-" << screenY+screenH << endl;
	cout << "Viewport ingame: " << viewX << "-" << viewX+viewW << " " << viewY << "-" << viewY+viewH << endl;

	glOrtho(0, 320, 200, 0, 0, FURTHEST_Z); //ATARI STe Fullscreen 320x200
	cout << "Projection:      320x200" << endl << endl;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glShadeModel(GL_FLAT);

	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.1);
	glAlphaFunc(GL_NOTEQUAL, 0);

	glDisable(GL_LIGHTING);
	// Disable texture mapping by default (only DrawTrack() enables it)
	pd3dDevice.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);

	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 3, fAspect, 0.5f, FURTHEST_Z);
	pd3dDevice.SetTransform(D3DTS_PROJECTION, &matProj);
	sound_init();
	CreateFonts();
	LoadTextures();

	if (!InitialiseData())
	{
		printf("Error initialising data\n");
		exit(-3);
	}

	CreateBuffers(&pd3dDevice);

	DSInit();
	DSSetMode();

	glClearColor(0, 0, 0, 1);

	bool run = true;
	double fTime = 0.0;
	double fLastTime = 0.0;

	double prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;
	unsigned int counter = 0;
	unsigned int FPS = 0;

	while (run)
	{

		fTime = DXUTGetTime();

		// calculate FPS
		crntTime = fTime;
		timeDiff = crntTime - prevTime;
		counter++;
		if (timeDiff >= 1.0)
		{
			FPS = (int)round((1.0 / timeDiff) * counter);
			prevTime = crntTime;
			counter = 0;
		}

		run = process_events();
		OnFrameMove(&pd3dDevice, fTime, fTime - fLastTime, NULL);
		OnFrameRender(&pd3dDevice, fTime, fTime - fLastTime, NULL, FPS);

		SDL_GL_SwapWindow(window);

		int32_t timetowait = (1.0f / 50.0f - (fTime - fLastTime)) * 1000;
		// int32_t timetowait = (1.0f/60.0f - (fTime-fLastTime))*1000;
		if (timetowait > 0)
			SDL_Delay(timetowait);

		fLastTime = fTime;
	}

	FreeData();

	sound_destroy();
	//Quit SDL subsystems
	TTF_Quit();
    SDL_Quit();

	exit(0);
}
