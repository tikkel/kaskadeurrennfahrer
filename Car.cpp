/**************************************************************************

	Car.cpp - Functions for manipulating car (excluding player's car behaviour)

 **************************************************************************/

#include "dx_linux.h"

#include "Car.h"
#include "Car_Behaviour.h"
#include "StuntCarRacer.h"
#include "3D_Engine.h"
#include "Atlas.h"

#define SCR_BASE_COLOUR 26

#define MAX_VERTICES_PER_CAR (142 * 3)

/*	======================================================================================= */
/*	Function:		DrawCar																	*/
/*																							*/
/*	Description:	Draw the car using the supplied viewpoint								*/
/*	======================================================================================= */
static IDirect3DVertexBuffer9 *pCarVB = NULL;
static long numCarVertices = 0;

static void StoreCarTriangle(COORD_3D *c1, COORD_3D *c2, COORD_3D *c3, UTVERTEX *pVertices, DWORD colour)
{
	D3DXVECTOR3 v1, v2, v3; //, edge1, edge2, surface_normal;

	if ((numCarVertices + 3) > MAX_VERTICES_PER_CAR)
	{
		MessageBox(NULL, "Exceeded numCarVertices", "StoreCarTriangle", MB_OK);
		return;
	}

	v1 = D3DXVECTOR3((float)c1->x, (float)c1->y, (float)c1->z);
	v2 = D3DXVECTOR3((float)c2->x, (float)c2->y, (float)c2->z);
	v3 = D3DXVECTOR3((float)c3->x, (float)c3->y, (float)c3->z);

	/*
	// Calculate surface normal
	edge1 = v2-v1; edge2 = v3-v2;
	D3DXVec3Cross( &surface_normal, &edge1, &edge2 );
	D3DXVec3Normalize( &surface_normal, &surface_normal );
	*/

	pVertices[numCarVertices].pos = v1;
	//	pVertices[numCarVertices].normal = surface_normal;
	pVertices[numCarVertices].color = colour;
	++numCarVertices;

	pVertices[numCarVertices].pos = v2;
	//	pVertices[numCarVertices].normal = surface_normal;
	pVertices[numCarVertices].color = colour;
	++numCarVertices;

	pVertices[numCarVertices].pos = v3;
	//	pVertices[numCarVertices].normal = surface_normal;
	pVertices[numCarVertices].color = colour;
	++numCarVertices;
}

