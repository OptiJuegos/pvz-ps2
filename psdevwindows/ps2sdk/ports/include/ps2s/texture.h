/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_texture_h
#define ps2s_texture_h

/********************************************
 * includes
 */

// PLIN
//  #include "eetypes.h" // need to include eetypes.h for eestruct.h
//  #include "eestruct.h"

#include "ps2s/gs.h"
#include "ps2s/imagepackets.h"
#include "ps2s/packet.h"
#include "ps2s/types.h"

class CImageUploadPkt;
class CClutUploadPkt;

namespace GS {

/********************************************
    * typedefs
    */

namespace TexWrapMode {
    typedef enum {
        kRepeat,
        kClamp
    } tTexWrapMode;
}
using TexWrapMode::tTexWrapMode;

namespace TexMode {
    typedef enum {
        kModulate,
        kDecal,
        kHighlight,
        kHighlight2
    } tTexMode;
}
using TexMode::tTexMode;

namespace MagMode {
    typedef enum {
        kNearest,
        kLinear
    } tMagMode;
}
using MagMode::tMagMode;

namespace MinMode {
    typedef enum {
        kNearest,
        kLinear,
        kNearestMipmapNearest,
        kNearestMipmapLinear,
        kLinearMipmapNearest,
        kLinearMipmapLinear
    } tMinMode;
}
using MinMode::tMinMode;

/********************************************
    * CTexEnv
    */

class CTexEnv {
public:
    // if you must use the short constructor, be sure to call SetDimensions() and SetPSM()!!
    CTexEnv(GS::tContext context = GS::kContext1);
    CTexEnv(GS::tContext context, uint32_t width, uint32_t height, GS::tPSM psm);
    virtual ~CTexEnv(void) {}

    // accessors

    inline uint32_t GetImageGsAddr(void) const { return gsrTex0.tb_addr * 64; }
    inline uint32_t GetClutGsAddr(void) const { return gsrTex0.cb_addr * 64; }
    inline GS::tContext GetContext(void) const;
    inline GS::tPSM GetPSM(void) const { return (GS::tPSM)gsrTex0.psm; }
    inline uint32_t GetW(void) const { return uiTexPixelWidth; }
    inline uint32_t GetH(void) const { return uiTexPixelHeight; }

    // mutators

    inline void ClearRegion(void);
    inline void SetClutLoadConditions(int mode) { gsrTex0.clut_loadmode = mode; }
    virtual void SetClutGsAddr(uint32_t gsMemWordAddress);
    void SetContext(GS::tContext context);
    virtual void SetDimensions(uint32_t w, uint32_t h);
    inline void SetFlush(bool flush);
    virtual void SetImageGsAddr(uint32_t gsMemWordAddress);
    inline void SetMagMode(tMagMode newMode) { gsrTex1.mmag = (uint64_t)newMode; }
    inline void SetMinMode(tMinMode newMode) { gsrTex1.mmin = (uint64_t)newMode; }
    void SetPSM(GS::tPSM newPSM);
    void SetRegion(uint32_t originU, uint32_t originV, uint32_t w, uint32_t h);
    inline void SetTexMode(tTexMode newMode) { gsrTex0.tex_funtion = (uint64_t)newMode; }
    void SetWrapModeS(tTexWrapMode sMode);
    void SetWrapModeT(tTexWrapMode tMode);
    inline void SetWrapMode(tTexWrapMode sMode, tTexWrapMode tMode)
    {
        SetWrapModeS(sMode);
        SetWrapModeT(tMode);
    }
    inline void SetUseTexAlpha(bool useTexAlpha);

    // other

    void SendSettings(bool waitForEnd = false, bool flushCache = true);
    void SendSettings(CSCDmaPacket& packet);
    void SendSettings(CVifSCDmaPacket& packet);

    // assignment
    CTexEnv(const CTexEnv& rhs);
    CTexEnv& operator=(const CTexEnv& rhs);

    inline void* operator new(size_t size) { return Core::New16(size); }
    inline void operator delete(void* p) { Core::Delete16(p); }

protected:
    // gs packet to setup texture environment
    struct {
        // DMA tag + GIF tag + 5 register settings
        tSourceChainTag SettingsDmaTag;
        tGifTag SettingsGifTag;
        GS::tTexflush gsrTexflush;
        uint64_t TexflushAddr;
        GS::tClamp gsrClamp;
        uint64_t ClampAddr;
        GS::tTex1 gsrTex1;
        uint64_t Tex1Addr;
        GS::tTex0 gsrTex0;
        uint64_t Tex0Addr;
        GS::tTexa gsrTexA;
        uint64_t TexAAddr;
    } __attribute__((packed,aligned(16)));

