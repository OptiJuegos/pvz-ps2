/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef zero_matrix_h
#define zero_matrix_h

#include "ps2s/matrix.h"
#include "ps2s/zero_vector.h"

class zero_33 {
public:
    zero_33() {}

    void set_zero() {}

    // negate

    zero_33 operator-() const { return zero_33(); }

    // transpose

    zero_33 transpose() const { return zero_33(); }
    void transpose_in_place() {}

    // zero_matrix/scalar operations

    zero_33 operator*(float scale) const { return zero_33(); }
    zero_33 operator*(vec_x scale) const { return zero_33(); }
    zero_33 operator*(vec_y scale) const { return zero_33(); }
    zero_33 operator*(vec_z scale) const { return zero_33(); }
    zero_33 operator*(vec_w scale) const { return zero_33(); }

    void operator*=(float scale) {}
    void operator*=(vec_x scale) {}
    void operator*=(vec_y scale) {}
    void operator*=(vec_z scale) {}
    void operator*=(vec_w scale) {}

    // zero_matrix/vector operations

    zero_3 operator*(vec_3 vec) const { return zero_3(); }
    zero_3 trans_mult(vec_3 vec) const { return zero_3(); }
    zero_33 mult_tilde(vec_3 vec) const { return zero_33(); }

    // zero_matrix/matrix operations

    mat_33 operator+(const mat_33& mat) const;
    mat_33 operator-(const mat_33& mat) const;

    zero_33 operator*(const mat_33& mat) const;
    zero_34 operator*(const mat_34& mat) const;

    zero_33 trans_mult(const mat_33& mat) const;
    zero_34 trans_mult(const mat_34& mat) const;

    zero_33 mult_trans(const mat_33& mat) const;
    zero_34 mult_trans(const mat_43& mat) const;

    // zero_matrix/zero_matrix operations

    zero_33 operator+(const zero_33 zero) const { return zero_33(); }
    zero_33 operator-(const zero_33 zero) const { return zero_33(); }

    void operator+=(const zero_33 zero) {}
    void operator-=(const zero_33 zero) {}

    zero_33 operator*(const zero_33 zero) const;
    zero_34 operator*(const zero_34 zero) const;

    zero_33 trans_mult(const zero_33 zero) const;
    zero_34 trans_mult(const zero_34 zero) const;

    zero_33 mult_trans(const zero_33 zero) const;
    zero_34 mult_trans(const zero_43 zero) const;

    zero_33&
    operator=(const zero_33 zero)
    {
        return *this;
    }
};

class zero_44 {
public:
    zero_44() {}

    void set_zero() {}

    // negate

    zero_44 operator-() const { return zero_44(); }

    // transpose

    zero_44 transpose() const { return zero_44(); }
    void transpose_in_place() {}

    // zero_matrix/scalar operations

    zero_44 operator*(float scale) const { return zero_44(); }
    zero_44 operator*(vec_x vec) const { return zero_44(); }
    zero_44 operator*(vec_y vec) const { return zero_44(); }
    zero_44 operator*(vec_z vec) const { return zero_44(); }
    zero_44 operator*(vec_w vec) const { return zero_44(); }

    void operator*=(float scale) {}
    void operator*=(vec_x scale) {}
    void operator*=(vec_y scale) {}
    void operator*=(vec_z scale) {}
    void operator*=(vec_w scale) {}

    // zero_matrix/vector operations

    zero_4 operator*(vec_4 vec) const { return zero_4(); }
    zero_4 trans_mult(vec_4 vec) const { return zero_4(); }

    // zero_matrix/matrix operations

    mat_44 operator+(const mat_44& mat) const;
    mat_44 operator-(const mat_44& mat) const;

    zero_44 operator*(const mat_44& mat) const;
    zero_43 operator*(const mat_43& mat) const;

    zero_44 trans_mult(const mat_44& mat) const;
    zero_43 trans_mult(const mat_43& mat) const;

    zero_44 mult_trans(const mat_44& mat) const;
    zero_43 mult_trans(const mat_34& mat) const;

    // zero_matrix/zero_matrix operations

    zero_44 operator+(const zero_44 zero) const { return zero_44(); }
    zero_44 operator-(const zero_44 zero) const { return zero_44(); }

    void operator+=(const zero_44 zero) {}
    void operator-=(const zero_44 zero) {}