static void CreateCarInVB(UTVERTEX *pVertices)
{
	// car co-ordinates
	static COORD_3D car[16 + 8] = {
		// x,				y,					z
		{-VCAR_WIDTH / 2, -VCAR_HEIGHT / 4, -VCAR_LENGTH / 2}, // rear left wheel
		{-VCAR_WIDTH / 2, 0, -VCAR_LENGTH / 2},
		{-VCAR_WIDTH / 4, 0, -VCAR_LENGTH / 2},
		{-VCAR_WIDTH / 4, -VCAR_HEIGHT / 4, -VCAR_LENGTH / 2},

		{VCAR_WIDTH / 4, -VCAR_HEIGHT / 4, -VCAR_LENGTH / 2}, // rear right wheel
		{VCAR_WIDTH / 4, 0, -VCAR_LENGTH / 2},
		{VCAR_WIDTH / 2, 0, -VCAR_LENGTH / 2},
		{VCAR_WIDTH / 2, -VCAR_HEIGHT / 4, -VCAR_LENGTH / 2},

		{-VCAR_WIDTH / 2, -VCAR_HEIGHT / 4, VCAR_LENGTH / 2}, // front left wheel
		{-VCAR_WIDTH / 2, 0, VCAR_LENGTH / 2},
		{-VCAR_WIDTH / 4, 0, VCAR_LENGTH / 2},
		{-VCAR_WIDTH / 4, -VCAR_HEIGHT / 4, VCAR_LENGTH / 2},

		{VCAR_WIDTH / 4, -VCAR_HEIGHT / 4, VCAR_LENGTH / 2}, // front right wheel
		{VCAR_WIDTH / 4, 0, VCAR_LENGTH / 2},
		{VCAR_WIDTH / 2, 0, VCAR_LENGTH / 2},
		{VCAR_WIDTH / 2, -VCAR_HEIGHT / 4, VCAR_LENGTH / 2},

		{-VCAR_WIDTH / 4, -VCAR_HEIGHT / 8, -VCAR_LENGTH / 2}, // car rear points
		{-(3 * VCAR_WIDTH) / 16, VCAR_HEIGHT / 4, -VCAR_LENGTH / 2},
		{(3 * VCAR_WIDTH) / 16, VCAR_HEIGHT / 4, -VCAR_LENGTH / 2},
		{VCAR_WIDTH / 4, -VCAR_HEIGHT / 8, -VCAR_LENGTH / 2},

		{-VCAR_WIDTH / 4, -VCAR_HEIGHT / 8, VCAR_LENGTH / 2}, // car front points
		{-VCAR_WIDTH / 4, 0, VCAR_LENGTH / 2},
		{VCAR_WIDTH / 4, 0, VCAR_LENGTH / 2},
		{VCAR_WIDTH / 4, -VCAR_HEIGHT / 8, VCAR_LENGTH / 2}};

	// rear left wheel
	DWORD colour = SCRGB(SCR_BASE_COLOUR + 0);
	/**/
#define vertices pVertices
	// viewing from back
	StoreCarTriangle(&car[0], &car[1], &car[2], vertices, colour);
	StoreCarTriangle(&car[0], &car[2], &car[3], vertices, colour);
	// viewing from front
	StoreCarTriangle(&car[3], &car[2], &car[1], vertices, colour);
	StoreCarTriangle(&car[3], &car[1], &car[0], vertices, colour);

	// rear right wheel
	// viewing from back
	StoreCarTriangle(&car[0 + 4], &car[1 + 4], &car[2 + 4], vertices, colour);
	StoreCarTriangle(&car[0 + 4], &car[2 + 4], &car[3 + 4], vertices, colour);
	// viewing from front
	StoreCarTriangle(&car[3 + 4], &car[2 + 4], &car[1 + 4], vertices, colour);
	StoreCarTriangle(&car[3 + 4], &car[1 + 4], &car[0 + 4], vertices, colour);
	/**/
	/**/
	// front left wheel
	// viewing from back
	StoreCarTriangle(&car[0 + 8], &car[1 + 8], &car[2 + 8], vertices, colour);
	StoreCarTriangle(&car[0 + 8], &car[2 + 8], &car[3 + 8], vertices, colour);
	// viewing from front
	StoreCarTriangle(&car[3 + 8], &car[2 + 8], &car[1 + 8], vertices, colour);
	StoreCarTriangle(&car[3 + 8], &car[1 + 8], &car[0 + 8], vertices, colour);

	// front right wheel
	// viewing from back
	StoreCarTriangle(&car[0 + 12], &car[1 + 12], &car[2 + 12], vertices, colour);
	StoreCarTriangle(&car[0 + 12], &car[2 + 12], &car[3 + 12], vertices, colour);
	// viewing from front
	StoreCarTriangle(&car[3 + 12], &car[2 + 12], &car[1 + 12], vertices, colour);
	StoreCarTriangle(&car[3 + 12], &car[1 + 12], &car[0 + 12], vertices, colour);
	/**/

	// car left side
	colour = SCRGB(SCR_BASE_COLOUR + 12);
	StoreCarTriangle(&car[4 + 16], &car[5 + 16], &car[1 + 16], vertices, colour);
	StoreCarTriangle(&car[4 + 16], &car[1 + 16], &car[0 + 16], vertices, colour);
	// car right side
	StoreCarTriangle(&car[3 + 16], &car[2 + 16], &car[6 + 16], vertices, colour);
	StoreCarTriangle(&car[3 + 16], &car[6 + 16], &car[7 + 16], vertices, colour);

	// car back
	colour = SCRGB(SCR_BASE_COLOUR + 10);
	StoreCarTriangle(&car[0 + 16], &car[1 + 16], &car[2 + 16], vertices, colour);
	StoreCarTriangle(&car[0 + 16], &car[2 + 16], &car[3 + 16], vertices, colour);
	// car front
	StoreCarTriangle(&car[7 + 16], &car[6 + 16], &car[5 + 16], vertices, colour);
	StoreCarTriangle(&car[7 + 16], &car[5 + 16], &car[4 + 16], vertices, colour);

	// car top
	colour = SCRGB(SCR_BASE_COLOUR + 15);
	StoreCarTriangle(&car[1 + 16], &car[5 + 16], &car[6 + 16], vertices, colour);
	StoreCarTriangle(&car[1 + 16], &car[6 + 16], &car[2 + 16], vertices, colour);
	// car bottom
	colour = SCRGB(SCR_BASE_COLOUR + 9);
	StoreCarTriangle(&car[3 + 16], &car[7 + 16], &car[4 + 16], vertices, colour);
	StoreCarTriangle(&car[3 + 16], &car[4 + 16], &car[0 + 16], vertices, colour);
#undef vertices
}

