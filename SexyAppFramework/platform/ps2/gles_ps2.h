// gles_ps2.h — GL type definitions, constants, and function declarations for PS2.
// Implementations are in gles_ps2.cpp (linked into every PS2 build).
#pragma once
#ifdef PS2_PLATFORM

// ---------------------------------------------------------------------------
// GL base types
// ---------------------------------------------------------------------------
typedef unsigned int    GLenum;
typedef unsigned int    GLbitfield;
typedef unsigned char   GLboolean;
typedef int             GLint;
typedef int             GLsizei;
typedef unsigned char   GLubyte;
typedef unsigned int    GLuint;
typedef float           GLfloat;
typedef float           GLclampf;
typedef double          GLdouble;
typedef void            GLvoid;

// ---------------------------------------------------------------------------
// GL constants
// ---------------------------------------------------------------------------

// Primitives
#define GL_POINTS              0x0000
#define GL_LINES               0x0001
#define GL_LINE_LOOP           0x0002
#define GL_LINE_STRIP          0x0003
#define GL_TRIANGLES           0x0004
#define GL_TRIANGLE_STRIP      0x0005
#define GL_TRIANGLE_FAN        0x0006
#define GL_QUADS               0x0007
#define GL_QUAD_STRIP          0x0008
#define GL_POLYGON             0x0009

// Data types
#define GL_BYTE                0x1400
#define GL_UNSIGNED_BYTE       0x1401
#define GL_SHORT               0x1402
#define GL_UNSIGNED_SHORT      0x1403
#define GL_INT                 0x1404
#define GL_UNSIGNED_INT        0x1405
#define GL_FLOAT               0x1406
#define GL_DOUBLE              0x140A

// Matrix modes
#define GL_MODELVIEW           0x1700
#define GL_PROJECTION          0x1701
#define GL_TEXTURE             0x1702

// Get target
#define GL_MODELVIEW_MATRIX    0x0BA6
#define GL_PROJECTION_MATRIX   0x0BA7
#define GL_VIEWPORT            0x0BA2
#define GL_MAX_TEXTURE_SIZE    0x0D33
#define GL_NORMALIZE           0x0BA1
#define GL_RESCALE_NORMAL      0x803A
#define GL_RESCALE_NORMAL_EXT  0x803A

// Alpha / blend comparison
#define GL_NEVER               0x0200
#define GL_LESS                0x0201
#define GL_EQUAL               0x0202
#define GL_LEQUAL              0x0203
#define GL_GREATER             0x0204
#define GL_NOTEQUAL            0x0205
#define GL_GEQUAL              0x0206
#define GL_ALWAYS              0x0207

// Blend factors
#define GL_ZERO                0x0000
#define GL_ONE                 0x0001
#define GL_SRC_COLOR           0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA           0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA           0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR           0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE  0x0308

// Blend equations / extended blend helpers
#define GL_FUNC_ADD            0x8006
#define GL_MIN                 0x8007
#define GL_MAX                 0x8008
#define GL_FUNC_SUBTRACT       0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_CONSTANT_COLOR      0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA      0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004

// Enables
#define GL_FOG                 0x0B60
#define GL_LIGHTING            0x0B50
#define GL_DEPTH_TEST          0x0B71
#define GL_STENCIL_TEST        0x0B90
#define GL_NORMALIZE_EXT       0x0BA1
#define GL_ALPHA_TEST          0x0BC0
#define GL_BLEND               0x0BE2
#define GL_DITHER              0x0BD0
#define GL_COLOR_LOGIC_OP      0x0BF2
#define GL_SCISSOR_TEST        0x0C11
#define GL_LINE_SMOOTH         0x0B20
#define GL_CULL_FACE           0x0B44
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TEXTURE_2D          0x0DE1

// Client state
#define GL_VERTEX_ARRAY        0x8074
#define GL_NORMAL_ARRAY        0x8075
#define GL_COLOR_ARRAY         0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078

// Fog
#define GL_FOG_INDEX           0x0B61
#define GL_FOG_DENSITY         0x0B62
#define GL_FOG_START           0x0B63
#define GL_FOG_END             0x0B64
#define GL_FOG_MODE            0x0B65
#define GL_FOG_COLOR           0x0B66
#define GL_EXP                 0x0800
#define GL_EXP2                0x0801
#define GL_LINEAR              0x2601

// Front/back face
#define GL_CW                  0x0900
#define GL_CCW                 0x0901
#define GL_FRONT               0x0404
#define GL_BACK                0x0405
#define GL_FRONT_AND_BACK      0x0408

// Shading
#define GL_FLAT                0x1D00
#define GL_SMOOTH              0x1D01

