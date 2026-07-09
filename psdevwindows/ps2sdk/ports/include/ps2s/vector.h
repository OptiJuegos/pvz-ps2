/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef vector_h
#define vector_h

#include <math.h>

/********************************************
 * vec_*
 *
 * These are the vector classes the user deals with.  They contain
 * code that cannot/should not be generalized across all vectors.
 * General code is contained in the vec_template class, which is
 * located in "vector_common.h".
 *
 * Use:
 *
 * Here are the vec_* types currently defined:
 *   vec_x, vec_y, vec_z, vec_w, vec_xy, vec_xyz, vec_xyzw
 *
 * These classes probably behave mostly as you'd expect, with the most
 * notable exception being operations involving different types.
 * Operations involving two vectors must be of the same type, except
 * those involving single-element types (vec_?).  When only one vec_?
 * is involved it is used as a broadcast and the return type of the
 * expression will be the type of the other operand.  When two vec_?s
 * are used as operands, the right-hand side is the broadcast and the
 * lhs is the return type. Assignment works on the common set of
 * fields.  For example:
 *
 *   vec_xyzw xyzw;
 *   vec_xyz  xyz;
 *   vec_x    x;
 *   vec_y    y;
 *   vec_z    z;
 *
 *   xyzw = xyzw + xyz;		// error -- different types
 *   xyzw = vec_xyz(xyzw) + xyz; // ok -- xyzw.xyz = xyzw.xyz + xyz.xyz
 *   xyzw = xyzw * x;		// xyzw.xyzw = xyzw.xyzw * x.x (broadcast)
 *   xyzw = vec_x(xyzw) * x;	// xyzw.x = xyzw.x * x.x
 *   xyzw = vec_y(5.0f);	// xyzw.y = 5.0f;
 *   xyzw.set_y(5.0f);		// same as above
 *
 * One thing to BE CAREFUL about:  'xyzw += x' is a broadcast
 * (xyzw.xyzw = xyzw.xzyw + x.x)!!!
 *
 * See the libps2stuff/examples directory for more examples.
 * (bezier_patch.h)
 *
 * Caveats:
 *
 *   'float * vec_type' doesn't work yet because of a gcc bug.. use
 *	  'vec_type * float'
 *
 * Also see "vector_common.h" for
 * more implementation info.
 *
 *
 * Generic API
 *
 * Much of the interface is common to all these classes, although some
 * exceptions exist.  Some of the member functions (eg: dot product)
 * have dimension-specific implementations.  Others are written in a
 * generic style (eg: length) but are included in each vector class
 * definition.  Ideally these definitions would appear only once in
 * the source, but this duplication of code was judged to be the least
 * objectionable because of other technical and stylistic constraints.
 * This chart list many of the functions supported by each class:
 *
 *                             x   y   z   w   xy   xyz   xyzw
 *
 *  +                          @   @   @   @   @    @     @
 *  -                          @   @   @   @   @    @     @
 *  *                          @   @   @   @   @    @     @
 *  /                          @   @   @   @   @    @     @
 *  =                          @   @   @   @   @    @     @
 *  set                        @   @   @   @   @    @     @
 *  abs                        @   @   @   @   @    @     @
 *  sign                       @   @   @   @   @    @     @
 *  max                        @   @   @   @   @    @     @
 *  min                        @   @   @   @   @    @     @
 *  print                      @   @   @   @   @    @     @
 *  set_x, set_* etc.                          @    @     @
 *  ==                                         @    @     @
 *  !=                                         @    @     @
 *  dot                                        @    @     @
 *  mag                                        @    @     @
 *  length                                     @    @     @
 *  mag_sqr                                    @    @     @
 *  length_sqr                                 @    @     @
 *  one_over                                   @    @     @
 *  normalize                                  @    @     @
 *  normalized                                 @    @     @
 *  interpolate                                @    @     @
 *  distance_from                              @    @     @
 *  distance_from_line                         @    @     @
 *  set_length                                 @    @     @
 *  truncate_length                            @    @     @
 *  parallel_component                         @    @     @
 *  perpendicular_component                    @    @     @
 *  is_zero                                    @    @     @
 *  cross                                           @     @
 *  normalize3                                            @
 *  normalized3                                           @
 *
 *
 ********************************************
 * vector_t & point_t
 *
 * These types enforce the mathematical distinctions between 3d points
 * and vectors.  They mainly interact with each other and transform_t.
 *
 * 4d types:
 *
 * When mixed with vec_* and mat_* types, vector_t and point_t are 4d
 * types with "implied" w fields of 0 and 1, respectively.  Thus
 * mat_44 * point_t is legal. (Note that this returns a vec_xyzw since
 * the resulting w may not be 1).  Similarly, transform_t is a 4x4
 * matrix with an implied 4th row of (0,0,0,1).
 *
 * WARNING: the w fields are never actually read or set and not
 * guaranteed to be anything in particular.  Don't, for example, use
 * the 4th element after a cast to a float array.  Note that casts to
 * vec_4 or cpu_vec_4 will explicitly set w to 0 or 1.
 *
 * Not requiring a particular w value means that casts to and from
 * vec_xyz or between vector_t and point_t are defined as a move of a
 * 128bit type, without any modification of the data.  Thus, these
 * casts don't copy data needlessly.
 *
 * Undefined operations:
 *
 * Certain operations on point_t and vector_t are left undefined if
 * they aren't legal math.  Here are some examples:
 *
 * point_t pnt;
 * vector_t vec;
 * vec_x scale;
 *
 * pnt = pnt * scale // error - result w would be scale
 * pnt = pnt + pnt   // error - result w would be 2
 * pnt = pnt + vec   // ok - result w is 1
 * vec = pnt - pnt   // ok - result w is 0
 *
 * However, not all legal math is defined.  For example, point
 * averaging requires some casts.
 *
 * point_t average, pnt0, pnt1, pnt2;
 *
 * average = point_t((vec_xyz(pnt0) +
 *                    vec_xyz(pnt1) +
 *                    vec_xyz(pnt2)) * (1.0f/3.0f));
 *
 * Transformations:
 *
 * transform_t has a rotation/scale part and a translation part.  If
 * you apply it to a vector_t, only the rotation/scale is applied, but
 * if you apply it to a point_t, the translation is also added.
 *
 * If you want to just rotate a point_t, rather than zero out the
 * translation part of a transform_t, you again need some casts:
 *
 * mat_33 rot;
 *
 * pnt = point_t(rot * vec_xyz(pnt));
 * pnt = point_t(xfrm * vector_t(pnt));
 *
 * Clearly, the point_t and vector_t types aren't very convenient for
 * certain operations.  If you find they have too many restrictions,
 * consider using the vec_* types instead.
 *
 */

#include "ps2s/vector_common.h"

class vec_x;
class vec_y;
class vec_z;
class vec_w;
class vec_xy;
class vec_xyz;
class vec_xyzw;
class point_t;
class vector_t;
class cpu_vec_3;
class cpu_vec_4;
class mat_33;
class mat_34;
class mat_43;
class mat_44;
class zero_3;
class zero_4;
class zero_33;
class zero_34;
class zero_43;
class zero_44;

/********************************************
 * vec_x
 */

class vec_x : public vec_template<x_base> {
public:
    inline vec_x() {}
    inline vec_x(const vec128_t value) { vec128 = value; }
    inline vec_x(const vec_template<x_base> rhs) { vec128 = rhs.vec128; }

    inline vec_x(float x)
    {
        asm(
            " ### init vec_x with a float ###       \n"
            "  .if %A0                              \n"
            "  move %[_this], %[new_x]              \n"
            "  .endif                               \n"
            "                                       \n"
            "  .if %A1                              \n"
            "  lw %[_this], %[new_x]                \n"
            "  .endif                               \n"
            "                                       \n"
            "  .if %A2                              \n"
            "  mfc1 %[_this], %[new_x]              \n"
            "  .endif                               \n"
            "                                       \n"
            "  .if %A3                              \n"
            "  qmtc2 %[new_x], %[_this]             \n"
            "  .endif                               \n"
            : [_this] "=r,r,r,j"(vec128)
            : [new_x] "r,m,f,r"(x));
    }

    explicit inline vec_x(const vec_xyz rhs);
    explicit inline vec_x(const vec_xyzw rhs);
    explicit inline vec_x(const vector_t rhs);
    explicit inline vec_x(const point_t rhs);

    inline operator float() const
    {
        float new_float;
        asm(
            " ### vec_x to float ###                \n"
            "  .if %A0                              \n"
            "  move %[new_float], %[_this]          \n"
            "  .endif                               \n"
            "                                       \n"
            "  .if %A1                              \n"
            "  sw %[_this], %[new_float]            \n"
            "  .endif                               \n"
            "                                       \n"
            "  .if %A2                              \n"
            "  mtc1 %[_this], %[new_float]          \n"
            "  .endif                               \n"
            "                                       \n"
            "  .if %A3                              \n"
            "  qmfc2 %[new_float], %[_this]         \n"
            "  .endif                               \n"
            : [new_float] "=r,m,f,r"(new_float)
            : [_this] "r,r,r,j"(*this));
        return new_float;
    }

    // + and * that have a single-el vector on the left
    // get their order switched by a couple templates defined
    // at the bottom of this file.  Since we don't want that
    // to happen for operations on two singles, we explicitly
    // define them here, so that they will be called instead
    // of the more general templates.
    // Note that this could also be done with partially
    // specialized templates, but because of a (imo) misunderstanding
    // by the gcc developers, gcc doesn't allow pst methods.  The
    // workaround would be just as ugly as this is, but even less
    // clear.

    inline vec_x operator+(const vec_x rhs) const;
    inline vec_x operator+(const vec_y rhs) const;
    inline vec_x operator+(const vec_z rhs) const;
    inline vec_x operator+(const vec_w rhs) const;
    inline vec_x operator+(float rhs) const;

    inline vec_x operator*(const vec_x rhs) const;
    inline vec_x operator*(const vec_y rhs) const;
    inline vec_x operator*(const vec_z rhs) const;
    inline vec_x operator*(const vec_w rhs) const;
    inline vec_x operator*(float rhs) const;