HRESULT CreateCarVertexBuffer(IDirect3DDevice9 *pd3dDevice)
{
	if (pCarVB == NULL)
	{
		if (FAILED(pd3dDevice->CreateVertexBuffer(MAX_VERTICES_PER_CAR * sizeof(UTVERTEX),
												  D3DUSAGE_WRITEONLY, D3DFVF_UTVERTEX, D3DPOOL_DEFAULT, &pCarVB, NULL)))
			return E_FAIL;
	}

	UTVERTEX *pVertices;
	if (FAILED(pCarVB->Lock(0, 0, (void **)&pVertices, 0)))
		return E_FAIL;
	numCarVertices = 0;
	CreateCarInVB(pVertices);
	pCarVB->Unlock();
	return S_OK;
}

void FreeCarVertexBuffer(void)
{
	if (pCarVB)
		pCarVB->Release(), pCarVB = NULL;
}

void DrawCar(IDirect3DDevice9 *pd3dDevice)
{
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	pd3dDevice->SetStreamSource(0, pCarVB, 0, sizeof(UTVERTEX));
	pd3dDevice->SetFVF(D3DFVF_UTVERTEX);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, numCarVertices / 3); // 3 points per triangle
}

struct TRANSFORMEDTEXVERTEX
{
	FLOAT x, y, z, rhw; // The transformed position for the vertex.
	FLOAT u, v;			// Texture
};
#define D3DFVF_TRANSFORMEDTEXVERTEX (D3DFVF_XYZRHW | D3DFVF_TEX1)

struct TRANSFORMEDCOLVERTEX
{
	FLOAT x, y, z, rhw; // The transformed position for the vertex.
	DWORD color;		// Color
};
#define D3DFVF_TRANSFORMEDCOLVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

static IDirect3DVertexBuffer9 *pCockpitVB = NULL, *pSpeedBarCB = NULL;
#define MAX_COCKIPTVB 512
static int old_speedbar = -1;
static int old_leftwheel = -1, old_rightwheel = -1;

extern IDirect3DTexture9 *g_pAtlas;
extern long front_left_amount_below_road, front_right_amount_below_road;
extern long leftwheel_angle, rightwheel_angle;
extern long boost_activated;

HRESULT CreateCockpitVertexBuffer(IDirect3DDevice9 *pd3dDevice)
{
	if (pCockpitVB == NULL)
	{
		if (FAILED(pd3dDevice->CreateVertexBuffer(MAX_COCKIPTVB * sizeof(TRANSFORMEDTEXVERTEX),
												  D3DUSAGE_WRITEONLY, D3DFVF_TRANSFORMEDTEXVERTEX, D3DPOOL_DEFAULT, &pCockpitVB, NULL)))
			return E_FAIL;
	}
	if (pSpeedBarCB == NULL)
	{
		if (FAILED(pd3dDevice->CreateVertexBuffer(4 * sizeof(TRANSFORMEDCOLVERTEX),
												  D3DUSAGE_WRITEONLY, D3DFVF_TRANSFORMEDCOLVERTEX, D3DPOOL_DEFAULT, &pSpeedBarCB, NULL)))
			return E_FAIL;
	}
	return S_OK;
}

void FreeCockpitVertexBuffer(void)
{
	if (pCockpitVB)
		pCockpitVB->Release(), pCockpitVB = NULL;
	if (pSpeedBarCB)
		pSpeedBarCB->Release(), pSpeedBarCB = NULL;
	/*if (pLeftwheelVB) pLeftwheelVB->Release(), pLeftwheelVB = NULL;
	if (pRightwheelVB) pRightwheelVB->Release(), pRightwheelVB = NULL;*/
}

extern long CalculateDisplaySpeed(void);