    zero_44 operator*(const zero_44 zero) const;
    zero_43 operator*(const zero_43 zero) const;

    zero_44 trans_mult(const zero_44 zero) const;
    zero_43 trans_mult(const zero_43 zero) const;

    zero_44 mult_trans(const zero_44 zero) const;
    zero_43 mult_trans(const zero_34 zero) const;

    zero_44&
    operator=(const zero_44 zero)
    {
        return *this;
    }
};

class zero_43 {
public:
    zero_43() {}

    void set_zero() {}

    // negate

    zero_43 operator-() const { return zero_43(); }

    // transpose

    zero_34 transpose() const;

    // zero_matrix/scalar operations

    zero_43 operator*(float scale) const { return zero_43(); }
    zero_43 operator*(vec_x vec) const { return zero_43(); }
    zero_43 operator*(vec_y vec) const { return zero_43(); }
    zero_43 operator*(vec_z vec) const { return zero_43(); }
    zero_43 operator*(vec_w vec) const { return zero_43(); }

    void operator*=(float scale) {}
    void operator*=(vec_x scale) {}
    void operator*=(vec_y scale) {}
    void operator*=(vec_z scale) {}
    void operator*=(vec_w scale) {}

    // zero_matrix/vector operations

    zero_4 operator*(vec_3 vec) const { return zero_4(); }
    zero_3 trans_mult(vec_4 vec) const { return zero_3(); }
    zero_43 mult_tilde(vec_3 vec) const { return zero_43(); }

    // zero_matrix/matrix operations

    mat_43 operator+(const mat_43& mat) const;
    mat_43 operator-(const mat_43& mat) const;

    zero_43 operator*(const mat_33& mat) const;
    zero_44 operator*(const mat_34& mat) const;

    zero_33 trans_mult(const mat_43& mat) const;
    zero_34 trans_mult(const mat_44& mat) const;

    zero_44 mult_trans(const mat_43& mat) const;
    zero_43 mult_trans(const mat_33& mat) const;

    // zero_matrix/zero_matrix operations

    zero_43 operator+(const zero_43 zero) const { return zero_43(); }
    zero_43 operator-(const zero_43 zero) const { return zero_43(); }

    void operator+=(const zero_43 zero) {}
    void operator-=(const zero_43 zero) {}

    zero_43 operator*(const zero_33 zero) const;
    zero_44 operator*(const zero_34 zero) const;

    zero_33 trans_mult(const zero_43 zero) const;
    zero_34 trans_mult(const zero_44 zero) const;

    zero_44 mult_trans(const zero_43 zero) const;
    zero_43 mult_trans(const zero_33 zero) const;

    zero_43&
    operator=(const zero_43 zero)
    {
        return *this;
    }
};

class zero_34 {
public:
    zero_34() {}

    void set_zero() {}

    // negate

    zero_34 operator-() const { return zero_34(); }

    // transpose

    zero_43 transpose() const { return zero_43(); }

    // zero_matrix/scalar operations

    zero_34 operator*(float scale) const { return zero_34(); }
    zero_34 operator*(vec_x vec) const { return zero_34(); }
    zero_34 operator*(vec_y vec) const { return zero_34(); }
    zero_34 operator*(vec_z vec) const { return zero_34(); }
    zero_34 operator*(vec_w vec) const { return zero_34(); }

    void operator*=(float scale) {}
    void operator*=(vec_x scale) {}
    void operator*=(vec_y scale) {}
    void operator*=(vec_z scale) {}
    void operator*=(vec_w scale) {}

    // zero_matrix/vector operations

    zero_3 operator*(vec_4 vec) const { return zero_3(); }
    zero_4 trans_mult(vec_3 vec) const { return zero_4(); }

    // zero_matrix/matrix operations

    mat_34 operator+(const mat_34& mat) const;
    mat_34 operator-(const mat_34& mat) const;

    zero_33 operator*(const mat_43& mat) const;
    zero_34 operator*(const mat_44& mat) const;

    zero_43 trans_mult(const mat_33& mat) const;
    zero_44 trans_mult(const mat_34& mat) const;

    zero_33 mult_trans(const mat_34& mat) const;
    zero_34 mult_trans(const mat_44& mat) const;

    // zero_matrix/zero_matrix operations

    zero_34 operator+(const zero_34 zero) const { return zero_34(); }
    zero_34 operator-(const zero_34 zero) const { return zero_34(); }

