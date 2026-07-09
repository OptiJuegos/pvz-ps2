#ifdef PS2_PLATFORM

#include <gsKit.h>
#include <gsPrimitive.h>
#include <gsTexture.h>
#include <gsMisc.h>
#include <gsCore.h>
#include <gsInline.h>
#include <libvux.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include <cstdio>

#ifdef PS2_ENABLE_FAST_DRAW
#include "Ps2FastDraw.h"
#endif
#include "Ps2ClipGuard.h"
#include "Ps2TextureConversion.h"
#include "Ps2VramAllocator.h"

#include "gles_ps2.h"

// yo soy chad lo pongo como define en vez de un .bat cual hay!!
#define PS2_ENABLE_PSMT8 1
// ---- Matrix stack ----

typedef float Mat4[16]; // column-major, OpenGL convention

static void m4_identity(Mat4 m) {
    m[0]=1;m[1]=0;m[2]=0;m[3]=0;
    m[4]=0;m[5]=1;m[6]=0;m[7]=0;
    m[8]=0;m[9]=0;m[10]=1;m[11]=0;
    m[12]=0;m[13]=0;m[14]=0;m[15]=1;
}
static void m4_copy(Mat4 dst, const Mat4 src) {
    memcpy(dst, src, 16*sizeof(float));
}
// out = a * b  (column-major)
static void m4_mul(Mat4 out, const Mat4 a, const Mat4 b) {
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a[k*4+r] * b[c*4+k];
            out[c*4+r] = s;
        }
}
// right-multiply current by t: cur = cur * t
static void m4_rmul(Mat4 cur, const Mat4 t) {
    Mat4 tmp; m4_mul(tmp, cur, t); m4_copy(cur, tmp);
}
// transform point (x,y,z,1) by m
static void m4_pt(const Mat4 m, float x, float y, float z,
                  float& ox, float& oy, float& oz, float& ow) {
    ox = m[0]*x + m[4]*y + m[8]*z  + m[12];
    oy = m[1]*x + m[5]*y + m[9]*z  + m[13];
    oz = m[2]*x + m[6]*y + m[10]*z + m[14];
    ow = m[3]*x + m[7]*y + m[11]*z + m[15];
}

// Copy Mat4 into VU_MATRIX for Vu0ApplyMatrix.
// Vu0ApplyMatrix loads vf1..vf4 as consecutive 16-byte blocks and computes
//   out.x = vf1.x*vx + vf2.x*vy + vf3.x*vz + vf4.x*vw
// With column-major Mat4: mat4[0..3]=col0, mat4[4..7]=col1, etc.
//   vf1.x = mat4[0] = M00, vf2.x = mat4[4] = M01 → out.x = M00*vx+M01*vy+... ✓
// Do NOT transpose — just memcpy.
static inline VU_MATRIX mat4_to_vu(const Mat4 m) {
    VU_MATRIX out;
    __builtin_memcpy(&out, m, 16 * sizeof(float));
    return out;
}

#define MV_STACK_DEPTH 32
static Mat4  s_proj  = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
static Mat4  s_mv[MV_STACK_DEPTH] = {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
static int   s_mv_top  = 0;
static bool  s_in_proj = false; // current glMatrixMode

static float* cur_mat() { return s_in_proj ? s_proj : s_mv[s_mv_top]; }

// gsGlobal created in main_ps2.cpp
extern GSGLOBAL* gsGlobal;

// ---- Clear color ----

static u8 s_clearR = 0, s_clearG = 0, s_clearB = 0;

void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat /*a*/) {
    auto clamp = [](GLfloat v) -> u8 {
        if (v <= 0.0f) return 0;
        if (v >= 1.0f) return 255;
        return (u8)(v * 255.0f);
    };
    s_clearR = clamp(r);
    s_clearG = clamp(g);
    s_clearB = clamp(b);
}

void glClear(GLbitfield mask) {
    if (!gsGlobal) return;
    if (mask & GL_COLOR_BUFFER_BIT)
        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(s_clearR, s_clearG, s_clearB, 0x80, 0));
}

// ---- VBO table ----

#define PS2_MAX_VBO 2048
struct PS2VBO { void* data; GLsizei size; bool valid; };
static PS2VBO   s_vbo[PS2_MAX_VBO];
static GLuint   s_bound_vbo = 0;

// Called from anGenBuffers() in gles.cpp — scans for free slots.
void ps2_gen_buffers(GLsizei n, GLuint* ids) {
    GLsizei found = 0;
    for (GLuint i = 1; i < PS2_MAX_VBO && found < n; i++) {
        if (!s_vbo[i].valid && !s_vbo[i].data) {
            s_vbo[i].valid = true;
            ids[found++] = i;
        }
    }
    if (found < n)
        printf("[PS2] ps2_gen_buffers EXHAUSTED: wanted %d, got %d (PS2_MAX_VBO=%d)\n",
               (int)n, (int)found, PS2_MAX_VBO);
    while (found < n)
        ids[found++] = 0;
}

// ---- Texture table ----

// PvZ subdivides every image into texture pieces (GLInterface TextureData), so
// the live texture count easily reaches the thousands. 256 slots ran out during
// the main menu already: glGenTextures returned id 0, the draw path then fell
// through to the untextured branch and rendered solid white/garbled quads.
#define PS2_MAX_TEX 4096

struct PS2Tex {
    GSTEXTURE gs;
    u32*      cpuMem;  // CT32 path: pixel copy for glTexSubImage2D; 64-byte aligned for GS DMA
#ifdef PS2_ENABLE_PSMT8
    u8*       cpuIdx;  // T8 path: 8-bit palette indices (this is gs.Mem at upload time)
    u32*      clut;    // T8 path: 256 CT32 entries, CSM1-swizzled, 64-byte aligned
    u16*      remap;   // T8 path: lazy 4444-key->index memo for glTexSubImage2D (0xFFFF = empty)
    u32       clutVramSize;
#endif
    u32       vramSize; // bytes reserved in GS VRAM (for the reuse free-list)
    bool      valid;
    bool      uploaded;
    bool      dirtyUpload; // glTexSubImage2D touched cpu copy; re-send on next bind
    u8        shrink;      // log2 of the internal downscale (see PS2_TEX_SHRINK_SHIFT)
};

// Textures are stored at half resolution per axis inside this shim: quarter
// the GS VRAM and quarter the CPU re-upload copy. Draw calls use normalized
// UVs, so callers never see the real dimensions; only glTexSubImage2D and the
// RecoverBits readback translate coordinates (PS2Tex::shrink). This replaces
// IMG_DOWNSCALE>1 on PS2: that halves the *logical* image dims too, which
// scrambles every hardcoded 800x600 coordinate (reanim transforms, widget
// positions) because the game has no global coordinate scaling.
#define PS2_TEX_SHRINK_SHIFT 1
static PS2Tex s_tex[PS2_MAX_TEX];

// ---- GS VRAM texture residency ----

static void ps2_tex_evict_all_textures() {
    for (int i = 1; i < PS2_MAX_TEX; i++) {
        s_tex[i].uploaded = false;
        s_tex[i].gs.Vram = 0;
        s_tex[i].vramSize = 0;
#ifdef PS2_ENABLE_PSMT8
        s_tex[i].gs.VramClut = 0;
        s_tex[i].clutVramSize = 0;
#endif
    }
    Ps2VramEvictAll();
}

// ---- GL state ----

struct PS2GL {
    // Vertex arrays
    const void* vp;  GLsizei vstride; int vsize;
    const void* tp;  GLsizei tstride;
    const void* cp;  GLsizei cstride; int csize; bool cfloat;

    // Client state
    bool ven, ten, cen;

    // Cap state
    bool tex2d, blend, cullFace;
    GLenum cullMode, frontFace;

    // Flat color (default white)
    GLubyte cr, cg, cb, ca;

    // Bound texture
    GLuint boundTex;

    // Ortho projection
    float ol, or_, ob, ot;
    bool  ortho;

    // Alpha test (cutout). When on, GS discards texels whose alpha fails the
    // comparison instead of drawing them opaque (= the black squares bug).
    bool   alphaTest;
    GLenum alphaFunc;
    u8     alphaRef;   // 0..255

    // Linear fog (software, applied per-vertex using clip-space W as depth).
    bool  fog;
    float fogStart, fogEnd;
    float fogR, fogG, fogB;
};

static PS2GL st = { nullptr,0,3, nullptr,0, nullptr,0,4,false, false,false,false, false,false,false, GL_BACK,GL_CCW, 255,255,255,255, 0, 0,1,0,1, false, false,GL_GREATER,25, false,0.0f,32.0f,0.7f,0.85f,1.0f };

// ---- Viewport (splitscreen) ----
//
// Renders into a sub-rectangle of the frame: projection maps NDC/ortho into
// viewport-relative coords (0..vpW/vpH) so all clip/offscreen/backface math
// is untouched, while the GS primitive offset (XYOFFSET, 12.4 fixed) shifts
// everything to the viewport position and the GS scissor clips strays
// (including gsKit_clear, which only wipes the scissored region).
static float s_vpW = 0.0f, s_vpH = 0.0f;     // 0 = full screen
static int   s_vpBaseOffX = -1, s_vpBaseOffY = -1;

static inline float ps2_vp_w() { return s_vpW > 0.0f ? s_vpW : (float)gsGlobal->Width; }
static inline float ps2_vp_h() { return s_vpH > 0.0f ? s_vpH : (float)gsGlobal->Height; }

extern "C" void ps2_set_viewport(int x, int y, int w, int h) {
    if (!gsGlobal) return;
    if (s_vpBaseOffX < 0) { s_vpBaseOffX = gsGlobal->OffsetX; s_vpBaseOffY = gsGlobal->OffsetY; }

    if (w <= 0 || h <= 0 ||
        (x == 0 && y == 0 && w >= (int)gsGlobal->Width && h >= (int)gsGlobal->Height)) {
        s_vpW = s_vpH = 0.0f;
        gsGlobal->OffsetX = s_vpBaseOffX;
        gsGlobal->OffsetY = s_vpBaseOffY;
        gsKit_set_scissor(gsGlobal,
            GS_SETREG_SCISSOR(0, gsGlobal->Width - 1, 0, gsGlobal->Height - 1));
        return;
    }
    s_vpW = (float)w;
    s_vpH = (float)h;
    gsGlobal->OffsetX = s_vpBaseOffX + (x << 4);
    gsGlobal->OffsetY = s_vpBaseOffY + (y << 4);
    gsKit_set_scissor(gsGlobal, GS_SETREG_SCISSOR(x, x + w - 1, y, y + h - 1));
}

// ---- Helpers ----

static inline float gsx(float x) {
    if (!st.ortho) return x;
    return (x - st.ol) / (st.or_ - st.ol) * ps2_vp_w();
}
static inline float gsy(float y) {
    if (!st.ortho) return y;
    // GL Y up → GS Y down
    return (1.0f - (y - st.ob) / (st.ot - st.ob)) * ps2_vp_h();
}
static inline u64 mkcol(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    return GS_SETREG_RGBAQ(r, g, b, (u8)(a >> 1), 0);
}
static inline GLubyte clamp_colorf(float v) {
    if (v <= 0.0f) return 0;
    if (v >= 1.0f) return 255;
    return (GLubyte)(v * 255.0f);
}
static int s_clampMode = -1;
static int s_regionUFix = -1;
static int s_regionVFix = -1;

static void ps2_set_texture_clamp_normal() {
    if (!gsGlobal || !gsGlobal->Clamp) return;
    if (s_clampMode != GS_CMODE_CLAMP) {
        gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);
        s_clampMode = GS_CMODE_CLAMP;
        s_regionUFix = s_regionVFix = -1;
    }
}

