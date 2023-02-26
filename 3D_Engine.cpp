/**************************************************************************

	3D Engine.cpp - Functions for producing 3D graphics

 **************************************************************************/

#include "dx_linux.h"

#include "StuntCarRacer.h"
#include "3D_Engine.h"

#define SIN_COS_TABLE_SIZE MAX_ANGLE + (MAX_ANGLE / 4) // makes use of sine/cosine overlap

extern DWORD Fill_Colour, Line_Colour;

static short Sin_Cos[SIN_COS_TABLE_SIZE];

short Trig_Coeffs2[NUM_TRIG_COEFFS];
static short Trig_Coeffs[NUM_TRIG_COEFFS];
static long World_X_Offset, World_Y_Offset, World_Z_Offset;

/*	The location of transformed co-ordinates can now be changed - for DrawZClippedPiece */
static COORD_3D Default_Transformed_Coords[MAX_COORDS];
static COORD_3D *Transformed_Coords = Default_Transformed_Coords;

/*	The location of screen co-ordinates can now be changed - for DrawPiece */
static COORD_2D Default_Screen_Coords[MAX_COORDS]; // could use POINT structure
static COORD_2D *Screen_Coords = Default_Screen_Coords;

/*	===================== */
/*	Function declarations */
/*	===================== */
static long LockAngle(long opposite,
					  long adjacent,
					  long clockwise);

/*	======================================================================================= */
/*	Function:		CreateSinCosTable														*/
/*																							*/
/*	Description:	Calculate and store sine/cosine values needed for 3D rotation			*/
/*	======================================================================================= */

void CreateSinCosTable(void)
{
	long i;
	double angle, step, value;

	angle = 0;
	step = ((double)2 * (double)PI) / (double)MAX_ANGLE;
	for (i = 0; i < SIN_COS_TABLE_SIZE; i++)
	{
		value = sin(angle);
		value = value * (double)PRECISION;

		Sin_Cos[i] = (short)value;
		angle += step;
	}
}

/*	======================================================================================= */
/*	Function:		GetSinCos																*/
/*																							*/
/*	Description:	Provide sine/cosine of supplied angle									*/
/*	======================================================================================= */

void GetSinCos(long angle,
			   short *sin,
			   short *cos)
{
	*sin = Sin_Cos[angle];
	*cos = Sin_Cos[angle + (MAX_ANGLE / 4)];
}

/*	======================================================================================= */
/*	Function:		CalcYXZTrigCoefficients													*/
/*																							*/
/*	Description:	Calculate all coefficients needed for 3D rotation in order Y, X, Z		*/
/*																							*/
/*					NOTE: Uses '3D Maths' rotations :-										*/
/*							Y-Rotation - Anti-clockwise										*/
/*							X-Rotation - Anti-Clockwise										*/
/*							Z-Rotation - Anti-Clockwise										*/
/*	======================================================================================= */

