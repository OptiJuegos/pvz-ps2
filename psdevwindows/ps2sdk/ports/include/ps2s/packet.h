/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_packet_h
#define ps2s_packet_h

/********************************************
 * includes
 */

#include <stdlib.h>

#include "dma.h"

#include "ps2s/dmac.h"
#include "ps2s/types.h"
#include "ps2s/vif.h"

#include "ps2s/core.h"
#include "ps2s/utils.h"

/********************************************
 * constants
 */

namespace Packet {

// constructors' isFull = ?
static const bool kFull           = true;
static const bool kNotFull        = false;
static const bool kWait           = true;
static const bool kDontWait       = false;
static const bool kFlushCache     = true;
static const bool kDontFlushCache = false;

// CSCDmaPacket
static const bool kXferTags     = true;
static const bool kDontXferTags = false;
static const bool kIrq          = true;
static const bool kNoIrq        = false;
static const bool kFromSp       = true;
static const bool kFromMem      = false;

// CVifSCDmaPacket
static const bool kDoubleBuff = true;
static const bool kSingleBuff = false;
static const bool kMasked     = true;
static const bool kNotMasked  = false;

} // namespace Packet

/********************************************
 * class DmaPacket
 */

class CDmaPacket {
public:
    // need to add support for different memory mappings
    CDmaPacket(uint32_t bufferQWSize, tDmaChannelId channel, uint32_t memMapping = Core::MemMappings::Normal);
    CDmaPacket(uint128_t* buffer, uint32_t bufferQWSize, tDmaChannelId channel, uint32_t memMapping = Core::MemMappings::Normal, bool isFull = false);

    virtual ~CDmaPacket(void);

    // mutators

    template <class dataType>
    inline void operator+=(const dataType data);
    template <class dataType>
    inline dataType* Add(const dataType data);
    template <class dataType>
    inline dataType* Add(const dataType* data, uint32_t num);

    // to use these you MUST cast to CDmaPacket&!! (otherwise you'll get a compiler error..  I
    // could make inlines for all of CDmaPacket's descendants, but I will certainly forget to
    // update them at some point in the future, when I may not be lucky enough to have
    // the compiler catch it...)
    void operator+=(const CDmaPacket& otherPkt);
    inline uint128_t* Add(const CDmaPacket& otherPkt);

    inline void Reset(void) { pNext = pBase; }
    inline void SetDmaChannel(tDmaChannelId channel);
    virtual void Send(bool waitForEnd = false, bool flushCache = true);

    // accessors

    uint128_t* GetBase(void) const { return (uint128_t*)pBase; }
    uint8_t* GetNextPtr(void) const { return pNext; }
    uint32_t GetByteLength(void) const { return (uint32_t)pNext - (uint32_t)pBase; }

    static void* AllocBuffer(int numQwords, unsigned int memMapping);
    // be VERY careful using this.. it swaps its internal dma buffer with the new,
    // returning the old..  be aware of where memory is being deallocated..
    void* SwapOutBuffer(void* newBuffer);

    void HexDump(uint32_t numQwords = 0);
    virtual void Print(void);

protected:
    uint8_t *pBase, *pNext;
    tDmaChannelId dmaChannelId;
    uint32_t uiBufferQwordSize;

private:
    // And on the third day it was proclaimed: "Thou shalt not copy dma packets!"
    // For those who lack faith, the reason is that although a copy constructor
    // might occasionally be useful, making it illegal will more often catch
    // unexpected temporaries.
    CDmaPacket(const CDmaPacket& pktToCopy);
    CDmaPacket& operator=(const CDmaPacket& pktToCopy);

    bool bDeallocateBuffer;
};

/********************************************
 * class SCDmaPacket (Source Chain dma packet)
 */

class CSCDmaPacket : public CDmaPacket {
public:
    CSCDmaPacket(uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping = Core::MemMappings::Normal);
    CSCDmaPacket(uint128_t* buffer, uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping = Core::MemMappings::Normal, bool isFull = false);
    virtual ~CSCDmaPacket(void) {}

    template <class dataType>
    inline void operator+=(const dataType data);
    template <class dataType>
    inline dataType* Add(const dataType data);
    template <class dataType>
    inline dataType* Add(const dataType* data, uint32_t num);

