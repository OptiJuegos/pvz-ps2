#pragma once

#if defined(PS2_PLATFORM)

#include <libvux.h>

// Guard band and color helpers shared by gles_ps2.cpp and the fast draw paths.
// They live here because all PS2 render paths already include this file.
#ifndef PS2_GUARD_BAND_PX
#define PS2_GUARD_BAND_PX 256.0f
#endif

static inline float ps2_clamp_guard(float v, float extent)
{
    const float lo = -PS2_GUARD_BAND_PX;
    const float hi = extent + PS2_GUARD_BAND_PX;
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline bool ps2_tri_offscreen(float x0, float y0,
                                     float x1, float y1,
                                     float x2, float y2,
                                     float w, float h)
{
    if (x0 < 0.0f && x1 < 0.0f && x2 < 0.0f) return true;
    if (x0 > w    && x1 > w    && x2 > w)    return true;
    if (y0 < 0.0f && y1 < 0.0f && y2 < 0.0f) return true;
    if (y0 > h    && y1 > h    && y2 > h)    return true;
    return false;
}

static inline int ps2_texcol_scale(int c)
{
    if (c < 0) c = 0;
    if (c > 255) c = 255;
    // GS texture modulation uses 128 as the neutral value, not 255.
    return (c * 128 + 127) / 255;
}

#ifndef PS2_TEXCOL
#define PS2_TEXCOL(v) ps2_texcol_scale((int)(v))
#endif

#ifndef PS2_TEXCOL_GUI
#define PS2_TEXCOL_GUI(v) ps2_texcol_scale((int)(v))
#endif

static inline void ps2_vu0_xform3(const VU_MATRIX* m,
                                  const VU_VECTOR* in0,
                                  const VU_VECTOR* in1,
                                  const VU_VECTOR* in2,
                                  VU_VECTOR* out0,
                                  VU_VECTOR* out1,
                                  VU_VECTOR* out2)
{
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in0, out0);
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in1, out1);
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in2, out2);
}

static inline void ps2_vu0_xform4(const VU_MATRIX* m,
                                  const VU_VECTOR* in0,
                                  const VU_VECTOR* in1,
                                  const VU_VECTOR* in2,
                                  const VU_VECTOR* in3,
                                  VU_VECTOR* out0,
                                  VU_VECTOR* out1,
                                  VU_VECTOR* out2,
                                  VU_VECTOR* out3)
{
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in0, out0);
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in1, out1);
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in2, out2);
    Vu0ApplyMatrix((VU_MATRIX*)m, (VU_VECTOR*)in3, out3);
}

// Shared near/guard-band clipper for the PS2 GL compatibility path and the
// fast-draw backends. It clips in clip-space against:
//   w >= near, -gx*w <= x <= gx*w, -gy*w <= y <= gy*w.

static const float PS2_NEAR_CLIP_W = 0.0001f;
static const int PS2_CLIP_MAX_POLY = 16;

struct ClipVert
{
    float cx, cy, cz, cw;
    float u, v;
    unsigned char r, g, bl, a;
};

enum
{
    PS2_CLIP_NEAR  = 1 << 0,
    PS2_CLIP_LEFT  = 1 << 1,
    PS2_CLIP_RIGHT = 1 << 2,
    PS2_CLIP_DOWN  = 1 << 3,
    PS2_CLIP_UP    = 1 << 4
};

static inline int ps2_clip_outcode(float x, float y, float w, float gx, float gy)
{
    int c = 0;
    if (w < PS2_NEAR_CLIP_W) c |= PS2_CLIP_NEAR;
    if (x < -gx * w) c |= PS2_CLIP_LEFT;
    if (x >  gx * w) c |= PS2_CLIP_RIGHT;
    if (y < -gy * w) c |= PS2_CLIP_DOWN;
    if (y >  gy * w) c |= PS2_CLIP_UP;
    return c;
}

