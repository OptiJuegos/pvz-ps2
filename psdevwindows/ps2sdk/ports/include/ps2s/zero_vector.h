/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef zero_vector_h
#define zero_vector_h

#include "ps2s/vector.h"

class zero_3 {
public:
    zero_3() {}

    void set_zero() {}

    vec_3 operator+(const vec_3 vec) const { return vec; }
    vec_3 operator-(const vec_3 vec) const { return -vec; }

    zero_3 operator+(const zero_3 zero) const { return zero_3(); }
    zero_3 operator-(const zero_3 zero) const { return zero_3(); }

    zero_3 operator*(const float scale) const { return zero_3(); }
    zero_3 operator-() const { return zero_3(); }

    void operator+=(const zero_3 zero) {}
    void operator-=(const zero_3 zero) {}
    void operator*=(const float scale) {}

    zero_3 operator*(const mat_33& mat) const;
    zero_4 operator*(const mat_34& mat) const;

    zero_3 operator*(const zero_33 zero) const;
    zero_4 operator*(const zero_34 zero) const;

    zero_33 tensor_mult(const vec_3 vec) const;
    zero_43 tensor_mult(const vec_4 vec) const;

    zero_33 tensor_mult(const zero_3 zero) const;
    zero_43 tensor_mult(const zero_4 zero) const;
};

class zero_4 {
public:
    zero_4() {}

    void set_zero() {}

    vec_4 operator+(const vec_4 vec) const { return vec; }
    vec_4 operator-(const vec_4 vec) const { return -vec; }

    zero_4 operator+(const zero_4 zero) const { return zero_4(); }
    zero_4 operator-(const zero_4 zero) const { return zero_4(); }

    zero_4 operator*(const float scale) const { return zero_4(); }
    zero_4 operator-() const { return zero_4(); }

    void operator+=(const zero_4 zero) {}
    void operator-=(const zero_4 zero) {}
    void operator*=(const float scale) {}

    zero_3 operator*(const mat_43& mat) const;
    zero_4 operator*(const mat_44& mat) const;

    zero_3 operator*(const zero_43 zero) const;
    zero_4 operator*(const zero_44 zero) const;

    zero_34 tensor_mult(const vec_3 vec) const;
    zero_44 tensor_mult(const vec_4 vec) const;

    zero_34 tensor_mult(const zero_3 zero) const;
    zero_44 tensor_mult(const zero_4 zero) const;
};

// vec_xyz

inline vec_xyz vec_xyz::operator+(const zero_3 zero) const { return *this; }
inline vec_xyz vec_xyz::operator-(const zero_3 zero) const { return *this; }
inline vec_xyz& vec_xyz::operator=(const zero_3 zero)
{
    set_zero();
    return *this;
}

// vec_xyzw

inline vec_xyzw vec_xyzw::operator+(const zero_4 zero) const { return *this; }
inline vec_xyzw vec_xyzw::operator-(const zero_4 zero) const { return *this; }
inline vec_xyzw& vec_xyzw::operator=(const zero_4 zero)
{
    set_zero();
    return *this;
}

#endif