    inline void operator+=(const CDmaPacket& otherPkt);
    inline uint128_t* Add(const CDmaPacket& otherPkt);

    inline CSCDmaPacket& Cnt(bool irq = false, uint32_t pce = 0, bool sp = false);
    inline CSCDmaPacket& Next(const tDmaTag* nextTag, bool irq = false, bool sp = false, uint32_t pce = 0);
    inline CSCDmaPacket& Ref(const void* refData, uint32_t dataQWLen, bool irq = false, bool sp = false, uint32_t pce = 0);
    inline CSCDmaPacket& Refs(const void* refData, uint32_t dataQWLen, bool irq = false, bool sp = false, uint32_t pce = 0);
    inline CSCDmaPacket& Refe(const void* refData, uint32_t dataQWLen, bool irq = false, bool sp = false, uint32_t pce = 0);
    inline CSCDmaPacket& Call(const void* nextTag, bool irq = false, bool sp = false, uint32_t pce = 0);
    inline CSCDmaPacket& Call(const CSCDmaPacket& pkt, bool irq = false, bool sp = false, uint32_t pce = 0);
    inline CSCDmaPacket& Ret(bool irq = false, uint32_t pce = 0);
    inline CSCDmaPacket& End(bool irq = false, uint32_t pce = 0);
    inline CSCDmaPacket& CloseTag(void);

    CSCDmaPacket& Pad96(uint32_t padData);
    CSCDmaPacket& Pad128(uint32_t padData);

    virtual void Send(bool waitForEnd = false, bool flushCache = true);

    bool GetTTE(void) const { return bTTE; }
    void SetTTE(bool onOff) { bTTE = onOff; }

    bool HasOpenTag() const { return pOpenTag != NULL; }

protected:
    void SetDmaTag(tDmaTag* tag, uint32_t QWC, uint32_t PCE, uint32_t ID, uint32_t IRQ, const uint128_t* ADDR, uint32_t SPR);

    bool bTTE;
    tDmaTag* pOpenTag;
    uint32_t uiTTEBytesLeft;

private:
    // these will be passed as u32's as the QWC field, which is only 16 bits wide
    // so it should be ok to use the upper half-word
    static const uint32_t countQWC     = 1 << 16;
    static const uint32_t dontCountQWC = 1 << 17;

    inline void AddDmaTag(uint32_t QWC, uint32_t PCE, uint32_t ID, uint32_t IRQ, const uint128_t* ADDR, uint32_t SPR);

    // see the note in CDmaPacket
    CSCDmaPacket(const CSCDmaPacket& pktToCopy);
    CSCDmaPacket& operator=(const CSCDmaPacket& pktToCopy);
};

/********************************************
 * class VifSCDmaPacket
 */

class CVifSCDmaPacket : public CSCDmaPacket {
public:
    CVifSCDmaPacket(uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping = Core::MemMappings::Normal);
    CVifSCDmaPacket(uint128_t* buffer, uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping = Core::MemMappings::Normal, bool isFull = false);

    virtual ~CVifSCDmaPacket(void) {}

    inline CVifSCDmaPacket& Nop(bool irq = false);
    inline CVifSCDmaPacket& Stcycl(uint32_t wl, uint32_t cl, bool irq = false);
    inline CVifSCDmaPacket& Offset(uint32_t offset, bool irq = false);
    inline CVifSCDmaPacket& Base(uint32_t base, bool irq = false);
    inline CVifSCDmaPacket& Itop(uint32_t itops, bool irq = false);
    inline CVifSCDmaPacket& Stmod(uint32_t mode, bool irq = false);
    inline CVifSCDmaPacket& Mskpath3(uint32_t mask, bool irq = false);
    inline CVifSCDmaPacket& Mark(uint32_t value, bool irq = false);
    inline CVifSCDmaPacket& Flushe(bool irq = false);
    inline CVifSCDmaPacket& Flush(bool irq = false);
    inline CVifSCDmaPacket& Flusha(bool irq = false);
    inline CVifSCDmaPacket& Mscal(uint32_t addr, bool irq = false);
    inline CVifSCDmaPacket& Mscnt(bool irq = false);
    inline CVifSCDmaPacket& Mscalf(uint32_t addr, bool irq = false);
    inline CVifSCDmaPacket& Stmask(Vifs::tMask mask, bool irq = false);
    inline CVifSCDmaPacket& Strow(const void* rowArray, bool irq = false);
    inline CVifSCDmaPacket& Stcol(const void* colArray, bool irq = false);
    inline CVifSCDmaPacket& Mpg(uint32_t num, uint32_t addr, bool irq = false);

