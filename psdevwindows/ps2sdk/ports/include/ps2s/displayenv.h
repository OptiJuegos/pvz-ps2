/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_displayenv_h
#define ps2s_displayenv_h

/********************************************
 * includes
 */

// PLIN
//  #include "eeregs.h"
//  #include "libgraph.h"

#include "ps2s/core.h"
#include "ps2s/gs.h"
#include "ps2s/types.h"

namespace GS {

/********************************************
    * types
    */

typedef struct {
    unsigned DX : 12;  //
    unsigned DY : 11;  //
    unsigned MAGH : 4; //
    unsigned MAGV : 2; //
    unsigned p0 : 3;
    unsigned DW : 12; //
    unsigned DH : 11; //
    unsigned p1 : 9;
} tGS_DISPLAY1;

typedef struct {
    unsigned FBP : 9; // Base pointer
    unsigned FBW : 6; // Buffer width
    unsigned PSM : 5; // Pixel store mode
    unsigned p0 : 12;
    unsigned DBX : 11; //
    unsigned DBY : 11; //
    unsigned p1 : 10;
} tGS_DISPFB1;

typedef struct {
    unsigned EN1 : 1;   //
    unsigned EN2 : 1;   //
    unsigned CRTMD : 3; // CRT mode
    unsigned MMOD : 1;  //
    unsigned AMOD : 1;  //
    unsigned SLBG : 1;  //
    unsigned ALP : 8;   //
    unsigned p0 : 16;   //
    unsigned int p1;
} tGS_PMODE;

typedef struct {
    unsigned R : 8; // Background color Bulue
    unsigned G : 8; // Background color Green
    unsigned B : 8; // Background color Red
    unsigned p0 : 8;
    unsigned int p1;
} tGS_BGCOLOR;

// this list will certainly need to be expanded sooner or
// later...

namespace DisplayModes {
    typedef enum { ntsc,
        pal,
        dtv } tDisplayMode;
}
typedef DisplayModes::tDisplayMode tDisplayMode;

/********************************************
    * class def
    */

class CDisplayEnv {
public:
    CDisplayEnv(void);

    // settings for the two read circuits
    inline void SetUseReadCircuit1(bool use) { gsrPMode.EN1 = use; }
    inline void SetUseReadCircuit2(bool use) { gsrPMode.EN2 = use; }

    inline void SetFB1(unsigned int wordAddr, unsigned int bufWidth,
        unsigned int xOffsetInBuf = 0, unsigned int yOffsetInBuf = 0,
        unsigned int psm = GS::kPsm32);
    inline void SetFB2(unsigned int wordAddr, unsigned int bufWidth,
        unsigned int xOffsetInBuf = 0, unsigned int yOffsetInBuf = 0,
        unsigned int psm = GS::kPsm32);
    inline void SetFB1Addr(unsigned int wordAddr) { gsrDispFB1.FBP = wordAddr / 2048; }
    inline void SetFB2Addr(unsigned int wordAddr) { gsrDispFB2.FBP = wordAddr / 2048; }

    inline void SetDisplay1(unsigned int width, unsigned int height,
        unsigned int screenX = 0, unsigned int screenY = 0,
        unsigned int magH = 4, unsigned int magV = 1);
    inline void SetDisplay2(unsigned int width, unsigned int height,
        unsigned int screenX = 0, unsigned int screenY = 0,
        unsigned int magH = 4, unsigned int magV = 1);

    inline void SetDisplayMode(tDisplayMode mode);

    // blending the two read circuits
    inline void BlendRC1WithBG(void) { gsrPMode.SLBG = 1; }
    inline void BlendRC1WithRC2(void) { gsrPMode.SLBG = 0; }
    inline void BlendUsingRC1Alpha(void) { gsrPMode.MMOD = 0; }
    inline void BlendUsingConstAlpha(uint8_t alpha)
    {
        gsrPMode.MMOD = 1;
        gsrPMode.ALP  = alpha;
    }
    void DontBlend(void);

    inline void SetBGColor(uint8_t r, uint8_t g, uint8_t b)
    {
        gsrBGColor.R = r;
        gsrBGColor.G = g;
        gsrBGColor.B = b;
    }

    void SendSettings(void);

    inline void* operator new(size_t size) { return Core::New16(size); }
    inline void operator delete(void* p) { Core::Delete16(p); }

protected:
private:
    // these are really the same for both 1 and 2....how annoying
    typedef tGS_DISPLAY1 tGsrDisplay;
    typedef tGS_DISPFB1 tGsrDispFB;

    tGS_PMODE gsrPMode;
    tGsrDispFB gsrDispFB1;
    tGsrDispFB gsrDispFB2;
    tGsrDisplay gsrDisplay1;
    tGsrDisplay gsrDisplay2;
    tGS_BGCOLOR gsrBGColor;

    unsigned int HorizontalOverScan, VerticalOverScan;

    void SetDisplay(tGsrDisplay* displayReg,
        unsigned int width, unsigned int height,
        unsigned int screenX, unsigned int screenY,
        unsigned int magH, unsigned int magV);
    void SetFB(tGsrDispFB* dispFBReg,
        unsigned int wordAddr, unsigned int bufWidth,
        unsigned int xOffsetInBuf, unsigned int yOffsetInBuf,
        unsigned int psm);
} __attribute((aligned(16)));

void CDisplayEnv::SetDisplayMode(tDisplayMode mode)
{
    using namespace DisplayModes;

    switch (mode) {
    case ntsc:
        HorizontalOverScan = 158 * 4;
        VerticalOverScan   = 50 * 1;
        break;
    case pal:
        HorizontalOverScan = 158 * 4; // untested!
        VerticalOverScan   = 50 * 1;  // untested!
        break;
    case dtv:
        HorizontalOverScan = 302;
        VerticalOverScan   = 24;
        break;
    default:
        mError("This display mode isn't handled yet..somebody type in the values.");
        return;
    }
}

void CDisplayEnv::SetFB1(unsigned int wordAddr, unsigned int bufWidth,
    unsigned int xOffsetInBuf, unsigned int yOffsetInBuf,
    unsigned int psm)
{
    SetFB(&gsrDispFB1, wordAddr, bufWidth, xOffsetInBuf, yOffsetInBuf, psm);
}

void CDisplayEnv::SetFB2(unsigned int wordAddr, unsigned int bufWidth,
    unsigned int xOffsetInBuf, unsigned int yOffsetInBuf,
    unsigned int psm)
{
    SetFB(&gsrDispFB2, wordAddr, bufWidth, xOffsetInBuf, yOffsetInBuf, psm);
}

void CDisplayEnv::SetDisplay1(unsigned int width, unsigned int height,
    unsigned int screenX, unsigned int screenY,
    unsigned int magH, unsigned int magV)
{
    SetDisplay(&gsrDisplay1, width, height, screenX, screenY, magH, magV);
}

void CDisplayEnv::SetDisplay2(unsigned int width, unsigned int height,
    unsigned int screenX, unsigned int screenY,
    unsigned int magH, unsigned int magV)
{
    SetDisplay(&gsrDisplay2, width, height, screenX, screenY, magH, magV);
}

} // namespace GS

#endif // ps2s_displayenv_h