static int cockpit_vtx = 0;
static void AddQuad(TRANSFORMEDTEXVERTEX *pVertices, float x1, float y1, float x2, float y2, float z, int idx, int revX, float w)
{
	float u1 = (revX) ? atlas_tx2[idx] : atlas_tx1[idx], v1 = atlas_ty1[idx];
	float u2 = (revX) ? atlas_tx1[idx] : atlas_tx2[idx], v2 = atlas_ty2[idx];
	if (w != 1.0f)
	{
		u2 = u1 + (u2 - u1) * w;
	}
	pVertices += cockpit_vtx;
	pVertices[0].x = x1;
	pVertices[0].y = y1;
	pVertices[0].z = z;
	pVertices[0].rhw = 1.0f;
	pVertices[1].x = x2;
	pVertices[1].y = y1;
	pVertices[1].z = z;
	pVertices[1].rhw = 1.0f;
	pVertices[2].x = x2;
	pVertices[2].y = y2;
	pVertices[2].z = z;
	pVertices[2].rhw = 1.0f;
	pVertices[0].u = u1;
	pVertices[0].v = v1;
	pVertices[1].u = u2;
	pVertices[1].v = v1;
	pVertices[2].u = u2;
	pVertices[2].v = v2;
	cockpit_vtx += 3;
	pVertices += 3;
	pVertices[0].x = x1;
	pVertices[0].y = y1;
	pVertices[0].z = z;
	pVertices[0].rhw = 1.0f;
	pVertices[1].x = x2;
	pVertices[1].y = y2;
	pVertices[1].z = z;
	pVertices[1].rhw = 1.0f;
	pVertices[2].x = x1;
	pVertices[2].y = y2;
	pVertices[2].z = z;
	pVertices[2].rhw = 1.0f;
	pVertices[0].u = u1;
	pVertices[0].v = v1;
	pVertices[1].u = u2;
	pVertices[1].v = v2;
	pVertices[2].u = u1;
	pVertices[2].v = v2;
	cockpit_vtx += 3;
}