    uint32_t uiNumSettingsGSRegs;
    uint32_t uiTexPixelWidth, uiTexPixelHeight;

    CSCDmaPacket SettingsPacket;

private:
    void InitCommon(GS::tContext context);

};

/********************************************
    * CTexture
    */

// this class is meant to be subclassed, so it cannot
// be instanciated.

// this needs a lot of work, but it's pretty useful so I'll leave it
// in here.  In particular the *UploadPkts might not have been a good
// idea...

class CTexture : public CTexEnv {
public:
    // mutators

    virtual void SetImageGsAddr(uint32_t gsMemWordAddress);
    virtual void SetClutGsAddr(uint32_t gsMemWordAddress);

    // other

    void SendImage(bool waitForEnd = false, bool flushCache = true);
    void SendImage(CSCDmaPacket& packet);
    void SendImage(CVifSCDmaPacket& packet);

    void SendClut(bool waitForEnd = false, bool flushCache = true);
    void SendClut(CSCDmaPacket& packet);
    void SendClut(CVifSCDmaPacket& packet);

protected:
    uint128_t *pImage, *pClut;
    uint32_t uiGsAddr;

    CImageUploadPkt* pImageUploadPkt;
    CClutUploadPkt* pClutUploadPkt;

    // be sure to call SetImage, SetImageGsAddr, and SetClutGsAddr if applicable
    CTexture(GS::tContext context);
    virtual ~CTexture(void);

    uint128_t* AllocMem(uint32_t w, uint32_t h, GS::tPSM psm);
    virtual void SetImage(uint128_t* imagePtr, uint32_t w, uint32_t h, GS::tPSM psm, uint32_t* clutPtr = NULL);

    void Reset();

private:
    bool bFreeMemOnExit;

    // thou shalt not assign CTextures
    CTexture(const CTexture& rhs);
    CTexture& operator=(const CTexture& rhs);

    void InitCommon(GS::tContext context);

};

/********************************************
    * CClut
    */

class CClut {
    CClutUploadPkt* UploadPkt;
    unsigned int GsAddr;

public:
    CClut(const void* table, int numEntries);
    ~CClut() { delete UploadPkt; }

    void SetGsAddr(unsigned int wordAddr)
    {
        GsAddr = wordAddr;
        UploadPkt->SetGsAddr(wordAddr);
    }
    unsigned int GetGsAddr() const { return GsAddr; }

    void Send(bool waitForEnd = false, bool flushCache = true)
    {
        UploadPkt->Send(waitForEnd, flushCache);
    }
    void Send(CSCDmaPacket& packet) { UploadPkt->Send(packet); }
    void Send(CVifSCDmaPacket& packet) { UploadPkt->Send(packet); }
};

/********************************************
    * CCheckTex
    */

class CCheckTex : public CTexture {
public:
    CCheckTex(GS::tContext context,
        uint32_t width, uint32_t height,
        uint32_t xCellSize, uint32_t yCellSize,
        uint32_t color1, uint32_t color2);
    virtual ~CCheckTex(void) {}

private:
    void MakeCheckerboard(uint32_t xCellSize, uint32_t yCellSize, uint32_t color1, uint32_t color2);
};

/********************************************
    * CTexEnv inlines
    */

inline GS::tContext
CTexEnv::GetContext(void) const
{
    if (ClampAddr == (unsigned int)GS::RegAddrs::clamp_1)
        return GS::kContext1;
    else
        return GS::kContext2;
}

inline void
CTexEnv::SetFlush(bool flush)
{
    if (flush)
        TexflushAddr = GS::RegAddrs::texflush;
    else
        TexflushAddr = GS::RegAddrs::nop;
}

inline void
CTexEnv::SetUseTexAlpha(bool useTexAlpha)
{
    gsrTex0.tex_cc = useTexAlpha;
}

inline void
CTexEnv::ClearRegion(void)
{
    if (gsrClamp.wrap_mode_s & 2)
        gsrClamp.wrap_mode_s = 3 - gsrClamp.wrap_mode_s;
    if (gsrClamp.wrap_mode_t & 2)
        gsrClamp.wrap_mode_t = 3 - gsrClamp.wrap_mode_t;
}

} // namespace GS

#endif // ps2s_texture_h