static void ps2_set_texture_clamp_for_uv(GSTEXTURE* tex,
                                         float u0, float v0,
                                         float u1, float v1,
                                         float u2, float v2) {
    if (!gsGlobal || !gsGlobal->Clamp || !tex) return;

    float minU = u0, maxU = u0;
    float minV = v0, maxV = v0;
    if (u1 < minU) minU = u1; if (u1 > maxU) maxU = u1;
    if (u2 < minU) minU = u2; if (u2 > maxU) maxU = u2;
    if (v1 < minV) minV = v1; if (v1 > maxV) maxV = v1;
    if (v2 < minV) minV = v2; if (v2 > maxV) maxV = v2;

    // Simple full-texture repeat: UV exceeds texture bounds (e.g. tiled dirt background).
    // Only applies to non-atlas textures; the 256x256 terrain atlas uses REGION_REPEAT below.
    if ((tex->Width != 256 || tex->Height != 256) &&
            (maxU > (float)tex->Width + 0.01f || maxV > (float)tex->Height + 0.01f)) {
        if (s_clampMode != GS_CMODE_REPEAT) {
            gsKit_set_clamp(gsGlobal, GS_CMODE_REPEAT);
            s_clampMode = GS_CMODE_REPEAT;
            s_regionUFix = s_regionVFix = -1;
        }
        return;
    }

    const bool wantsRegionRepeat =
        !st.ortho &&
        tex->Width == 256 && tex->Height == 256 &&
        ((maxU - minU) > 16.01f || (maxV - minV) > 16.01f);

    if (!wantsRegionRepeat) {
        ps2_set_texture_clamp_normal();
        return;
    }

    int ufix = ((int)floorf(minU)) & ~15;
    int vfix = ((int)floorf(minV)) & ~15;
    if (ufix < 0) ufix = 0; if (ufix > 240) ufix = 240;
    if (vfix < 0) vfix = 0; if (vfix > 240) vfix = 240;

    if (s_clampMode == GS_CMODE_REGION_REPEAT &&
            s_regionUFix == ufix && s_regionVFix == vfix) {
        return;
    }

    gsGlobal->Clamp->MINU = 15;
    gsGlobal->Clamp->MAXU = ufix;
    gsGlobal->Clamp->MINV = 15;
    gsGlobal->Clamp->MAXV = vfix;
    gsKit_set_clamp(gsGlobal, GS_CMODE_REGION_REPEAT);
    s_clampMode = GS_CMODE_REGION_REPEAT;
    s_regionUFix = ufix;
    s_regionVFix = vfix;
}
static const float* vp_(int i) {
    int s2 = st.vstride ? st.vstride : st.vsize * 4;
    return (const float*)((const char*)st.vp + i * s2);
}
static const float* tp_(int i) {
    int s2 = st.tstride ? st.tstride : 2 * 4;
    return (const float*)((const char*)st.tp + i * s2);
}
static void getcol(int i, GLubyte& r, GLubyte& g, GLubyte& b, GLubyte& a) {
    if (!st.cen || !st.cp) { r=st.cr; g=st.cg; b=st.cb; a=st.ca; return; }
    if (st.cfloat) {
        int s2 = st.cstride ? st.cstride : st.csize * 4;
        const float* c = (const float*)((const char*)st.cp + i * s2);
        r=clamp_colorf(c[0]); g=clamp_colorf(c[1]); b=clamp_colorf(c[2]);
        a = (st.csize>=4) ? clamp_colorf(c[3]) : 255;
    } else {
        int s2 = st.cstride ? st.cstride : st.csize;
        const GLubyte* c = (const GLubyte*)st.cp + i * s2;
        r=c[0]; g=c[1]; b=c[2]; a=(st.csize>=4)?c[3]:255;
    }
}

// ---- GL implementations ----

// Stored blend factors, updated by glBlendFunc
static GLenum s_blendSrc = GL_SRC_ALPHA;
static GLenum s_blendDst = GL_ONE_MINUS_SRC_ALPHA;
static bool s_primAlphaEnableValid = false;
static bool s_primAlphaEnable = false;
static bool s_blendAlphaValid = false;
static u64 s_blendAlphaReg = 0;
static GLenum s_blendEquationRGB = GL_FUNC_ADD;
static GLenum s_blendEquationA   = GL_FUNC_ADD;

// Extra fixed-function state. Several desktop GL calls used by Minecraft were
// previously inline no-ops in gles_ps2.h. Keeping them here prevents state leaks
// between world, GUI, inventory and transparent overlay draws.
static bool s_depthTestEnabled = true;
static bool s_depthMaskEnabled = true;
static GLenum s_depthFunc = GL_LEQUAL;
static bool s_colorMaskR = true, s_colorMaskG = true, s_colorMaskB = true, s_colorMaskA = true;
static GLenum s_shadeModel = GL_SMOOTH;

static void ps2_set_prim_alpha_enable(bool enabled) {
    if (!gsGlobal) return;
    if (s_primAlphaEnableValid && s_primAlphaEnable == enabled)
        return;
    gsGlobal->PrimAlphaEnable = enabled ? GS_SETTING_ON : GS_SETTING_OFF;
    s_primAlphaEnableValid = true;
    s_primAlphaEnable = enabled;
}

static void ps2_set_blend_alpha(u64 alphaReg) {
    if (!gsGlobal) return;
    if (s_blendAlphaValid && s_blendAlphaReg == alphaReg)
        return;
    gsKit_set_primalpha(gsGlobal, alphaReg, 0);
    s_blendAlphaValid = true;
    s_blendAlphaReg = alphaReg;
}

// Apply the current blend equation to gsGlobal
static void ps2_apply_blend() {
    if (!gsGlobal) return;
    // GS ALPHA register: (A-B)*C/128 + D
    // A=0=Cs A=1=Cd A=2=0
    // B=0=Cs B=1=Cd B=2=0
    // C=0=As C=1=Ad C=2=FIX
    // D=0=Cs D=1=Cd D=2=0
    u64 alphaReg;

    // Minecraft normally uses FUNC_ADD. Other equations are approximated so
    // code using the desktop API still has deterministic output on PS2.
    if (s_blendEquationRGB == GL_FUNC_REVERSE_SUBTRACT) {
        // Cd - Cs, approximated at full strength.
        alphaReg = GS_SETREG_ALPHA(1,0,2,2,0x80);
    } else if (s_blendEquationRGB == GL_FUNC_SUBTRACT) {
        // Cs - Cd, approximated at full strength.
        alphaReg = GS_SETREG_ALPHA(0,1,2,2,0x80);
    } else if (s_blendSrc == GL_SRC_ALPHA && s_blendDst == GL_ONE_MINUS_SRC_ALPHA) {
        // Standard alpha: (Cs-Cd)*As/128+Cd
        alphaReg = GS_SETREG_ALPHA(0,1,0,1,0);
    } else if (s_blendSrc == GL_ONE && s_blendDst == GL_ONE_MINUS_SRC_ALPHA) {
        // Pre-multiplied alpha: use the standard equation as closest match.
        alphaReg = GS_SETREG_ALPHA(0,1,0,1,0);
    } else if (s_blendSrc == GL_SRC_ALPHA && s_blendDst == GL_ONE) {
        // Additive particle/lightning style: (Cs-0)*As+Cd
        alphaReg = GS_SETREG_ALPHA(0,2,0,1,0);
    } else if (s_blendSrc == GL_ONE && s_blendDst == GL_ONE) {
        // Additive: Cs+Cd = (Cs-0)*1+Cd
        alphaReg = GS_SETREG_ALPHA(0,2,2,1,0x80);
    } else if (s_blendSrc == GL_DST_COLOR && s_blendDst == GL_SRC_COLOR) {
        // Multiplicative overlays are not directly representable; keep a sane
        // darkening-ish approximation instead of leaving stale blend state.
        alphaReg = GS_SETREG_ALPHA(1,0,2,2,0x40);
    } else if (s_blendSrc == GL_ZERO && s_blendDst == GL_ONE_MINUS_SRC_COLOR) {
        // Vignette/pumpkin overlay approximation.
        alphaReg = GS_SETREG_ALPHA(1,0,2,2,0x80);
    } else if (s_blendSrc == GL_ONE_MINUS_DST_COLOR && s_blendDst == GL_ONE_MINUS_SRC_COLOR) {
        // Crosshair/cursor invert: approximate with standard alpha.
        alphaReg = GS_SETREG_ALPHA(0,1,0,1,0);
    } else {
        // Fallback: standard alpha blend.
        alphaReg = GS_SETREG_ALPHA(0,1,0,1,0);
    }
    ps2_set_blend_alpha(alphaReg);
}

void glBlendFunc(GLenum src, GLenum dst) {
    s_blendSrc = src;
    s_blendDst = dst;
    if (st.blend)
        ps2_apply_blend();
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum, GLenum) {
    // GS uses one equation for color/alpha. Minecraft cares about the RGB part.
    glBlendFunc(srcRGB, dstRGB);
}

void glBlendEquation(GLenum mode) {
    s_blendEquationRGB = mode;
    s_blendEquationA = mode;
    if (st.blend)
        ps2_apply_blend();
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    s_blendEquationRGB = modeRGB;
    s_blendEquationA = modeAlpha;
    if (st.blend)
        ps2_apply_blend();
}

// ---- Alpha test (GS TEST register) ----
// Cached so we only emit a TEST GIF packet when the state actually changes.
static bool s_alphaTestValid = false;
static bool s_alphaTestOn    = false;
static u8   s_alphaTestAtst  = 0;
static u8   s_alphaTestAref  = 0;

// Map a GL comparison func to the GS ATST field.
static u8 ps2_gl_func_to_atst(GLenum f) {
    switch (f) {
        case GL_NEVER:    return 0; // NEVER
        case GL_LESS:     return 2; // LESS
        case GL_LEQUAL:   return 3; // LEQUAL
        case GL_EQUAL:    return 4; // EQUAL
        case GL_GEQUAL:   return 5; // GEQUAL
        case GL_GREATER:  return 6; // GREATER
        case GL_NOTEQUAL: return 7; // NOTEQUAL
        case GL_ALWAYS:
        default:          return 1; // ALWAYS
    }
}

// Push the current alpha-test state into the GS. Must run inside the draw
// frame (called from glDrawArrays), where gsKit_set_test queues the packet.
static void ps2_apply_alpha_test() {
    if (!gsGlobal || !gsGlobal->Test) return;

    u8 atst = ps2_gl_func_to_atst(st.alphaFunc);
    u8 aref = st.alphaRef;

    if (s_alphaTestValid && s_alphaTestOn == st.alphaTest
            && s_alphaTestAtst == atst && s_alphaTestAref == aref)
        return;

    if (st.alphaTest) {
        gsGlobal->Test->ATST  = atst;
        gsGlobal->Test->AREF  = aref;
        gsGlobal->Test->AFAIL = 0; // KEEP: failed texels write nothing (clean cutout)
        gsKit_set_test(gsGlobal, GS_ATEST_ON);
    } else {
        gsKit_set_test(gsGlobal, GS_ATEST_OFF);
    }

    s_alphaTestValid = true;
    s_alphaTestOn    = st.alphaTest;
    s_alphaTestAtst  = atst;
    s_alphaTestAref  = aref;
}

void glAlphaFunc(GLenum func, GLclampf ref) {
    st.alphaFunc = func;
    if (ref <= 0.0f)      st.alphaRef = 0;
    else if (ref >= 1.0f) st.alphaRef = 255;
    else                  st.alphaRef = (u8)(ref * 255.0f);
}

#define PS2_GL_FOG        0x0B60
#define PS2_GL_FOG_START  0x0B63
#define PS2_GL_FOG_END    0x0B64
#define PS2_GL_FOG_COLOR  0x0B66

void glEnable(GLenum cap) {
    if (cap == GL_TEXTURE_2D) st.tex2d = true;
    else if (cap == GL_BLEND) {
        st.blend = true;
        ps2_apply_blend();
    }
    else if (cap == GL_ALPHA_TEST) st.alphaTest = true;
    else if (cap == GL_CULL_FACE) st.cullFace = true;
    else if (cap == GL_DEPTH_TEST) s_depthTestEnabled = true;
    else if (cap == PS2_GL_FOG) st.fog = true;
    else if (cap == GL_NORMALIZE || cap == GL_NORMALIZE_EXT || cap == GL_RESCALE_NORMAL || cap == GL_RESCALE_NORMAL_EXT) {
        // Lighting normal normalization is not evaluated on the current PS2 path.
    }
}
void glDisable(GLenum cap) {
    if (cap == GL_TEXTURE_2D) st.tex2d = false;
    else if (cap == GL_BLEND) st.blend = false;
    else if (cap == GL_ALPHA_TEST) st.alphaTest = false;
    else if (cap == GL_CULL_FACE) st.cullFace = false;
    else if (cap == GL_DEPTH_TEST) s_depthTestEnabled = false;
    else if (cap == PS2_GL_FOG) st.fog = false;
    else if (cap == GL_NORMALIZE || cap == GL_NORMALIZE_EXT || cap == GL_RESCALE_NORMAL || cap == GL_RESCALE_NORMAL_EXT) {
        // No-op, but accepted.
    }
}
void glFogf(GLenum p, GLfloat v) {
    if (p == PS2_GL_FOG_START) st.fogStart = v;
    else if (p == PS2_GL_FOG_END) st.fogEnd = v;
}
void glFogfv(GLenum p, const GLfloat* v) {
    if (p == PS2_GL_FOG_COLOR) { st.fogR = v[0]; st.fogG = v[1]; st.fogB = v[2]; }
}

void glCullFace(GLenum mode) {
    if (mode == GL_FRONT || mode == GL_BACK) st.cullMode = mode;
}
void glFrontFace(GLenum mode) {
    if (mode == GL_CW || mode == GL_CCW) st.frontFace = mode;
}

void glDepthFunc(GLenum func) {
    s_depthFunc = func;
    // The PS2 path maps clip depth to reversed GS Z (near = larger value).
    // gsKit's current setup already uses the correct Z test for that mapping;
    // keep the value for push/pop and future native GS TEST tuning.
}

