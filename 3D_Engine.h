#ifndef _3D_ENGINE
#define _3D_ENGINE

#define PI 3.1415926535898

#define MAX_ANGLE 65536 // value that is equivalent to 360 degrees

#define _360_DEGREES (MAX_ANGLE)
#define _270_DEGREES (3 * MAX_ANGLE / 4)
#define _180_DEGREES (MAX_ANGLE / 2)
#define _90_DEGREES (MAX_ANGLE / 4)
#define _0_DEGREES (0)

// could use enumerated type for the following
#define X_X_COMP 0 // rotated x components
#define X_Y_COMP 1
#define X_Z_COMP 2

#define Y_X_COMP 3 // rotated y components
#define Y_Y_COMP 4
#define Y_Z_COMP 5

#define Z_X_COMP 6 // rotated z components
#define Z_Y_COMP 7
#define Z_Z_COMP 8

#define NUM_TRIG_COEFFS Z_Z_COMP + 1

#define PRECISION 16384
#define LOG_PRECISION 14 // to base 2

#define MAX_POLY_SIDES 8

#define MAX_COORDS 200

#define FOCUS 265	// zoom the skyline
#define LOG_FOCUS 9 // zoom the horizon

typedef struct
{
	long x;
	long y;
	long z;
} COORD_3D;

typedef struct
{
	long x;
	long y;
} COORD_2D;

extern void CreateSinCosTable(void);

extern void GetSinCos(long angle,
					  short *sin,
					  short *cos);

extern void SetWorldOffset(long x_offset,
						   long y_offset,
						   long z_offset);

extern void DefaultCoords(void);

extern void CalcYXZTrigCoefficients(long x_angle,
									long y_angle,
									long z_angle);

extern short *TrigCoefficients(void);

extern void RotateCoordinate(long *xptr,
							 long *yptr,
							 long *zptr);

extern void WorldOffset(long *xptr,
						long *yptr,
						long *zptr);

extern long TransformedZ(long offset);

extern long TexturedPolygon(long *cptr,
							long sides,
							long *vptr);

extern long PolygonVisible(long *cptr);

extern long Polygon(long *cptr,
					long sides);

extern long PolygonEx(long *cptr,
					  long sides,
					  long *optr);

extern void Line(long c1,
				 long c2);

extern long PolygonZClipped(long *cptr,
							long sides,
							long check_orientation,
							long *on_screen);

extern void LineZClipped(long c1,
						 long c2);

extern void LockViewpointToTarget(long viewpoint_x,
								  long viewpoint_y,
								  long viewpoint_z,
								  long target_x,
								  long target_y,
								  long target_z,
								  long *viewpoint_x_angle,
								  long *viewpoint_y_angle);

extern HRESULT CreatePolygonVertexBuffer(IDirect3DDevice9 *pd3dDevice);
extern void FreePolygonVertexBuffer(void);

extern void DrawPolygon(POINT *pptr,
						long sides);

extern void DrawFilledRectangle(long x1, long y1, long x2, long y2, DWORD colour);

#endif /* _3D_ENGINE */