// Clear bits
#define GL_COLOR_BUFFER_BIT    0x00004000
#define GL_DEPTH_BUFFER_BIT    0x00000100
#define GL_STENCIL_BUFFER_BIT  0x00000400

// Texture wrap / filter
#define GL_NEAREST             0x2600
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST  0x2701
#define GL_NEAREST_MIPMAP_LINEAR  0x2702
#define GL_LINEAR_MIPMAP_LINEAR   0x2703
#define GL_TEXTURE_MAG_FILTER  0x2800
#define GL_TEXTURE_MIN_FILTER  0x2801
#define GL_TEXTURE_WRAP_S      0x2802
#define GL_TEXTURE_WRAP_T      0x2803
#define GL_CLAMP               0x2900
#define GL_REPEAT              0x2901
#define GL_CLAMP_TO_EDGE       0x812F
#define GL_CLAMP_TO_BORDER     0x812D

// Texture formats / internal
#define GL_RGB                 0x1907
#define GL_RGBA                0x1908
#define GL_BGR                 0x80E0
#define GL_BGRA                0x80E1
#define GL_RGBA8               0x8058
#define GL_RGBA4               0x8056
#define GL_RGB8                0x8051
#define GL_LUMINANCE           0x1909
#define GL_LUMINANCE_ALPHA     0x190A
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_5_6_5 0x8363

// Buffer objects
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW          0x88E0
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8

// Stencil ops
#define GL_KEEP                0x1E00
#define GL_REPLACE             0x1E01
#define GL_INCR                0x1E02
#define GL_DECR                0x1E03
#define GL_INVERT              0x150A
#define GL_INCR_WRAP           0x8507
#define GL_DECR_WRAP           0x8508

// Pixel store
#define GL_UNPACK_ALIGNMENT    0x0CF5
#define GL_PACK_ALIGNMENT      0x0D05

// Errors
#define GL_NO_ERROR            0
#define GL_INVALID_ENUM        0x0500
#define GL_INVALID_VALUE       0x0501
#define GL_INVALID_OPERATION   0x0502
#define GL_OUT_OF_MEMORY       0x0505

// Texture rectangle (used by some renderers)
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5

// glGetString tokens
#define GL_VENDOR                0x1F00
#define GL_RENDERER              0x1F01
#define GL_VERSION               0x1F02
#define GL_EXTENSIONS            0x1F03

// Vertex buffer object hint (no-op on PS2)
#define GL_WRITE_ONLY          0x88B9

// Depth
#define GL_DEPTH_COMPONENT     0x1902

// Attrib masks
#define GL_CURRENT_BIT         0x00000001
#define GL_POINT_BIT           0x00000002
#define GL_LINE_BIT            0x00000004
#define GL_POLYGON_BIT         0x00000008
#define GL_COLOR_BUFFER_BIT_ATTR 0x00004000
#define GL_ENABLE_BIT          0x00002000
#define GL_DEPTH_BUFFER_BIT_ATTR 0x00000100
#define GL_ALL_ATTRIB_BITS     0x000FFFFF

// Misc
#define GL_TRUE                1
#define GL_FALSE               0

// ---------------------------------------------------------------------------
// GL function declarations (implemented in gles_ps2.cpp)
// ---------------------------------------------------------------------------

void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClear(GLbitfield mask);

void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glEnableClientState(GLenum a);
void glDisableClientState(GLenum a);

void glBlendFunc(GLenum src, GLenum dst);
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void glBlendEquation(GLenum mode);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void glAlphaFunc(GLenum func, GLclampf ref);

void glFogf(GLenum p, GLfloat v);
void glFogfv(GLenum p, const GLfloat* v);
inline void glFogi(GLenum, GLint)                                        {}

void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

void glVertexPointer(GLint sz, GLenum type, GLsizei stride, const GLvoid* p);
void glTexCoordPointer(GLint sz, GLenum type, GLsizei stride, const GLvoid* p);
void glColorPointer(GLint sz, GLenum type, GLsizei stride, const GLvoid* p);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid* p);

void glBindBuffer(GLenum target, GLuint id);
void glBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage);
void glDeleteBuffers(GLsizei n, const GLuint* ids);
void glGenBuffers(GLsizei n, GLuint* ids);

// Desktop Minecraft code still references ARB VBO names. On PS2 they map to
// the same lightweight buffer implementation/stubs used by the non-ARB names.
#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#endif
#ifndef GL_STREAM_DRAW_ARB
#define GL_STREAM_DRAW_ARB GL_STREAM_DRAW
#endif
inline void glGenBuffersARB(GLsizei n, GLuint* ids)                     { glGenBuffers(n, ids); }
inline void glBindBufferARB(GLenum target, GLuint id)                   { glBindBuffer(target, id); }
inline void glBufferDataARB(GLenum target, GLsizei size, const GLvoid* data, GLenum usage) { glBufferData(target, size, data, usage); }