void glDepthMask(GLboolean flag) {
    s_depthMaskEnabled = (flag != GL_FALSE);
    // Z write masking is not exposed cleanly through the current gsKit path.
    // Store it so state is not lost; transparent PS2 draws are already sorted
    // conservatively by the Minecraft render order.
}

void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    s_colorMaskR = (r != GL_FALSE);
    s_colorMaskG = (g != GL_FALSE);
    s_colorMaskB = (b != GL_FALSE);
    s_colorMaskA = (a != GL_FALSE);
}

void glShadeModel(GLenum mode) {
    if (mode == GL_FLAT || mode == GL_SMOOTH)
        s_shadeModel = mode;
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    ps2_set_viewport((int)x, (int)y, (int)w, (int)h);
}

void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    if (!gsGlobal) return;
    if (w <= 0 || h <= 0) return;
    // OpenGL scissor uses bottom-left origin; GS uses top-left.
    int sx0 = (int)x;
    int sy0 = (int)gsGlobal->Height - ((int)y + (int)h);
    if (sx0 < 0) sx0 = 0;
    if (sy0 < 0) sy0 = 0;
    int sx1 = sx0 + (int)w - 1;
    int sy1 = sy0 + (int)h - 1;
    if (sx1 >= (int)gsGlobal->Width) sx1 = (int)gsGlobal->Width - 1;
    if (sy1 >= (int)gsGlobal->Height) sy1 = (int)gsGlobal->Height - 1;
    gsKit_set_scissor(gsGlobal, GS_SETREG_SCISSOR(sx0, sx1, sy0, sy1));
}

struct PS2AttribState {
    bool tex2d, blend, alphaTest, cullFace, fog;
    bool depthTest, depthMask;
    bool cmr, cmg, cmb, cma;
    GLenum blendSrc, blendDst, blendEqRGB, blendEqA;
    GLenum depthFunc;
    GLenum shadeModel;
    GLubyte cr, cg, cb, ca;
};
static PS2AttribState s_attribStack[16];
static int s_attribTop = 0;

void glPushAttrib(GLbitfield) {
    if (s_attribTop >= 16) return;
    PS2AttribState& a = s_attribStack[s_attribTop++];
    a.tex2d = st.tex2d; a.blend = st.blend; a.alphaTest = st.alphaTest;
    a.cullFace = st.cullFace; a.fog = st.fog;
    a.depthTest = s_depthTestEnabled; a.depthMask = s_depthMaskEnabled;
    a.cmr = s_colorMaskR; a.cmg = s_colorMaskG; a.cmb = s_colorMaskB; a.cma = s_colorMaskA;
    a.blendSrc = s_blendSrc; a.blendDst = s_blendDst;
    a.blendEqRGB = s_blendEquationRGB; a.blendEqA = s_blendEquationA;
    a.depthFunc = s_depthFunc; a.shadeModel = s_shadeModel;
    a.cr = st.cr; a.cg = st.cg; a.cb = st.cb; a.ca = st.ca;
}

void glPopAttrib() {
    if (s_attribTop <= 0) return;
    const PS2AttribState& a = s_attribStack[--s_attribTop];
    st.tex2d = a.tex2d; st.blend = a.blend; st.alphaTest = a.alphaTest;
    st.cullFace = a.cullFace; st.fog = a.fog;
    s_depthTestEnabled = a.depthTest; s_depthMaskEnabled = a.depthMask;
    s_colorMaskR = a.cmr; s_colorMaskG = a.cmg; s_colorMaskB = a.cmb; s_colorMaskA = a.cma;
    s_blendSrc = a.blendSrc; s_blendDst = a.blendDst;
    s_blendEquationRGB = a.blendEqRGB; s_blendEquationA = a.blendEqA;
    s_depthFunc = a.depthFunc; s_shadeModel = a.shadeModel;
    st.cr = a.cr; st.cg = a.cg; st.cb = a.cb; st.ca = a.ca;
    if (st.blend) {
        s_blendAlphaValid = false;
        ps2_apply_blend();
    }
}
void glEnableClientState(GLenum a) {
    if (a == GL_VERTEX_ARRAY)        st.ven = true;
    if (a == GL_TEXTURE_COORD_ARRAY) st.ten = true;
    if (a == GL_COLOR_ARRAY)         st.cen = true;
}
void glDisableClientState(GLenum a) {
    if (a == GL_VERTEX_ARRAY)        st.ven = false;
    if (a == GL_TEXTURE_COORD_ARRAY) st.ten = false;
    if (a == GL_COLOR_ARRAY)         st.cen = false;
}
static inline const void* vbo_resolve(const GLvoid* p) {
    if (s_bound_vbo != 0) {
        if (s_bound_vbo < PS2_MAX_VBO
                && s_vbo[s_bound_vbo].valid && s_vbo[s_bound_vbo].data)
            return (const char*)s_vbo[s_bound_vbo].data + (uintptr_t)p;
        return nullptr;
    }
    return p;
}
void glVertexPointer(GLint sz, GLenum, GLsizei stride, const GLvoid* p) {
    st.vp=vbo_resolve(p); st.vstride=stride; st.vsize=sz;
}
void glTexCoordPointer(GLint, GLenum, GLsizei stride, const GLvoid* p) {
    st.tp=vbo_resolve(p); st.tstride=stride;
}
void glColorPointer(GLint sz, GLenum type, GLsizei stride, const GLvoid* p) {
    st.cp=vbo_resolve(p); st.cstride=stride; st.csize=sz; st.cfloat=(type==GL_FLOAT);
}
void glBindBuffer(GLenum, GLuint id) {
    s_bound_vbo = id;
}
void glBufferData(GLenum, GLsizei size, const GLvoid* data, GLenum) {
    if (s_bound_vbo == 0 || s_bound_vbo >= PS2_MAX_VBO) return;
    PS2VBO& b = s_vbo[s_bound_vbo];
    if (b.data) { free(b.data); b.data = nullptr; }
    b.size = 0;
    b.valid = true;
    if (data && size > 0) {
        b.data = malloc((size_t)size);
        if (b.data) { memcpy(b.data, data, (size_t)size); b.size = size; }
        else printf("[PS2] glBufferData OUT OF RAM: malloc(%d) failed (vbo %u)\n",
                    (int)size, s_bound_vbo);
    }
}
void glDeleteBuffers(GLsizei n, const GLuint* ids) {
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = ids[i];
        if (id > 0 && id < PS2_MAX_VBO && s_vbo[id].valid) {
            free(s_vbo[id].data); s_vbo[id].data = nullptr;
            s_vbo[id].valid = false; s_vbo[id].size = 0;
        }
    }
}

void glGenBuffers(GLsizei n, GLuint* ids) {
    ps2_gen_buffers(n, ids);
}

void glNormalPointer(GLenum, GLsizei, const GLvoid*) {
    // Minecraft Beta submits normals for fixed-function lighting. The PS2 path
    // does not evaluate GL lighting per vertex yet, so keep this as a no-op
    // while preserving the desktop API surface.
}

void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    st.cr=clamp_colorf(r); st.cg=clamp_colorf(g);
    st.cb=clamp_colorf(b); st.ca=clamp_colorf(a);
}
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    st.cr=r; st.cg=g; st.cb=b; st.ca=a;
}
void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat, GLfloat) {
    st.ol=l; st.or_=r; st.ob=b; st.ot=t; st.ortho=true;
}
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    glOrthof((float)l,(float)r,(float)b,(float)t,(float)n,(float)f);
}

// ---- Legacy immediate mode compatibility ----

#define PS2_IMM_MAX_VERTS 4096
static GLfloat s_immV[PS2_IMM_MAX_VERTS][3];
static GLfloat s_immT[PS2_IMM_MAX_VERTS][2];
static GLubyte s_immC[PS2_IMM_MAX_VERTS][4];
static GLfloat s_immCurS = 0.0f, s_immCurT = 0.0f;
static GLenum  s_immMode = GL_TRIANGLES;
static int     s_immCount = 0;
static bool    s_immActive = false;

void glBegin(GLenum mode) {
    s_immMode = mode;
    s_immCount = 0;
    s_immActive = true;
    s_immCurS = 0.0f;
    s_immCurT = 0.0f;
}

void glTexCoord2f(GLfloat s, GLfloat t) {
    s_immCurS = s;
    s_immCurT = t;
}

void glTexCoord2i(GLint s, GLint t) {
    glTexCoord2f((GLfloat)s, (GLfloat)t);
}

static void ps2_imm_vertex(GLfloat x, GLfloat y, GLfloat z) {
    if (!s_immActive || s_immCount >= PS2_IMM_MAX_VERTS) return;
    int i = s_immCount++;
    s_immV[i][0] = x; s_immV[i][1] = y; s_immV[i][2] = z;
    s_immT[i][0] = s_immCurS; s_immT[i][1] = s_immCurT;
    s_immC[i][0] = st.cr; s_immC[i][1] = st.cg; s_immC[i][2] = st.cb; s_immC[i][3] = st.ca;
}

void glVertex2i(GLint x, GLint y) { ps2_imm_vertex((GLfloat)x, (GLfloat)y, 0.0f); }
void glVertex2f(GLfloat x, GLfloat y) { ps2_imm_vertex(x, y, 0.0f); }
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) { ps2_imm_vertex(x, y, z); }
void glVertex3d(GLdouble x, GLdouble y, GLdouble z) { ps2_imm_vertex((GLfloat)x, (GLfloat)y, (GLfloat)z); }
void glVertex2d(GLdouble x, GLdouble y) { ps2_imm_vertex((GLfloat)x, (GLfloat)y, 0.0f); }

void glEnd() {
    if (!s_immActive) return;
    s_immActive = false;
    if (s_immCount <= 0) return;

    PS2GL old = st;
    st.vp = s_immV; st.vstride = sizeof(s_immV[0]); st.vsize = 3;
    st.tp = s_immT; st.tstride = sizeof(s_immT[0]);
    st.cp = s_immC; st.cstride = sizeof(s_immC[0]); st.csize = 4; st.cfloat = false;
    st.ven = true; st.ten = true; st.cen = true;
    glDrawArrays(s_immMode, 0, s_immCount);
    st = old;
}

// ---- Matrix stack functions ----

void glMatrixMode(GLenum mode) {
    s_in_proj = (mode == GL_PROJECTION);
}
void glLoadIdentity() {
    m4_identity(cur_mat());
    if (s_in_proj) st.ortho = false; // perspective setup clears ortho flag
}
void glPushMatrix() {
    if (!s_in_proj && s_mv_top < MV_STACK_DEPTH-1) {
        m4_copy(s_mv[s_mv_top+1], s_mv[s_mv_top]);
        s_mv_top++;
    } else if (!s_in_proj) {
        printf("[PS2] glPushMatrix STACK FULL (top=%d) — matrix leak!\n", s_mv_top);
    }
}
void glPopMatrix() {
    if (!s_in_proj && s_mv_top > 0) s_mv_top--;
}
extern "C" int ps2_dbg_mv_top() { return s_mv_top; }

// Frustum culler reads these back to build its planes. A no-op stub left the
// caller's array uninitialized -> garbage frustum -> every chunk culled as
// offscreen -> black world. Return the real matrices.
void glGetFloatv(GLenum pname, GLfloat* params) {
    if (!params) return;
    if (pname == GL_PROJECTION_MATRIX)
        m4_copy(params, s_proj);
    else if (pname == GL_MODELVIEW_MATRIX)
        m4_copy(params, s_mv[s_mv_top]);
}

void glLoadMatrixf(const GLfloat* m) {
    m4_copy(cur_mat(), m);
}
void glMultMatrixf(const GLfloat* m) {
    m4_rmul(cur_mat(), m);
}
void glTranslatef(GLfloat tx, GLfloat ty, GLfloat tz) {
    Mat4 t; m4_identity(t);
    t[12]=tx; t[13]=ty; t[14]=tz;
    m4_rmul(cur_mat(), t);
}
void glScalef(GLfloat sx, GLfloat sy, GLfloat sz) {
    Mat4 s; m4_identity(s);
    s[0]=sx; s[5]=sy; s[10]=sz;
    m4_rmul(cur_mat(), s);
}
void glRotatef(GLfloat angle, GLfloat ax, GLfloat ay, GLfloat az) {
    float len = sqrtf(ax*ax + ay*ay + az*az);
    if (len < 1e-6f) return;
    ax/=len; ay/=len; az/=len;
    float rad = angle * (3.14159265f/180.0f);
    float c = cosf(rad), s = sinf(rad), ic = 1.0f-c;
    Mat4 r;
    r[0] =c+ax*ax*ic;       r[4] =ax*ay*ic-az*s;  r[8] =ax*az*ic+ay*s;  r[12]=0;
    r[1] =ay*ax*ic+az*s;    r[5] =c+ay*ay*ic;     r[9] =ay*az*ic-ax*s;  r[13]=0;
    r[2] =az*ax*ic-ay*s;    r[6] =az*ay*ic+ax*s;  r[10]=c+az*az*ic;     r[14]=0;
    r[3] =0;                r[7] =0;               r[11]=0;              r[15]=1;
    m4_rmul(cur_mat(), r);
}