    inline CVifSCDmaPacket& OpenUnpack(uint32_t mode, uint32_t vuAddr, bool dblBuffered,
        bool masked = false, bool usigned = true, bool irq = false);
    CVifSCDmaPacket& CloseUnpack(void);
    inline CVifSCDmaPacket& CloseUnpack(uint32_t wl, uint32_t cl);
    inline CVifSCDmaPacket& CloseUnpack(uint32_t unpackNUM);

    inline CVifSCDmaPacket& OpenDirect(bool irq = false);
    inline CVifSCDmaPacket& CloseDirect(void);
    inline CVifSCDmaPacket& CloseDirect(uint32_t numQuads);

    inline CVifSCDmaPacket& Pad96(void);
    inline CVifSCDmaPacket& Pad128(void);

private:
    static const uint32_t Unused = 0;
    Vifs::tVifCode* pOpenVifCode;
    uint32_t uiWL, uiCL;
};

/********************************************
 * CDmaPacket inlines
 */

#define mCheckFreeSpace(__type) \
    mErrorIf(pNext + sizeof(__type) > (pBase + uiBufferQwordSize * 16), "Not enough space in packet!")
#define mCheckFreeSpaceN(__type, __num) \
    mErrorIf(pNext + (__num) * sizeof(__type) > (pBase + uiBufferQwordSize * 16), "Not enough space in packet!")

#define mCheckPktAlignment(__type)     \
    mWarnIf(sizeof(__type) == 16       \
            && (uint32_t)pNext & (16 - 1), \
        "You're trying to add 16-byte data to this packet on a non-16-byte boundary.. Are you sure this is right?")

#define mAddData(__type, __data)                \
    *reinterpret_cast<__type*>(pNext) = __data; \
    pNext += sizeof(__type);

template <class dataType>
inline void
CDmaPacket::operator+=(const dataType data)
{
    mCheckFreeSpace(dataType);
    mCheckPktAlignment(dataType);

    mAddData(dataType, data);
}

template <class dataType>
inline dataType*
CDmaPacket::Add(const dataType data)
{
    mCheckFreeSpace(dataType);
    mCheckPktAlignment(dataType);

    dataType* dataStart = reinterpret_cast<dataType*>(pNext);
    mAddData(dataType, data);
    return dataStart;
}

template <class dataType>
inline dataType*
CDmaPacket::Add(const dataType* data, uint32_t num)
{
    mCheckFreeSpaceN(dataType, num);
    mCheckPktAlignment(dataType);

    dataType* dataStart = reinterpret_cast<dataType*>(pNext);
    dataType* nextStore = dataStart;
    for (uint32_t i = 0; i < num; i++)
        *nextStore++ = *data++;
    pNext            = reinterpret_cast<uint8_t*>(nextStore);

    return dataStart;
}

inline void
CDmaPacket::operator+=(const CDmaPacket& otherPkt)
{
    uint32_t numBytes = otherPkt.GetByteLength();
    mErrorIf(numBytes & (16 - 1), "Can only add packets that are an even # of quads.");

    uint32_t numQuads = numBytes / 16;
    Add(otherPkt.GetBase(), numQuads);
}

inline uint128_t*
CDmaPacket::Add(const CDmaPacket& otherPkt)
{
    uint128_t* dataStart = reinterpret_cast<uint128_t*>(pNext);
    operator+=(otherPkt);
    return dataStart;
}

inline void
CDmaPacket::SetDmaChannel(tDmaChannelId channel)
{
    dmaChannelId = channel;
}