void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);

void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);

void glMatrixMode(GLenum mode);
void glLoadIdentity();
void glPushMatrix();
void glPopMatrix();
void glGetFloatv(GLenum pname, GLfloat* params);
void glLoadMatrixf(const GLfloat* m);
void glMultMatrixf(const GLfloat* m);
void glTranslatef(GLfloat tx, GLfloat ty, GLfloat tz);
void glScalef(GLfloat sx, GLfloat sy, GLfloat sz);
void glRotatef(GLfloat angle, GLfloat ax, GLfloat ay, GLfloat az);

void glGenTextures(GLsizei n, GLuint* ids);
void glBindTexture(GLenum target, GLuint id);
void glDeleteTextures(GLsizei n, const GLuint* ids);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                  GLsizei w, GLsizei h, GLint border,
                  GLenum format, GLenum type, const GLvoid* data);
void glTexSubImage2D(GLenum target, GLint level,
                     GLint xoff, GLint yoff, GLsizei w, GLsizei h,
                     GLenum format, GLenum type, const GLvoid* data);
inline void glGetTexImage(GLenum, GLint, GLenum, GLenum, GLvoid*)        {}
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);

void glDrawArrays(GLenum mode, GLint first, GLsizei count);

// PS2-specific helpers (gles_ps2.cpp).
extern "C" void ps2_draw_cursor(float x, float y);
extern "C" int  ps2_gles_read_texture(GLuint id, unsigned int* dst, int dstPitchPx, int w, int h);
extern "C" void ps2_gles_tex_image_indexed(GLsizei w, GLsizei h,
                                           const GLubyte* indices, GLsizei indexPitch,
                                           const GLuint* palette, GLsizei paletteCount);
extern "C" void ps2_gles_tex_sub_image_indexed(GLint xoff, GLint yoff,
                                               GLsizei w, GLsizei h,
                                               const GLubyte* indices, GLsizei indexPitch,
                                               const GLuint* palette, GLsizei paletteCount);

// Stateful wrappers implemented in gles_ps2.cpp. Some are approximations,
// but they must not be no-ops because Minecraft relies on GL state isolation
// for GUI overlays, inventory preview, alpha/cutout rendering and occlusion paths.
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h);
void glDepthFunc(GLenum func);
inline void glClearDepth(GLdouble)                                      {}
inline void glClearDepthf(GLclampf)                                     {}
void glDepthMask(GLboolean flag);
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glShadeModel(GLenum mode);
inline void glLineWidth(GLfloat)                                         {}
inline void glPointSize(GLfloat)                                         {}
inline void glPolygonOffset(GLfloat, GLfloat)                            {}
inline void glPixelStorei(GLenum, GLint)                                 {}
inline void glPixelStoref(GLenum, GLfloat)                               {}
// 512, not the GS's 1024: caps texture pieces at 512x512 so the static
// upload scratch in GLInterface.cpp (1MB) always fits and no piece ever
// needs a multi-MB contiguous heap allocation.
inline void glGetIntegerv(GLenum pname, GLint* v)                        { if (v) *v = (pname == GL_MAX_TEXTURE_SIZE) ? 512 : 0; }
inline GLenum glGetError()                                               { return GL_NO_ERROR; }
inline void glFlush()                                                    {}
inline void glFinish()                                                   {}
inline void glStencilFunc(GLenum, GLint, GLuint)                         {}
inline void glStencilOp(GLenum, GLenum, GLenum)                          {}
inline void glStencilMask(GLuint)                                        {}
inline void glLogicOp(GLenum)                                            {}
inline void glNormal3f(GLfloat, GLfloat, GLfloat)                        {}
inline void glColor3f(GLfloat r, GLfloat g, GLfloat b)                   { glColor4f(r, g, b, 1.0f); }
inline void glRasterPos2i(GLint, GLint)                                  {}
inline void glRasterPos2f(GLfloat, GLfloat)                              {}
inline void glCopyTexImage2D(GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint) {}
inline void glCopyTexSubImage2D(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei) {}
inline const GLubyte* glGetString(GLenum)                               { return (const GLubyte*)"PS2"; }

// Small immediate-mode compatibility layer. Minecraft mostly uses
// Tessellator/vertex arrays, but startup/logo and a few legacy paths still call
// glBegin/glEnd.
void glBegin(GLenum mode);
void glEnd();
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord2i(GLint s, GLint t);
void glVertex2i(GLint x, GLint y);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3d(GLdouble x, GLdouble y, GLdouble z);
void glVertex2d(GLdouble x, GLdouble y);