void glGenTextures(GLsizei n, GLuint* ids) {
    GLsizei found = 0;
    for (GLuint i = 1; i < PS2_MAX_TEX && found < n; i++) {
        if (!s_tex[i].valid) {
            s_tex[i].valid = true; s_tex[i].uploaded = false;
            s_tex[i].cpuMem = nullptr;
            s_tex[i].dirtyUpload = false;
#ifdef PS2_ENABLE_PSMT8
            s_tex[i].cpuIdx = nullptr;
            s_tex[i].clut = nullptr;
            s_tex[i].remap = nullptr;
            s_tex[i].clutVramSize = 0;
#endif
            memset(&s_tex[i].gs, 0, sizeof(GSTEXTURE));
            ids[found++] = i;
        }
    }
    while (found < n)
        ids[found++] = 0;
}
void glBindTexture(GLenum, GLuint id) {
    st.boundTex = id;
}

static void ps2_tex_release_storage(PS2Tex& te) {
    if (te.cpuMem) { free(te.cpuMem); te.cpuMem = nullptr; }
    Ps2VramFree(te.gs.Vram, te.vramSize);
    te.vramSize = 0;
#ifdef PS2_ENABLE_PSMT8
    if (te.cpuIdx) { free(te.cpuIdx); te.cpuIdx = nullptr; }
    if (te.clut)   { free(te.clut);   te.clut = nullptr; }
    if (te.remap)  { free(te.remap);  te.remap = nullptr; }
    Ps2VramFree(te.gs.VramClut, te.clutVramSize);
    te.clutVramSize = 0;
#endif
    memset(&te.gs, 0, sizeof(GSTEXTURE));
    te.uploaded = false;
    te.dirtyUpload = false;
    te.shrink = 0;
}

void glDeleteTextures(GLsizei n, const GLuint* ids) {
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = ids[i];
        if (id > 0 && id < PS2_MAX_TEX && s_tex[id].valid) {
            ps2_tex_release_storage(s_tex[id]);
            s_tex[id].valid = false;
        }
    }
}

// Box-downscale RGBA by 2^shrink per axis, clamped at odd edges. RGB is
// alpha-weighted so the (usually black) RGB under transparent texels doesn't
// darken cutout edges into halos. Same filter as the CT32 inline path.
static void ps2_downscale_rgba(const u8* src, GLsizei w, GLsizei h, int shrink,
                               u8* dst, GLsizei tw, GLsizei th) {
    for (GLsizei dy = 0; dy < th; dy++) {
        for (GLsizei dx = 0; dx < tw; dx++) {
            int sx = dx << shrink, sy = dy << shrink;
            int x1 = sx + 1 < w ? sx + 1 : w - 1;
            int y1 = sy + 1 < h ? sy + 1 : h - 1;
            const u8* p00 = src + (sy * w + sx) * 4;
            const u8* p01 = src + (sy * w + x1) * 4;
            const u8* p10 = src + (y1 * w + sx) * 4;
            const u8* p11 = src + (y1 * w + x1) * 4;
            u32 aSum = (u32)p00[3] + p01[3] + p10[3] + p11[3];
            u8* out = dst + (dy * tw + dx) * 4;
            if (aSum == 0) {
                out[0] = p00[0]; out[1] = p00[1]; out[2] = p00[2]; out[3] = 0;
            } else {
                out[0] = (u8)(((u32)p00[0]*p00[3] + (u32)p01[0]*p01[3] + (u32)p10[0]*p10[3] + (u32)p11[0]*p11[3]) / aSum);
                out[1] = (u8)(((u32)p00[1]*p00[3] + (u32)p01[1]*p01[3] + (u32)p10[1]*p10[3] + (u32)p11[1]*p11[3]) / aSum);
                out[2] = (u8)(((u32)p00[2]*p00[3] + (u32)p01[2]*p01[3] + (u32)p10[2]*p10[3] + (u32)p11[2]*p11[3]) / aSum);
                out[3] = (u8)(aSum >> (2 * shrink));
            }
        }
    }
}

void glTexImage2D(GLenum, GLint level, GLint, GLsizei w, GLsizei h,
                  GLint, GLenum, GLenum, const GLvoid* pixels) {
    if (level != 0 || !pixels || !gsGlobal) return;
    GLuint id = st.boundTex;
    if (id == 0 || id >= PS2_MAX_TEX || !s_tex[id].valid) return;

    PS2Tex& te = s_tex[id];
    // Re-specifying an existing texture: recycle its old CPU/VRAM backing first.
    ps2_tex_release_storage(te);

    te.gs.Width  = (u32)w;
    te.gs.Height = (u32)h;
    // Bilinear: the 200x150 game canvas is magnified ~3x to the screen, so
    // NEAREST sampling shows every texel as a hard block. GS bilinear is free.
    te.gs.Filter = GS_FILTER_LINEAR;
    te.gs.Delayed = 0;

    u32 npx = (u32)(w * h);

#ifdef PS2_ENABLE_PSMT8
    // 8-bit palettized: 1 byte/pixel + 1 KB CT32 CLUT. Exact for <=256 unique
    // RGBA4444 buckets, nearest-match quantized beyond that (see
    // Ps2PalettizeT8). Stored at half resolution per axis like the CT32 path:
    // quarter the CPU re-upload copy and quarter the GS VRAM.
    const int shrink = (w >= 2 && h >= 2) ? PS2_TEX_SHRINK_SHIFT : 0;
    GLsizei tw = ((w - 1) >> shrink) + 1; // ceil(w / 2^shrink)
    GLsizei th = ((h - 1) >> shrink) + 1;
    const u8* palSrc = (const u8*)pixels;
    u8* shrunk = nullptr;
    if (shrink) {
        shrunk = (u8*)memalign(64, (u32)(tw * th) * 4);
        if (shrunk)
            ps2_downscale_rgba((const u8*)pixels, w, h, shrink, shrunk, tw, th);
    }
    if (shrunk) {
        palSrc = shrunk;
        te.shrink = (u8)shrink;
    } else {
        // Scratch alloc failed: fall back to a full-res upload.
        tw = w; th = h;
        te.shrink = 0;
    }
    te.gs.Width  = (u32)tw;
    te.gs.Height = (u32)th;
    npx = (u32)(tw * th);

    te.cpuIdx = (u8*)memalign(64, npx);
    te.clut   = (u32*)memalign(64, 256 * sizeof(u32));
    u32 paletteLin[256];
    int nPal = 0;
    if (te.cpuIdx && te.clut)
        nPal = Ps2PalettizeT8(palSrc, npx, te.cpuIdx, paletteLin);
    if (shrunk) free(shrunk);
    if (nPal <= 0) {
        printf("[PS2] glTexImage2D: palettize failed for %dx%d tex id=%u\n", w, h, id);
        if (te.cpuIdx) { free(te.cpuIdx); te.cpuIdx = nullptr; }
        if (te.clut)   { free(te.clut);   te.clut = nullptr; }
        te.valid = false;
        return;
    }
    // CSM1 wants index bits 3/4 swapped in storage; unused entries stay
    // transparent black.
    memset(te.clut, 0, 256 * sizeof(u32));
    for (int i = 0; i < nPal; i++)
        te.clut[Ps2ClutCsm1Pos(i)] = paletteLin[i];

    te.gs.PSM             = GS_PSM_T8;
    te.gs.ClutPSM         = GS_PSM_CT32;
    te.gs.Clut            = (u32*)te.clut;
    te.gs.ClutStorageMode = GS_CLUT_STORAGE_CSM1;
    gsKit_setup_tbw(&te.gs);

    // VRAM residency is deferred: the first draw that binds this texture
    // allocates VRAM and uploads it (see ps2_tex_ensure_resident).
#else
    // PSM_CT32: full 8-bit color + real alpha. PvZ leans heavily on alpha
    // gradients (font antialiasing, shadows, fades); CT16's 1-bit alpha turned
    // text into unreadable dots and 5-bit color banded the sky gradients.
    (void)npx;

    // Store at half resolution (PS2_TEX_SHRINK_SHIFT): the full-res CT32
    // working set overran the texture arena several times per frame.
    const bool bigTex = (w * h >= 512 * 512);
    if (bigTex) printf("[PS2] glTexImage2D %dx%d id=%u...\n", w, h, id);
    const int shrink = (w >= 2 && h >= 2) ? PS2_TEX_SHRINK_SHIFT : 0;
    const GLsizei tw = ((w - 1) >> shrink) + 1; // ceil(w / 2^shrink)
    const GLsizei th = ((h - 1) >> shrink) + 1;
    te.gs.Width  = (u32)tw;
    te.gs.Height = (u32)th;
    te.shrink    = (u8)shrink;

    u32* px32 = (u32*)memalign(64, (u32)(tw * th) * sizeof(u32));
    if (!px32) { te.valid = false; return; }
    {
        const u8* src = (const u8*)pixels;
        if (shrink == 0) {
            for (u32 p = 0; p < (u32)(tw * th); p++)
                px32[p] = Ps2RgbaToPsmct32(src + p * 4);
        } else {
            for (GLsizei dy = 0; dy < th; dy++) {
                for (GLsizei dx = 0; dx < tw; dx++) {
                    // 2x2 box, clamped at odd edges. RGB is alpha-weighted so
                    // the (usually black) RGB under transparent texels doesn't
                    // darken cutout edges into halos.
                    int sx = dx << shrink, sy = dy << shrink;
                    int x1 = sx + 1 < w ? sx + 1 : w - 1;
                    int y1 = sy + 1 < h ? sy + 1 : h - 1;
                    const u8* p00 = src + (sy * w + sx) * 4;
                    const u8* p01 = src + (sy * w + x1) * 4;
                    const u8* p10 = src + (y1 * w + sx) * 4;
                    const u8* p11 = src + (y1 * w + x1) * 4;
                    u32 aSum = (u32)p00[3] + p01[3] + p10[3] + p11[3];
                    u8 out[4];
                    if (aSum == 0) {
                        out[0] = p00[0]; out[1] = p00[1]; out[2] = p00[2]; out[3] = 0;
                    } else {
                        out[0] = (u8)(((u32)p00[0]*p00[3] + (u32)p01[0]*p01[3] + (u32)p10[0]*p10[3] + (u32)p11[0]*p11[3]) / aSum);
                        out[1] = (u8)(((u32)p00[1]*p00[3] + (u32)p01[1]*p01[3] + (u32)p10[1]*p10[3] + (u32)p11[1]*p11[3]) / aSum);
                        out[2] = (u8)(((u32)p00[2]*p00[3] + (u32)p01[2]*p01[3] + (u32)p10[2]*p10[3] + (u32)p11[2]*p11[3]) / aSum);
                        out[3] = (u8)(aSum >> (2 * shrink));
                    }
                    px32[dy * tw + dx] = Ps2RgbaToPsmct32(out);
                }
            }
        }
    }

    te.gs.PSM    = GS_PSM_CT32;
    te.gs.ClutStorageMode = GS_CLUT_NONE;
    te.gs.Clut = nullptr; te.gs.VramClut = 0;
    te.gs.ClutPSM = 0;

    gsKit_setup_tbw(&te.gs);

    // VRAM residency is deferred: the first draw that binds this texture
    // allocates VRAM and uploads it (see ps2_tex_ensure_resident).
    te.cpuMem = px32;
    if (bigTex) printf("[PS2] glTexImage2D id=%u shrunk to %ux%u OK\n", id, (unsigned)te.gs.Width, (unsigned)te.gs.Height);
#endif
    te.uploaded = false;
}

#ifdef PS2_ENABLE_PSMT8
static u32 ps2_argb_to_psmct32(GLuint argb)
{
    // MemoryImage palettes are 0xAARRGGBB. On the little-endian EE their byte
    // order is BGRA, matching the normal glTexImage2D source convention.
    const u8 bgra[4] = {
        (u8)(argb),
        (u8)(argb >> 8),
        (u8)(argb >> 16),
        (u8)(argb >> 24)
    };
    return Ps2RgbaToPsmct32(bgra);
}

static void ps2_copy_indexed_clut(PS2Tex& te, const GLuint* palette, GLsizei paletteCount)
{
    memset(te.clut, 0, 256 * sizeof(u32));
    if (!palette)
        return;

    if (paletteCount < 0)
        paletteCount = 0;
    if (paletteCount > 256)
        paletteCount = 256;

    for (int i = 0; i < paletteCount; i++)
        te.clut[Ps2ClutCsm1Pos(i)] = ps2_argb_to_psmct32(palette[i]);

    if (te.remap) {
        free(te.remap);
        te.remap = nullptr;
    }
}