    void operator+=(const zero_34 zero) {}
    void operator-=(const zero_34 zero) {}

    zero_33 operator*(const zero_43 zero) const;
    zero_34 operator*(const zero_44 zero) const;

    zero_43 trans_mult(const zero_33 zero) const;
    zero_44 trans_mult(const zero_34 zero) const;

    zero_33 mult_trans(const zero_34 zero) const;
    zero_34 mult_trans(const zero_44 zero) const;

    zero_34&
    operator=(const zero_34 zero)
    {
        return *this;
    }
};

// zero_matrix operations

// zero_3

inline zero_3 zero_3::operator*(const mat_33& mat) const { return zero_3(); }
inline zero_4 zero_3::operator*(const mat_34& mat) const { return zero_4(); }

inline zero_3 zero_3::operator*(const zero_33 zero) const { return zero_3(); }
inline zero_4 zero_3::operator*(const zero_34 zero) const { return zero_4(); }

inline zero_33 zero_3::tensor_mult(const vec_3 vec) const { return zero_33(); }
inline zero_43 zero_3::tensor_mult(const vec_4 vec) const { return zero_43(); }

inline zero_33 zero_3::tensor_mult(const zero_3 zero) const { return zero_33(); }
inline zero_43 zero_3::tensor_mult(const zero_4 zero) const { return zero_43(); }

// zero_4

inline zero_3 zero_4::operator*(const mat_43& mat) const { return zero_3(); }
inline zero_4 zero_4::operator*(const mat_44& mat) const { return zero_4(); }

inline zero_3 zero_4::operator*(const zero_43 zero) const { return zero_3(); }
inline zero_4 zero_4::operator*(const zero_44 zero) const { return zero_4(); }

inline zero_34 zero_4::tensor_mult(const vec_3 vec) const { return zero_34(); }
inline zero_44 zero_4::tensor_mult(const vec_4 vec) const { return zero_44(); }

inline zero_34 zero_4::tensor_mult(const zero_3 zero) const { return zero_34(); }
inline zero_44 zero_4::tensor_mult(const zero_4 zero) const { return zero_44(); }

// zero_33

inline mat_33 zero_33::operator+(const mat_33& mat) const return result;
{
    result = mat;
}
inline mat_33 zero_33::operator-(const mat_33& mat) const return result;
{
    result = -mat;
}
inline zero_33 zero_33::operator*(const mat_33& mat) const { return zero_33(); }
inline zero_34 zero_33::operator*(const mat_34& mat) const { return zero_34(); }
inline zero_33 zero_33::trans_mult(const mat_33& mat) const { return zero_33(); }
inline zero_34 zero_33::trans_mult(const mat_34& mat) const { return zero_34(); }
inline zero_33 zero_33::mult_trans(const mat_33& mat) const { return zero_33(); }
inline zero_34 zero_33::mult_trans(const mat_43& mat) const { return zero_34(); }

inline zero_33 zero_33::operator*(const zero_33 zero) const { return zero_33(); }
inline zero_34 zero_33::operator*(const zero_34 zero) const { return zero_34(); }
inline zero_33 zero_33::trans_mult(const zero_33 zero) const { return zero_33(); }
inline zero_34 zero_33::trans_mult(const zero_34 zero) const { return zero_34(); }
inline zero_33 zero_33::mult_trans(const zero_33 zero) const { return zero_33(); }
inline zero_34 zero_33::mult_trans(const zero_43 zero) const { return zero_34(); }

// zero_43

inline zero_34 zero_43::transpose() const { return zero_34(); }
inline mat_43 zero_43::operator+(const mat_43& mat) const return result;
{
    result = mat;
}
inline mat_43 zero_43::operator-(const mat_43& mat) const return result;
{
    result = -mat;
}
inline zero_43 zero_43::operator*(const mat_33& mat) const { return zero_43(); }
inline zero_44 zero_43::operator*(const mat_34& mat) const { return zero_44(); }
inline zero_33 zero_43::trans_mult(const mat_43& mat) const { return zero_33(); }
inline zero_34 zero_43::trans_mult(const mat_44& mat) const { return zero_34(); }
inline zero_44 zero_43::mult_trans(const mat_43& mat) const { return zero_44(); }
inline zero_43 zero_43::mult_trans(const mat_33& mat) const { return zero_43(); }

