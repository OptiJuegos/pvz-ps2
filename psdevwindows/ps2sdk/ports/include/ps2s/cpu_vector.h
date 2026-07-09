/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_cpu_vector_h
#define ps2s_cpu_vector_h

#include <math.h>   // for sqrtf()
#include <stdio.h>  // for printf()
#include <stdlib.h> // for exit()

#ifndef NO_VU0_VECTORS
#include "ps2s/vector.h"
#endif

/********************************************
 * cpu_vec_3 (or cpu_vec_xyz)
 */

class cpu_vec_3 {
public:
    float x, y, z;

    inline cpu_vec_3() {}

    inline cpu_vec_3(const cpu_vec_3& vec)
    {
        x = vec.x;
        y = vec.y;
        z = vec.z;
    };

    inline cpu_vec_3(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    inline void set(float _x, float _y, float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

#ifndef NO_VU0_VECTORS
    inline void set(const vec128_t vec)
    {
        vec128_t temp0;
        asm volatile(
            ".if %A0 \n "

            "mtsab  $0, 4                # get ready to shift right 4 bytes  \n"
            "move    _x, _vec            # x = value.x  \n"
            "qfsrv  _temp0, $0, _vec    # temp0 = value >> 32  \n"
            "move    _y, _temp0          # y = value.y \n"
            "qfsrv  _temp0, $0, _temp0  # temp0 >>= 32 \n"
            "move    _z, _temp0          # z = value.z \n"

            ".else \n"

            "mtsab  $0, 4                # get ready to shift right 4 bytes  \n"
            "mtc1    _vec, _x            # x = value.x  \n"
            "qfsrv  _temp0, $0, _vec    # temp0 = value >> 32  \n"
            "mtc1    _temp0, _y          # y = value.y \n"
            "qfsrv  _temp0, $0, _temp0  # temp0 >>= 32 \n"
            "mtc1    _temp0, _z          # z = value.z \n"

            ".endif \n"
            : "=&r,f _x"(x), "=&r,f _y"(y), "=&r,f _z"(z), "=&r,&r _temp0"(temp0)
            : "r,r _vec"(vec));
    }

    explicit inline cpu_vec_3(const vec_3 vec) { set(vec.vec128); }
    explicit inline cpu_vec_3(const vector_t vec) { set(vec.vec128); }
    explicit inline cpu_vec_3(const point_t pnt) { set(pnt.vec128); }
#endif

    inline float& operator[](const int index)
    {
        if (index == 0)
            return x;
        else if (index == 1)
            return y;
        else if (index == 2)
            return z;
        else {
            printf("cpu_vec_3::operator[]: invalid index = %d\n", index);
            exit(-1);
            return x;
        }
    }

    // use () to access individual elements when you don't need write access
    inline float operator()(const int index) const
    {
        if (index == 0)
            return x;
        else if (index == 1)
            return y;
        else if (index == 2)
            return z;
        else {
            printf("cpu_vec_3::operator(): invalid index = %d\n", index);
            exit(-1);
            return x;
        }
    }

    inline cpu_vec_3& operator=(const cpu_vec_3& vec)
    {
        x = vec.x;
        y = vec.y;
        z = vec.z;
        return *this;
    }

    inline bool operator==(const cpu_vec_3& rhs)
    {
        return (x == rhs.x
            && y == rhs.y
            && z == rhs.z);
    }

    inline bool operator!=(const cpu_vec_3& rhs)
    {
        return (x != rhs.x
            || y != rhs.y
            || z != rhs.z);
    }

    inline cpu_vec_3 operator+(const cpu_vec_3& vec);
    inline cpu_vec_3 operator-(const cpu_vec_3& vec);
    inline cpu_vec_3 operator-();
    inline cpu_vec_3 operator*(float scalar);
    inline float dot(const cpu_vec_3& vec) const;
    inline cpu_vec_3 cross(const cpu_vec_3& vec);
    inline cpu_vec_3 normalized();
    inline void normalize()
    {
        *this = this->normalized();
    }

    void print() const
    {
        printf("(%f %f %f *)\n", x, y, z);
    }
};

typedef cpu_vec_3 cpu_vec_xyz;

/********************************************
 * cpu_vec_4 (or cpu_vec_xyzw)
 */

class cpu_vec_4 {
public:
    float x, y, z, w;

    inline cpu_vec_4() {}

    inline cpu_vec_4(const cpu_vec_4& vec)
    {
        x = vec.x;
        y = vec.y;
        z = vec.z;
        w = vec.w;
    };

    inline cpu_vec_4(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

    inline void set(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

#ifndef NO_VU0_VECTORS
    inline void set(const vec128_t vec)
    {
        vec128_t temp0;
        asm volatile(

            ".if %A0 \n "

            "mtsab  $0, 4                # get ready to shift right 4 bytes  \n"
            "move    _x, _vec            # x = value.x  \n"
            "qfsrv  _temp0, $0, _vec    # temp0 = value >> 32  \n"
            "move    _y, _temp0          # y = value.y \n"
            "qfsrv  _temp0, $0, _temp0  # temp0 >>= 32 \n"
            "move    _z, _temp0          # z = value.z \n"
            "qfsrv  _temp0, $0, _temp0  # temp0 >>= 32 \n"
            "move    _w, _temp0          # w = value.w \n"

            ".else \n"

            "mtsab  $0, 4                # get ready to shift right 4 bytes  \n"
            "mtc1    _vec, _x            # x = value.x  \n"
            "qfsrv  _temp0, $0, _vec    # temp0 = value >> 32  \n"
            "mtc1    _temp0, _y          # y = value.y \n"
            "qfsrv  _temp0, $0, _temp0  # temp0 >>= 32 \n"
            "mtc1    _temp0, _z          # z = value.z \n"
            "qfsrv  _temp0, $0, _temp0  # temp0 >>= 32 \n"
            "mtc1    _temp0, _w          # w = value.w \n"

            ".endif \n"
            : "=&r,f _x"(x), "=&r,f _y"(y), "=&r,f _z"(z), "=&r,f _w"(w), "=&r,&r _temp0"(temp0)
            : "r,r _vec"(vec));
    }

    explicit inline cpu_vec_4(const vec_4 vec) { set(vec.vec128); }
    explicit inline cpu_vec_4(const vector_t vec)
    {
        set(vec.vec128);
        w = 0.0f;
    }
    explicit inline cpu_vec_4(const point_t vec)
    {
        set(vec.vec128);
        w = 1.0f;
    }
#endif

    inline float& operator[](const int index)
    {
        if (index == 0)
            return x;
        else if (index == 1)
            return y;
        else if (index == 2)
            return z;
        else if (index == 3)
            return w;
        else {
            printf("cpu_vec_4::operator[]: invalid index = %d\n", index);
            exit(-1);
            return x;
        }
    }

    // use () to access individual elements when you don't need write access
    inline float operator()(const int index) const
    {
        if (index == 0)
            return x;
        else if (index == 1)
            return y;
        else if (index == 2)
            return z;
        else if (index == 3)
            return w;
        else {
            printf("cpu_vec_4::operator(): invalid index = %d\n", index);
            exit(-1);
            return x;
        }
    }

    inline cpu_vec_4& operator=(const cpu_vec_4& vec)
    {
        x = vec.x;
        y = vec.y;
        z = vec.z;
        w = vec.w;
        return *this;
    }

    inline cpu_vec_4& operator=(const cpu_vec_3& vec)
    {
        x = vec.x;
        y = vec.y;
        z = vec.z;
        return *this;
    }

    inline bool operator==(const cpu_vec_4& rhs)
    {
        return (x == rhs.x
            && y == rhs.y
            && z == rhs.z
            && w == rhs.w);
    }

    inline bool operator!=(const cpu_vec_4& rhs)
    {
        return (x != rhs.x
            || y != rhs.y
            || z != rhs.z
            || w != rhs.w);
    }

    inline cpu_vec_4 operator+(const cpu_vec_4& vec);
    inline cpu_vec_4 operator-(const cpu_vec_4& vec);
    inline cpu_vec_4 operator-();
    inline cpu_vec_4 operator*(const cpu_vec_4& vec);
    inline cpu_vec_4 operator*(float scalar);
    inline float dot(const cpu_vec_4& vec) const;
    inline float length() const;
    inline cpu_vec_4 normalized();
    inline const cpu_vec_4& normalize();

    void print() const
    {
        printf("(%f %f %f %f)\n", x, y, z, w);
    }
};

typedef cpu_vec_4 cpu_vec_xyzw;

/********************************************
 * cpu_vec_3 methods
 */

inline cpu_vec_3
cpu_vec_3::operator+(const cpu_vec_3& vec)
{
    cpu_vec_3 result;
#ifdef NO_ASM

    result.x = x + vec.x;
    result.y = y + vec.y;
    result.z = z + vec.z;

#else

    asm(" ### cpu_vec_3 + cpu_vec_3 ### \n"
        "add.s rx, v0x, v1x \n"
        "add.s ry, v0y, v1y \n"
        "add.s rz, v0z, v1z \n"
        : "=&f rx"(result.x), "=&f ry"(result.y), "=&f rz"(result.z)
        : "f v0x"(x), "f v0y"(y), "f v0z"(z),
        "f v1x"(vec.x), "f v1y"(vec.y), "f v1z"(vec.z));

#endif
    return result;
}

inline cpu_vec_3
cpu_vec_3::operator-(const cpu_vec_3& vec)
{
    cpu_vec_3 result;
#ifdef NO_ASM

    result.x = x - vec.x;
    result.y = y - vec.y;
    result.z = z - vec.z;

#else

    asm(" ### cpu_vec_3 - cpu_vec_3 ### \n"
        "sub.s rx, v0x, v1x \n"
        "sub.s ry, v0y, v1y \n"
        "sub.s rz, v0z, v1z \n"
        : "=&f rx"(result.x), "=&f ry"(result.y), "=&f rz"(result.z)
        : "f v0x"(x), "f v0y"(y), "f v0z"(z),
        "f v1x"(vec.x), "f v1y"(vec.y), "f v1z"(vec.z));

#endif
    return result;
}

inline cpu_vec_3
cpu_vec_3::operator-()
{
    cpu_vec_3 result;
#ifdef NO_ASM

    result.x = -x;
    result.y = -y;
    result.z = -z;

#else

    asm(" ### -cpu_vec_3 ### \n"
        "neg.s %[rx], %[vx] \n"
        "neg.s %[ry], %[vy] \n"
        "neg.s %[rz], %[vz] \n"
        : [rx] "=&f"(result.x), [ry] "=&f"(result.y), [rz] "=&f"(result.z)
        : [vx] "f"(x), [vy] "f"(y), [vz] "f"(z));

#endif
    return result;
}

inline cpu_vec_3
    cpu_vec_3::operator*(float scalar)
{
    cpu_vec_3 result;
#ifdef NO_ASM

    result.x = x * scalar;
    result.y = y * scalar;
    result.z = z * scalar;

#else

    asm(" ### cpu_vec_3 * scalar ### \n"
        "mul.s %[rx], %[vx], %[scalar] \n"
        "mul.s %[ry], %[vy], %[scalar] \n"
        "mul.s %[rz], %[vz], %[scalar] \n"
        : [rx] "=&f"(result.x), [ry] "=&f"(result.y), [rz] "=&f"(result.z)
        : [vx] "f"(x), [vy] "f"(y), [vz] "f"(z), [scalar] "f"(scalar));

#endif
    return result;
}

inline float
cpu_vec_3::dot(const cpu_vec_3& vec) const
{
    float result;

#ifdef NO_ASM

    result = (*this)(0) * vec(0)
        + (*this)(1) * vec(1)
        + (*this)(2) * vec(2);

#else

    asm(" ### cpu_vec_3 dot product ### \n"
        "mula.s  %[v0x], %[v1x] \n"
        "madda.s %[v0y], %[v1y] \n"
        "madd.s  %[result], %[v0z], %[v1z] \n"
        : [result] "=&f"(result)
        : [v0x] "f"(x), [v0y] "f"(y), [v0z] "f"(z),
        [v1x] "f"(vec.x), [v1y] "f"(vec.y), [v1z] "f"(vec.z));

#endif

    return result;
}

inline cpu_vec_3
cpu_vec_3::cross(const cpu_vec_3& vec)
{
    cpu_vec_3 result;
#ifdef NO_ASM

    result.x = y * vec.z - z * vec.y;
    result.y = z * vec.x - x * vec.z;
    result.z = x * vec.y - y * vec.x;

#else

    asm(" ### cpu_vec_3 cross product ### \n"
        "mula.s %[v0y], %[v1z] \n"
        "msub.s %[rx], %[v0z], %[v1y] \n"
        "mula.s %[v0z], %[v1x] \n"
        "msub.s %[ry], %[v0x], %[v1z] \n"
        "mula.s %[v0x], %[v1y] \n"
        "msub.s %[rz], %[v0y], %[v1x] \n"
        : [rx] "=&f"(result.x), [ry] "=&f"(result.y), [rz] "=&f"(result.z)
        : [v0x] "f"(x), [v0y] "f"(y), [v0z] "f"(z),
        [v1x] "f"(vec.x), [v1y] "f"(vec.y), [v1z] "f"(vec.z));

#endif
    return result;
}

inline cpu_vec_3
cpu_vec_3::normalized()
{
    cpu_vec_3 result;

    float lengthInv = 1.0f / sqrtf(this->dot(*this));
    result          = *this * lengthInv;

    return result;
}

/********************************************
 * cpu_vec_4 methods
 */

inline cpu_vec_4
cpu_vec_4::operator+(const cpu_vec_4& vec)
{
    cpu_vec_4 result;

    result.x = x + vec.x;
    result.y = y + vec.y;
    result.z = z + vec.z;
    result.w = w + vec.w;

    return result;
}

inline cpu_vec_4
cpu_vec_4::operator-(const cpu_vec_4& vec)
{
    cpu_vec_4 result;

    result.x = x - vec.x;
    result.y = y - vec.y;
    result.z = z - vec.z;
    result.w = w - vec.w;

    return result;
}

inline cpu_vec_4
cpu_vec_4::operator-()
{
    cpu_vec_4 result;

    result.x = -x;
    result.y = -y;
    result.z = -z;
    result.w = -w;

    return result;
}

inline cpu_vec_4
    cpu_vec_4::operator*(const cpu_vec_4& vec)
{
    cpu_vec_4 result;

    result.x = x * vec.x;
    result.y = y * vec.y;
    result.z = z * vec.z;
    result.w = w * vec.w;

    return result;
}

inline cpu_vec_4
    cpu_vec_4::operator*(float scalar)
{
    cpu_vec_4 result;

    result.x = x * scalar;
    result.y = y * scalar;
    result.z = z * scalar;
    result.w = w * scalar;

    return result;
}

inline float
cpu_vec_4::dot(const cpu_vec_4& vec) const
{
    float result;

#ifdef NO_ASM

    result = x * vec.x + y * vec.y + z * vec.z + w * vec.w;

#else

    asm(" ### cpu_vec_4 dot product ### \n"
        "mula.s  %[v0x], %[v1x] \n"
        "madda.s %[v0y], %[v1y] \n"
        "madda.s %[v0z], %[v1z] \n"
        "madd.s  %[result], %[v0w], %[v1w] \n"
        : [result] "=&f"(result)
        : [v0x] "f"(x), [v0y] "f"(y), [v0z] "f"(z), [v0w] "f"(w),
        [v1x] "f"(vec.x), [v1y] "f"(vec.y), [v1z] "f"(vec.z), [v1w] "f"(vec.w));

#endif

    return result;
}

inline float
cpu_vec_4::length() const
{
    return sqrtf(this->dot(*this));
}

inline cpu_vec_4
cpu_vec_4::normalized()
{
    cpu_vec_4 result;

    float lengthInv = 1.0f / length();
    result          = *this * lengthInv;

    return result;
}

const cpu_vec_4&
cpu_vec_4::normalize()
{
    *this = normalized();
    return *this;
}

/********************************************
 * initialization of vu0 vectors with cpu_vectors
 */

#ifndef NO_VU0_VECTORS
inline vec_xyz::vec_xyz(const cpu_vec_3& rhs)
{

    vec128_t temp;
    asm("### construct vec_3 with cpu_vec_3 ### \n"
        "pcpyld %[_temp], %[_y], %[_x] \n"
        "ppacw %[_result], %[_z], %[_temp] \n"
        : [_result] "=&r"(vec128), [_temp] "=&r"(temp)
        : [_x] "r"(rhs.x), [_y] "r"(rhs.y), [_z] "r"(rhs.z));
}

inline vector_t::vector_t(const cpu_vec_3& rhs)
{

    vec128_t temp;
    asm("### construct vector_t with cpu_vec_3 ### \n"
        "pcpyld %[_temp], %[_y], %[_x] \n"
        "ppacw %[_result], %[_z], %[_temp] \n"
        : [_result] "=&r"(vec128), [_temp] "=&r"(temp)
        : [_x] "r"(rhs.x), [_y] "r"(rhs.y), [_z] "r"(rhs.z));
}

inline point_t::point_t(const cpu_vec_3& rhs)
{

    vec128_t temp;
    asm("### construct point_t with cpu_vec_3 ### \n"
        "pcpyld %[_temp], %[_y], %[_x] \n"
        "ppacw %[_result], %[_z], %[_temp] \n"
        : [_result] "=&r"(vec128), [_temp] "=&r"(temp)
        : [_x] "r"(rhs.x), [_y] "r"(rhs.y), [_z] "r"(rhs.z));
}

inline vec_xyzw::vec_xyzw(const cpu_vec_4& rhs)
{
    vec128_t tempxz, tempyw;

    asm("### construct vec_4 with cpu_vec_4 ### \n"
        "pextlw %[tempxz], %[_z], %[_x] \n"
        "pextlw %[tempyw], %[_w], %[_y] \n"
        "pextlw %[result], %[tempyw], %[tempxz]\n"
        : [result] "=&r"(vec128), [tempxz] "=&r"(tempxz), [tempyw] "=&r"(tempyw)
        : [_x] "r"(rhs.x), [_y] "r"(rhs.y), [_z] "r"(rhs.z), [_w] "r"(rhs.w));
}
#endif // NO_VU0_VECTORS

#endif // ps2s_cpu_vector_h