/*

// I'll leave this here as an example of how I could trick gcc
// into partially specializing the Add templates based on data size...

template< class dataType > inline void
CDmaPacket::operator += ( const dataType data ) {
	Helper<dataType, sizeof(dataType)>::AddSize ( *this, data );
}

template< class dataType, int byteSize >
class CDmaPacket::Helper {
	public:
		static inline void AddSize( CDmaPacket& packet, const dataType data ) {
			mErrorIf( packet.pNext+byteSize > (packet.pBase + packet.uiBufferQwordSize*16), "Not enough space in packet!" );
			mErrorIf( (uint32_t)packet.pNext & (byteSize - 1), "Free space in packet not properly aligned!" );

			*(dataType*)packet.pNext = data; packet.pNext += byteSize;
		}
};

template< class dataType >
class CDmaPacket::Helper < dataType, 16 > {
	public:
		static inline void AddSize( CDmaPacket& packet, const dataType data ) {
			mErrorIf( packet.pNext+sizeof(data) > (packet.pBase + packet.uiBufferQwordSize*16), "Not enough space in packet!" );
			mErrorIf( (uint32_t)packet.pNext & (sizeof(data) - 1), "Free space in packet not properly aligned!" );

			union { dataType realData; uint128_t qword; } uData = { data };
			*(uint128_t*)packet.pNext = uData.qword; packet.pNext += 16;
			// mAdd128( uData.qword );
		}
};
*/

#undef mCheckFreeSpace
#undef mCheckPktAlignment
#undef mAddData

/********************************************
 * CSCDmaPacket inlines
 */

// adds

#define mTemporaryTTEHandler(__dataSize)

#define mCheckTTESpaceN(__dataType, __num)                                   \
    mErrorIf((!pOpenTag) && uiTTEBytesLeft < ((__num) * sizeof(__dataType)), \
        "Don't you think you should open a dma tag before adding data?")

#define mCheckXferAddrAlign(_addr) \
    mErrorIf((uint32_t)(_addr) & (16 - 1), "I suggest you only point to qword-aligned memory..")

template <class dataType>
inline dataType*
CSCDmaPacket::Add(const dataType data)
{
    mCheckTTESpaceN(dataType, 1);
    dataType* retValue = CDmaPacket::Add<dataType>(data);
    mTemporaryTTEHandler(sizeof(dataType));
    return retValue;
}

template <class dataType>
inline void
CSCDmaPacket::operator+=(const dataType data)
{
    CSCDmaPacket::Add(data);
}

template <class dataType>
inline dataType*
CSCDmaPacket::Add(const dataType* data, uint32_t num)
{
    mCheckTTESpaceN(dataType, num);
    dataType* retValue = CDmaPacket::Add<dataType>(data, num);
    mTemporaryTTEHandler(sizeof(dataType) * num);
    return retValue;
}

#undef mTemporaryTTEHandler
#undef mCheckTTESpaceN

inline void
CSCDmaPacket::operator+=(const CDmaPacket& otherPkt)
{
    CDmaPacket::operator+=(otherPkt);
}
inline uint128_t*
CSCDmaPacket::Add(const CDmaPacket& otherPkt)
{
    return CDmaPacket::Add(otherPkt);
}

// dma tags

inline void
CSCDmaPacket::SetDmaTag(tDmaTag* tag, uint32_t QWC, uint32_t PCE, uint32_t ID, uint32_t IRQ, const uint128_t* ADDR, uint32_t SPR)
{
    tag->QWC  = QWC;
    tag->PCE  = PCE;
    tag->ID   = ID;
    tag->IRQ  = IRQ;
    tag->ADDR = (uint64_t)((uint32_t)ADDR);
    tag->SPR  = SPR;
}

inline CSCDmaPacket&
CSCDmaPacket::CloseTag(void)
{
    mErrorIf(!pOpenTag, "You called CloseTag(), but no dma tags are open!");
    mErrorIf(((uint32_t)pNext & (16 - 1)) != 0, "Packet is not qword aligned");
    // set the qwc field of any open tags.. (- 1 is so that we don't count the qword
    // containing the *pOpenTag)
    if (pOpenTag)
        pOpenTag->QWC = (((uint32_t)pNext - (uint32_t)pOpenTag) / 16 - 1);

    pOpenTag = NULL;
    return *this;
}