static u16 ps2_psmct32_to_psmct16(u32 v)
{
    u16 r = (u16)(( v        & 0xFF) >> 3);
    u16 g = (u16)(((v >>  8) & 0xFF) >> 3);
    u16 b = (u16)(((v >> 16) & 0xFF) >> 3);
    u16 a = (u16)(((v >> 24) & 0xFF) >= 64 ? 1 : 0);
    return (u16)((a << 15) | (b << 10) | (g << 5) | r);
}
#endif

extern "C" void ps2_gles_tex_image_indexed(GLsizei w, GLsizei h,
                                           const GLubyte* indices, GLsizei indexPitch,
                                           const GLuint* palette, GLsizei paletteCount)
{
#ifdef PS2_ENABLE_PSMT8
    if (!indices || !palette || w <= 0 || h <= 0 || !gsGlobal) return;
    GLuint id = st.boundTex;
    if (id == 0 || id >= PS2_MAX_TEX || !s_tex[id].valid) return;

    PS2Tex& te = s_tex[id];
    ps2_tex_release_storage(te);

    const GLsizei srcPitch = (indexPitch > 0) ? indexPitch : w;
    // Same half-res storage as the RGBA path. Indices can't be averaged, so
    // nearest-sample the top-left of each block.
    const int shrink = (w >= 2 && h >= 2) ? PS2_TEX_SHRINK_SHIFT : 0;
    const GLsizei tw = ((w - 1) >> shrink) + 1;
    const GLsizei th = ((h - 1) >> shrink) + 1;
    const u32 npx = (u32)(tw * th);
    te.cpuIdx = (u8*)memalign(64, npx);
    te.clut   = (u32*)memalign(64, 256 * sizeof(u32));
    if (!te.cpuIdx || !te.clut) {
        ps2_tex_release_storage(te);
        te.valid = false;
        return;
    }

    if (shrink == 0) {
        for (int row = 0; row < h; row++)
            memcpy(te.cpuIdx + row * w, indices + row * srcPitch, w);
    } else {
        for (GLsizei dy = 0; dy < th; dy++)
            for (GLsizei dx = 0; dx < tw; dx++)
                te.cpuIdx[dy * tw + dx] = indices[(dy << shrink) * srcPitch + (dx << shrink)];
    }
    ps2_copy_indexed_clut(te, palette, paletteCount);
    te.shrink = (u8)shrink;

    te.gs.Width           = (u32)tw;
    te.gs.Height          = (u32)th;
    te.gs.PSM             = GS_PSM_T8;
    te.gs.ClutPSM         = GS_PSM_CT32;
    te.gs.Clut            = (u32*)te.clut;
    te.gs.ClutStorageMode = GS_CLUT_STORAGE_CSM1;
    te.gs.Filter          = GS_FILTER_LINEAR;
    te.gs.Delayed         = 0;
    gsKit_setup_tbw(&te.gs);
    te.uploaded = false;
    te.dirtyUpload = false;
#else
    (void)w; (void)h; (void)indices; (void)indexPitch; (void)palette; (void)paletteCount;
#endif
}

extern "C" void ps2_gles_tex_sub_image_indexed(GLint xoff, GLint yoff,
                                               GLsizei w, GLsizei h,
                                               const GLubyte* indices, GLsizei indexPitch,
                                               const GLuint* palette, GLsizei paletteCount)
{
#ifdef PS2_ENABLE_PSMT8
    if (!indices || w <= 0 || h <= 0) return;
    GLuint id = st.boundTex;
    if (id == 0 || id >= PS2_MAX_TEX || !s_tex[id].valid) return;

    PS2Tex& te = s_tex[id];
    if (!te.cpuIdx || !te.clut || te.gs.PSM != GS_PSM_T8) return;

    // Incoming coords are in full-res image space; the stored copy may be
    // shrunk (te.shrink). Translate to storage space with nearest sampling.
    const GLsizei srcPitch = (indexPitch > 0) ? indexPitch : w;
    const int s = te.shrink;
    GLint dx0 = xoff >> s, dy0 = yoff >> s;
    GLint dw = ((w - 1) >> s) + 1, dh = ((h - 1) >> s) + 1;
    if (dx0 < 0 || dy0 < 0 || dx0 >= (GLint)te.gs.Width || dy0 >= (GLint)te.gs.Height) return;
    if (dx0 + dw > (GLint)te.gs.Width)
        dw = (GLint)te.gs.Width - dx0;
    if (dy0 + dh > (GLint)te.gs.Height)
        dh = (GLint)te.gs.Height - dy0;
    if (dw <= 0 || dh <= 0) return;

    for (int row = 0; row < dh; row++)
        for (int col = 0; col < dw; col++)
            te.cpuIdx[(dy0 + row) * (int)te.gs.Width + dx0 + col] =
                indices[(row << s) * srcPitch + (col << s)];

    if (palette)
        ps2_copy_indexed_clut(te, palette, paletteCount);

    te.dirtyUpload = true;
#else
    (void)xoff; (void)yoff; (void)w; (void)h; (void)indices; (void)indexPitch; (void)palette; (void)paletteCount;
#endif
}

// Make the texture resident in GS VRAM (allocate + DMA upload) if it is not
// already, and flush a pending glTexSubImage2D re-send. If VRAM is full, the
// pending draw queue is executed and the whole texture arena is rewound; only
// a texture too large for the arena itself can still fail (returns false ->
// caller skips the draw).
static bool ps2_tex_ensure_resident(PS2Tex& te) {
    if (te.uploaded) {
        if (te.dirtyUpload) {
#ifdef PS2_ENABLE_PSMT8
            te.gs.Mem = (u32*)te.cpuIdx;
#else
            te.gs.Mem = (u32*)te.cpuMem;
#endif
            gsKit_texture_upload(gsGlobal, &te.gs);
            te.gs.Mem = nullptr;
            te.dirtyUpload = false;
        }
        return true;
    }

#ifdef PS2_ENABLE_PSMT8
    if (!te.cpuIdx || !te.clut) return false;
    u32 vsz  = gsKit_texture_size(te.gs.Width, te.gs.Height, GS_PSM_T8);
    u32 cvsz = gsKit_texture_size(16, 16, GS_PSM_CT32);
#else
    if (!te.cpuMem) return false;
    u32 vsz  = gsKit_texture_size(te.gs.Width, te.gs.Height, te.gs.PSM);
    u32 cvsz = 0;
#endif

    for (int attempt = 0; attempt < 2; attempt++) {
        // NOTE: this toolchain's patched gsKit defines GSKIT_ALLOC_ERROR as 0,
        // so "no CLUT needed" must NOT be represented as 0 and then compared
        // against the error value (that made every allocation look failed).
        u32 vram = Ps2VramAlloc(vsz);
        bool ok = (vram != GSKIT_ALLOC_ERROR);
        u32 vramClut = 0;
        if (ok && cvsz > 0) {
            vramClut = Ps2VramAlloc(cvsz);
            ok = (vramClut != GSKIT_ALLOC_ERROR);
        }
        (void)vramClut;
        if (ok) {
            te.gs.Vram = vram;
            te.vramSize = vsz;
#ifdef PS2_ENABLE_PSMT8
            te.gs.VramClut = vramClut;
            te.clutVramSize = cvsz;
            te.gs.Mem = (u32*)te.cpuIdx;
#else
            te.gs.Mem = (u32*)te.cpuMem;
#endif
            gsKit_texture_upload(gsGlobal, &te.gs);
            te.gs.Mem = nullptr;
            te.uploaded = true;
            te.dirtyUpload = false;
            return true;
        }
        if (vram != GSKIT_ALLOC_ERROR) Ps2VramFree(vram, vsz);

        if (attempt == 0) {
            // VRAM full: send everything queued so far to the GS (in-order GIF
            // consumption means those draws still see their textures), then
            // rewind the arena and retry with a clean slate.
            printf("[PS2] texture VRAM full (%ux%u) — flushing + rewinding arena\n",
                   (unsigned)te.gs.Width, (unsigned)te.gs.Height);
            if (gsGlobal->Os_Queue && gsGlobal->CurQueue == gsGlobal->Os_Queue) {
                gsKit_queue_exec(gsGlobal);
                gsKit_queue_reset(gsGlobal->Os_Queue);
            }
            ps2_tex_evict_all_textures();
        }
    }
    printf("[PS2] texture %ux%u does not fit in VRAM at all — draw skipped\n",
           (unsigned)te.gs.Width, (unsigned)te.gs.Height);
    return false;
}

#ifdef PS2_ENABLE_PSMT8
// Map one CT16 color to the texture's palette index. Lazy 64K-entry memo
// (built only for textures that actually receive glTexSubImage2D traffic —
// in practice just terrain.png's animated water/lava/fire tiles, whose
// colors come from the original texture and therefore hit the palette).
static u8 ps2_t8_remap_color(PS2Tex& te, u16 v) {
    if (!te.remap) {
        te.remap = (u16*)malloc(65536 * sizeof(u16));
        if (!te.remap) return 0;
        memset(te.remap, 0xFF, 65536 * sizeof(u16));
        for (int i = 0; i < 256; i++) {
            u16 c = ps2_psmct32_to_psmct16(te.clut[Ps2ClutCsm1Pos(i)]);
            if (te.remap[c] == 0xFFFF)
                te.remap[c] = (u16)i;
        }
    }
    u16 m = te.remap[v];
    if (m != 0xFFFF) return (u8)m;
    int best = 0, bestD = 0x7FFFFFFF;
    for (int i = 0; i < 256; i++) {
        int d = Ps2Ct16Dist(v, te.clut[Ps2ClutCsm1Pos(i)]);
        if (d < bestD) { bestD = d; best = i; }
    }
    te.remap[v] = (u16)best;
    return (u8)best;
}
#endif

void glTexParameteri(GLenum, GLenum, GLint) {
    // Texture wrap/filter state is currently baked into the gsKit texture upload
    // path. Accept the call so Java/desktop render code can run unchanged.
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    glTexParameteri(target, pname, (GLint)param);
}

void glTexSubImage2D(GLenum, GLint level, GLint xoff, GLint yoff,
                     GLsizei w, GLsizei h, GLenum, GLenum, const GLvoid* pixels) {
    if (level != 0 || !pixels) return;
    GLuint id = st.boundTex;
    if (id == 0 || id >= PS2_MAX_TEX || !s_tex[id].valid) return;
    PS2Tex& te = s_tex[id];
    const u8* src = (const u8*)pixels;
#ifdef PS2_ENABLE_PSMT8
    if (!te.cpuIdx || !te.clut) return;
    if (te.shrink == 0) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int dp = (yoff+row)*(int)te.gs.Width + (xoff+col);
                int sp = row*w + col;
                te.cpuIdx[dp] = ps2_t8_remap_color(te, Ps2RgbaToPsmct16(src + sp * 4));
            }
        }
    } else {
        // Sub-rect coords arrive in full-res image space but the stored copy
        // is shrunk: box-average the covered source texels, then palette-remap.
        const int s = te.shrink;
        int dx0 = xoff >> s, dy0 = yoff >> s;
        int dx1 = (xoff + w - 1) >> s, dy1 = (yoff + h - 1) >> s;
        if (dx1 >= (int)te.gs.Width)  dx1 = (int)te.gs.Width - 1;
        if (dy1 >= (int)te.gs.Height) dy1 = (int)te.gs.Height - 1;
        for (int dy = dy0; dy <= dy1; dy++) {
            for (int dx = dx0; dx <= dx1; dx++) {
                u32 rSum = 0, gSum = 0, bSum = 0, aSum = 0, n = 0;
                for (int oy = 0; oy < (1 << s); oy++) {
                    int sy = (dy << s) + oy - yoff;
                    if (sy < 0 || sy >= h) continue;
                    for (int ox = 0; ox < (1 << s); ox++) {
                        int sx = (dx << s) + ox - xoff;
                        if (sx < 0 || sx >= w) continue;
                        const u8* p = src + (sy * w + sx) * 4;
                        rSum += p[0]; gSum += p[1]; bSum += p[2]; aSum += p[3]; n++;
                    }
                }
                if (n == 0) continue;
                u8 out[4] = { (u8)(rSum/n), (u8)(gSum/n), (u8)(bSum/n), (u8)(aSum/n) };
                te.cpuIdx[dy * (int)te.gs.Width + dx] = ps2_t8_remap_color(te, Ps2RgbaToPsmct16(out));
            }
        }
    }