// glFrustum — builds a perspective matrix and multiplies the current matrix.
inline void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
                      GLdouble n, GLdouble f)
{
    GLfloat m[16] = {0};
    m[0]  = (GLfloat)(2.0 * n / (r - l));
    m[5]  = (GLfloat)(2.0 * n / (t - b));
    m[8]  = (GLfloat)((r + l) / (r - l));
    m[9]  = (GLfloat)((t + b) / (t - b));
    m[10] = (GLfloat)(-(f + n) / (f - n));
    m[11] = -1.0f;
    m[14] = (GLfloat)(-2.0 * f * n / (f - n));
    glMultMatrixf(m);
}

// Double-precision scale (delegate to float version).
inline void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
inline void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
inline void glRotated(GLdouble a, GLdouble x, GLdouble y, GLdouble z)
{
    glRotatef((GLfloat)a, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

// Hints — no-op.
#define GL_FOG_HINT          0x0C54
#define GL_NICEST            0x1102
#define GL_FASTEST           0x1101
#define GL_DONT_CARE         0x1100
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT           0x0C51
#define GL_LINE_SMOOTH_HINT            0x0C52
#define GL_POLYGON_SMOOTH_HINT         0x0C53
inline void glHint(GLenum, GLenum)                                       {}

// Color material — no-op on PS2.
#define GL_COLOR_MATERIAL    0x0B57
#define GL_AMBIENT           0x1200
#define GL_DIFFUSE           0x1201
#define GL_SPECULAR          0x1202
#define GL_EMISSION          0x1600
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_LIGHT0            0x4000
#define GL_LIGHT1            0x4001
#define GL_POSITION          0x1203
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
inline void glColorMaterial(GLenum, GLenum)                              {}
inline void glLightfv(GLenum, GLenum, const GLfloat*)                    {}
inline void glLightModelfv(GLenum, const GLfloat*)                       {}

// Display lists — no-op on PS2 (not supported by OpenGL ES / gsKit).
// Font rendering falls back to silently not drawing; runtime issue only.
#define GL_COMPILE           0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301
inline GLuint glGenLists(GLsizei n)
{
    // PS2 has no real OpenGL display lists, but desktop Minecraft code treats
    // a zero return as fatal. Return dummy non-zero IDs and keep glNewList/
    // glCallList as no-ops.
    static GLuint nextList = 1;
    GLuint base = nextList;
    nextList += (n > 0) ? (GLuint)n : 1;
    return base;
}
inline void   glNewList(GLuint, GLenum)                                  {}
inline void   glEndList()                                                {}
inline void   glCallList(GLuint)                                         {}
inline void   glCallLists(GLsizei, GLenum, const GLvoid*)               {}
inline void   glListBase(GLuint)                                         {}
inline void   glDeleteLists(GLuint, GLsizei)                             {}

// Misc legacy functions.
#define GL_TEXTURE_GEN_S     0x0C60
#define GL_TEXTURE_GEN_T     0x0C61
#define GL_OBJECT_LINEAR     0x2401
#define GL_EYE_LINEAR        0x2400
#define GL_SPHERE_MAP        0x2402
#define GL_TEXTURE_GEN_MODE  0x2500
inline void glTexGeni(GLenum, GLenum, GLint)                             {}
inline void glTexGenfv(GLenum, GLenum, const GLfloat*)                  {}
inline void glRectf(GLfloat, GLfloat, GLfloat, GLfloat)                {}
inline void glRecti(GLint, GLint, GLint, GLint)                         {}
inline void glBitmap(GLsizei, GLsizei, GLfloat, GLfloat,
                     GLfloat, GLfloat, const GLubyte*)                  {}
inline void glReadPixels(GLint, GLint, GLsizei, GLsizei,
                         GLenum, GLenum, GLvoid*)                       {}
inline void glAccum(GLenum, GLfloat)                                    {}
inline void glPassThrough(GLfloat)                                      {}
inline void glSelectBuffer(GLsizei, GLuint*)                            {}
inline GLint glRenderMode(GLenum)                                       { return 0; }
inline void glInitNames()                                               {}
inline void glPushName(GLuint)                                          {}
inline void glPopName()                                                 {}
inline void glLoadName(GLuint)                                          {}
void glPushAttrib(GLbitfield mask);
void glPopAttrib() ;
inline void glMatrixFrustum(GLenum, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble) {} // not standard, guard-against

#endif // PS2_PLATFORM
