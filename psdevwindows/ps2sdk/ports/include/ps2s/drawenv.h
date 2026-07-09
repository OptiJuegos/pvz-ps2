/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_drawenv_h
#define ps2s_drawenv_h

/********************************************
 * includes
 */

#include "ps2s/core.h"
#include "ps2s/dmac.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"
#include "ps2s/types.h"

namespace GS {

/********************************************
    * constants
    */

static const bool kDontAddHalfPixel = false;
static const bool kAddHalfPixel     = true;

/********************************************
    * typedefs
    */

// the "using"s after each namespace { ... } are to make the type
// available from the GS namespace (ie, so you can say "GS::tAlphaBlendVal")

namespace ABlend {
    typedef enum {
        kSourceRGB   = 0,
        kDestRGB     = 1,
        kSourceAlpha = 0,
        kDestAlpha   = 1,
        kZero        = 2,
        kFix         = 2
    } tAlphaBlendVal;
}
using ABlend::tAlphaBlendVal;

namespace ATest {
    typedef enum {
        kNever,
        kAlways,
        kLess,
        kLEqual,
        kEqual,
        kGEqual,
        kGreater,
        kNotEqual
    } tAlphaTestPassMode;

    typedef enum {
        kKeep,
        kFrameBuffOnly,
        kZBuffOnly,
        kRGBOnly
    } tAlphaTestFailAction;
}
using ATest::tAlphaTestPassMode;
using ATest::tAlphaTestFailAction;

namespace ZTest {
    typedef enum {
        kNever,
        kAlways,
        kGEqual,
        kGreater
    } tZTestPassMode;
}
using ZTest::tZTestPassMode;

namespace Dither {
    typedef enum {
        kMinusFour  = 0x4,
        kMinusThree = 0x5,
        kMinusTwo   = 0x6,
        kMinusOne   = 0x7,
        kZero       = 0,
        kOne        = 1,
        kTwo        = 2,
        kThree      = 3
    } tDitherVal;
}
using Dither::tDitherVal;

/********************************************
    * class definition
    */

class CDrawEnv {
public:
    CDrawEnv(GS::tContext context);
    CDrawEnv(const CDrawEnv& rhs);
    CDrawEnv(GS::tContext context, uint32_t fbW, uint32_t fbH, uint32_t fbWordAddr, uint32_t zbufWordAddr = NULL);

    void operator=(const CDrawEnv& otherDE);

    void SetContext(GS::tContext context);

    inline void EnableAlphaTest(void) { gsrTest.atest_enable = 1; }
    inline void DisableAlphaTest(void) { gsrTest.atest_enable = 0; }
    inline void SetAlphaTestPassMode(tAlphaTestPassMode mode) { gsrTest.atest_method = (uint64_t)mode; }
    inline void SetAlphaTestFailAction(tAlphaTestFailAction action) { gsrTest.atest_fail_method = (uint64_t)action; }
    inline void SetAlphaRefVal(uint8_t refVal) { gsrTest.atest_reference = refVal; }

    inline void EnableSelectiveAlphaBlend(void) { gsrPABE.enable = 1; }
    inline void DisableSelectiveAlphaBlend(void) { gsrPABE.enable = 0; }

    inline void SetAlphaBlendFunc(tAlphaBlendVal a, tAlphaBlendVal b, tAlphaBlendVal c, tAlphaBlendVal d, uint32_t fix);

    inline void EnableColorClamp(void) { gsrColClamp.clamp = 1; }
    inline void DisableColorClamp(void) { gsrColClamp.clamp = 0; }

    inline void EnableDestAlphaTest(void) { gsrTest.datest_enable = 1; }
    inline void DisableDestAlphaTest(void) { gsrTest.datest_enable = 0; }
    inline void SetDestAlpha0Pass(void) { gsrTest.datest_mode = 0; }
    inline void SetDestAlpha1Pass(void) { gsrTest.datest_mode = 1; }

    inline void SetDepthTestPassMode(tZTestPassMode passMode)
    {
        gsrTest.ztest_method = (uint64_t)passMode;
        eZTestPassMode       = passMode;
    }
    inline void SetDepthBufferAddr(uint32_t wordAddress) { gsrZBuf.fb_addr = wordAddress / 2048; }
    inline void SetDepthBufferPSM(uint32_t psm) { gsrZBuf.psm = (uint64_t)psm; }
    inline void EnableDepthTest(void)
    {
        gsrZBuf.update_mask = 0;
        SetDepthTestPassMode(eZTestPassMode);
    }
    inline void DisableDepthTest(void)
    {
        gsrZBuf.update_mask  = 1;
        gsrTest.ztest_method = (uint64_t)ZTest::kAlways;
    }
    inline void SetDepthWriteEnabled(bool write) { gsrZBuf.update_mask = !write; }

    inline void SetFrameBufferAddr(uint32_t wordAddress)
    {
        mAssert((wordAddress & 2047) == 0);
        gsrFrame.fb_addr = wordAddress / 2048;
    }
    inline void SetFrameBufferDrawMask(uint32_t drawMask) { gsrFrame.draw_mask = drawMask; }
    void SetFrameBufferDim(uint32_t pixelW, uint32_t pixelH);
    inline void SetFrameBufferPSM(uint32_t psm) { gsrFrame.psm = psm; }
    float GetInterlacedPixelOffset() const { return InterlacedOffset; }
    void SetInterlacedPixelOffset(float offset) { InterlacedOffset = offset; }
    void CalculateClippedFBXYOffsets(bool addHalfPixel);
    inline void SetXYOffsets(uint16_t offsetX, uint16_t offsetY)
    {
        gsrXYOffset.offset_x = offsetX << 4;
        gsrXYOffset.offset_y = offsetY << 4;
    }
    inline void SetXYOffsetsFix4(uint16_t offsetX, uint16_t offsetY)
    {
        gsrXYOffset.offset_x = offsetX;
        gsrXYOffset.offset_y = offsetY;
    }