void DrawTitle(IDirect3DDevice9 *pd3dDevice)
{
	if (GL_MSAA)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);

	// Prepare title drawing
	TRANSFORMEDTEXVERTEX *pVertices;
	cockpit_vtx = 0;
	if (FAILED(pCockpitVB->Lock(0, 0, (void **)&pVertices, 0)))
		return;

	AddQuad(pVertices, 62, 4, 62 + 197, 4 + 60, 0.90, eTitle, 0, 1);

	pCockpitVB->Unlock();

	// Set options
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_BLENDDIFFUSEALPHA);
	pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTSS_COLORARG1);
	pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

	// Draw title
	pd3dDevice->SetTexture(0, g_pAtlas);
	pd3dDevice->SetStreamSource(0, pCockpitVB, 0, sizeof(TRANSFORMEDTEXVERTEX));
	pd3dDevice->SetFVF(D3DFVF_TRANSFORMEDTEXVERTEX);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, cockpit_vtx / 3);

	// Restore options
	pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void DrawCockpit(IDirect3DDevice9 *pd3dDevice)
{
	if (GL_MSAA)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);

	// Prepare Cockpit drawing
	TRANSFORMEDTEXVERTEX *pVertices;
	cockpit_vtx = 0;
	if (FAILED(pCockpitVB->Lock(0, 0, (void **)&pVertices, 0)))
		return;

	// aus-null-ein gefedert: 0-14-30
	// 0<-->4607
	old_leftwheel = (front_left_amount_below_road / 23);
	if (old_leftwheel > 30)
		old_leftwheel = 30;
	float X1 = 32, X2 = 32 + 24;
	float Y1 = 135, Y2 = 135 + 55;
	Y1 -= old_leftwheel;
	Y2 -= old_leftwheel;
	AddQuad(pVertices, X1, Y1, X2, Y2, 0.8f, eWheel0 + (leftwheel_angle >> 16) % 6, 0, 1);
	old_rightwheel = (front_right_amount_below_road / 23);
	if (old_rightwheel > 30)
		old_rightwheel = 30;
	X1 = 264, X2 = 264 + 24;
	Y1 = 135, Y2 = 135 + 55;
	Y1 -= old_rightwheel;
	Y2 -= old_rightwheel;
	AddQuad(pVertices, X1, Y1, X2, Y2, 0.8f, eWheel0 + (rightwheel_angle >> 16) % 6, 1, 1);

	int engineFrame = eEngine;
	if (boost_activated)
	{
		static int frame = 0;
		if (!bPaused && (GameMode != GAME_OVER))
			frame = (frame + 1) % 16;
		const int engineframes[8] = {0, 0, 0, 1, 2, 2, 2, 1};
		engineFrame = eEngineFlames0 + engineframes[frame >> 1];
	}

	// Engine block
	AddQuad(pVertices, 42, 123, 42 + 236, 123 + 35, 0.90, engineFrame, 0, 1);

	// Cockpit top left right instruments
	AddQuad(pVertices, 41, 0, 41 + 238, 16, 0.90, eCockpitTop, 0, 1);
	AddQuad(pVertices, 0, 0, 41, 152, 0.90, eCockpitLeft, 0, 1);
	AddQuad(pVertices, 279, 0, 279 + 41, 152, 0.90, eCockpitRight, 0, 1);
	AddQuad(pVertices, 0, 152, 320, 200, 0.90, eCockpitBottom, 0, 1);

	/*
		//at widescreen, additional Cockpit site walls
		AddQuad(pVertices, 0.0f, 99.f * 2.4f, 40.f * 2.f, 480.0f, 0.9f, eCockpitWL, 0, 1);
		AddQuad(pVertices, 800.f - 82.f, 98.f * 2.4f, 800.f, 480.0f, 0.9f, eCockpitWR, 0, 1);
	*/

	if (new_damage)
	{
		// Cracking... width is 238, offset is 41 (in 320x200 screen space)
		float dam = new_damage;
		if (dam > 238)
			dam = 238;
		X1 = 41;
		X2 = 41 + dam;
		Y1 = 0;
		Y2 = 8;
		AddQuad(pVertices, X1, Y1, X2, Y2, 0.91, eCracking, 0, dam / 238);
	}
	for (int i = 0; i < nholes; i++)
	{
		// Holes... width is 10
		X1 = 48 + 24 * (9 - i);
		X2 = X1 + 10;
		Y1 = 0;
		Y2 = 8;
		AddQuad(pVertices, X1, Y1, X2, Y2, 0.92, eHole, 0, 1);
	}

	pCockpitVB->Unlock();

	// Prepare speedbar
	if (old_speedbar != CalculateDisplaySpeed())
	{
		old_speedbar = CalculateDisplaySpeed();
		TRANSFORMEDCOLVERTEX *pVertices;
		if (FAILED(pSpeedBarCB->Lock(0, 0, (void **)&pVertices, 0)))
			return;
		// Speedbar... is 126px long, speed 0-240
		float sbX1 = 97, sbX2 = 97 + ((old_speedbar > 240) ? (old_speedbar - 240) : old_speedbar) / 240.0f * 122;
		float sbY1 = 174, sbY2 = 176;

#define SPEEDCOL1 0xff00ffff // ABGR
#define SPEEDCOL2 0xff00ccff // ABGR

		pVertices[0].x = sbX1;
		pVertices[0].y = sbY1;
		pVertices[0].z = 0.91f;
		pVertices[0].rhw = 1.0f;
		pVertices[0].color = (old_speedbar > 240) ? SPEEDCOL2 : SPEEDCOL1;
		pVertices[1].x = sbX2;
		pVertices[1].y = sbY1;
		pVertices[1].z = 0.91f;
		pVertices[1].rhw = 1.0f;
		pVertices[1].color = (old_speedbar > 240) ? SPEEDCOL2 : SPEEDCOL1;
		pVertices[2].x = sbX2;
		pVertices[2].y = sbY2;
		pVertices[2].z = 0.91f;
		pVertices[2].rhw = 1.0f;
		pVertices[2].color = (old_speedbar > 240) ? SPEEDCOL2 : SPEEDCOL1;
		pVertices[3].x = sbX1;
		pVertices[3].y = sbY2;
		pVertices[3].z = 0.91f;
		pVertices[3].rhw = 1.0f;
		pVertices[3].color = (old_speedbar > 240) ? SPEEDCOL2 : SPEEDCOL1;
		pSpeedBarCB->Unlock();
	}

	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_BLENDDIFFUSEALPHA);
	pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTSS_COLORARG1);
	pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
#ifdef _WIN32
	// pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	// pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
#endif
	// Draw Cockpit
	pd3dDevice->SetTexture(0, g_pAtlas);
	pd3dDevice->SetStreamSource(0, pCockpitVB, 0, sizeof(TRANSFORMEDTEXVERTEX));

	pd3dDevice->SetFVF(D3DFVF_TRANSFORMEDTEXVERTEX);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, cockpit_vtx / 3); // 3 points per triangle

	// Draw Speed bar
	pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	pd3dDevice->SetStreamSource(0, pSpeedBarCB, 0, sizeof(TRANSFORMEDCOLVERTEX));

	pd3dDevice->SetFVF(D3DFVF_TRANSFORMEDCOLVERTEX);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2); // 3 points per triangle

	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}