/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_imagepackets_h
#define ps2s_imagepackets_h

// PLIN
//  #include "eestruct.h"
//  #include "libgraph.h"

#include "ps2s/core.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"

class CTexture;

/********************************************
 * CImageUploadPkt
 */

class CImageUploadPkt : protected CVifSCDmaPacket {
    friend class CTexture;

public:
    CImageUploadPkt(uint128_t* imagePtr, uint32_t w, uint32_t h, GS::tPSM psm, uint32_t gsBufWidth = 0, uint32_t gsWordAddress = 0);
    virtual ~CImageUploadPkt(void) {}

    // call these before trying to xfer the image!
    virtual void SetGsAddr(uint32_t gsMemWordAddress)
    {
        gsrBitBltBuf.dest_addr = gsMemWordAddress / 64;
    }
    void SetGsBufferWidth(uint32_t gsBufWidth)
    {
        mAssert(gsBufWidth >= 64);
        gsrBitBltBuf.dest_width = gsBufWidth / 64;
    }

    /// Use this to change between 8/8h 4/4hh/4hl
    void ChangePsm(GS::tPSM psm)
    {
        gsrBitBltBuf.dest_pixmode = psm;
    }

    // if you use this constructor you need to call these functions before
    // trying to xfer the image:  SetImage, SetGsBufferWidth, SetGsAddr
    CImageUploadPkt(void);

    void SetImage(uint128_t* imagePtr, uint32_t w, uint32_t h, GS::tPSM psm)
    {
        mAssert(((uint32_t)imagePtr & 0xf) == 0);
        pImage                    = imagePtr;
        gsrBitBltBuf.dest_pixmode = psm;
        gsrTrxReg.trans_w         = w;
        gsrTrxReg.trans_h         = h;

        BuildXferTags();
    }

    void Reset() { CVifSCDmaPacket::Reset(); }

    void Send(bool waitForEnd = false, bool flushCache = true)
    {
        CSCDmaPacket::Send(waitForEnd, flushCache);
    }

    void Send(CSCDmaPacket& packet);
    void Send(CVifSCDmaPacket& packet);

    inline void* operator new(size_t size) { return Core::New16(size); }
    inline void operator delete(void* p) { Core::Delete16(p); }

protected:
private:
    // gs packet to setup the texture image transfer
    struct {
        // DMA tag + GIF tag + 4 register settings (+ 15 rest ?)
        uint128_t FirstDmaTag;
        tGifTag ImageXferSettingsGifTag;
        GS::tBitbltbuf gsrBitBltBuf;
        uint64_t BitBltBufAddr;
        GS::tTrxpos gsrTrxPos;
        uint64_t TrxPosAddr;
        GS::tTrxreg gsrTrxReg;
        uint64_t TrxRegAddr;
        GS::tTrxdir gsrTrxDir;
        uint64_t TrxDirAddr;
        uint128_t RestOfPacket[15];
    } __attribute__((packed,aligned(16)));

    uint32_t uiNumXferGSRegs;
    uint128_t* pImage;

    void InitCommon(void);
    void BuildXferTags(void);
};

/********************************************
 * CClutUploadPkt
 */

class CClutUploadPkt : protected CImageUploadPkt {
    friend class CTexture;

public:
    CClutUploadPkt(uint32_t* clutPtr, uint32_t gsMemWordAddr)
    {
        SetGsBufferWidth(64);
        SetGsAddr(gsMemWordAddr);
        SetClut(clutPtr);
    }

    CClutUploadPkt(void)
    {
        SetGsBufferWidth(64);
    }
    virtual ~CClutUploadPkt(void) {}

    void SetGsAddr(uint32_t gsMemWordAddress)
    {
        CImageUploadPkt::SetGsAddr(gsMemWordAddress);
    }
    void SetClut(uint32_t* clutPtr)
    {
        SetImage((uint128_t*)clutPtr, 16, 16, GS::kPsm32);
    }
    void Reset() { CVifSCDmaPacket::Reset(); }
    void Send(bool waitForEnd = false, bool flushCache = true)
    {
        CImageUploadPkt::Send(waitForEnd, flushCache);
    }

    inline void Send(CSCDmaPacket& packet) { CImageUploadPkt::Send(packet); }
    inline void Send(CVifSCDmaPacket& packet) { CImageUploadPkt::Send(packet); }

    inline void* operator new(size_t size) { return Core::New16(size); }
    inline void operator delete(void* p) { Core::Delete16(p); }
};

#endif // ps2s_imagepackets_h