    template <class rhs_type>
    inline vec_x operator-(const rhs_type rhs) const
    {
        return vec_x(vec_template<x_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_x operator/(const rhs_type rhs) const
    {
        return vec_x(vec_template<x_base>::operator/(rhs));
    }
    inline vec_x operator-() const
    {
        return vec_x(vec_template<x_base>::no_w_negate());
    }
    inline vec_x aadd() const
    {
        return vec_x(vec_template<x_base>::aadd());
    }
    inline vec_x asub() const
    {
        return vec_x(vec_template<x_base>::asub());
    }
    template <class rhs_type>
    inline vec_x madd(const rhs_type rhs) const
    {
        return vec_x(vec_template<x_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_x msub(const rhs_type rhs) const
    {
        return vec_x(vec_template<x_base>::msub(rhs));
    }
    inline vec_x abs() const
    {
        return vec_x(vec_template<x_base>::abs());
    }
    inline vec_x sign() const
    {
        return vec_x(vec_template<x_base>::sign());
    }
    template <class rhs_type>
    inline vec_x max(const rhs_type rhs) const
    {
        return vec_x(vec_template<x_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_x min(const rhs_type rhs) const
    {
        return vec_x(vec_template<x_base>::min(rhs));
    }

    inline void print(void) const
    {
        float x;
        asm volatile(
            "mtc1 %[vec], %[x]   # x = value.x  \n"
            : [x] "=f"(x)
            : [vec] "r"(vec128));

        printf("(%f * * *)\n", x);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: ", vec_name);
        print();
    }
};

/********************************************
 * vec_y
 */

class vec_y : public vec_template<y_base> {
public:
    inline vec_y() {}
    inline vec_y(const vec128_t value) { vec128 = value; }
    inline vec_y(const vec_template<y_base> rhs) { vec128 = rhs.vec128; }

    inline vec_y(float y)
    {
        asm(
            " ### init vec_y with a float ### \n"
            "ctc2	%[f_y], $vi21 \n"
            "vnop \n"
            "vaddi.y	%[_this], vf00, I \n"
            : [_this] "=j"(vec128)
            : [f_y] "r"(y));
    }

    explicit inline vec_y(const vec_xyz rhs);
    explicit inline vec_y(const vec_xyzw rhs);
    explicit inline vec_y(const vector_t rhs);
    explicit inline vec_y(const point_t rhs);

    inline operator float() const
    {
        float new_float;
        asm(
            " ### vec_y to float ### \n"
            "prot3w	%[new_float], %[_this] \n"
            : [new_float] "=r"(new_float)
            : [_this] "r"(*this));
        return new_float;
    }

    // see note in vec_x

    inline vec_y operator+(const vec_x rhs) const;
    inline vec_y operator+(const vec_y rhs) const;
    inline vec_y operator+(const vec_z rhs) const;
    inline vec_y operator+(const vec_w rhs) const;
    inline vec_y operator+(float rhs) const;

    inline vec_y operator*(const vec_x rhs) const;
    inline vec_y operator*(const vec_y rhs) const;
    inline vec_y operator*(const vec_z rhs) const;
    inline vec_y operator*(const vec_w rhs) const;
    inline vec_y operator*(float rhs) const;

    template <class rhs_type>
    inline vec_y operator-(const rhs_type rhs) const
    {
        return vec_y(vec_template<y_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_y operator/(const rhs_type rhs) const
    {
        return vec_y(vec_template<y_base>::operator/(rhs));
    }
    inline vec_y operator-() const
    {
        return vec_y(vec_template<y_base>::no_w_negate());
    }
    inline vec_y aadd() const
    {
        return vec_y(vec_template<y_base>::aadd());
    }
    inline vec_y asub() const
    {
        return vec_y(vec_template<y_base>::asub());
    }
    template <class rhs_type>
    inline vec_y madd(const rhs_type rhs) const
    {
        return vec_y(vec_template<y_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_y msub(const rhs_type rhs) const
    {
        return vec_y(vec_template<y_base>::msub(rhs));
    }
    inline vec_y abs() const
    {
        return vec_y(vec_template<y_base>::abs());
    }
    inline vec_y sign() const
    {
        return vec_y(vec_template<y_base>::sign());
    }
    template <class rhs_type>
    inline vec_y max(const rhs_type rhs) const
    {
        return vec_y(vec_template<y_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_y min(const rhs_type rhs) const
    {
        return vec_y(vec_template<y_base>::min(rhs));
    }

    inline void print(void) const
    {
        float y;
        vec128_t temp0;
        asm volatile(
            "mtsab $0, 4    # get ready to shift right 4 bytes \n"
            "qfsrv %[temp0], $0, %[vec]   # temp0 = value >> 8 \n"
            "mtc1 %[temp0], %[y]          # y = value.y        \n"
            : [y] "=f"(y), [temp0] "=r"(temp0)
            : [vec] "r"(vec128));

        printf("(* %f * *)\n", y);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: ", vec_name);
        print();
    }
};

/********************************************
 * vec_z
 */

class vec_z : public vec_template<z_base> {
public:
    inline vec_z() {}
    inline vec_z(const vec128_t value) { vec128 = value; }
    inline vec_z(const vec_template<z_base> rhs) { vec128 = rhs.vec128; }

    inline vec_z(float z)
    {
        asm(
            " ### init vec_z with a float ### \n"
            "ctc2	%[f_z], $vi21 \n"
            "vnop \n"
            "vaddi.z	%[_this], vf00, I \n"
            : [_this] "=j"(vec128)
            : [f_z] "r"(z));
    }

    explicit inline vec_z(const vec_xyz rhs);
    explicit inline vec_z(const vec_xyzw rhs);
    explicit inline vec_z(const vector_t rhs);
    explicit inline vec_z(const point_t rhs);

    inline operator float() const
    {
        float new_float;
        asm(
            " ### vec_z to float ### \n"
            "pextuw	%[new_float], $0, %[_this] \n"
            : [new_float] "=r"(new_float)
            : [_this] "r"(*this));
        return new_float;
    }

    // see note in vec_x

    inline vec_z operator+(const vec_x rhs) const;
    inline vec_z operator+(const vec_y rhs) const;
    inline vec_z operator+(const vec_z rhs) const;
    inline vec_z operator+(const vec_w rhs) const;
    inline vec_z operator+(float rhs) const;

    inline vec_z operator*(const vec_x rhs) const;
    inline vec_z operator*(const vec_y rhs) const;
    inline vec_z operator*(const vec_z rhs) const;
    inline vec_z operator*(const vec_w rhs) const;
    inline vec_z operator*(float rhs) const;

    template <class rhs_type>
    inline vec_z operator-(const rhs_type rhs) const
    {
        return vec_z(vec_template<z_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_z operator/(const rhs_type rhs) const
    {
        return vec_z(vec_template<z_base>::operator/(rhs));
    }
    inline vec_z operator-() const
    {
        return vec_z(vec_template<z_base>::no_w_negate());
    }
    inline vec_z aadd() const
    {
        return vec_z(vec_template<z_base>::aadd());
    }
    inline vec_z asub() const
    {
        return vec_z(vec_template<z_base>::asub());
    }
    template <class rhs_type>
    inline vec_z madd(const rhs_type rhs) const
    {
        return vec_z(vec_template<z_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_z msub(const rhs_type rhs) const
    {
        return vec_z(vec_template<z_base>::msub(rhs));
    }
    inline vec_z abs() const
    {
        return vec_z(vec_template<z_base>::abs());
    }
    inline vec_z sign() const
    {
        return vec_z(vec_template<z_base>::sign());
    }
    template <class rhs_type>
    inline vec_z max(const rhs_type rhs) const
    {
        return vec_z(vec_template<z_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_z min(const rhs_type rhs) const
    {
        return vec_z(vec_template<z_base>::min(rhs));
    }

    inline void print(void) const
    {
        float z;
        vec128_t temp0;
        asm volatile(
            "mtsab	$0, 8			# get ready to shift right 8 bytes \n"
            "qfsrv	%[temp0], $0, %[vec]	# temp0 = value >> 8 \n"
            "mtc1	%[temp0], %[z]		# z = value.z \n"
            : [z] "=f"(z), [temp0] "=r"(temp0)
            : [vec] "r"(vec128));

        printf("(* * %f *)\n", z);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: ", vec_name);
        print();
    }
};

/********************************************
 * vec_w
 */

class vec_w : public vec_template<w_base> {
public:
    inline vec_w() {}
    inline vec_w(const vec128_t value) { vec128 = value; }
    inline vec_w(const vec_template<w_base> rhs) { vec128 = rhs.vec128; }

    inline vec_w(float w)
    {
        asm(
            " ### init vec_w with a float ### \n"
            "ctc2		%[f_w], $vi21 \n"
            "vnop \n"
            "vmuli.w	%[_this], vf00, I \n"
            : [_this] "=j"(vec128)
            : [f_w] "r"(w));
    }

    explicit inline vec_w(const vec_xyzw rhs);
    explicit inline vec_w(const vector_t rhs);
    explicit inline vec_w(const point_t rhs);

    inline operator float() const
    {
        float new_float;
        asm(
            " ### vec_w to float ### \n"
            "mtsab	$0, 12	# shift right 12 bytes \n"
            "qfsrv	%[new_float], $0, %[_this] \n"
            : [new_float] "=r"(new_float)
            : [_this] "r"(*this));
        return new_float;
    }

    // see note in vec_x

    inline vec_w operator+(const vec_x rhs) const;
    inline vec_w operator+(const vec_y rhs) const;
    inline vec_w operator+(const vec_z rhs) const;
    inline vec_w operator+(const vec_w rhs) const;
    inline vec_w operator+(float rhs) const;

    inline vec_w operator*(const vec_x rhs) const;
    inline vec_w operator*(const vec_y rhs) const;
    inline vec_w operator*(const vec_z rhs) const;
    inline vec_w operator*(const vec_w rhs) const;
    inline vec_w operator*(float rhs) const;

    template <class rhs_type>
    inline vec_w operator-(const rhs_type rhs) const
    {
        return vec_w(vec_template<w_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_w operator/(const rhs_type rhs) const
    {
        return vec_w(vec_template<w_base>::operator/(rhs));
    }
    inline vec_w operator-() const
    {
        return vec_w(vec_template<w_base>::operator-());
    }
    inline vec_w aadd() const
    {
        return vec_w(vec_template<w_base>::aadd());
    }
    inline vec_w asub() const
    {
        return vec_w(vec_template<w_base>::asub());
    }
    template <class rhs_type>
    inline vec_w madd(const rhs_type rhs) const
    {
        return vec_w(vec_template<w_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_w msub(const rhs_type rhs) const
    {
        return vec_w(vec_template<w_base>::msub(rhs));
    }
    inline vec_w abs() const
    {
        return vec_w(vec_template<w_base>::abs());
    }
    inline vec_w sign() const
    {
        return vec_w(vec_template<w_base>::sign());
    }
    template <class rhs_type>
    inline vec_w max(const rhs_type rhs) const
    {
        return vec_w(vec_template<w_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_w min(const rhs_type rhs) const
    {
        return vec_w(vec_template<w_base>::min(rhs));
    }

    inline void print(void) const
    {
        float w;
        vec128_t temp0;
        asm volatile(
            "mtsab	$0, 12			# get ready to shift right 12 bytes	\n"
            "qfsrv	%[temp0], $0, %[vec]	# temp0 = value >> 8	\n"
            "mtc1	%[temp0], %[w]		# w = value.w \n"
            : [w] "=f"(w), [temp0] "=r"(temp0)
            : [vec] "r"(vec128));

        printf("(* * * %f)\n", w);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: ", vec_name);
        print();
    }
};

/********************************************
 * definitions of the 'single + single' and 'single * single'
 * methods.  The need to be after all the class definitions
 * so that they will inline.
 * See the note in vec_w above.
 */

// vec_x

inline vec_x vec_x::operator+(const vec_x rhs) const
{
    return vec_x(vec_template<x_base>::operator+(rhs));
}
inline vec_x vec_x::operator+(const vec_y rhs) const
{
    return vec_x(vec_template<x_base>::operator+(rhs));
}
inline vec_x vec_x::operator+(const vec_z rhs) const
{
    return vec_x(vec_template<x_base>::operator+(rhs));
}
inline vec_x vec_x::operator+(const vec_w rhs) const
{
    return vec_x(vec_template<x_base>::operator+(rhs));
}
inline vec_x vec_x::operator+(float rhs) const
{
    return vec_x(vec_template<x_base>::operator+(rhs));
}
inline vec_x vec_x::operator*(const vec_x rhs) const
{
    return vec_x(vec_template<x_base>::operator*(rhs));
}
inline vec_x vec_x::operator*(const vec_y rhs) const
{
    return vec_x(vec_template<x_base>::operator*(rhs));
}
inline vec_x vec_x::operator*(const vec_z rhs) const
{
    return vec_x(vec_template<x_base>::operator*(rhs));
}
inline vec_x vec_x::operator*(const vec_w rhs) const
{
    return vec_x(vec_template<x_base>::operator*(rhs));
}
inline vec_x vec_x::operator*(float rhs) const
{
    return vec_x(vec_template<x_base>::operator*(rhs));
}

// vec_y

inline vec_y vec_y::operator+(const vec_x rhs) const
{
    return vec_y(vec_template<y_base>::operator+(rhs));
}
inline vec_y vec_y::operator+(const vec_y rhs) const
{
    return vec_y(vec_template<y_base>::operator+(rhs));
}
inline vec_y vec_y::operator+(const vec_z rhs) const
{
    return vec_y(vec_template<y_base>::operator+(rhs));
}
inline vec_y vec_y::operator+(const vec_w rhs) const
{
    return vec_y(vec_template<y_base>::operator+(rhs));
}
inline vec_y vec_y::operator+(float rhs) const
{
    return vec_y(vec_template<y_base>::operator+(rhs));
}
inline vec_y vec_y::operator*(const vec_x rhs) const
{
    return vec_y(vec_template<y_base>::operator*(rhs));
}
inline vec_y vec_y::operator*(const vec_y rhs) const
{
    return vec_y(vec_template<y_base>::operator*(rhs));
}
inline vec_y vec_y::operator*(const vec_z rhs) const
{
    return vec_y(vec_template<y_base>::operator*(rhs));
}
inline vec_y vec_y::operator*(const vec_w rhs) const
{
    return vec_y(vec_template<y_base>::operator*(rhs));
}
inline vec_y vec_y::operator*(float rhs) const
{
    return vec_y(vec_template<y_base>::operator*(rhs));
}

// vec_z

inline vec_z vec_z::operator+(const vec_x rhs) const
{
    return vec_z(vec_template<z_base>::operator+(rhs));
}
inline vec_z vec_z::operator+(const vec_y rhs) const
{
    return vec_z(vec_template<z_base>::operator+(rhs));
}
inline vec_z vec_z::operator+(const vec_z rhs) const
{
    return vec_z(vec_template<z_base>::operator+(rhs));
}
inline vec_z vec_z::operator+(const vec_w rhs) const
{
    return vec_z(vec_template<z_base>::operator+(rhs));
}
inline vec_z vec_z::operator+(float rhs) const
{
    return vec_z(vec_template<z_base>::operator+(rhs));
}
inline vec_z vec_z::operator*(const vec_x rhs) const
{
    return vec_z(vec_template<z_base>::operator*(rhs));
}
inline vec_z vec_z::operator*(const vec_y rhs) const
{
    return vec_z(vec_template<z_base>::operator*(rhs));
}
inline vec_z vec_z::operator*(const vec_z rhs) const
{
    return vec_z(vec_template<z_base>::operator*(rhs));
}
inline vec_z vec_z::operator*(const vec_w rhs) const
{
    return vec_z(vec_template<z_base>::operator*(rhs));
}
inline vec_z vec_z::operator*(float rhs) const
{
    return vec_z(vec_template<z_base>::operator*(rhs));
}

// vec_w

inline vec_w vec_w::operator+(const vec_x rhs) const
{
    return vec_w(vec_template<w_base>::operator+(rhs));
}
inline vec_w vec_w::operator+(const vec_y rhs) const
{
    return vec_w(vec_template<w_base>::operator+(rhs));
}
inline vec_w vec_w::operator+(const vec_z rhs) const
{
    return vec_w(vec_template<w_base>::operator+(rhs));
}
inline vec_w vec_w::operator+(const vec_w rhs) const
{
    return vec_w(vec_template<w_base>::operator+(rhs));
}
inline vec_w vec_w::operator+(float rhs) const
{
    return vec_w(vec_template<w_base>::operator+(rhs));
}
inline vec_w vec_w::operator*(const vec_x rhs) const
{
    return vec_w(vec_template<w_base>::operator*(rhs));
}
inline vec_w vec_w::operator*(const vec_y rhs) const
{
    return vec_w(vec_template<w_base>::operator*(rhs));
}
inline vec_w vec_w::operator*(const vec_z rhs) const
{
    return vec_w(vec_template<w_base>::operator*(rhs));
}
inline vec_w vec_w::operator*(const vec_w rhs) const
{
    return vec_w(vec_template<w_base>::operator*(rhs));
}
inline vec_w vec_w::operator*(float rhs) const
{
    return vec_w(vec_template<w_base>::operator*(rhs));
}

/********************************************
 * vec_xy (or vec_2)
 */

class vec_xy : public vec_template<xy_base> {
public:
    inline vec_xy() {}
    inline vec_xy(const vec128_t value) { vec128 = value; }
    inline vec_xy(const vec_template<xy_base> rhs) { vec128 = rhs.vec128; }

    explicit inline vec_xy(const vec_xyz rhs);
    explicit inline vec_xy(const vec_xyzw rhs);
    explicit inline vec_xy(const vector_t rhs);
    explicit inline vec_xy(const point_t rhs);

    inline void
    set(float x, float y)
    {
        vec128_t temp0;
        asm(
            " ### init vec_xy with floats ### \n"
            ".balign	4 \n"
            "mfc1	%[_this], %[f_x]		# vec128.x = x \n"
            "mfc1	%[temp0], %[f_y]		# temp0.x = y \n"
            "nop \n"
            "pextlw	%[_this], %[temp0], %[_this]	# vec128 = (temp0.x << 32) | vec128.x  \n"
            : [_this] "=r"(vec128), [temp0] "=r"(temp0)
            : [f_x] "f"(x), [f_y] "f"(y));
    }

    template <class rhs_type>
    inline void
    set(rhs_type rhs) { vec_template<xy_base>::set(rhs); }

    inline vec_xy(float x, float y)
    {
        set(x, y);
    }

    template <class rhs_type>
    inline vec_xy operator+(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::operator+(rhs));
    }
    template <class rhs_type>
    inline vec_xy operator-(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_xy operator*(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::operator*(rhs));
    }
    template <class rhs_type>
    inline vec_xy operator/(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::operator/(rhs));
    }
    inline vec_xy operator-() const
    {
        return vec_xy(vec_template<xy_base>::no_w_negate());
    }
    inline vec_xy aadd() const
    {
        return vec_xy(vec_template<xy_base>::aadd());
    }
    inline vec_xy asub() const
    {
        return vec_xy(vec_template<xy_base>::asub());
    }
    template <class rhs_type>
    inline vec_xy madd(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_xy msub(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::msub(rhs));
    }
    inline vec_xy abs() const
    {
        return vec_xy(vec_template<xy_base>::abs());
    }
    inline vec_xy sign() const
    {
        return vec_xy(vec_template<xy_base>::sign());
    }
    template <class rhs_type>
    inline vec_xy max(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_xy min(const rhs_type rhs) const
    {
        return vec_xy(vec_template<xy_base>::min(rhs));
    }

    inline vec_xy& operator=(const vec_x rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xy& operator=(const vec_y rhs)
    {
        set(rhs);
        return *this;
    }

    // for convenience

    inline void set_x(float rhs) { *this = vec_x(rhs); }
    inline void set_y(float rhs) { *this = vec_y(rhs); }

    inline void print(void) const
    {
        float x, y;
        vec128_t temp0;
        asm volatile(
            "mtsab $0, 4		# get ready to shift right 4 bytes \n"
            "mtc1  %[vec], %[x]		# x = value.x	\n"
            "qfsrv %[temp0], $0, %[vec]	# temp0 = value >> 8	\n"
            "mtc1  %[temp0], %[y]	# y = value.y \n"
            : [x] "=f"(x), [y] "=f"(y), [temp0] "=r"(temp0)
            : [vec] "r"(vec128));

        printf("(%f %f * *)\n", x, y);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: \n", vec_name);
        print();
    }

    // dot product
    inline vec_x
    dot(const vec_xy rhs) const
    {
        vec128_t result;
        asm(
            " ### vec_xy dot vec_xy ### \n"
            "vmul     %[result], %[lhs], %[rhs] \n"
            "vaddy.x  %[result], %[result], %[result] \n"
            : [result] "=j"(result)
            : [lhs] "j"(*this), [rhs] "j"(rhs));
        return vec_x(result);
    }

    // returns a normalized (unit length) version of a vector without
    // modifying it.  (Returns zero vector when given a zero vector.)
    inline vec_xy
    normalized() const
    {
        vec128_t result, dot;
        int cond;
        asm(
            "### vec_xy normalized ###\n"
            "vmul	%[dot], %[_this], %[_this] \n"
            "vaddy.x	%[dot], %[dot], %[dot] \n"
            "vrsqrt	Q, vf00w, %[dot]x\n"
            "cfc2	%[cond], $vi17\n"
            "vsub	%[result], %[result], %[result]\n"
            "andi	%[cond], %[cond], 8\n"
            "bgtz	%[cond], 0f \n"
            "nop\n"
            "vwaitq\n"
            "vmulq	%[result], %[_this], Q\n"
            "0:\n"
            : [result] "=&j"(result), [dot] "=&j"(dot), [cond] "=r"(cond)
            : [_this] "j"(*this));
        return vec_xy(result);
    }

    inline vec_xy one_over() const
    {
        vec128_t result;
        asm(" ### reciprocal of vec_xy ### \n"
            "vdiv		Q, vf00w, %[_this]x \n"
            "vwaitq \n"
            "vaddq.x	%[result], vf00, Q \n"
            "vdiv		Q, vf00w, %[_this]y \n"
            "vwaitq \n"
            "vaddq.y	%[result], vf00, Q \n"
            : [result] "=&j"(result)
            : [_this] "j"(*this));
        return vec_xy(result);
    }

    // ========== BEGIN BLOCK OF DUPLICATED GENERIC FUNCTIONS ==========
    // if you modify or add definitions here, please make the
    // corresponding change to the other blocks of duplicated code

    // magnitude (length) squared
    inline vec_x mag_sqr() const { return this->dot(*this); }
    inline vec_x length_sqr() const { return mag_sqr(); }

    // magnitude (length)
    inline vec_x mag() const { return sqrtf(mag_sqr()); }
    inline vec_x length() const { return mag(); }

    // overwrite a vector with its normalized value
    inline vec_xy
    normalize()
    {
        *this = this->normalized();
        return *this;
    }

    // return parallel vector with given length
    inline vec_xy
    set_length(const float length) const
    {
        return (this->normalized()) * length;
    }

    // ensure vector is no longer than given threshold (clip if needed)
    inline vec_xy
    truncate_length(const float maxLength) const
    {
        const float maxLengthSquared = maxLength * maxLength;
        const float vecLengthSquared = this->mag_sqr();
        if (vecLengthSquared < maxLengthSquared)
            return *this;
        else
            return (*this) * (maxLength / sqrtf(vecLengthSquared));
    }

    // return component of vector parallel to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))
    inline vec_xy
    parallel_component(const vec_xy unitBasis) const
    {
        const float projection = this->dot(unitBasis);
        return unitBasis * projection;
    }

    // return component of vector perpendicular to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))
    inline vec_xy
    perpendicular_component(const vec_xy unitBasis) const
    {
        return (*this) - (this->parallel_component(unitBasis));
    }

    // distance between this vector and another
    // (treats them as "points" offsets from a common origin)
    inline vec_x
    distance_from(const vec_xy rhs) const
    {
        return ((*this) - rhs).length();
    }

    // distance between this vector and a line, defined in terms of
    // a point on the line ("origin") and a UNIT vector parallel to
    // the line ("tangent")
    inline vec_x
    distance_from_line(const vec_xy origin, const vec_xy tangent) const
    {
        const vec_xy offset        = (*this) - origin;
        const vec_xy perpendicular = offset.perpendicular_component(tangent);
        return perpendicular.length();
    }

    // are two vectors identically equal?
    // (for approximately-equal testing use "distance_from" or "mag_sqr")
    inline int
    operator==(const vec_xy rhs) const
    {
        return (this->distance_from(rhs)) == 0.0f;
    }
    inline int
    operator!=(const vec_xy rhs) const
    {
        return !((*this) == rhs);
    }

    // is this vector identically zero?
    inline int
    is_zero(void) const
    {
        return (this->length()) == 0.0f;
    }

    // linear interpolation:
    // blends from "this" to "other" as "alpha" goes from 0 to 1
    // no clipping: does extrapolation when alpha<0 or alpha>1
    inline vec_xy
    interpolate(const float alpha, const vec_xy& other) const
    {
        return (*this) + ((other - (*this)) * alpha);
    }

    // ==========  END BLOCK OF DUPLICATED GENERIC FUNCTIONS  ==========
};

typedef vec_xy vec_2;

/********************************************
 * vec_xyz (or vec_3)
 */

class vec_xyz : public vec_template<xyz_base> {
public:
    inline vec_xyz() {}
    inline vec_xyz(const vec128_t value) { vec128 = value; }
    inline vec_xyz(const vec_template<xyz_base> rhs) { vec128 = rhs.vec128; }
    inline vec_xyz(const cpu_vec_3& vec);

    explicit inline vec_xyz(const vec_xyzw rhs);
    explicit inline vec_xyz(const vector_t rhs);
    explicit inline vec_xyz(const point_t rhs);

    inline void
    set(float x, float y, float z)
    {
        vec128_t temp0;
        asm(
            " ### init vec_xyz with floats ### \n"
            ".balign	4 \n"
            "mfc1	%[_this], %[f_x]		# vec128.x = x \n"
            "mfc1	%[temp0], %[f_y]		# temp0.x = y \n"
            "nop \n"
            "pextlw	%[_this], %[temp0], %[_this]	# vec128 = (temp0.x << 32) | vec128.x  \n"
            "mfc1	%[temp0], %[f_z]		# temp0.x = z \n"
            "pcpyld	%[_this], %[temp0], %[_this]	# vec128 |= (temp0 << 64) \n"
            : [_this] "=r"(vec128), [temp0] "=r"(temp0)
            : [f_x] "f"(x), [f_y] "f"(y), [f_z] "f"(z));
    }

    template <class rhs_type>
    inline void
    set(rhs_type rhs) { vec_template<xyz_base>::set(rhs); }

    inline vec_xyz(float x, float y, float z)
    {
        set(x, y, z);
    }

    template <class rhs_type>
    inline vec_xyz operator+(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::operator+(rhs));
    }
    template <class rhs_type>
    inline vec_xyz operator-(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_xyz operator*(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::operator*(rhs));
    }
    template <class rhs_type>
    inline vec_xyz operator/(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::operator/(rhs));
    }
    inline vec_xyz operator-() const
    {
        return vec_xyz(vec_template<xyz_base>::no_w_negate());
    }
    inline vec_xyz aadd() const
    {
        return vec_xyz(vec_template<xyz_base>::aadd());
    }
    inline vec_xyz asub() const
    {
        return vec_xyz(vec_template<xyz_base>::asub());
    }
    template <class rhs_type>
    inline vec_xyz madd(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_xyz msub(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::msub(rhs));
    }
    inline vec_xyz abs() const
    {
        return vec_xyz(vec_template<xyz_base>::abs());
    }
    inline vec_xyz sign() const
    {
        return vec_xyz(vec_template<xyz_base>::sign());
    }
    template <class rhs_type>
    inline vec_xyz max(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_xyz min(const rhs_type rhs) const
    {
        return vec_xyz(vec_template<xyz_base>::min(rhs));
    }

    inline vec_x max() const
    {
        vec128_t result;
        asm("### vec_xyz max element ###\n"
            "vmaxy.x	%[result], %[_this], %[_this] \n"
            "vmaxz.x	%[result], %[result], %[_this] \n"
            : [result] "=j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_x min() const
    {
        vec128_t result;
        asm("### vec_xyz min element ###\n"
            "vminiy.x	%[result], %[_this], %[_this] \n"
            "vminiz.x	%[result], %[result], %[_this] \n"
            : [result] "=j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_xyz one_over() const
    {
        vec128_t result;
        asm(" ### reciprocal of vec_xyz ### \n"
            "vdiv		Q, vf00w, %[_this]x \n"
            "vwaitq \n"
            "vaddq.x	%[result], vf00, Q \n"
            "vdiv		Q, vf00w, %[_this]y \n"
            "vwaitq \n"
            "vaddq.y	%[result], vf00, Q \n"
            "vdiv		Q, vf00w, %[_this]z \n"
            "vwaitq \n"
            "vaddq.z	%[result], vf00, Q \n"
            : [result] "=&j"(result)
            : [_this] "j"(*this));
        return vec_xyz(result);
    }

    inline vec_xyz& operator=(const vec_x rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyz& operator=(const vec_y rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyz& operator=(const vec_z rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyz& operator=(const vec_xy rhs)
    {
        set(rhs);
        return *this;
    }

    // for convenience

    inline void set_x(float rhs) { *this = vec_x(rhs); }
    inline void set_y(float rhs) { *this = vec_y(rhs); }
    inline void set_z(float rhs) { *this = vec_z(rhs); }

    inline vec_x
    dot(const vec_xyz rhs) const
    {
        vec128_t result, one;
        asm(
            " ### vec_xyz dot vec_xyz ### \n"
            "vmul		%[result], %[lhs], %[rhs] \n"
            "vaddw.x	%[one], vf00, vf00 \n"
            "vadday.x	ACC, %[result], %[result] \n"
            "vmaddz.x	%[result], %[one], %[result] \n"
            : [result] "=j"(result), [one] "=j"(one)
            : [lhs] "j"(*this), [rhs] "j"(rhs));
        return vec_x(result);
    }

    // when square of length = 0, this returns zero vector

    inline vec_xyz
    normalized() const
    {
        vec128_t result, dot, one;
        int cond;
        asm(
            "### vec_xyz normalized ###\n"
            "vmul	%[dot], %[_this], %[_this] \n"
            "vaddw.x	%[one], vf00, vf00 \n"
            "vadday.x	ACC, %[dot], %[dot] \n"
            "vmaddz.x	%[dot], %[one], %[dot] \n"
            "vrsqrt	Q, vf00w, %[dot]x\n"
            "cfc2	%[cond], $vi17\n"
            "vsub	%[result], %[result], %[result]\n"
            "andi	%[cond], %[cond], 8\n"
            "bgtz	%[cond], 0f \n"
            "nop\n"
            "vwaitq\n"
            "vmulq	%[result], %[_this], Q\n"
            "0:\n"
            : [result] "=&j"(result), [one] "=&j"(one), [dot] "=&j"(dot), [cond] "=r"(cond)
            : [_this] "j"(*this));

        return vec_xyz(result);
    }

    inline vec_xyz
    cross(const vec_xyz rhs) const
    {
        vec128_t result;
        asm(
            " ### vec_xyz cross vec_xyz ### \n"
            "vopmula.xyz	ACC, %[lhs], %[rhs] \n"
            "vopmsub.xyz	%[result], %[rhs], %[lhs] \n"
            : [result] "=j"(result)
            : [lhs] "j"(*this), [rhs] "j"(rhs));
        return vec_xyz(result);
    }

    // operations with matrix types

    inline mat_33 operator~() const;

    inline mat_33 tilde_mult(const mat_33& mat) const;
    inline mat_34 tilde_mult(const mat_34& mat) const;

    inline vec_xyz operator*(const mat_33& mat) const;
    inline vec_xyzw operator*(const mat_34& mat) const;

    inline mat_33 tensor_mult(const vec_xyz vec) const;
    inline mat_43 tensor_mult(const vec_xyzw vec) const;

    // operations with zero_vector and zero_matrix types

    inline vec_xyz operator+(const zero_3 zero) const;
    inline vec_xyz operator-(const zero_3 zero) const;

    inline zero_33 tilde_mult(const zero_33 zero) const;
    inline zero_34 tilde_mult(const zero_34 zero) const;

    inline zero_3 operator*(const zero_33 zero) const;
    inline zero_4 operator*(const zero_34 zero) const;

    inline zero_33 tensor_mult(const zero_3 zero) const;
    inline zero_43 tensor_mult(const zero_4 zero) const;

    inline vec_xyz& operator=(const zero_3 zero);

    inline void print(void) const
    {
        float x, y, z;
        vec128_t temp0;
        asm volatile(
            "mtsab	$0, 4			# get ready to shift right 4 bytes \n"
            "mtc1	%[vec], %[x]		# x = value.x	\n"
            "qfsrv	%[temp0], $0, %[vec]	# temp0 = value >> 8	\n"
            "mtc1	%[temp0], %[y]		# y = value.y \n"
            "qfsrv	%[temp0], $0, %[temp0]	# temp0 >>= 8 \n"
            "mtc1	%[temp0], %[z]		# z = value.z \n"
            : [x] "=f"(x), [y] "=f"(y), [z] "=f"(z), [temp0] "=r"(temp0)
            : [vec] "r"(vec128));

        printf("(%f %f %f *)\n", x, y, z);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: \n", vec_name);
        print();
    }

    // ========== BEGIN BLOCK OF DUPLICATED GENERIC FUNCTIONS ==========
    // if you modify or add definitions here, please make the
    // corresponding change to the other blocks of duplicated code

    // magnitude (length) squared
    inline vec_x mag_sqr() const { return this->dot(*this); }
    inline vec_x length_sqr() const { return mag_sqr(); }

    // magnitude (length)
    inline vec_x mag() const { return sqrtf(mag_sqr()); }
    inline vec_x length() const { return mag(); }

    // overwrite a vector with its normalized value
    inline vec_xyz
    normalize()
    {
        *this = this->normalized();
        return *this;
    }

    // return parallel vector with given length
    inline vec_xyz
    set_length(const float length) const
    {
        return (this->normalized()) * length;
    }

    // ensure vector is no longer than given threshold (clip if needed)
    inline vec_xyz
    truncate_length(const float maxLength) const
    {
        const float maxLengthSquared = maxLength * maxLength;
        const float vecLengthSquared = this->mag_sqr();
        if (vecLengthSquared < maxLengthSquared)
            return *this;
        else
            return (*this) * (maxLength / sqrtf(vecLengthSquared));
    }

    // return component of vector parallel to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))
    inline vec_xyz
    parallel_component(const vec_xyz unitBasis) const
    {
        const float projection = this->dot(unitBasis);
        return unitBasis * projection;
    }

    // return component of vector perpendicular to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))
    inline vec_xyz
    perpendicular_component(const vec_xyz unitBasis) const
    {
        return (*this) - (this->parallel_component(unitBasis));
    }

    // distance between this vector and another
    // (treats them as "points" offsets from a common origin)
    inline vec_x
    distance_from(const vec_xyz rhs) const
    {
        return ((*this) - rhs).length();
    }

    // distance between this vector and a line, defined in terms of
    // a point on the line ("origin") and a UNIT vector parallel to
    // the line ("tangent")
    inline vec_x
    distance_from_line(const vec_xyz origin, const vec_xyz tangent) const
    {
        const vec_xyz offset        = (*this) - origin;
        const vec_xyz perpendicular = offset.perpendicular_component(tangent);
        return perpendicular.length();
    }

    // are two vectors identically equal?
    // (for approximately-equal testing use "distance_from" or "mag_sqr")
    inline int
    operator==(const vec_xyz rhs) const
    {
        return (this->distance_from(rhs)) == 0.0f;
    }
    inline int
    operator!=(const vec_xyz rhs) const
    {
        return !((*this) == rhs);
    }

    // is this vector identically zero?
    inline int
    is_zero(void) const
    {
        return (this->length()) == 0.0f;
    }

    // linear interpolation:
    // blends from "this" to "other" as "alpha" goes from 0 to 1
    // no clipping: does extrapolation when alpha<0 or alpha>1
    inline vec_xyz
    interpolate(const float alpha, const vec_xyz& other) const
    {
        return (*this) + ((other - (*this)) * alpha);
    }

    // ==========  END BLOCK OF DUPLICATED GENERIC FUNCTIONS  ==========
};

typedef vec_xyz vec_3;

/********************************************
 * vec_xyzw (or vec_4)
 */

class vec_xyzw : public vec_template<xyzw_base> {
public:
    inline vec_xyzw() {}
    inline vec_xyzw(const vec128_t value) { vec128 = value; }
    inline vec_xyzw(const vec_template<xyzw_base> rhs) { vec128 = rhs.vec128; }
    inline vec_xyzw(const vector_t rhs);
    inline vec_xyzw(const point_t rhs);
    inline vec_xyzw(const cpu_vec_4& vec);

    inline void
    set(float x, float y, float z, float w)
    {
        vec128_t temp0, temp1;
        asm(
            " ### init vec_xyzw with floats ### \n"
            ".balign	4 \n"
            "mfc1	%[_this], %[f_x]		# vec128.x = x \n"

            "mfc1	%[temp0], %[f_y]		# temp0.x = y \n"
            "nop \n"

            "pextlw	%[_this], %[temp0], %[_this]	# vec128 = (temp0.x << 32) | vec128.x  \n"
            "mfc1	%[temp0], %[f_z]		# temp0.x = z \n"

            "mfc1	%[temp1], %[f_w]		# temp1.x = w \n"
            "nop \n"

            "pextlw	%[temp0], %[temp1], %[temp0]	# temp0 = (temp1.x << 32) | temp0.x  \n"
            "nop \n"

            "pcpyld	%[_this], %[temp0], %[_this]	# vec128 |= (temp0 << 64) \n"
            : [_this] "=r"(vec128), [temp0] "=r"(temp0), [temp1] "=r"(temp1)
            : [f_x] "f"(x), [f_y] "f"(y), [f_z] "f"(z), [f_w] "f"(w));
    }

    template <class rhs_type>
    inline void
    set(rhs_type rhs) { vec_template<xyzw_base>::set(rhs); }

    inline vec_xyzw(float x, float y, float z, float w)
    {
        set(x, y, z, w);
    }

    template <class rhs_type>
    inline vec_xyzw operator+(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::operator+(rhs));
    }
    template <class rhs_type>
    inline vec_xyzw operator-(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::operator-(rhs));
    }
    template <class rhs_type>
    inline vec_xyzw operator*(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::operator*(rhs));
    }
    template <class rhs_type>
    inline vec_xyzw operator/(rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::operator/(rhs));
    }
    inline vec_xyzw operator-() const
    {
        return vec_xyzw(vec_template<xyzw_base>::operator-());
    }
    inline vec_xyzw aadd() const
    {
        return vec_xyzw(vec_template<xyzw_base>::aadd());
    }
    inline vec_xyzw asub() const
    {
        return vec_xyzw(vec_template<xyzw_base>::asub());
    }
    template <class rhs_type>
    inline vec_xyzw madd(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::madd(rhs));
    }
    template <class rhs_type>
    inline vec_xyzw msub(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::msub(rhs));
    }
    inline vec_xyzw abs() const
    {
        return vec_xyzw(vec_template<xyzw_base>::abs());
    }
    inline vec_xyzw sign() const
    {
        return vec_xyzw(vec_template<xyzw_base>::sign());
    }
    template <class rhs_type>
    inline vec_xyzw max(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::max(rhs));
    }
    template <class rhs_type>
    inline vec_xyzw min(const rhs_type rhs) const
    {
        return vec_xyzw(vec_template<xyzw_base>::min(rhs));
    }

    inline vec_x max() const
    {
        vec128_t result;
        asm("### vec_xyzw max element ###\n"
            "vmaxy.x	%[result], %[_this],  %[_this] \n"
            "vmaxz.x	%[result], %[result], %[_this] \n"
            "vmaxw.x	%[result], %[result], %[_this] \n"
            : [result] "=&j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_x min() const
    {
        vec128_t result;
        asm("### vec_xyzw min element ###\n"
            "vminiy.x	%[result], %[_this],  %[_this] \n"
            "vminiz.x	%[result], %[result], %[_this] \n"
            "vminiw.x	%[result], %[result], %[_this] \n"
            : [result] "=&j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_xyzw one_over() const
    {
        vec128_t result;
        asm(" ### reciprocal of vec_xyzw ### \n"
            "vdiv		Q, vf00w, %[_this]x \n"
            "vwaitq \n"
            "vaddq.x	%[result], vf00, Q \n"
            "vdiv		Q, vf00w, %[_this]y \n"
            "vwaitq \n"
            "vaddq.y	%[result], vf00, Q \n"
            "vdiv		Q, vf00w, %[_this]z \n"
            "vwaitq \n"
            "vaddq.z	%[result], vf00, Q \n"
            "vdiv		Q, vf00w, %[_this]w \n"
            "vwaitq \n"
            "vmulq.w	%[result], vf00, Q \n"
            : [result] "=&j"(result)
            : [_this] "j"(*this));
        return vec_xyzw(result);
    }

    inline vec_xyzw& operator=(const vec_x rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyzw& operator=(const vec_y rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyzw& operator=(const vec_z rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyzw& operator=(const vec_w rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyzw& operator=(const vec_xy rhs)
    {
        set(rhs);
        return *this;
    }
    inline vec_xyzw& operator=(const vec_xyz rhs)
    {
        set(rhs);
        return *this;
    }

    // for convenience

    inline void set_x(float rhs) { *this = vec_x(rhs); }
    inline void set_y(float rhs) { *this = vec_y(rhs); }
    inline void set_z(float rhs) { *this = vec_z(rhs); }
    inline void set_w(float rhs) { *this = vec_w(rhs); }

    inline vec_x
    dot(const vec_xyzw rhs) const
    {
        vec128_t result, one;
        asm(
            " ### vec_xyzw dot vec_xyzw ### \n"
            "vmul	%[result], %[lhs], %[rhs] \n"
            "vaddw.x	%[one], vf00, vf00 \n"
            "vadday.x	ACC, %[result], %[result] \n"
            "vmaddaz.x	ACC, %[one], %[result] \n"
            "vmaddw.x	%[result], %[one], %[result] \n"
            : [result] "=j"(result), [one] "=j"(one)
            : [lhs] "j"(*this), [rhs] "j"(rhs));
        return vec_x(result);
    }

    // when square of length = 0, this returns zero vector

    inline vec_xyzw
    normalized() const
    {
        vec128_t result, dot, one;
        int cond;
        asm(
            "### vec_xyzw normalized ###\n"
            "vmul	%[dot], %[_this], %[_this] \n"
            "vaddw.x	%[one, vf00, vf00 \n"
            "vadday.x	ACC, %[dot], %[dot] \n"
            "vmaddaz.x	ACC, %[one], %[dot] \n"
            "vmaddw.x	%[dot, %[one, %[dot] \n"
            "vrsqrt	Q, vf00w, %[dot]x\n"
            "cfc2	%[cond], $vi17\n"
            "vsub	%[result], %[result], %[result]\n"
            "andi	%[cond], %[cond], 8\n"
            "bgtz	%[cond], 0f \n"
            "nop\n"
            "vwaitq\n"
            "vmulq	%[result], %[_this], Q\n"
            "0:\n"
            : [result] "=&j"(result), [one] "=&j"(one), [dot] "=&j"(dot), [cond] "=r"(cond)
            : [_this] "j"(*this));

        return vec_xyzw(result);
    }

    inline vec_xyzw normalize3();

    inline vec_xyzw normalized3();

    // this returns 0 in the w field
    inline vec_xyzw
    cross(const vec_xyzw rhs) const
    {
        vec128_t result;
        asm(
            " ### vec_xyzw cross vec_xyzw ### \n"
            "vsub.w		%[result], %[result], %[result] \n"
            "vopmula.xyz	ACC, %[lhs], %[rhs] \n"
            "vopmsub.xyz	%[result], %[rhs], %[lhs] \n"
            : [result] "=j"(result)
            : [lhs] "j"(*this), [rhs] "j"(rhs));
        return vec_xyzw(result);
    }

    // operations with matrix types

    inline vec_xyz operator*(const mat_43& mat) const;
    inline vec_xyzw operator*(const mat_44& mat) const;

    inline mat_34 tensor_mult(const vec_xyz vec) const;
    inline mat_44 tensor_mult(const vec_xyzw vec) const;

    // operations with zero_vector and zero_matrix types

    inline vec_xyzw operator+(const zero_4 zero) const;
    inline vec_xyzw operator-(const zero_4 zero) const;

    inline zero_3 operator*(const zero_43 zero) const;
    inline zero_4 operator*(const zero_44 zero) const;

    inline zero_34 tensor_mult(const zero_3 vec) const;
    inline zero_44 tensor_mult(const zero_4 vec) const;

    inline vec_xyzw& operator=(const zero_4 zero);

    // ========== BEGIN BLOCK OF DUPLICATED GENERIC FUNCTIONS ==========
    // if you modify or add definitions here, please make the
    // corresponding change to the other blocks of duplicated code

    // magnitude (length) squared
    inline vec_x mag_sqr() const { return this->dot(*this); }
    inline vec_x length_sqr() const { return mag_sqr(); }

    // magnitude (length)
    inline vec_x mag() const { return sqrtf(mag_sqr()); }
    inline vec_x length() const { return mag(); }

    // overwrite a vector with its normalized value
    inline vec_xyzw
    normalize()
    {
        *this = this->normalized();
        return *this;
    }

    // return parallel vector with given length
    inline vec_xyzw
    set_length(const float length) const
    {
        return (this->normalized()) * length;
    }

    // ensure vector is no longer than given threshold (clip if needed)
    inline vec_xyzw
    truncate_length(const float maxLength) const
    {
        const float maxLengthSquared = maxLength * maxLength;
        const float vecLengthSquared = this->mag_sqr();
        if (vecLengthSquared < maxLengthSquared)
            return *this;
        else
            return (*this) * (maxLength / sqrtf(vecLengthSquared));
    }

    // return component of vector parallel to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))
    inline vec_xyzw
    parallel_component(const vec_xyzw unitBasis) const
    {
        const float projection = this->dot(unitBasis);
        return unitBasis * projection;
    }

    // return component of vector perpendicular to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))
    inline vec_xyzw
    perpendicular_component(const vec_xyzw unitBasis) const
    {
        return (*this) - (this->parallel_component(unitBasis));
    }

    // distance between this vector and another
    // (treats them as "points" offsets from a common origin)
    inline vec_x
    distance_from(const vec_xyzw rhs) const
    {
        return ((*this) - rhs).length();
    }

    // distance between this vector and a line, defined in terms of
    // a point on the line ("origin") and a UNIT vector parallel to
    // the line ("tangent")
    inline vec_x
    distance_from_line(const vec_xyzw origin, const vec_xyzw tangent) const
    {
        const vec_xyzw offset        = (*this) - origin;
        const vec_xyzw perpendicular = offset.perpendicular_component(tangent);
        return perpendicular.length();
    }

    // are two vectors identically equal?
    // (for approximately-equal testing use "distance_from" or "mag_sqr")
    inline int
    operator==(const vec_xyzw rhs) const
    {
        return (this->distance_from(rhs)) == 0.0f;
    }
    inline int
    operator!=(const vec_xyzw rhs) const
    {
        return !((*this) == rhs);
    }

    // is this vector identically zero?
    inline int
    is_zero(void) const
    {
        return (this->length()) == 0.0f;
    }

    // linear interpolation:
    // blends from "this" to "other" as "alpha" goes from 0 to 1
    // no clipping: does extrapolation when alpha<0 or alpha>1
    inline vec_xyzw
    interpolate(const float alpha, const vec_xyzw& other) const
    {
        return (*this) + ((other - (*this)) * alpha);
    }

    // ==========  END BLOCK OF DUPLICATED GENERIC FUNCTIONS  ==========
};

typedef vec_xyzw vec_4;

/********************************************
 * vector_t - a three-element "vector" (as opposed to a "point") designed
 * to interact mainly with the point_t and transform_t types.
 *
 */

class vector_t : public vec_template<vector_base> {
public:
    inline vector_t() {}
    inline vector_t(const vec128_t rhs) { vec128 = rhs; }
    inline vector_t(const vec_template<vector_base> rhs) { vec128 = rhs.vec128; }

    // explicit constructors:
    // never want to implicitly convert between points and vectors, or from 3 dimensional
    // types to points or vectors.

    explicit inline vector_t(const vec_template<point_base> rhs) { vec128 = rhs.vec128; }
    explicit inline vector_t(const vec_template<xyz_base> rhs) { vec128 = rhs.vec128; }
    explicit inline vector_t(const cpu_vec_3& rhs);

    inline void
    set(float x, float y, float z)
    {
        vec128_t temp0, temp1;
        asm(
            " ### init vector_t with floats ### \n"
            ".balign	4 \n"
            "mfc1		%[_this], %[f_x]		# vec128.x = x \n"

            "mfc1		%[_temp0], %[f_y]		# temp0.x = y \n"
            "nop \n"

            "pextlw		%[_this], %[_temp0], %[_this]	# vec128 = (temp0.x << 32) | vec128.x  \n"
            "mfc1		%[_temp0], %[f_z]		# temp0.x = z \n"

            "mfc1		%[_temp1], %[f_w]		# temp1.x = w \n"
            "nop \n"

            "pextlw		%[_temp0], %[_temp1], %[_temp0]	# temp0 = (temp1.x << 32) | temp0.x  \n"
            "nop \n"

            "pcpyld		%[_this], %[_temp0], %[_this]	# vec128 |= (temp0 << 64) \n"
            : [_this] "=r"(vec128), [_temp0] "=r"(temp0), [_temp1] "=r"(temp1)
            : [f_x] "f"(x), [f_y] "f"(y), [f_z] "f"(z), [f_w] "f"(0.0f));
    }

    template <class rhs_type>
    inline void set(rhs_type rhs) { vec_template<vector_base>::set(rhs); }

    inline vector_t(float x, float y, float z) { set(x, y, z); }

    inline vector_t& operator=(const vec_x rhs)
    {
        set(rhs);
        return *this;
    }
    inline vector_t& operator=(const vec_y rhs)
    {
        set(rhs);
        return *this;
    }
    inline vector_t& operator=(const vec_z rhs)
    {
        set(rhs);
        return *this;
    }

    inline void set_x(float rhs) { *this = vec_x(rhs); }
    inline void set_y(float rhs) { *this = vec_y(rhs); }
    inline void set_z(float rhs) { *this = vec_z(rhs); }

private:
    // leave general template functions undefined.  See notes at beginning of file.

    template <class rhs_type>
    inline vector_t operator+(const rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t operator-(const rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t operator*(const rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t operator/(rhs_type rhs) const;
    template <class rhs_type>
    inline void operator+=(const rhs_type rhs);
    template <class rhs_type>
    inline void operator-=(const rhs_type rhs);
    template <class rhs_type>
    inline void operator*=(const rhs_type rhs);
    template <class rhs_type>
    inline void operator/=(const rhs_type rhs);
    template <class rhs_type>
    inline void mula(rhs_type rhs) const;
    template <class rhs_type>
    inline void adda(rhs_type rhs) const;
    template <class rhs_type>
    inline void suba(rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t madd(rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t msub(rhs_type rhs) const;
    template <class rhs_type>
    inline void madda(rhs_type rhs) const;
    template <class rhs_type>
    inline void msuba(rhs_type rhs) const;

public:
    // define legal arithmetic.

    inline point_t operator+(const point_t rhs) const;
    inline vector_t operator+(const vector_t rhs) const
    {
        return vector_t(vec_template<vector_base>::operator+(rhs));
    }
    inline vector_t operator-(const vector_t rhs) const
    {
        return vector_t(vec_template<vector_base>::operator-(rhs));
    }

    inline vector_t operator*(const vec_x rhs) const
    {
        return vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline vector_t operator*(const vec_y rhs) const
    {
        return vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline vector_t operator*(const vec_z rhs) const
    {
        return vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline vector_t operator*(const vec_w rhs) const
    {
        return vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline vector_t operator*(const float rhs) const
    {
        return vector_t(vec_template<vector_base>::operator*(vec_x(rhs)));
    }

    inline vector_t operator/(const vec_x rhs) const
    {
        return vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline vector_t operator/(const vec_y rhs) const
    {
        return vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline vector_t operator/(const vec_z rhs) const
    {
        return vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline vector_t operator/(const vec_w rhs) const
    {
        return vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline vector_t operator/(const float rhs) const
    {
        return vector_t(vec_template<vector_base>::operator/(rhs));
    }

    inline void operator+=(const vector_t rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator+(rhs));
    }
    inline void operator-=(const vector_t rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator-(rhs));
    }

    inline void operator*=(const vec_x rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline void operator*=(const vec_y rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline void operator*=(const vec_z rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline void operator*=(const vec_w rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator*(rhs));
    }
    inline void operator*=(const float rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator*(vec_x(rhs)));
    }

    inline void operator/=(const vec_x rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline void operator/=(const vec_y rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline void operator/=(const vec_z rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline void operator/=(const vec_w rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator/(rhs));
    }
    inline void operator/=(const float rhs)
    {
        *this = vector_t(vec_template<vector_base>::operator/(rhs));
    }

    inline vector_t operator-() const
    {
        return vector_t(vec_template<vector_base>::no_w_negate());
    }

    inline vector_t aadd() const { return vector_t(vec_template<vector_base>::aadd()); }
    inline vector_t asub() const { return vector_t(vec_template<vector_base>::asub()); }
    inline void adda(const vector_t rhs) const { vec_template<vector_base>::adda(rhs); }
    inline void adda(const point_t rhs) const;
    inline void suba(const vector_t rhs) const { vec_template<vector_base>::suba(rhs); }

    inline void madda(const vec_x rhs) const { vec_template<vector_base>::madda(rhs); }
    inline void madda(const vec_y rhs) const { vec_template<vector_base>::madda(rhs); }
    inline void madda(const vec_z rhs) const { vec_template<vector_base>::madda(rhs); }
    inline void madda(const vec_w rhs) const { vec_template<vector_base>::madda(rhs); }
    inline void madda(const float rhs) const { vec_template<vector_base>::madda(rhs); }

    inline void msuba(const vec_x rhs) const { vec_template<vector_base>::msuba(rhs); }
    inline void msuba(const vec_y rhs) const { vec_template<vector_base>::msuba(rhs); }
    inline void msuba(const vec_z rhs) const { vec_template<vector_base>::msuba(rhs); }
    inline void msuba(const vec_w rhs) const { vec_template<vector_base>::msuba(rhs); }
    inline void msuba(const float rhs) const { vec_template<vector_base>::msuba(rhs); }

    inline void madd(const vec_x rhs) const { vec_template<vector_base>::madd(rhs); }
    inline void madd(const vec_y rhs) const { vec_template<vector_base>::madd(rhs); }
    inline void madd(const vec_z rhs) const { vec_template<vector_base>::madd(rhs); }
    inline void madd(const vec_w rhs) const { vec_template<vector_base>::madd(rhs); }
    inline void madd(const float rhs) const { vec_template<vector_base>::madd(rhs); }

    inline void msub(const vec_x rhs) const { vec_template<vector_base>::msub(rhs); }
    inline void msub(const vec_y rhs) const { vec_template<vector_base>::msub(rhs); }
    inline void msub(const vec_z rhs) const { vec_template<vector_base>::msub(rhs); }
    inline void msub(const vec_w rhs) const { vec_template<vector_base>::msub(rhs); }
    inline void msub(const float rhs) const { vec_template<vector_base>::msub(rhs); }

    inline vector_t abs() const { return vector_t(vec_template<vector_base>::abs()); }
    inline vector_t sign() const { return vector_t(vec_template<vector_base>::sign()); }

    inline vector_t max(const vector_t rhs) const { return vector_t(vec_template<vector_base>::max(rhs)); }
    inline vector_t min(const vector_t rhs) const { return vector_t(vec_template<vector_base>::min(rhs)); }

    inline vec_x max() const
    {
        vec128_t result;
        asm("### vector_t max element ###\n"
            "vmaxy.x	%[result], %[_this],  %[_this] \n"
            "vmaxz.x	%[result], %[result], %[_this] \n"
            : [result] "=j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_x min() const
    {
        vec128_t result;
        asm("### vector_t min element ###\n"
            "vminiy.x	%[result], %[_this],  %[_this] \n"
            "vminiz.x	%[result], %[result], %[_this] \n"
            : [result] "=j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_x
    dot(const vector_t rhs) const
    {
        vec128_t temp, result;
        asm(
            " ### vector_t dot vector_t ### \n"
            "vmul		%[result], %[lhs], %[rhs] \n"
            "vaddw.x	%[temp], vf00, vf00w \n"
            "vadday.x	ACC, %[result], %[result] \n"
            "vmaddz.x	%[result], %[temp], %[result] \n"
            : [result] "=j"(result), [temp] "=j"(temp)
            : [lhs] "j"(this->get128()), [rhs] "j"(rhs.get128()));
        return vec_x(result);
    }

    inline vec_x
    mag_sqr() const { return this->dot(*this); }

    inline vec_x
    mag() const { return sqrtf(mag_sqr()); }

    inline vec_x
    length_sqr() const { return mag_sqr(); }

    inline vec_x
    length() const { return mag(); }

    // when square of length = 0, this returns zero vector

    inline vector_t
    normalized() const
    {
        vec128_t result, dot, one;
        int cond;
        asm(
            "### vector_t normalized ###\n"
            "vmul	%[dot], %[_this], %[_this] \n"
            "vaddw.x	%[one], vf00, vf00 \n"
            "vadday.x	ACC, %[dot], %[dot] \n"
            "vmaddz.x	%[dot], %[one], %[dot] \n"
            "vrsqrt	Q, vf00w, %[dot]x\n"
            "cfc2	%[cond], $vi17\n"
            "vsub	%[result], %[result], %[result]\n"
            "andi	%[cond], %[cond], 8\n"
            "bgtz	%[cond], 0f \n"
            "nop\n"
            "vwaitq\n"
            "vmulq	%[result], %[_this], Q\n"
            "0:\n"
            : [result] "=&j"(result), [one] "=&j"(one), [dot] "=&j"(dot), [cond] "=r"(cond)
            : [_this] "j"(*this));

        return vector_t(result);
    }

    inline vector_t
    normalize()
    {
        *this = this->normalized();
        return *this;
    }

    inline vector_t
    cross(const vector_t rhs) const
    {
        vec128_t result;
        asm(
            " ### vector_t cross vector_t ### \n"
            "vopmula.xyz	ACC, %[lhs], %[rhs] \n"
            "vopmsub.xyz	%[result], %[rhs], %[lhs] \n"
            : [result] "=j"(result)
            : [lhs] "j"(this->get128()), [rhs] "j"(rhs.get128()));
        return vector_t(result);
    }

    inline void print(void) const
    {
        float x, y, z;
        vec128_t temp0;
        asm volatile(
            "mtsab		$0, 4				# get ready to shift right 4 bytes	\n"
            "mtc1		%[_vec, %[_x]			# x = value.x	\n"
            "qfsrv		%[_temp0], $0, %[_vec]		# temp0 = value >> 8	\n"
            "mtc1		%[_temp0], %[_y]		# y = value.y \n"
            "qfsrv		%[_temp0], $0, %[_temp0]	# temp0 >>= 8 \n"
            "mtc1		%[_temp0], %[_z]		# z = value.z \n"
            : [_x] "=f"(x), [_y] "=f"(y), [_z] "=f"(z), [_temp0] "=r"(temp0)
            : [_vec] "r"(vec128));

        printf("(%f %f %f (0.0))\n", x, y, z);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: \n", vec_name);
        print();
    }
};

/********************************************
 * point_t - a "point" (as opposed to a "vector")
 * see the note for vector_t
 */

class point_t : public vec_template<point_base> {
public:
    inline point_t() {}
    inline point_t(const vec128_t value) { vec128 = value; }
    inline point_t(const vec_template<point_base> rhs) { vec128 = rhs.vec128; }

    explicit inline point_t(const vec_template<vector_base> rhs) { vec128 = rhs.vec128; }
    explicit inline point_t(const vec_template<xyz_base> rhs) { vec128 = rhs.vec128; }
    explicit inline point_t(const cpu_vec_3& rhs);

    inline void
    set(float x, float y, float z)
    {
        vec128_t temp0, temp1;
        asm(
            " ### init point_t with floats ### \n"
            ".balign	4 \n"
            "mfc1		%[_this], %[f_x]		# vec128.x = x \n"

            "mfc1		%[_temp0], %[f_y]		# temp0.x = y \n"
            "nop \n"

            "pextlw		%[_this], %[_temp0], %[_this]	# vec128 = (temp0.x << 32) | vec128.x  \n"
            "mfc1		%[_temp0], %[f_z]		# temp0.x = z \n"

            "mfc1		%[_temp1], %[f_w]		# temp1.x = w \n"
            "nop \n"

            "pextlw		%[_temp0], %[_temp1], %[_temp0]	# temp0 = (temp1.x << 32) | temp0.x  \n"
            "nop \n"

            "pcpyld		%[_this], %[_temp0], %[_this]	# vec128 |= (temp0 << 64) \n"
            : [_this] "=r"(vec128), [_temp0] "=r"(temp0), [_temp1] "=r"(temp1)
            : [f_x] "f"(x), [f_y] "f"(y), [f_z] "f"(z), [f_w] "f"(1.0f));
    }

    template <class rhs_type>
    inline void set(rhs_type rhs) { vec_template<point_base>::set(rhs); }

    inline point_t(float x, float y, float z)
    {
        set(x, y, z);
    }

    inline point_t& operator=(const vec_x rhs)
    {
        set(rhs);
        return *this;
    }
    inline point_t& operator=(const vec_y rhs)
    {
        set(rhs);
        return *this;
    }
    inline point_t& operator=(const vec_z rhs)
    {
        set(rhs);
        return *this;
    }

    inline void set_x(float rhs) { *this = vec_x(rhs); }
    inline void set_y(float rhs) { *this = vec_y(rhs); }
    inline void set_z(float rhs) { *this = vec_z(rhs); }

private:
    // leave general templates undefined.  See notes at beginning of file.

    template <class rhs_type>
    inline point_t operator+(const rhs_type rhs) const;
    template <class rhs_type>
    inline point_t operator-(const rhs_type rhs) const;
    template <class rhs_type>
    inline point_t operator*(const rhs_type rhs) const;
    template <class rhs_type>
    inline point_t operator/(rhs_type rhs) const;
    template <class rhs_type>
    inline void operator+=(const rhs_type rhs) const;
    template <class rhs_type>
    inline void operator-=(const rhs_type rhs) const;
    template <class rhs_type>
    inline void operator*=(const rhs_type rhs) const;
    template <class rhs_type>
    inline void operator/=(rhs_type rhs) const;
    template <class rhs_type>
    inline void mula(rhs_type rhs) const;
    template <class rhs_type>
    inline void adda(rhs_type rhs) const;
    template <class rhs_type>
    inline void suba(rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t madd(rhs_type rhs) const;
    template <class rhs_type>
    inline vector_t msub(rhs_type rhs) const;
    template <class rhs_type>
    inline void madda(rhs_type rhs) const;
    template <class rhs_type>
    inline void msuba(rhs_type rhs) const;

    inline point_t operator-() const;

public:
    // define legal arithmetic.

    inline point_t operator+(const vector_t rhs) const
    {
        return point_t(vec_template<point_base>::operator+(rhs));
    }
    inline point_t operator-(const vector_t rhs) const
    {
        return point_t(vec_template<point_base>::operator-(rhs));
    }
    inline vector_t operator-(const point_t rhs) const
    {
        return vector_t(vec_template<point_base>::operator-(rhs));
    }

    inline void operator+=(const vector_t rhs)
    {
        *this = point_t(vec_template<point_base>::operator+(rhs));
    }
    inline void operator-=(const vector_t rhs)
    {
        *this = point_t(vec_template<point_base>::operator-(rhs));
    }

    inline point_t aadd() const { return point_t(vec_template<point_base>::aadd()); }
    inline point_t asub() const { return point_t(vec_template<point_base>::asub()); }
    inline void adda(const vector_t rhs) const { vec_template<point_base>::adda(rhs); }
    inline void suba(const vector_t rhs) const { vec_template<point_base>::suba(rhs); }

    inline point_t abs() const { return point_t(vec_template<point_base>::abs()); }
    inline point_t sign() const { return point_t(vec_template<point_base>::sign()); }

    inline point_t max(const point_t rhs) const { return point_t(vec_template<point_base>::max(rhs)); }
    inline point_t min(const point_t rhs) const { return point_t(vec_template<point_base>::min(rhs)); }

    inline vec_x max() const
    {
        vec128_t result;
        asm("### point_t max element ###\n"
            "vmaxy.x	%[result], %[_this],  %[_this] \n"
            "vmaxz.x	%[result], %[result], %[_this] \n"
            : [result] "=j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_x min() const
    {
        vec128_t result;
        asm("### point_t min element ###\n"
            "vminiy.x	%[result], %[_this],  %[_this] \n"
            "vminiz.x	%[result], %[result], %[_this] \n"
            : [result] "=j"(result)
            : [_this] "j"(*this));
        return vec_x(result);
    }

    inline vec_x
    dot(const vector_t rhs) const
    {
        vec128_t temp, result;
        asm(
            " ### point_t projection onto vector_t ### \n"
            "vmul		%[result], %[lhs], %[rhs] \n"
            "vaddw.x	%[temp], vf00, vf00w \n"
            "vadday.x	ACC, %[result], %[result] \n"
            "vmaddz.x	%[result], %[temp], %[result] \n"
            : [result] "=j"(result), [temp] "=j"(temp)
            : [lhs] "j"(this->get128()), [rhs] "j"(rhs.get128()));
        return vec_x(result);
    }

    inline vec_x
    distance_sqr_from(const point_t rhs) const
    {
        vector_t temp = *this - rhs;
        return temp.dot(temp);
    }

    inline vec_x
    distance_from(const point_t rhs) const
    {
        return sqrtf(this->distance_sqr_from(rhs));
    }

    inline point_t
    midpoint(const point_t rhs) { return point_t((vec_template<point_base>::operator+(rhs)) * vec_x(0.5f)); }

    inline point_t
    fraction_to(const point_t rhs, vec_x fraction) { return *this + (rhs - *this) * fraction; }

    inline void print(void) const
    {
        float x, y, z;
        vec128_t temp0;
        asm volatile(
            "mtsab		$0, 4				# get ready to shift right 4 bytes	\n"
            "mtc1		%[_vec], %[_x]			# x = value.x	\n"
            "qfsrv		%[_temp0], $0, %[_vec]		# temp0 = value >> 8	\n"
            "mtc1		%[_temp0], %[_y]		# y = value.y \n"
            "qfsrv		%[_temp0], $0, %[_temp0]	# temp0 >>= 8 \n"
            "mtc1		%[_temp0], %[_z]		# z = value.z \n"
            : [_x] "=f"(x), [_y] "=f"(y), [_z] "=f"(z), [_temp0] "=r"(temp0)
            : [_vec] "r"(vec128));

        printf("(%f %f %f (1.0))\n", x, y, z);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: \n", vec_name);
        print();
    }
};

/********************************************
 * vector + point - needs to be here to inline
 */

inline point_t
vector_t::operator+(const point_t rhs) const
{
    return point_t(vec_template<vector_base>::operator+(rhs));
}

inline void
vector_t::adda(const point_t rhs) const
{
    vec_template<vector_base>::adda(rhs);
}

/********************************************
 * Conversions/constructors that need to be after all class
 * definitions so they will inline.
 */

inline vec_x::vec_x(const vec_xyz rhs) { vec128 = rhs.get128(); }
inline vec_x::vec_x(const vec_xyzw rhs) { vec128 = rhs.get128(); }
inline vec_x::vec_x(const vector_t rhs) { vec128 = rhs.vec128; }
inline vec_x::vec_x(const point_t rhs) { vec128 = rhs.vec128; }

inline vec_y::vec_y(const vec_xyz rhs) { vec128 = rhs.get128(); }
inline vec_y::vec_y(const vec_xyzw rhs) { vec128 = rhs.get128(); }
inline vec_y::vec_y(const vector_t rhs) { vec128 = rhs.vec128; }
inline vec_y::vec_y(const point_t rhs) { vec128 = rhs.vec128; }

inline vec_z::vec_z(const vec_xyz rhs) { vec128 = rhs.get128(); }
inline vec_z::vec_z(const vec_xyzw rhs) { vec128 = rhs.get128(); }
inline vec_z::vec_z(const vector_t rhs) { vec128 = rhs.vec128; }
inline vec_z::vec_z(const point_t rhs) { vec128 = rhs.vec128; }

inline vec_w::vec_w(const vec_xyzw rhs) { vec128 = rhs.get128(); }
inline vec_w::vec_w(const vector_t rhs)
{
    asm(
        " ### set vec_w to 0.0 ### \n"
        " vsub.w	%[_this], %[_this], %[_this] \n"
        : [_this] "=j"(*this));
}
inline vec_w::vec_w(const point_t rhs)
{
    asm(
        " ### set vec_w to 1.0 ### \n"
        " vmove.w	%[_this], vf00 \n"
        : [_this] "=j"(*this));
}

inline vec_xy::vec_xy(const vec_xyz rhs) { vec128 = rhs.get128(); }
inline vec_xy::vec_xy(const vec_xyzw rhs) { vec128 = rhs.get128(); }

inline vec_xyz::vec_xyz(const vec_xyzw rhs) { vec128 = rhs.get128(); }
inline vec_xyz::vec_xyz(const vector_t rhs) { vec128 = rhs.vec128; }
inline vec_xyz::vec_xyz(const point_t rhs) { vec128 = rhs.vec128; }

inline vec_xyzw::vec_xyzw(const vector_t rhs)
{
    vec128 = rhs.vec128;
    asm(
        " ### set w of vec_4 to 0.0 ### \n"
        " vsub.w	%[_this], %[_this], %[_this] \n"
        : [_this] "+j"(*this));
}
inline vec_xyzw::vec_xyzw(const point_t rhs)
{
    vec128 = rhs.vec128;
    asm(
        " ### set w of vec_4 to 1.0 ### \n"
        " vmove.w	%[_this], vf00 \n"
        : [_this] "+j"(*this));
}

/********************************************
 * We need to switch the order of expressions of the form
 * scalar * vector or scalar + vector, where 'scalar' is
 * a float or vec_? and 'vector' is a multi-element vector.
 *
 * At first I tried switching things in my compiler patch,
 * but I couldn't find a way to switch the return type
 * (these need to return multi-element vectors).
 */

//  Temporarily Out of Order
//  template <class vec_t> inline vec_t
//  operator + ( float scalar, vec_t vec ) {
//  	return vec + scalar;
//  }
template <class vec_t>
inline vec_t
operator+(vec_x scalar, vec_t vec)
{
    return vec + scalar;
}
template <class vec_t>
inline vec_t
operator+(vec_y scalar, vec_t vec)
{
    return vec + scalar;
}
template <class vec_t>
inline vec_t
operator+(vec_z scalar, vec_t vec)
{
    return vec + scalar;
}
template <class vec_t>
inline vec_t
operator+(vec_w scalar, vec_t vec)
{
    return vec + scalar;
}

//  Temporarily Out of Order
//  template <class vec_t> inline vec_t
//  operator * ( float scalar, vec_t vec ) {
//  	return vec * scalar;
//  }
template <class vec_t>
inline vec_t
operator*(vec_x scalar, vec_t vec)
{
    return vec * scalar;
}
template <class vec_t>
inline vec_t
operator*(vec_y scalar, vec_t vec)
{
    return vec * scalar;
}
template <class vec_t>
inline vec_t
operator*(vec_z scalar, vec_t vec)
{
    return vec * scalar;
}
template <class vec_t>
inline vec_t
operator*(vec_w scalar, vec_t vec)
{
    return vec * scalar;
}

inline vec_xyzw
vec_xyzw::normalize3()
{
    *this = *this / vec_xyz(*this).mag();
    return *this;
}

inline vec_xyzw
vec_xyzw::normalized3()
{
    return *this / vec_xyz(*this).mag();
}

#endif // vector_h