    inline void SetFrameBufAlphaORMask(uint32_t mask) { gsrFBA.alpha = mask; }
    inline void SetFogColor(uint32_t r, uint32_t g, uint32_t b)
    {
        gsrFogCol.r = r;
        gsrFogCol.g = g;
        gsrFogCol.b = b;
    }

    inline void EnableDither(void) { gsrDTHE.enable = 1; }
    inline void DisableDither(void) { gsrDTHE.enable = 0; }
    inline void SetDitherMatrix(tDitherVal dm00, tDitherVal dm01, tDitherVal dm02, tDitherVal dm03,
        tDitherVal dm10, tDitherVal dm11, tDitherVal dm12, tDitherVal dm13,
        tDitherVal dm20, tDitherVal dm21, tDitherVal dm22, tDitherVal dm23,
        tDitherVal dm30, tDitherVal dm31, tDitherVal dm32, tDitherVal dm33);

    inline void SetScissorArea(uint32_t scX, uint32_t scY, uint32_t scWidth, uint32_t scHeight); // must be after SetFrameBufferDim() to have effect

    void SendSettings(bool waitForEnd = false, bool flushCache = true);
    void SendSettings(CSCDmaPacket& packet);
    void SendSettings(CVifSCDmaPacket& packet);

    // accessors
    uint32_t GetFrameBufferAddr(void) { return gsrFrame.fb_addr * 2048; }

    inline void* operator new(size_t size) { return Core::New16(size); }
    inline void operator delete(void* p) { Core::Delete16(p); }

protected:
    struct {
        // DMA tag + GIF tag + 13 register settings
        tSourceChainTag SettingsDmaTag;
        tGifTag SettingsGifTag;
        GS::tFrame gsrFrame;
        uint64_t FrameAddr;
        GS::tZbuf gsrZBuf;
        uint64_t ZBufAddr;
        GS::tXyoffset gsrXYOffset;
        uint64_t XYOffsetAddr;
        GS::tPrmodecont gsrPrModeCont;
        uint64_t PrModeContAddr;
        GS::tColclamp gsrColClamp;
        uint64_t ColClampAddr;
        GS::tTest gsrTest;
        uint64_t TestAddr;
        GS::tAlpha gsrAlpha;
        uint64_t AlphaAddr;
        GS::tPabe gsrPABE;
        uint64_t PABEAddr;
        GS::tFba gsrFBA;
        uint64_t FBAAddr;
        GS::tDthe gsrDTHE;
        uint64_t DTHEAddr;
        GS::tDimx gsrDIMX;
        uint64_t DIMXAddr;
        GS::tScissor gsrScissor;
        uint64_t ScissorAddr;
        GS::tFogcol gsrFogCol;
        uint64_t FogColAddr;
    } __attribute__((packed,aligned(16)));
    uint32_t uiNumGSRegs;

    CSCDmaPacket GifPacket;

    tZTestPassMode eZTestPassMode;
    uint32_t uiFBWidth, uiFBHeight;
    float InterlacedOffset;

private:
    void InitCommon(GS::tContext context);

};

/********************************************
    * inline methods
    */

inline void
CDrawEnv::SetAlphaBlendFunc(tAlphaBlendVal a, tAlphaBlendVal b, tAlphaBlendVal c, tAlphaBlendVal d, uint32_t fix)
{
    gsrAlpha.a     = a;
    gsrAlpha.b     = b;
    gsrAlpha.c     = c;
    gsrAlpha.d     = d;
    gsrAlpha.alpha = fix;
}

inline void
CDrawEnv::SetDitherMatrix(tDitherVal dm00, tDitherVal dm01, tDitherVal dm02, tDitherVal dm03,
    tDitherVal dm10, tDitherVal dm11, tDitherVal dm12, tDitherVal dm13,
    tDitherVal dm20, tDitherVal dm21, tDitherVal dm22, tDitherVal dm23,
    tDitherVal dm30, tDitherVal dm31, tDitherVal dm32, tDitherVal dm33)
{
    gsrDIMX.DIMX00 = (uint64_t)dm00;
    gsrDIMX.DIMX01 = (uint64_t)dm01;
    gsrDIMX.DIMX02 = (uint64_t)dm02;
    gsrDIMX.DIMX03 = (uint64_t)dm03;
    gsrDIMX.DIMX10 = (uint64_t)dm10;
    gsrDIMX.DIMX11 = (uint64_t)dm11;
    gsrDIMX.DIMX12 = (uint64_t)dm12;
    gsrDIMX.DIMX13 = (uint64_t)dm13;
    gsrDIMX.DIMX20 = (uint64_t)dm20;
    gsrDIMX.DIMX21 = (uint64_t)dm21;
    gsrDIMX.DIMX22 = (uint64_t)dm22;
    gsrDIMX.DIMX23 = (uint64_t)dm23;
    gsrDIMX.DIMX30 = (uint64_t)dm30;
    gsrDIMX.DIMX31 = (uint64_t)dm31;
    gsrDIMX.DIMX32 = (uint64_t)dm32;
    gsrDIMX.DIMX33 = (uint64_t)dm33;
}

inline void
CDrawEnv::SetScissorArea(uint32_t scX, uint32_t scY, uint32_t scWidth, uint32_t scHeight)
{
    gsrScissor.clip_x0 = scX;
    gsrScissor.clip_y0 = scY;
    gsrScissor.clip_x1 = scX + scWidth - 1;
    gsrScissor.clip_y1 = scY + scHeight - 1;
}

} // namespace GS

#endif // ps2s_drawenv_h
