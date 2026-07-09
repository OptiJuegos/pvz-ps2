/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_gsmem_h
#define ps2s_gsmem_h

/********************************************
 * includes
 */

#include <list>

#include "ps2s/gs.h"

// There are 5 possible types of slot:
// 32bit, 24bit, high 8 bit, high-low 4bit, and high-high 4bit
// 32bit textures fit into 32bit memory
// 24bit textures fit into 32bit or 24bit
// 16bit textures fit into 32bit or 24bit
// 8bit textures fit into 32bit or h8bit
// 4bit textures fit into 32bit, hl4bit, or hh4bit

/********************************************
 * CMemSlot
 */

namespace GS {

class CMemArea;
class CMemSlotList;

class CMemSlot {
    int FirstPage, PageLength;
    GS::tPSM PixFormat;
    CMemArea* BoundMemArea;
    int LastFrameUsed;
    CMemSlotList* List;
    bool Locked;

    // with all the pointers around, lets disallow copy constructing
    CMemSlot(const CMemSlot& rhs);

public:
    CMemSlot(int firstPage, int pageLength, GS::tPSM pixFormat)
        : FirstPage(firstPage)
        , PageLength(pageLength)
        , PixFormat(pixFormat)
        , BoundMemArea(NULL)
        , LastFrameUsed(0)
        , List(NULL)
        , Locked(false)
    {
    }
    ~CMemSlot();

    void SetOwningList(CMemSlotList* slotList) { List = slotList; }

    int GetLastFrameUsed() const { return LastFrameUsed; }
    inline void RecordAccess(int curFrame);

    int GetFirstPage() const { return FirstPage; }
    int GetPageLength() const { return PageLength; }
    GS::tPSM GetPixFormat() const { return PixFormat; }

    void Bind(CMemArea& memArea, int curFrame);
    void Unbind();
    bool IsBound() const { return (BoundMemArea != NULL); }

    inline void Lock();
    inline void Unlock();
    bool IsLocked() const { return Locked; }

    void Print();
};

/********************************************
 * CMemSlotList
 */

class CMemSlotList {
    std::list<CMemSlot*> Slots;
    int PageLength;
    GS::tPSM PixFormat;
    typedef std::list<CMemSlot*>::iterator tSlotIter;

public:
    CMemSlotList(int pageLength, GS::tPSM pixFormat)
        : PageLength(pageLength)
        , PixFormat(pixFormat)
    {
    }
    ~CMemSlotList();

    bool HasSameTypeAs(const CMemSlot& rhs)
    {
        return PageLength == rhs.GetPageLength() && PixFormat == rhs.GetPixFormat();
    }

    void AddSlot(CMemSlot* newSlot)
    {
        Slots.push_back(newSlot);
        newSlot->SetOwningList(this);
    }
    void RemoveSlot(CMemSlot* slot);

    void MakeSlotLRU(CMemSlot* slot);
    void MakeSlotMRU(CMemSlot* slot);

    GS::tPSM GetPixFormat() const { return PixFormat; }
    int GetPageLength() const { return PageLength; }

    CMemSlot* GetLRUSlot() const { return Slots.back(); }

    void RemoveAllSlots();

    void PrintSlots();
};

// needs to be after the definition of CMemSlotList
void CMemSlot::RecordAccess(int curFrame)
{
    LastFrameUsed = curFrame;
    List->MakeSlotMRU(this);
}

/********************************************
 * CMemManager
 */

class CMemArea;
class CMemManager {
    std::list<CMemSlotList*> SlotLists;
    CMemSlotList LockedSlots;
    int CurFrame;

    typedef std::list<CMemSlotList*>::iterator tSlotListIter;

    CMemSlotList* FindSlotListOfType(const CMemSlot& slot);

    void Alloc4(CMemArea& memArea);
    void Alloc8(CMemArea& memArea);
    void Alloc16(CMemArea& memArea);
    void Alloc24(CMemArea& memArea);
    void Alloc32(CMemArea& memArea);

    int GetFreePriority(CMemSlot& slot, int areaPageLength);
    CMemSlot* FindLRUSlot(GS::tPSM pixFormat, int pageLength);

public:
    CMemManager()
        : LockedSlots(0, (GS::tPSM)-1)
        , CurFrame(0)
    {
    }
    ~CMemManager();

    CMemSlot* AddSlot(int firstPage, int pageLength, GS::tPSM pixFormat);
    void AddSlotList(CMemSlotList* newList);

    void AddLockedSlot(CMemSlot* slot) { LockedSlots.AddSlot(slot); }
    void RemoveLockedSlot(CMemSlot* slot)
    {
        LockedSlots.RemoveSlot(slot);
        CMemSlotList* slotList = FindSlotListOfType(*slot);
        slotList->AddSlot(slot);
    }

    void Alloc(CMemArea& memArea);
    void Free(CMemArea& memArea);

    void RemoveAllSlots();

    int GetCurFrame() const { return CurFrame; }
    void SetCurFrame(int frame) { CurFrame = frame + 1; }

    void PrintAllocation();
};

/********************************************
 * CMemArea
 */

typedef enum { kAlignBlock,
    kAlignPage } tMemAlignment;

class CMemArea {
    int Width, Height, PageLength;
    CMemSlot* Slot;
    unsigned int GSWordAddr;
    GS::tPSM PixFormat;
    tMemAlignment Alignment;

    void XformDimensions(int* width, int* height, GS::tPSM pixFormat);

    bool IsResident() const { return (Slot != NULL); }

protected:
    friend void CMemSlot::Lock();
    friend void CMemSlot::Unlock();
    // this should probably just be in the GS:: namespace and called directly
    // for Alloc and Free
    static CMemManager* MemManager;

public:
    CMemArea(int width, int height,
        GS::tPSM pixFormat,
        tMemAlignment alignment = kAlignPage);
    ~CMemArea();

    // I'd rather have MemManager be a static member, but for compatibility
    // with CodeWarrior it has to be this way..
    static void Init() { MemManager = new CMemManager; }
    static void Finish() { delete MemManager; }

    void Alloc();
    void Free();

    void Lock()
    {
        mErrorIf(Slot == NULL, "A MemArea must be allocated before it can be locked.");
        Slot->Lock();
    }
    void Unlock()
    {
        mAssert(Slot != NULL);
        mErrorIf(!Slot->IsLocked(),
            "This MemArea is not locked; how do you expect me to unlock it?!");
        Slot->Unlock();
    }
    bool IsLocked() const
    {
        mAssert(Slot != NULL);
        return Slot->IsLocked();
    }

    bool IsAllocated()
    {
        bool isRes = IsResident();
        if (isRes)
            Slot->RecordAccess(MemManager->GetCurFrame());
        return isRes;
    }

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    GS::tPSM GetPixFormat() const { return PixFormat; }
    int GetPageLength() const { return PageLength; }
    unsigned int GetWordAddr() const { return GSWordAddr; }

    // for debugging and compatibility
    void SetWordAddr(unsigned int addr) { GSWordAddr = addr; }

    void Bind(CMemSlot& slot);
    void Unbind();

    static CMemManager& GetMemManager() { return *MemManager; }
};

// needs to be after CMemArea
void CMemSlot::Lock()
{
    Locked = true;
    List->RemoveSlot(this);
    CMemArea::MemManager->AddLockedSlot(this);
}

void CMemSlot::Unlock()
{
    Locked = false;
    CMemArea::MemManager->RemoveLockedSlot(this);
}

} // namespace GS

#endif // ps2s_gsmem_h