#else
    if (!te.cpuMem) return;
    if (te.shrink == 0) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int dp = (yoff+row)*(int)te.gs.Width + (xoff+col);
                int sp = row*w + col;
                te.cpuMem[dp] = Ps2RgbaToPsmct32(src + sp * 4);
            }
        }
    } else {
        // Sub-rect coords arrive in full-res image space but the stored copy
        // is shrunk: box-average the source texels that fall on each covered
        // destination texel (edges of the sub-rect may cover only part of one).
        const int s = te.shrink;
        int dx0 = xoff >> s, dy0 = yoff >> s;
        int dx1 = (xoff + w - 1) >> s, dy1 = (yoff + h - 1) >> s;
        if (dx1 >= (int)te.gs.Width)  dx1 = (int)te.gs.Width - 1;
        if (dy1 >= (int)te.gs.Height) dy1 = (int)te.gs.Height - 1;
        for (int dy = dy0; dy <= dy1; dy++) {
            for (int dx = dx0; dx <= dx1; dx++) {
                u32 rSum = 0, gSum = 0, bSum = 0, aSum = 0, n = 0;
                for (int oy = 0; oy < (1 << s); oy++) {
                    int sy = (dy << s) + oy - yoff;
                    if (sy < 0 || sy >= h) continue;
                    for (int ox = 0; ox < (1 << s); ox++) {
                        int sx = (dx << s) + ox - xoff;
                        if (sx < 0 || sx >= w) continue;
                        const u8* p = src + (sy * w + sx) * 4;
                        rSum += p[0]; gSum += p[1]; bSum += p[2]; aSum += p[3]; n++;
                    }
                }
                if (n == 0) continue;
                u8 out[4] = { (u8)(rSum/n), (u8)(gSum/n), (u8)(bSum/n), (u8)(aSum/n) };
                te.cpuMem[dy * (int)te.gs.Width + dx] = Ps2RgbaToPsmct32(out);
            }
        }
    }
#endif
    // Don't re-send the whole texture per call: terrain.png gets several
    // animated-tile updates per frame (water/lava/fire), and each upload here
    // used to DMA the full texture again. Mark dirty; the next draw that
    // binds this texture sends it once.
    te.dirtyUpload = true;
}

// Project one vertex: returns false if behind near plane
static bool project_vertex(const Mat4 mvp, const float* v,
                            float& sx, float& sy, float& sw) {
    float ox, oy, oz, ow;
    m4_pt(mvp, v[0], v[1], (st.vsize >= 3 ? v[2] : 0.0f), ox, oy, oz, ow);
    if (ow < PS2_NEAR_CLIP_W) return false;
    float inv = 1.0f / ow;
    float W = ps2_vp_w(), H = ps2_vp_h();
    sx = ps2_clamp_guard((ox * inv + 1.0f) * 0.5f * W, W);
    sy = ps2_clamp_guard((-oy * inv + 1.0f) * 0.5f * H, H); // flip Y: GL bottom=0, GS top=0
    sw = oz * inv;                        // depth (unused, ZBuffering off)
    return true;
}

static bool is_culled_3d(float x0, float y0, float x1, float y1, float x2, float y2) {
    if (!st.cullFace || st.ortho) return false;

    const float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    if (fabsf(area) < 0.0001f) return false;

    // GS screen space is Y-down, so GL's default CCW front face appears CW here.
    const bool front = (st.frontFace == GL_CCW) ? (area < 0.0f) : (area > 0.0f);
    return (st.cullMode == GL_BACK) ? !front : front;
}

#ifdef PS2_RENDER_STATS
static long s_dbg_3d_prim = 0;     // 3D prims with all verts in front of camera
static long s_dbg_3d_clip = 0;     // 3D prims dropped (a vert had w<=0)
static long s_dbg_3d_backface = 0; // 3D prims dropped by backface culling
static long s_dbg_3d_calls = 0;    // 3D glDrawArrays calls
static long s_dbg_queue_flush = 0; // mid-frame GS queue flushes (near-overflow)
#define PS2_RENDER_STAT(expr) do { expr; } while (0)
#else
#define PS2_RENDER_STAT(expr) do { } while (0)
#endif

// GS oneshot queue budget. gsKit does NOT bounds-check pool_cur: a frame
// whose GIF packets exceed the 3MB Os_Queue pool writes straight past the
// end into the C++ heap (observed: Tile vtable corruption -> crash). Before
// queueing a draw call, estimate its worst-case packet size; if it might not
// fit in the remaining pool, kick the queue to the GS now and reset it —
// the same exec+reset the frame loop does, so rendering just continues into
// the same framebuffer and nothing is dropped.
static void ps2_gs_queue_guard(int vertexCount) {
    GSQUEUE* q = gsGlobal->Os_Queue;
    if (!q || gsGlobal->CurQueue != q)
        return;
    // ~128 bytes/vertex covers the fattest path (per-triangle GIF packets +
    // clamp register writes + near-clip doubling), plus fixed slack for tags.
    unsigned int needed = 8192u + (unsigned int)vertexCount * 128u;
    unsigned int remaining = (unsigned int)((u8*)q->pool_max[q->dbuf] - (u8*)q->pool_cur);
    if (remaining > needed)
        return;
    PS2_RENDER_STAT(s_dbg_queue_flush++);
    gsKit_queue_exec(gsGlobal);
    gsKit_queue_reset(gsGlobal->Os_Queue);
}

// ClipVert / lerp_cv / outcodes / ps2_clip_poly_guard live in Ps2ClipGuard.h
// (shared with the fast-draw paths).

// When on, every projected vertex is pinned to the near-most GS Z (0xFFFF) so
// the geometry always draws on top of the world. Used for the item-in-hand:
// the GS can't clear the depth buffer mid-frame, so without this the hand
// z-clips into nearby terrain. GameRenderer brackets renderItemInHand() with
// ps2glForceNearZ(true/false).
static bool s_forceNearZ = false;
void ps2glForceNearZ(bool on) { s_forceNearZ = on; }

static void project_cv(const ClipVert& v, float W, float H,
                        float& sx, float& sy, int& sz, float& q) {
    q = 1.0f / v.cw;
    sx = ps2_clamp_guard((v.cx * q + 1.0f) * 0.5f * W, W);
    sy = ps2_clamp_guard((-v.cy * q + 1.0f) * 0.5f * H, H);
    if (s_forceNearZ) { sz = 0xFFFF; return; }
    float d = (1.0f - v.cz * q) * 0.5f;
    if (d < 0.0f) d = 0.0f; if (d > 1.0f) d = 1.0f;
    sz = (int)(d * 65535.0f);
}

#ifdef PS2_ENABLE_FAST_DRAW
// The EE generic path transforms, clips and submits every terrain triangle
// individually.  It is useful as a fallback while debugging, but cannot keep
// up with even the PS2's deliberately tiny chunk set.  The CMake feature flag
// chooses whether this code exists; when it does, make it the normal path.
static bool s_fast_draw_enabled = true;
static bool s_fast_fully_inside = false;

extern "C" void ps2_fast_draw_set_enabled(int enabled) {
    s_fast_draw_enabled = enabled != 0;
    if (!s_fast_draw_enabled)
        s_fast_fully_inside = false;
}

// Per-chunk hint from RenderList: the chunk AABB is fully inside the view
// frustum this frame, so the fast path may skip per-triangle clip outcodes.
extern "C" void ps2_fast_draw_set_fully_inside(int fullyInside) {
    s_fast_fully_inside = fullyInside != 0;
}
#endif

extern "C" void ps2_dbg_draw_dump() {
#ifdef PS2_RENDER_STATS
    printf("[PS2] 3D draw: calls=%ld  prims=%ld  clipped=%ld  backface=%ld  qflush=%ld\n",
           s_dbg_3d_calls, s_dbg_3d_prim, s_dbg_3d_clip, s_dbg_3d_backface, s_dbg_queue_flush);
    s_dbg_3d_calls = s_dbg_3d_prim = s_dbg_3d_clip = s_dbg_3d_backface = s_dbg_queue_flush = 0;
#endif
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!gsGlobal || !st.vp || count < 1) return;
    if (!s_colorMaskR && !s_colorMaskG && !s_colorMaskB && !s_colorMaskA) return;
    gsGlobal->ZBuffering = (s_depthTestEnabled && !st.ortho) ? GS_SETTING_ON : GS_SETTING_OFF;
    if (!st.ortho) PS2_RENDER_STAT(s_dbg_3d_calls++);

    ps2_gs_queue_guard(count);

    // Keep this write explicit. Startup/menu/cursor code can touch
    // gsGlobal->PrimAlphaEnable directly, so caching it here can leave stale
    // blend state and break PNG transparency in GUI draws.
    // Transparency on this GS path is driven by GS alpha blending (PrimAlpha).
    // Anything that wants per-texel transparency — explicit GL_BLEND draws AND
    // alpha-test cutouts (2D items, particles, model skin overlay, terrain) —
    // must enable it. With our 1-bit texture alpha, SRC_ALPHA blend reproduces
    // the binary cutout (A=0 -> background, A=1 -> opaque) reliably, which the
    // GS alpha-test register alone did not on real hardware (black squares).
    bool wantAlpha = st.blend || st.alphaTest;
    gsGlobal->PrimAlphaEnable = wantAlpha ? GS_SETTING_ON : GS_SETTING_OFF;
    s_primAlphaEnableValid = true;
    s_primAlphaEnable = wantAlpha;
    if (st.blend) {
        s_blendAlphaValid = false;
        ps2_apply_blend();
    } else if (st.alphaTest) {
        // Cutout-only: force standard (Cs-Cd)*As/128+Cd blend.
        s_blendAlphaValid = false;
        ps2_set_blend_alpha(GS_SETREG_ALPHA(0,1,0,1,0));
    }

    // Also drive the real GS alpha test: discarded texels write no color/Z, so
    // cutouts keep correct depth (opaque texels still blend to themselves).
    ps2_apply_alpha_test();

    GSTEXTURE* tex = nullptr;
    if (st.tex2d && st.tp && st.ten && st.boundTex > 0 && st.boundTex < PS2_MAX_TEX
            && s_tex[st.boundTex].valid) {
        PS2Tex& bte = s_tex[st.boundTex];
        // Lazily allocate VRAM + upload (also flushes deferred glTexSubImage2D).
        if (ps2_tex_ensure_resident(bte))
            tex = &bte.gs;
    }

    // A sprite/reanim draw that enabled texturing and bound a texture which
    // failed to upload (VRAM full or palettize failure) leaves tex==nullptr.
    // Falling through to the untextured path would render the geometry as a
    // solid white blob (see the `else` branches below). A missing sprite is far
    // less jarring than a white shape, so skip the whole draw instead.
    if (tex == nullptr && st.tex2d && st.ten && st.tp && st.boundTex > 0)
        return;

    // Build MVP once per draw call for 3D mode.
    // Also build row-major VU_MATRIX copy for Vu0ApplyMatrix (VU0 macro mode HW transform).
    Mat4 mvp;
    VU_MATRIX vu_mvp;
    if (!st.ortho) {
        m4_mul(mvp, s_proj, s_mv[s_mv_top]);
        vu_mvp = mat4_to_vu(mvp);
    }

    const float scrW = ps2_vp_w(), scrH = ps2_vp_h();
    const float gbx = 1.0f + 2.0f * PS2_GUARD_BAND_PX / scrW;
    const float gby = 1.0f + 2.0f * PS2_GUARD_BAND_PX / scrH;

    // Build clip-space vertex from array index i (3D only — do not call for ortho).
    auto make_cv = [&](int i) -> ClipVert {
        const float* v = vp_(i);
        VU_VECTOR vert __attribute__((aligned(16))) = {v[0], v[1], (st.vsize>=3?v[2]:0.0f), 1.0f};
        VU_VECTOR clip __attribute__((aligned(16)));
        Vu0ApplyMatrix(&vu_mvp, &vert, &clip);
        ClipVert cv;
        cv.cx = clip.x; cv.cy = clip.y; cv.cz = clip.z; cv.cw = clip.w;
        if (tex && st.tp && st.ten) { const float* t=tp_(i); cv.u=t[0]; cv.v=t[1]; }
        else { cv.u = cv.v = 0.0f; }
        getcol(i, cv.r, cv.g, cv.bl, cv.a);
        if (st.fog && st.fogEnd > st.fogStart) {
            float depth = clip.w; // eye-space Z = clip-space W
            float range = st.fogEnd - st.fogStart;
            float f = (st.fogEnd - depth) / range;
            if (f < 0.0f) f = 0.0f; else if (f > 1.0f) f = 1.0f;
            cv.r  = (u8)(f * cv.r  + (1.0f - f) * st.fogR  * 255.0f);
            cv.g  = (u8)(f * cv.g  + (1.0f - f) * st.fogG  * 255.0f);
            cv.bl = (u8)(f * cv.bl + (1.0f - f) * st.fogB  * 255.0f);
        }
        return cv;
    };

    // Clip triangle against the near + guard-band planes (outcode trivial
    // accept/reject first) and submit all resulting triangles to GS.
    auto submit_clip_tri = [&](const ClipVert tri[3]) {
        int oc0 = ps2_clip_outcode(tri[0].cx, tri[0].cy, tri[0].cw, gbx, gby);
        int oc1 = ps2_clip_outcode(tri[1].cx, tri[1].cy, tri[1].cw, gbx, gby);
        int oc2 = ps2_clip_outcode(tri[2].cx, tri[2].cy, tri[2].cw, gbx, gby);
        if (oc0 & oc1 & oc2) { PS2_RENDER_STAT(s_dbg_3d_clip++); return; }

        ClipVert poly[PS2_CLIP_MAX_POLY];
        poly[0] = tri[0]; poly[1] = tri[1]; poly[2] = tri[2];
        int n = 3;
        int mask = oc0 | oc1 | oc2;
        if (mask) {
            n = ps2_clip_poly_guard(poly, 3, mask, gbx, gby);
            if (n == 0) { PS2_RENDER_STAT(s_dbg_3d_clip++); return; }
        }
        for (int k = 1; k + 1 < n; k++) {
            float x0,y0,x1,y1,x2,y2; float q0,q1,q2; int z0,z1,z2;
            project_cv(poly[0],   scrW, scrH, x0, y0, z0, q0);
            project_cv(poly[k],   scrW, scrH, x1, y1, z1, q1);
            project_cv(poly[k+1], scrW, scrH, x2, y2, z2, q2);
            if (ps2_tri_offscreen(x0,y0,x1,y1,x2,y2,scrW,scrH)) { PS2_RENDER_STAT(s_dbg_3d_clip++); continue; }
            if (is_culled_3d(x0,y0,x1,y1,x2,y2)) { PS2_RENDER_STAT(s_dbg_3d_backface++); continue; }
            PS2_RENDER_STAT(s_dbg_3d_prim++);
            const ClipVert& c0=poly[0]; const ClipVert& c1=poly[k]; const ClipVert& c2=poly[k+1];
            if (tex) {
                ps2_set_texture_clamp_for_uv(tex,
                    c0.u*(float)tex->Width, c0.v*(float)tex->Height,
                    c1.u*(float)tex->Width, c1.v*(float)tex->Height,
                    c2.u*(float)tex->Width, c2.v*(float)tex->Height);
                GSPRIMSTQPOINT pts[3];
                pts[0].rgbaq=color_to_RGBAQ(PS2_TEXCOL(c0.r),PS2_TEXCOL(c0.g),PS2_TEXCOL(c0.bl),(u8)(c0.a>>1),q0); pts[0].stq=vertex_to_STQ(c0.u*q0,c0.v*q0); pts[0].xyz2=vertex_to_XYZ2(gsGlobal,x0,y0,z0);
                pts[1].rgbaq=color_to_RGBAQ(PS2_TEXCOL(c1.r),PS2_TEXCOL(c1.g),PS2_TEXCOL(c1.bl),(u8)(c1.a>>1),q1); pts[1].stq=vertex_to_STQ(c1.u*q1,c1.v*q1); pts[1].xyz2=vertex_to_XYZ2(gsGlobal,x1,y1,z1);
                pts[2].rgbaq=color_to_RGBAQ(PS2_TEXCOL(c2.r),PS2_TEXCOL(c2.g),PS2_TEXCOL(c2.bl),(u8)(c2.a>>1),q2); pts[2].stq=vertex_to_STQ(c2.u*q2,c2.v*q2); pts[2].xyz2=vertex_to_XYZ2(gsGlobal,x2,y2,z2);
                ps2_gs_queue_guard(3);
                gsKit_prim_list_triangle_goraud_texture_stq_3d(gsGlobal, tex, 3, pts);
            } else {
                ps2_gs_queue_guard(3);
                gsKit_prim_triangle_gouraud_3d(gsGlobal, x0,y0,z0, x1,y1,z1, x2,y2,z2,
                    mkcol(c0.r,c0.g,c0.bl,c0.a),
                    mkcol(c1.r,c1.g,c1.bl,c1.a),
                    mkcol(c2.r,c2.g,c2.bl,c2.a));
            }
        }
    };