static inline ClipVert ps2_clip_lerp(const ClipVert& a, const ClipVert& b, float t)
{
    ClipVert o;
    o.cx = a.cx + (b.cx - a.cx) * t;
    o.cy = a.cy + (b.cy - a.cy) * t;
    o.cz = a.cz + (b.cz - a.cz) * t;
    o.cw = a.cw + (b.cw - a.cw) * t;
    o.u  = a.u  + (b.u  - a.u)  * t;
    o.v  = a.v  + (b.v  - a.v)  * t;
    o.r  = (unsigned char)(a.r  + (float)((int)b.r  - (int)a.r)  * t);
    o.g  = (unsigned char)(a.g  + (float)((int)b.g  - (int)a.g)  * t);
    o.bl = (unsigned char)(a.bl + (float)((int)b.bl - (int)a.bl) * t);
    o.a  = (unsigned char)(a.a  + (float)((int)b.a  - (int)a.a)  * t);
    return o;
}

static inline float ps2_clip_plane_value(const ClipVert& v, int plane, float gx, float gy)
{
    switch (plane) {
    case PS2_CLIP_NEAR:  return v.cw - PS2_NEAR_CLIP_W;
    case PS2_CLIP_LEFT:  return v.cx + gx * v.cw;
    case PS2_CLIP_RIGHT: return gx * v.cw - v.cx;
    case PS2_CLIP_DOWN:  return v.cy + gy * v.cw;
    case PS2_CLIP_UP:    return gy * v.cw - v.cy;
    default:             return 1.0f;
    }
}

static inline int ps2_clip_against_plane(const ClipVert* in, int inCount,
                                         ClipVert* out, int plane,
                                         float gx, float gy)
{
    if (inCount <= 0)
        return 0;

    int outCount = 0;
    ClipVert prev = in[inCount - 1];
    float prevVal = ps2_clip_plane_value(prev, plane, gx, gy);
    bool prevInside = prevVal >= 0.0f;

    for (int i = 0; i < inCount; ++i) {
        ClipVert cur = in[i];
        float curVal = ps2_clip_plane_value(cur, plane, gx, gy);
        bool curInside = curVal >= 0.0f;

        if (curInside != prevInside) {
            float denom = prevVal - curVal;
            float t = denom != 0.0f ? (prevVal / denom) : 0.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            if (outCount < PS2_CLIP_MAX_POLY)
                out[outCount++] = ps2_clip_lerp(prev, cur, t);
        }

        if (curInside && outCount < PS2_CLIP_MAX_POLY)
            out[outCount++] = cur;

        prev = cur;
        prevVal = curVal;
        prevInside = curInside;
    }
    return outCount;
}

static inline int ps2_clip_poly_guard(ClipVert* poly, int count, int mask, float gx, float gy)
{
    ClipVert tmpA[PS2_CLIP_MAX_POLY];
    ClipVert tmpB[PS2_CLIP_MAX_POLY];

    if (count <= 0)
        return 0;
    if (count > PS2_CLIP_MAX_POLY)
        count = PS2_CLIP_MAX_POLY;

    for (int i = 0; i < count; ++i)
        tmpA[i] = poly[i];

    ClipVert* in = tmpA;
    ClipVert* out = tmpB;
    int n = count;

    const int planes[5] = { PS2_CLIP_NEAR, PS2_CLIP_LEFT, PS2_CLIP_RIGHT, PS2_CLIP_DOWN, PS2_CLIP_UP };
    for (int p = 0; p < 5; ++p) {
        if ((mask & planes[p]) == 0)
            continue;
        n = ps2_clip_against_plane(in, n, out, planes[p], gx, gy);
        if (n <= 0)
            return 0;
        ClipVert* swap = in;
        in = out;
        out = swap;
    }

    for (int i = 0; i < n; ++i)
        poly[i] = in[i];
    return n;
}

#endif // PS2_PLATFORM