void CalcYXZTrigCoefficients(long x_angle,
							 long y_angle,
							 long z_angle)
{
	short sin_x, sin_y, sin_z;
	short cos_x, cos_y, cos_z;

	/*	========================= */
	/*	Get sine/cosine of angles */
	/*	========================= */
	sin_x = Sin_Cos[x_angle];
	sin_y = Sin_Cos[y_angle];
	sin_z = Sin_Cos[z_angle];

	cos_x = Sin_Cos[x_angle + (MAX_ANGLE / 4)];
	cos_y = Sin_Cos[y_angle + (MAX_ANGLE / 4)];
	cos_z = Sin_Cos[z_angle + (MAX_ANGLE / 4)];

	/*	================================ */
	/*	Calculate rotated x coefficients */
	/*	================================ */
	/*
	Trig_Coeffs[X_X_COMP] = ((cos_y * cos_z) - (((sin_x * sin_y) / PRECISION) * sin_z)) / PRECISION;
	Trig_Coeffs[X_Y_COMP] = -(cos_x * sin_z) / PRECISION;
	Trig_Coeffs[X_Z_COMP] = ((sin_y * cos_z) + (((sin_x * cos_y) / PRECISION) * sin_z)) / PRECISION;
	*/
	Trig_Coeffs[X_X_COMP] = (short)(((cos_y * cos_z) + (((sin_x * sin_y) / PRECISION) * sin_z)) / PRECISION);
	Trig_Coeffs[X_Y_COMP] = (short)(-(cos_x * sin_z) / PRECISION);
	Trig_Coeffs[X_Z_COMP] = (short)((-(sin_y * cos_z) + (((sin_x * cos_y) / PRECISION) * sin_z)) / PRECISION);

	/*	================================ */
	/*	Calculate rotated y coefficients */
	/*	================================ */
	/*
	Trig_Coeffs[Y_X_COMP] = ((cos_y * sin_z) + (((sin_x * sin_y) / PRECISION) * cos_z)) / PRECISION;
	Trig_Coeffs[Y_Y_COMP] = (cos_x * cos_z) / PRECISION;
	Trig_Coeffs[Y_Z_COMP] = ((sin_y * sin_z) - (((sin_x * cos_y) / PRECISION) * cos_z)) / PRECISION;
	*/
	Trig_Coeffs[Y_X_COMP] = (short)(((cos_y * sin_z) - (((sin_x * sin_y) / PRECISION) * cos_z)) / PRECISION);
	Trig_Coeffs[Y_Y_COMP] = (short)((cos_x * cos_z) / PRECISION);
	Trig_Coeffs[Y_Z_COMP] = (short)((-(sin_y * sin_z) - (((sin_x * cos_y) / PRECISION) * cos_z)) / PRECISION);

	/*	================================ */
	/*	Calculate rotated z coefficients */
	/*	================================ */
	/*
	Trig_Coeffs[Z_X_COMP] = -(cos_x * sin_y) / PRECISION;
	Trig_Coeffs[Z_Y_COMP] = sin_x;
	Trig_Coeffs[Z_Z_COMP] = (cos_x * cos_y) / PRECISION;
	*/
	Trig_Coeffs[Z_X_COMP] = (short)((cos_x * sin_y) / PRECISION);
	Trig_Coeffs[Z_Y_COMP] = sin_x;
	Trig_Coeffs[Z_Z_COMP] = (short)((cos_x * cos_y) / PRECISION);
}

/*	======================================================================================= */
/*	Function:		TrigCoefficients														*/
/*																							*/
/*	Description:	Return ptr. to coefficients needed for 3D rotation						*/
/*	======================================================================================= */

short *TrigCoefficients(void)
{
	return (Trig_Coeffs);
}

/*	======================================================================================= */
/*	Function:		WorldOffset (opposite of RotateCoordinate)								*/
/*																							*/
/*	Description:	Takes the input vector and provides the World vector					*/
/*					for the given rotation (i.e. sums X/Y/Z components)						*/
/*	======================================================================================= */

void WorldOffset(long *xptr,
				 long *yptr,
				 long *zptr)
{
	long x, y, z;

	x = *xptr;
	y = *yptr;
	z = *zptr;

	*xptr = (x * (long)Trig_Coeffs[X_X_COMP]) +
			(y * (long)Trig_Coeffs[Y_X_COMP]) +
			(z * (long)Trig_Coeffs[Z_X_COMP]);

	*yptr = (x * (long)Trig_Coeffs[X_Y_COMP]) +
			(y * (long)Trig_Coeffs[Y_Y_COMP]) +
			(z * (long)Trig_Coeffs[Z_Y_COMP]);

	*zptr = (x * (long)Trig_Coeffs[X_Z_COMP]) +
			(y * (long)Trig_Coeffs[Y_Z_COMP]) +
			(z * (long)Trig_Coeffs[Z_Z_COMP]);
}

/*	======================================================================================= */
/*	Function:		LockViewpointToTarget													*/
/*																							*/
/*	Description:	Calculate x/y angles required to place target within centre of view     */
/*	======================================================================================= */

void LockViewpointToTarget(long viewpoint_x,
						   long viewpoint_y,
						   long viewpoint_z,
						   long target_x,
						   long target_y,
						   long target_z,
						   long *viewpoint_x_angle,
						   long *viewpoint_y_angle)
{
	long opp, adj;
	double a, b, h;

	// y angle
	opp = target_x - viewpoint_x;
	adj = target_z - viewpoint_z;
	*viewpoint_y_angle = LockAngle(opp, adj, FALSE);

	// x angle
	a = (double)((target_x - viewpoint_x) >> LOG_PRECISION);
	b = (double)((target_z - viewpoint_z) >> LOG_PRECISION);
	h = sqrt((a * a) + (b * b));
	adj = (long)(h * PRECISION);
	opp = target_y - viewpoint_y;
	*viewpoint_x_angle = LockAngle(opp, adj, FALSE);

	return;
}