#ifdef PS2_ENABLE_FAST_DRAW
    // forceNearZ (item-in-hand) is only handled by the generic path below, and
    // the held item is just a few tris, so fall through when it's active.
    if (s_fast_draw_enabled && mode == GL_TRIANGLES && !st.ortho && !s_forceNearZ) {
        Ps2FastDrawState fastState;
        fastState.gsGlobal = gsGlobal;
        fastState.texture = tex;
        fastState.mvp = mvp;
        fastState.vertices = st.vp;
        fastState.vertexStride = st.vstride;
        fastState.vertexSize = st.vsize;
        fastState.texCoords = st.tp;
        fastState.texCoordStride = st.tstride;
        fastState.texCoordEnabled = st.ten;
        fastState.colors = st.cp;
        fastState.colorStride = st.cstride;
        fastState.colorSize = st.csize;
        fastState.colorEnabled = st.cen;
        fastState.colorFloat = st.cfloat;
        fastState.cullFace = st.cullFace;
        fastState.frontFaceCCW = st.frontFace == GL_CCW;
        fastState.cullBackFace = st.cullMode == GL_BACK;
        fastState.flatR = st.cr;
        fastState.flatG = st.cg;
        fastState.flatB = st.cb;
        fastState.flatA = st.ca;
        fastState.first = first;
        fastState.count = count;
        fastState.ortho = st.ortho;
        fastState.fullyInside = s_fast_fully_inside;
        fastState.viewW = ps2_vp_w();
        fastState.viewH = ps2_vp_h();
#ifdef PS2_RENDER_STATS
        fastState.debugPrims = &s_dbg_3d_prim;
        fastState.debugClipped = &s_dbg_3d_clip;
        fastState.debugBackface = &s_dbg_3d_backface;
#else
        fastState.debugPrims = nullptr;
        fastState.debugClipped = nullptr;
        fastState.debugBackface = nullptr;
#endif
        fastState.setTextureClampForUv = ps2_set_texture_clamp_for_uv;
        fastState.fogEnabled = st.fog;
        fastState.fogStart   = st.fogStart;
        fastState.fogEnd     = st.fogEnd;
        fastState.fogR       = st.fogR;
        fastState.fogG       = st.fogG;
        fastState.fogB       = st.fogB;
        if (ps2_fast_draw_triangles(fastState))
            return;
    }