inline zero_43 zero_43::operator*(const zero_33 zero) const { return zero_43(); }
inline zero_44 zero_43::operator*(const zero_34 zero) const { return zero_44(); }
inline zero_33 zero_43::trans_mult(const zero_43 zero) const { return zero_33(); }
inline zero_34 zero_43::trans_mult(const zero_44 zero) const { return zero_34(); }
inline zero_44 zero_43::mult_trans(const zero_43 zero) const { return zero_44(); }
inline zero_43 zero_43::mult_trans(const zero_33 zero) const { return zero_43(); }

// zero_34

inline mat_34 zero_34::operator+(const mat_34& mat) const return result;
{
    result = mat;
}
inline mat_34 zero_34::operator-(const mat_34& mat) const return result;
{
    result = -mat;
}
inline zero_33 zero_34::operator*(const mat_43& mat) const { return zero_33(); }
inline zero_34 zero_34::operator*(const mat_44& mat) const { return zero_34(); }
inline zero_43 zero_34::trans_mult(const mat_33& mat) const { return zero_43(); }
inline zero_44 zero_34::trans_mult(const mat_34& mat) const { return zero_44(); }
inline zero_33 zero_34::mult_trans(const mat_34& mat) const { return zero_33(); }
inline zero_34 zero_34::mult_trans(const mat_44& mat) const { return zero_34(); }

inline zero_33 zero_34::operator*(const zero_43 zero) const { return zero_33(); }
inline zero_34 zero_34::operator*(const zero_44 zero) const { return zero_34(); }
inline zero_43 zero_34::trans_mult(const zero_33 zero) const { return zero_43(); }
inline zero_44 zero_34::trans_mult(const zero_34 zero) const { return zero_44(); }
inline zero_33 zero_34::mult_trans(const zero_34 zero) const { return zero_33(); }
inline zero_34 zero_34::mult_trans(const zero_44 zero) const { return zero_34(); }

// zero_44

inline mat_44 zero_44::operator+(const mat_44& mat) const return result;
{
    result = mat;
}
inline mat_44 zero_44::operator-(const mat_44& mat) const return result;
{
    result = -mat;
}
inline zero_44 zero_44::operator*(const mat_44& mat) const { return zero_44(); }
inline zero_43 zero_44::operator*(const mat_43& mat) const { return zero_43(); }
inline zero_44 zero_44::trans_mult(const mat_44& mat) const { return zero_44(); }
inline zero_43 zero_44::trans_mult(const mat_43& mat) const { return zero_43(); }
inline zero_44 zero_44::mult_trans(const mat_44& mat) const { return zero_44(); }
inline zero_43 zero_44::mult_trans(const mat_34& mat) const { return zero_43(); }

inline zero_44 zero_44::operator*(const zero_44 zero) const { return zero_44(); }
inline zero_43 zero_44::operator*(const zero_43 zero) const { return zero_43(); }
inline zero_44 zero_44::trans_mult(const zero_44 zero) const { return zero_44(); }
inline zero_43 zero_44::trans_mult(const zero_43 zero) const { return zero_43(); }
inline zero_44 zero_44::mult_trans(const zero_44 zero) const { return zero_44(); }
inline zero_43 zero_44::mult_trans(const zero_34 zero) const { return zero_43(); }

// vec_xyz

inline zero_33 vec_xyz::tilde_mult(const zero_33 zero) const { return zero_33(); }
inline zero_34 vec_xyz::tilde_mult(const zero_34 zero) const { return zero_34(); }
inline zero_3 vec_xyz::operator*(const zero_33 zero) const { return zero_3(); }
inline zero_4 vec_xyz::operator*(const zero_34 zero) const { return zero_4(); }
inline zero_33 vec_xyz::tensor_mult(const zero_3 zero) const { return zero_33(); }
inline zero_43 vec_xyz::tensor_mult(const zero_4 zero) const { return zero_43(); }

// vec_xyzw

inline zero_3 vec_xyzw::operator*(const zero_43 zero) const { return zero_3(); }
inline zero_4 vec_xyzw::operator*(const zero_44 zero) const { return zero_4(); }
inline zero_34 vec_xyzw::tensor_mult(const zero_3 zero) const { return zero_34(); }
inline zero_44 vec_xyzw::tensor_mult(const zero_4 zero) const { return zero_44(); }

// mat_33