static long LockAngle(long opposite,
					  long adjacent,
					  long clockwise)
{
	long viewpoint_angle;
	double o, a, radians, angle;

	o = (double)opposite;
	a = (double)adjacent;

	// use inverse tan to calculate basic angle in radians
	if (a == 0)							  // prevent division by zero
		radians = (double)PI / (double)2; // 90 degrees
	else
		radians = atan(o / a); // inverse tan

	// convert radians to internal angle (also round up)
	angle = ((radians * (double)MAX_ANGLE) / ((double)2 * (double)PI));
	// convert to absolute and round up as follows (because abs() isn't for doubles)
	if (angle > 0)
		viewpoint_angle = (long)(angle + (double)0.5);
	else
		viewpoint_angle = (long)((double)0.5 - angle);

	// convert angle from first quadrant to full range
	if (o >= 0)
	{
		if (a >= 0)
		{
			// first quadrant
			viewpoint_angle = (long)angle;
		}
		else
		{
			// second quadrant
			viewpoint_angle = (long)angle + _180_DEGREES;
		}
	}
	else
	{
		if (a <= 0)
		{
			// third quadrant
			viewpoint_angle = (long)angle + _180_DEGREES;
		}
		else
		{
			// fourth quadrant
			viewpoint_angle = (long)angle + _360_DEGREES;
		}
	}

	// default is anti-clockwise, so convert to clockwise if necessary
	if (clockwise)
	{
		viewpoint_angle = (-viewpoint_angle) & (MAX_ANGLE - 1);
	}

	return (viewpoint_angle);
}

/*	======================================================================================= */
/*	Function:		CreatePolygonVertexBuffer,												*/
/*					FreePolygonVertexBuffer,												*/
/*					DrawPolygon,															*/
/*					DrawFilledRectangle														*/
/*																							*/
/*	Description:	Functions to draw polygon and filled rectangle using Direct3D			*/
/*	======================================================================================= */

struct TRANSFORMEDVERTEX
{
	FLOAT x, y, z, rhw; // The transformed position for the vertex.
	DWORD color;		// The vertex color.
};
#define D3DFVF_TRANSFORMEDVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

static IDirect3DVertexBuffer9 *pPolygonVB = NULL;

HRESULT CreatePolygonVertexBuffer(IDirect3DDevice9 *pd3dDevice)
{
	if (pPolygonVB == NULL)
	{
		if (FAILED(pd3dDevice->CreateVertexBuffer(MAX_POLY_SIDES * sizeof(TRANSFORMEDVERTEX),
												  D3DUSAGE_WRITEONLY, D3DFVF_TRANSFORMEDVERTEX, D3DPOOL_DEFAULT, &pPolygonVB, NULL)))
			return E_FAIL;
	}

	return S_OK;
}

void FreePolygonVertexBuffer(void)
{
	if (pPolygonVB)
		pPolygonVB->Release(), pPolygonVB = NULL;
}

// Draw flat polygon (no z information)
void DrawPolygon(POINT *pptr,
				 long sides)
{
	long i;
	IDirect3DDevice9 *pd3dDevice = DXUTGetD3DDevice();

	TRANSFORMEDVERTEX *pVertices;

	// finish if too many sides
	if (sides > MAX_POLY_SIDES)
		return;

	if (FAILED(pPolygonVB->Lock(0, sides * sizeof(TRANSFORMEDVERTEX), (void **)&pVertices, 0)))
		return;

	for (i = 0; i < sides; i++)
	{

		pVertices[i].x = (float)pptr[i].x; // screen x
		pVertices[i].y = (float)pptr[i].y; // screen y
		pVertices[i].z = (float)0.5f;	   // not needed unless Z buffering
		pVertices[i].rhw = (float)1.0f;
		pVertices[i].color = Fill_Colour;
	}
	pPolygonVB->Unlock();

	pd3dDevice->SetStreamSource(0, pPolygonVB, 0, sizeof(TRANSFORMEDVERTEX));
	pd3dDevice->SetFVF(D3DFVF_TRANSFORMEDVERTEX);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, sides - 2);

	return;
}

void DrawFilledRectangle(long x1, long y1, long x2, long y2, DWORD colour)
{
	glColor4ubv((GLubyte *)&colour);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2i(x1, y1);
	glVertex2i(x1, y2);
	glVertex2i(x2, y2);
	glVertex2i(x2, y1);
	glEnd();

	return;
}