#endif

    // Helper: get screen coords + GS depth for vertex index i.
    // Returns false if clipped (behind camera in 3D mode).
    // GS 16-bit Z: near = larger value (ztest GEQUAL). 2D/ortho gets max Z so
    // the HUD always draws on top of the world.
    auto sv = [&](int i, float& sx, float& sy, int& sz) -> bool {
        const float* v = vp_(i);
        if (st.ortho) {
            // Ortho GUI still uses the MODELVIEW stack for translated/scaled/rotated
            // item icons and the inventory player preview. The old PS2 path ignored
            // MODELVIEW in ortho mode, so those models were submitted near 0,0 or
            // behind the inventory background.
            float ox, oy, oz, ow;
            m4_pt(s_mv[s_mv_top], v[0], v[1], (st.vsize >= 3 ? v[2] : 0.0f), ox, oy, oz, ow);
            if (fabsf(ow) > 1e-6f) { ox /= ow; oy /= ow; oz /= ow; }
            sx = gsx(ox);
            sy = gsy(oy);
            sz = 0xFFFF;
            return true;
        }
        float sw;
        if (!project_vertex(mvp, v, sx, sy, sw))
            return false;
        if (s_forceNearZ) { sz = 0xFFFF; return true; }
        // sw is NDC depth (~ -1 near .. +1 far). Map to GS Z (near = large).
        float d = (1.0f - sw) * 0.5f;
        if (d < 0.0f) d = 0.0f;
        if (d > 1.0f) d = 1.0f;
        sz = (int)(d * 65535.0f);
        return true;
    };

    // Like sv but also returns q = 1/clip_w for perspective-correct STQ texturing.
    // Uses Vu0ApplyMatrix (VU0 macro mode) for the MVP multiply — hardware accelerated.
    auto svq = [&](int i, float& sx, float& sy, int& sz, float& q) -> bool {
        const float* v = vp_(i);
        if (st.ortho) {
            float ox, oy, oz, ow;
            m4_pt(s_mv[s_mv_top], v[0], v[1], (st.vsize >= 3 ? v[2] : 0.0f), ox, oy, oz, ow);
            if (fabsf(ow) > 1e-6f) { ox /= ow; oy /= ow; oz /= ow; }
            sx = gsx(ox); sy = gsy(oy); sz = 0xFFFF; q = 1.0f;
            return true;
        }
        VU_VECTOR vert __attribute__((aligned(16))) = {v[0], v[1], (st.vsize >= 3 ? v[2] : 0.0f), 1.0f};
        VU_VECTOR clip __attribute__((aligned(16)));
        Vu0ApplyMatrix(&vu_mvp, &vert, &clip);
        float ow = clip.w;
        if (ow < PS2_NEAR_CLIP_W) return false;
        q = 1.0f / ow;
        float W = ps2_vp_w(), H = ps2_vp_h();
        sx = ps2_clamp_guard((clip.x * q + 1.0f) * 0.5f * W, W);
        sy = ps2_clamp_guard((-clip.y * q + 1.0f) * 0.5f * H, H);
        if (s_forceNearZ) { sz = 0xFFFF; return true; }
        float d = (1.0f - clip.z * q) * 0.5f;
        if (d < 0.0f) d = 0.0f;
        if (d > 1.0f) d = 1.0f;
        sz = (int)(d * 65535.0f);
        return true;
    };

    if (mode == GL_QUADS) {
        int nq = count / 4;
        for (int qi = 0; qi < nq; qi++) {
            int b = first + qi*4;
            if (!st.ortho) {
                // 3D: clip each sub-triangle independently against near plane.
                ClipVert v0=make_cv(b+0), v1=make_cv(b+1), v2=make_cv(b+2), v3=make_cv(b+3);
                ClipVert tri1[3]={v0,v1,v2}; submit_clip_tri(tri1);
                ClipVert tri2[3]={v0,v2,v3}; submit_clip_tri(tri2);
            } else {
                // Ortho 2D: no clipping, svq always succeeds.
                float x0,y0,x1,y1,x2,y2,x3,y3, q0=1,q1=1,q2=1,q3=1;
                int z0=0xFFFF,z1=0xFFFF,z2=0xFFFF,z3=0xFFFF;
                svq(b+0,x0,y0,z0,q0); svq(b+1,x1,y1,z1,q1);
                svq(b+2,x2,y2,z2,q2); svq(b+3,x3,y3,z3,q3);
                GLubyte r0,g0,b0_,a0, r1,g1,b1_,a1, r2,g2,b2_,a2, r3,g3,b3_,a3;
                getcol(b+0,r0,g0,b0_,a0); getcol(b+1,r1,g1,b1_,a1);
                getcol(b+2,r2,g2,b2_,a2); getcol(b+3,r3,g3,b3_,a3);
                if (tex) {
                    const float* t0=tp_(b+0); const float* t1=tp_(b+1);
                    const float* t2=tp_(b+2); const float* t3=tp_(b+3);
                    ps2_set_texture_clamp_for_uv(tex,
                        t0[0]*tex->Width, t0[1]*tex->Height,
                        t1[0]*tex->Width, t1[1]*tex->Height,
                        t2[0]*tex->Width, t2[1]*tex->Height);
                    GSPRIMSTQPOINT pts[6];
                    pts[0].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r0),PS2_TEXCOL_GUI(g0),PS2_TEXCOL_GUI(b0_),(u8)(a0>>1),q0); pts[0].stq=vertex_to_STQ(t0[0]*q0,t0[1]*q0); pts[0].xyz2=vertex_to_XYZ2(gsGlobal,x0,y0,z0);
                    pts[1].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r1),PS2_TEXCOL_GUI(g1),PS2_TEXCOL_GUI(b1_),(u8)(a1>>1),q1); pts[1].stq=vertex_to_STQ(t1[0]*q1,t1[1]*q1); pts[1].xyz2=vertex_to_XYZ2(gsGlobal,x1,y1,z1);
                    pts[2].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r2),PS2_TEXCOL_GUI(g2),PS2_TEXCOL_GUI(b2_),(u8)(a2>>1),q2); pts[2].stq=vertex_to_STQ(t2[0]*q2,t2[1]*q2); pts[2].xyz2=vertex_to_XYZ2(gsGlobal,x2,y2,z2);
                    pts[3].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r0),PS2_TEXCOL_GUI(g0),PS2_TEXCOL_GUI(b0_),(u8)(a0>>1),q0); pts[3].stq=vertex_to_STQ(t0[0]*q0,t0[1]*q0); pts[3].xyz2=vertex_to_XYZ2(gsGlobal,x0,y0,z0);
                    pts[4].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r2),PS2_TEXCOL_GUI(g2),PS2_TEXCOL_GUI(b2_),(u8)(a2>>1),q2); pts[4].stq=vertex_to_STQ(t2[0]*q2,t2[1]*q2); pts[4].xyz2=vertex_to_XYZ2(gsGlobal,x2,y2,z2);
                    pts[5].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r3),PS2_TEXCOL_GUI(g3),PS2_TEXCOL_GUI(b3_),(u8)(a3>>1),q3); pts[5].stq=vertex_to_STQ(t3[0]*q3,t3[1]*q3); pts[5].xyz2=vertex_to_XYZ2(gsGlobal,x3,y3,z3);
                    ps2_gs_queue_guard(6);
                    gsKit_prim_list_triangle_goraud_texture_stq_3d(gsGlobal, tex, 6, pts);
                } else {
                    ps2_gs_queue_guard(4);
                    gsKit_prim_quad_gouraud_3d(gsGlobal,
                        x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3,
                        mkcol(r0,g0,b0_,a0), mkcol(r1,g1,b1_,a1),
                        mkcol(r2,g2,b2_,a2), mkcol(r3,g3,b3_,a3));
                }
            }
        }
    } else if (mode == GL_TRIANGLES) {
        int nt = count / 3;
        for (int t = 0; t < nt; t++) {
            int b = first + t*3;
            if (!st.ortho) {
                ClipVert tri[3] = {make_cv(b+0), make_cv(b+1), make_cv(b+2)};
                submit_clip_tri(tri);
            } else {
                float x0,y0,x1,y1,x2,y2, q0=1,q1=1,q2=1;
                int z0=0xFFFF,z1=0xFFFF,z2=0xFFFF;
                svq(b+0,x0,y0,z0,q0); svq(b+1,x1,y1,z1,q1); svq(b+2,x2,y2,z2,q2);
                GLubyte r0,g0,b0_,a0, r1,g1,b1_,a1, r2,g2,b2_,a2;
                getcol(b+0,r0,g0,b0_,a0); getcol(b+1,r1,g1,b1_,a1); getcol(b+2,r2,g2,b2_,a2);
                if (tex) {
                    const float* t0=tp_(b+0); const float* t1=tp_(b+1); const float* t2=tp_(b+2);
                    ps2_set_texture_clamp_for_uv(tex,
                        t0[0]*tex->Width, t0[1]*tex->Height,
                        t1[0]*tex->Width, t1[1]*tex->Height,
                        t2[0]*tex->Width, t2[1]*tex->Height);
                    GSPRIMSTQPOINT pts[3];
                    pts[0].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r0),PS2_TEXCOL_GUI(g0),PS2_TEXCOL_GUI(b0_),(u8)(a0>>1),q0); pts[0].stq=vertex_to_STQ(t0[0]*q0,t0[1]*q0); pts[0].xyz2=vertex_to_XYZ2(gsGlobal,x0,y0,z0);
                    pts[1].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r1),PS2_TEXCOL_GUI(g1),PS2_TEXCOL_GUI(b1_),(u8)(a1>>1),q1); pts[1].stq=vertex_to_STQ(t1[0]*q1,t1[1]*q1); pts[1].xyz2=vertex_to_XYZ2(gsGlobal,x1,y1,z1);
                    pts[2].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r2),PS2_TEXCOL_GUI(g2),PS2_TEXCOL_GUI(b2_),(u8)(a2>>1),q2); pts[2].stq=vertex_to_STQ(t2[0]*q2,t2[1]*q2); pts[2].xyz2=vertex_to_XYZ2(gsGlobal,x2,y2,z2);
                    ps2_gs_queue_guard(3);
                gsKit_prim_list_triangle_goraud_texture_stq_3d(gsGlobal, tex, 3, pts);
                } else {
                    ps2_gs_queue_guard(3);
                    gsKit_prim_triangle_gouraud_3d(gsGlobal,
                        x0,y0,z0, x1,y1,z1, x2,y2,z2,
                        mkcol(r0,g0,b0_,a0), mkcol(r1,g1,b1_,a1), mkcol(r2,g2,b2_,a2));
                }
            }
        }
    } else if (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN || mode == GL_POLYGON) {
        for (int i = 0; i + 2 < count; i++) {
            int b0, b1, b2;
            if (mode == GL_TRIANGLE_STRIP) {
                b0 = first + (i & 1 ? i+1 : i);
                b1 = first + (i & 1 ? i   : i+1);
                b2 = first + i+2;
            } else { // fan / convex polygon: (0, i+1, i+2)
                b0 = first;
                b1 = first + i + 1;
                b2 = first + i + 2;
            }
            if (!st.ortho) {
                ClipVert tri[3] = {make_cv(b0), make_cv(b1), make_cv(b2)};
                submit_clip_tri(tri);
            } else {
                float x0,y0,x1,y1,x2,y2, q0=1,q1=1,q2=1;
                int z0=0xFFFF,z1=0xFFFF,z2=0xFFFF;
                svq(b0,x0,y0,z0,q0); svq(b1,x1,y1,z1,q1); svq(b2,x2,y2,z2,q2);
                GLubyte r0,g0,b0_,a0, r1,g1,b1_,a1, r2,g2,b2_,a2;
                getcol(b0,r0,g0,b0_,a0); getcol(b1,r1,g1,b1_,a1); getcol(b2,r2,g2,b2_,a2);
                if (tex) {
                    const float* t0=tp_(b0); const float* t1=tp_(b1); const float* t2=tp_(b2);
                    ps2_set_texture_clamp_for_uv(tex,
                        t0[0]*tex->Width, t0[1]*tex->Height,
                        t1[0]*tex->Width, t1[1]*tex->Height,
                        t2[0]*tex->Width, t2[1]*tex->Height);
                    GSPRIMSTQPOINT pts[3];
                    pts[0].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r0),PS2_TEXCOL_GUI(g0),PS2_TEXCOL_GUI(b0_),(u8)(a0>>1),q0); pts[0].stq=vertex_to_STQ(t0[0]*q0,t0[1]*q0); pts[0].xyz2=vertex_to_XYZ2(gsGlobal,x0,y0,z0);
                    pts[1].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r1),PS2_TEXCOL_GUI(g1),PS2_TEXCOL_GUI(b1_),(u8)(a1>>1),q1); pts[1].stq=vertex_to_STQ(t1[0]*q1,t1[1]*q1); pts[1].xyz2=vertex_to_XYZ2(gsGlobal,x1,y1,z1);
                    pts[2].rgbaq=color_to_RGBAQ(PS2_TEXCOL_GUI(r2),PS2_TEXCOL_GUI(g2),PS2_TEXCOL_GUI(b2_),(u8)(a2>>1),q2); pts[2].stq=vertex_to_STQ(t2[0]*q2,t2[1]*q2); pts[2].xyz2=vertex_to_XYZ2(gsGlobal,x2,y2,z2);
                    ps2_gs_queue_guard(3);
                gsKit_prim_list_triangle_goraud_texture_stq_3d(gsGlobal, tex, 3, pts);
                } else {
                    ps2_gs_queue_guard(3);
                    gsKit_prim_triangle_gouraud_3d(gsGlobal,
                        x0,y0,z0, x1,y1,z1, x2,y2,z2,
                        mkcol(r0,g0,b0_,a0), mkcol(r1,g1,b1_,a1), mkcol(r2,g2,b2_,a2));
                }
            }
        }
    } else if (mode == GL_LINES || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP) {
        // Untextured colored lines (Graphics::DrawLine / debug overlays).
        int step = (mode == GL_LINES) ? 2 : 1;
        for (int i = 0; i + 1 < count; i += step) {
            int b0 = first + i, b1 = first + i + 1;
            float x0,y0,x1,y1;
            int z0=0xFFFF, z1=0xFFFF;
            if (!sv(b0,x0,y0,z0) || !sv(b1,x1,y1,z1)) continue;
            GLubyte r0,g0,bb0,a0, r1,g1,bb1,a1;
            getcol(b0,r0,g0,bb0,a0); getcol(b1,r1,g1,bb1,a1);
            ps2_gs_queue_guard(2);
            gsKit_prim_line_goraud_3d(gsGlobal, x0,y0,z0, x1,y1,z1,
                mkcol(r0,g0,bb0,a0), mkcol(r1,g1,bb1,a1));
        }
        if (mode == GL_LINE_LOOP && count > 2) {
            int b0 = first + count - 1, b1 = first;
            float x0,y0,x1,y1;
            int z0=0xFFFF, z1=0xFFFF;
            if (sv(b0,x0,y0,z0) && sv(b1,x1,y1,z1)) {
                GLubyte r0,g0,bb0,a0, r1,g1,bb1,a1;
                getcol(b0,r0,g0,bb0,a0); getcol(b1,r1,g1,bb1,a1);
                ps2_gs_queue_guard(2);
                gsKit_prim_line_goraud_3d(gsGlobal, x0,y0,z0, x1,y1,z1,
                    mkcol(r0,g0,bb0,a0), mkcol(r1,g1,bb1,a1));
            }
        }
    }
}

// ---- Software cursor ----
//
// PvZ on PC relies on the OS mouse cursor; the PS2 has none, so the pad-driven
// cursor position was invisible. Draw a classic white arrow with a black
// outline directly with GS prims, in full-screen coordinates (the same space
// Input.cpp moves the cursor in), on top of everything, right before the flip.
extern "C" void ps2_draw_cursor(float x, float y) {
    if (!gsGlobal) return;

    // The 4:3 viewport shifts the GS primitive offset; compensate so the
    // cursor lands at absolute screen coordinates.
    if (s_vpBaseOffX >= 0) {
        x -= (float)((gsGlobal->OffsetX - s_vpBaseOffX) >> 4);
        y -= (float)((gsGlobal->OffsetY - s_vpBaseOffY) >> 4);
    }

    ps2_gs_queue_guard(16);
    gsGlobal->ZBuffering = GS_SETTING_OFF;
    ps2_set_prim_alpha_enable(false);

    const u64 kBlack = GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0);
    const u64 kWhite = GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0);

    // Vertical extents are scaled by the pixel aspect (the framebuffer is
    // scanned out as 4:3 whatever its dimensions) so the arrow is not
    // stretched on non-square-pixel modes like 640x224.
    const float ay = ((float)gsGlobal->Height / (float)gsGlobal->Width) * (4.0f / 3.0f);

    // Arrow: tip at (x,y), left edge down, diagonal edge to the right.
    // Slightly larger black triangle behind the white one gives the outline.
    gsKit_prim_triangle_gouraud_3d(gsGlobal,
        x - 1.5f, y - 2.0f * ay, 0xFFFF,
        x - 1.5f, y + 18.0f * ay, 0xFFFF,
        x + 13.0f, y + 12.5f * ay, 0xFFFF,
        kBlack, kBlack, kBlack);
    gsKit_prim_triangle_gouraud_3d(gsGlobal,
        x, y, 0xFFFF,
        x, y + 14.0f * ay, 0xFFFF,
        x + 10.0f, y + 10.0f * ay, 0xFFFF,
        kWhite, kWhite, kWhite);
}

// ---- Texture readback (RecoverBits) ----
//
// The GS cannot read textures back, but every texture keeps a CPU copy for
// re-upload. Reconstruct 0xAARRGGBB pixels from it (lossy: CT16 5-bit color,
// 1-bit alpha). Returns 0 if the texture has no CPU copy.
extern "C" int ps2_gles_read_texture(GLuint id, unsigned int* dst, int dstPitchPx, int w, int h) {
    if (id == 0 || id >= PS2_MAX_TEX || !s_tex[id].valid || !dst) return 0;
    PS2Tex& te = s_tex[id];
    // Caller works in full-res image space; the CPU copy may be shrunk
    // (PS2Tex::shrink) — nearest-upsample it back (lossy, like the CT16 note).
    const int s = te.shrink;
    if (w > (int)(te.gs.Width  << s)) w = (int)(te.gs.Width  << s);
    if (h > (int)(te.gs.Height << s)) h = (int)(te.gs.Height << s);

    for (int row = 0; row < h; row++) {
        unsigned int* out = dst + row * dstPitchPx;
        for (int col = 0; col < w; col++) {
#ifdef PS2_ENABLE_PSMT8
            if (!te.cpuIdx || !te.clut) return 0;
            u32 v = te.clut[Ps2ClutCsm1Pos(te.cpuIdx[(row >> s) * te.gs.Width + (col >> s)])];
            unsigned int a = (v >> 24) & 0xFF;
            a = (a >= 0x80) ? 0xFF : (a << 1);
            unsigned int b = (v >> 16) & 0xFF;
            unsigned int g = (v >>  8) & 0xFF;
            unsigned int r =  v        & 0xFF;
            out[col] = (a << 24) | (r << 16) | (g << 8) | b;
#else
            if (!te.cpuMem) return 0;
            // PSMCT32 is A(0-128)<<24 | B<<16 | G<<8 | R -> 0xAARRGGBB
            u32 v = te.cpuMem[(row >> s) * te.gs.Width + (col >> s)];
            unsigned int a = (v >> 24) & 0xFF;
            a = (a >= 0x80) ? 0xFF : (a << 1);
            unsigned int b = (v >> 16) & 0xFF;
            unsigned int g = (v >>  8) & 0xFF;
            unsigned int r =  v        & 0xFF;
            out[col] = (a << 24) | (r << 16) | (g << 8) | b;
#endif
        }
    }
    return 1;
}

#endif // PS2_PLATFORM