inline void
CSCDmaPacket::AddDmaTag(uint32_t QWC, uint32_t PCE, uint32_t ID, uint32_t IRQ, const uint128_t* ADDR, uint32_t SPR)
{
    mErrorIf(((uint32_t)pNext & 0xf) != 0, "Free space in packet is not aligned properly.");
    mErrorIf(pOpenTag, "You need to close any open dma tags before opening another!");

    if (QWC == countQWC) {
        SetDmaTag((tDmaTag*)pNext, 0, PCE, ID, IRQ, ADDR, SPR);
        pOpenTag = (tDmaTag*)pNext;
    } else {
        SetDmaTag((tDmaTag*)pNext, QWC, PCE, ID, IRQ, ADDR, SPR);
        pOpenTag = NULL;
    }

    if (bTTE) {
        pNext += 8;
        uiTTEBytesLeft = 8;
    } else
        pNext += 16;
}

#undef mCheckFreeSpaceN

#define xlateAddr(__va)

inline CSCDmaPacket&
CSCDmaPacket::Cnt(bool irq, uint32_t pce, bool sp)
{
    AddDmaTag(countQWC, pce, DMAC::kCnt, irq, 0, sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Next(const tDmaTag* nextTag, bool irq, bool sp, uint32_t pce)
{
    mCheckXferAddrAlign(nextTag);
    xlateAddr(nextTag);
    AddDmaTag(countQWC, pce, DMAC::kNext, irq, (uint128_t*)nextTag, sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Ref(const void* refData, uint32_t dataQWLen, bool irq, bool sp, uint32_t pce)
{
    mCheckXferAddrAlign(refData);
    xlateAddr(refData);
    AddDmaTag(dataQWLen, pce, DMAC::kRef, irq, (const uint128_t*)refData, sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Refs(const void* refData, uint32_t dataQWLen, bool irq, bool sp, uint32_t pce)
{
    mCheckXferAddrAlign(refData);
    xlateAddr(refData);
    AddDmaTag(dataQWLen, pce, DMAC::kRefs, irq, (const uint128_t*)refData, sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Refe(const void* refData, uint32_t dataQWLen, bool irq, bool sp, uint32_t pce)
{
    mCheckXferAddrAlign(refData);
    xlateAddr(refData);
    AddDmaTag(dataQWLen, pce, DMAC::kRefe, irq, (const uint128_t*)refData, sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Call(const void* nextTag, bool irq, bool sp, uint32_t pce)
{
    mCheckXferAddrAlign(nextTag);
    xlateAddr(nextTag);
    AddDmaTag(countQWC, pce, DMAC::kCall, irq, (uint128_t*)nextTag, sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Call(const CSCDmaPacket& pkt, bool irq, bool sp, uint32_t pce)
{
    mCheckXferAddrAlign(pkt.pBase);
    AddDmaTag(countQWC, pce, DMAC::kCall, irq,
        Core::MakePtrNormal((uint128_t*)pkt.pBase), sp);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Ret(bool irq, uint32_t pce)
{
    AddDmaTag(countQWC, pce, DMAC::kRet, irq, 0, false);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::End(bool irq, uint32_t pce)
{
    AddDmaTag(countQWC, pce, DMAC::kEnd, irq, 0, false);
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Pad96(uint32_t padData)
{
    while ((((uint32_t)pNext + 4) & 0xf) != 0)
        *this += padData;
    return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Pad128(uint32_t padData)
{
    while (((uint32_t)pNext & 0xf) != 0)
        *this += padData;
    return *this;
}

#undef mCheckFreeSpace
#undef mCheckFreeSpaceN
#undef mCheckPktAlignment
#undef mAddData

#undef mCheckXferAddrAlign

/********************************************
 * CVifSCDmaPacket inlines
 */

// VifCodes

#define mMakeVifCode(_immediate, _num, _cmd, _irq) ((uint32_t)(_immediate) | ((uint32_t)(_num) << 16) | ((uint32_t)(_cmd) << 24) | ((uint32_t)(_irq) << 31))

inline CVifSCDmaPacket&
CVifSCDmaPacket::Nop(bool irq)
{
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::nop, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stcycl(uint32_t wl, uint32_t cl, bool irq)
{
    uiWL = wl;
    uiCL = cl;
    *this += mMakeVifCode(cl | (wl << 8), Unused, Vifs::Opcodes::stcycl, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Offset(uint32_t offset, bool irq)
{
    *this += mMakeVifCode(offset, Unused, Vifs::Opcodes::offset, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Base(uint32_t base, bool irq)
{
    *this += mMakeVifCode(base, Unused, Vifs::Opcodes::base, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Itop(uint32_t itops, bool irq)
{
    *this += mMakeVifCode(itops, Unused, Vifs::Opcodes::itop, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stmod(uint32_t mode, bool irq)
{
    *this += mMakeVifCode(mode, Unused, Vifs::Opcodes::stmod, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mskpath3(uint32_t mask, bool irq)
{
    *this += mMakeVifCode(mask, Unused, Vifs::Opcodes::mskpath3, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mark(uint32_t value, bool irq)
{
    *this += mMakeVifCode(value, Unused, Vifs::Opcodes::mark, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Flushe(bool irq)
{
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::flushe, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Flush(bool irq)
{
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::flush, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Flusha(bool irq)
{
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::flusha, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mscal(uint32_t addr, bool irq)
{
    *this += mMakeVifCode(addr, Unused, Vifs::Opcodes::mscal, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mscnt(bool irq)
{
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::mscnt, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mscalf(uint32_t addr, bool irq)
{
    *this += mMakeVifCode(addr, Unused, Vifs::Opcodes::mscalf, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stmask(Vifs::tMask mask, bool irq)
{
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::stmask, irq);
    *this += mask;
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Strow(const void* rowArray, bool irq)
{
    const uint32_t* wordArray = (const uint32_t*)rowArray;
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::strow, irq);
    *this += wordArray[0];
    *this += wordArray[1];
    *this += wordArray[2];
    *this += wordArray[3];
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stcol(const void* colArray, bool irq)
{
    const uint32_t* wordArray = (const uint32_t*)colArray;
    *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::stcol, irq);
    *this += wordArray[0];
    *this += wordArray[1];
    *this += wordArray[2];
    *this += wordArray[3];
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mpg(uint32_t num, uint32_t addr, bool irq)
{
    *this += mMakeVifCode(addr, num, Vifs::Opcodes::mpg, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::OpenUnpack(uint32_t mode, uint32_t vuAddr, bool dblBuffered, bool masked, bool usigned, bool irq)
{
    mErrorIf(pOpenVifCode != NULL, "There is still another vifcode open.");
    pOpenVifCode = (Vifs::tVifCode*)pNext;
    *this += mMakeVifCode(vuAddr | ((uint32_t)usigned << 14) | ((uint32_t)dblBuffered << 15),
        0,
        mode | ((uint32_t)masked << 4) | 0x60, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseUnpack(uint32_t unpackNUM)
{
    // make sure we're u32 aligned and a vifcode is open and it's an unpack
    mAssert(((uint32_t)pNext & 0x3) == 0 && pOpenVifCode && ((pOpenVifCode->cmd & 0x60) == 0x60));
    mAssert(unpackNUM <= 256);
    pOpenVifCode->num = (unpackNUM == 256) ? 0 : unpackNUM;
    pOpenVifCode      = NULL;
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseUnpack(uint32_t wl, uint32_t cl)
{
    uiWL = wl;
    uiCL = cl;
    return CloseUnpack();
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::OpenDirect(bool irq)
{
    mAssert(pOpenVifCode == NULL);
    pOpenVifCode = (Vifs::tVifCode*)pNext;
    *this += mMakeVifCode(0, Unused, Vifs::Opcodes::direct, irq);
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseDirect(uint32_t numQuads)
{
    mAssert(pOpenVifCode != NULL && (((uint32_t)pNext - ((uint32_t)pOpenVifCode + 4)) & 0xf) == 0);
    pOpenVifCode->immediate = numQuads;
    pOpenVifCode            = NULL;
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseDirect(void)
{
    return CloseDirect(((uint32_t)pNext - ((uint32_t)pOpenVifCode + 4)) / 16);
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Pad96(void)
{
    CSCDmaPacket::Pad96(mMakeVifCode(Unused, Unused, Vifs::Opcodes::nop, false));
    return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Pad128(void)
{
    CSCDmaPacket::Pad128(mMakeVifCode(Unused, Unused, Vifs::Opcodes::nop, false));
    return *this;
}

#undef mMakeVifCode

#endif // ps2s_packet_h
