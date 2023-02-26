#ifndef _STUNT_CAR_RACER
#define _STUNT_CAR_RACER

#define SCR_BASE_COLOUR 26

typedef enum
{
	TRACK_MENU = 0,
	TRACK_PREVIEW,
	GAME_IN_PROGRESS,
	GAME_OVER
} GameModeType;

#define D3DFVF_UTVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

extern bool GL_MSAA;
extern bool bPaused;
extern GameModeType GameMode;
extern int screenH, screenW, screenX, screenY;
extern int viewH, viewW, viewX, viewY;
extern float screenScale;

extern uint32_t SCRGB(long colour_index);

extern uint32_t SCColour(long colour_index);

extern void SetSolidColour(long colour_index);

extern void SetLineColour(long colour_index);

extern void SetTextureColour(long colour_index);

#endif /* _STUNT_CAR_RACER */