inline mat_33::mat_33(const zero_33) { set_zero(); }
inline mat_33 mat_33::operator+(const zero_33 zero) const return result;
{
    result = *this;
}
inline mat_33 mat_33::operator-(const zero_33 zero) const return result;
{
    result = *this;
}
inline void mat_33::operator+=(const zero_33 zero) {}
inline void mat_33::operator-=(const zero_33 zero) {}
inline mat_33& mat_33::operator=(const zero_33 zero)
{
    set_zero();
    return *this;
}
inline zero_33 mat_33::operator*(const zero_33 zero) const { return zero_33(); }
inline zero_34 mat_33::operator*(const zero_34 zero) const { return zero_34(); }
inline zero_33 mat_33::trans_mult(const zero_33 zero) const { return zero_33(); }
inline zero_34 mat_33::trans_mult(const zero_34 zero) const { return zero_34(); }
inline zero_33 mat_33::mult_trans(const zero_33 zero) const { return zero_33(); }
inline zero_34 mat_33::mult_trans(const zero_43 zero) const { return zero_34(); }

// mat_43

inline mat_43::mat_43(const zero_43) { set_zero(); }
inline mat_43 mat_43::operator+(const zero_43 zero) const return result;
{
    result = *this;
}
inline mat_43 mat_43::operator-(const zero_43 zero) const return result;
{
    result = *this;
}
inline void mat_43::operator+=(const zero_43 zero) {}
inline void mat_43::operator-=(const zero_43 zero) {}
inline mat_43& mat_43::operator=(const zero_43 zero)
{
    set_zero();
    return *this;
}
inline zero_43 mat_43::operator*(const zero_33 zero) const { return zero_43(); }
inline zero_44 mat_43::operator*(const zero_34 zero) const { return zero_44(); }
inline zero_33 mat_43::trans_mult(const zero_43 zero) const { return zero_33(); }
inline zero_34 mat_43::trans_mult(const zero_44 zero) const { return zero_34(); }
inline zero_44 mat_43::mult_trans(const zero_43 zero) const { return zero_44(); }
inline zero_43 mat_43::mult_trans(const zero_33 zero) const { return zero_43(); }

// mat_34

inline mat_34::mat_34(const zero_34) { set_zero(); }
inline mat_34 mat_34::operator+(const zero_34 zero) const return result;
{
    result = *this;
}
inline mat_34 mat_34::operator-(const zero_34 zero) const return result;
{
    result = *this;
}
inline void mat_34::operator+=(const zero_34 zero) {}
inline void mat_34::operator-=(const zero_34 zero) {}
inline mat_34& mat_34::operator=(const zero_34 zero)
{
    set_zero();
    return *this;
}
inline zero_33 mat_34::operator*(const zero_43 zero) const { return zero_33(); }
inline zero_34 mat_34::operator*(const zero_44 zero) const { return zero_34(); }
inline zero_43 mat_34::trans_mult(const zero_33 zero) const { return zero_43(); }
inline zero_44 mat_34::trans_mult(const zero_34 zero) const { return zero_44(); }
inline zero_33 mat_34::mult_trans(const zero_34 zero) const { return zero_33(); }
inline zero_34 mat_34::mult_trans(const zero_44 zero) const { return zero_34(); }

// mat_44

inline mat_44::mat_44(const zero_44) { set_zero(); }
inline mat_44 mat_44::operator+(const zero_44 zero) const return result;
{
    result = *this;
}
inline mat_44 mat_44::operator-(const zero_44 zero) const return result;
{
    result = *this;
}
inline void mat_44::operator+=(const zero_44 zero) {}
inline void mat_44::operator-=(const zero_44 zero) {}
inline mat_44& mat_44::operator=(const zero_44 zero)
{
    set_zero();
    return *this;
}
inline zero_44 mat_44::operator*(const zero_44 zero) const { return zero_44(); }
inline zero_43 mat_44::operator*(const zero_43 zero) const { return zero_43(); }
inline zero_44 mat_44::trans_mult(const zero_44 zero) const { return zero_44(); }
inline zero_43 mat_44::trans_mult(const zero_43 zero) const { return zero_43(); }
inline zero_44 mat_44::mult_trans(const zero_44 zero) const { return zero_44(); }
inline zero_43 mat_44::mult_trans(const zero_34 zero) const { return zero_43(); }

#endif
