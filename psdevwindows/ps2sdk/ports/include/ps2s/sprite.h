/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_sprite_h
#define ps2s_sprite_h

/********************************************
 * includes
 */

#include "ps2s/core.h"
#include "ps2s/dmac.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"

/********************************************
 * typedefs
 */

typedef union {
    uint32_t uv[4];
    float stq[4];
} tTexCoords;

/********************************************
 * class def
 */

class CSprite {
public:
    CSprite(GS::tContext context, uint32_t minX, uint32_t minY, uint32_t width, uint32_t height);

    void Draw(bool waitForEnd = false, bool flushCache = true)
    {
        GifPacket.Send(waitForEnd, flushCache);
    }
    void Draw(CSCDmaPacket& packet);
    void Draw(CVifSCDmaPacket& packet);

    void SetColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a, uint8_t fog = 255)
    {
        Color[0]   = r;
        Color[1]   = g;
        Color[2]   = b;
        Color[3]   = a;
        Vertex2[3] = (uint32_t)fog << 4;
    }
    void SetColor(uint32_t rgba)
    {
        Color[0] = 0xff & rgba;
        Color[1] = (0xff00 & rgba) >> 8;
        Color[2] = (0xff0000 & rgba) >> 16;
        Color[3] = (0xff000000 & rgba) >> 24;
    }

    void SetDepth(uint32_t depth) { Vertex1[2] = Vertex2[2] = depth; }

    void SetSTQs(float minS, float minT, float width, float height);

    void SetUVs(float minU, float minV, float width, float height);
    void SetUVs(uint16_t minU, uint16_t minV, uint16_t width, uint16_t height);
    void SetUVs(int32_t minU, int32_t minV, int32_t width, int32_t height)
    {
        SetUVs((uint16_t)minU, (uint16_t)minV, (uint16_t)width, (uint16_t)height);
    }
    void SetUVsFix4(uint16_t minU, uint16_t minV, uint16_t maxU, uint16_t maxV);

    void SetVertices(uint32_t minX, uint32_t minY, uint32_t width, uint32_t height)
    {
        Vertex1[0] = minX << 4;
        Vertex1[1] = minY << 4;
        Vertex2[0] = (minX + width) << 4;
        Vertex2[1] = (minY + height) << 4;
    }

    inline CSprite& SetUseAlphaBlend(bool useAlpha);
    inline CSprite& SetUseTexture(bool useTex);
    inline CSprite& SetUseFog(bool useFog);

    CDmaPacket& GetPacket(void) { return GifPacket; }

    inline void* operator new(size_t size) { return Core::New16(size); }
    inline void operator delete(void* p) { Core::Delete16(p); }

protected:
private:
    struct {
        // GIF tag + 5 qwords data
        tGifTag DrawGifTag;
        uint32_t Color[4];
        tTexCoords TexCoords1;
        uint32_t Vertex1[4];
        tTexCoords TexCoords2;
        uint32_t Vertex2[4];
    } __attribute__((packed,aligned(16)));

    CDmaPacket GifPacket;

};

/********************************************
 * inline methods
 */

inline CSprite&
CSprite::SetUseAlphaBlend(bool useAlpha)
{
    if (useAlpha)
        DrawGifTag.PRIM |= (uint64_t)1 << 6; // abe
    else
        DrawGifTag.PRIM &= ~((uint64_t)1 << 6);

    return *this;
}

inline CSprite&
CSprite::SetUseTexture(bool useTex)
{
    if (useTex)
        DrawGifTag.PRIM |= (uint64_t)1 << 4; // tme
    else
        DrawGifTag.PRIM &= ~((uint64_t)1 << 4);

    return *this;
}

inline CSprite&
CSprite::SetUseFog(bool useFog)
{
    if (useFog)
        DrawGifTag.PRIM |= (uint64_t)1 << 5; // fge
    else
        DrawGifTag.PRIM &= ~((uint64_t)1 << 5);

    return *this;
}

#endif // ps2s_sprite_h
