#include "dx_linux.h"
#include "Atlas.h"

float atlas_tx1[eLAST] = {0};
float atlas_tx2[eLAST] = {0};
float atlas_ty1[eLAST] = {0};
float atlas_ty2[eLAST] = {0};

void InitAtlasCoord() {
    const int x[eLAST] = { 
        160, 128, 96, 64, 32, 0, // eWheel 24x54
        0, 12, 0,                // eHole 12x8, eNotHole 12x8, eCracking 238x8
        41, 0, 279, 0,           // eCockpit 238x16 41x152 41x152 320x48
        42, 42, 42, 42,          // Engine 236x35
        620, 620, 620, 620,      // eRoad...
        620, 620,                // eRoad Black / White
        200                      // eTitle 197x60
    };
    const int y[eLAST] = { 
        1, 1, 1, 1, 1, 1,        // eWheel
        60, 60, 128,             // eHole, eNotHole, eCracking
        160, 160, 160, 312,      // eCockpit
        592, 627, 662, 697,      // Engine
        441, 551, 111, 221,      // eRoad...
        1, 331,                  // eRoad Black / White
        0                        // eTitle
    };
    const int w[eLAST]  = {
        24, 24, 24, 24, 24, 24,  // eWheel
        10, 10, 238,             // eHole, eNotHole, eCracking
        238, 41, 41, 320,        // eCockpit
        236, 236 ,236, 236,      // Engine
        400, 400, 400, 400,      // eRoad...
        400, 400,                // eRoad Black / White
        197                      // eTitle
    };
    const int h[eLAST] = {
        55, 55, 55, 55, 55, 55,  // eWheel
        8, 8, 8,                 // eHole, eNotHole, eCracking
        16, 152, 152, 48,        // eCockpit
        35, 35 ,35, 35,          // Engine
        98, 98, 98, 98,          // eRoad...
        98, 98,                  // eRoad Black / White
        60                       // eTitle
    };

    for (int i=0; i<eLAST; i++) {
        atlas_tx1[i] = (float)x[i] / 1024.0f;
        atlas_tx2[i] = (float)(x[i]+w[i]) / 1024.0f;
        atlas_ty1[i] = 1.0f-(float)y[i] / 1024.0f;
        atlas_ty2[i] = 1.0f-(float)(y[i]+h[i]) / 1024.0f;
    }

}