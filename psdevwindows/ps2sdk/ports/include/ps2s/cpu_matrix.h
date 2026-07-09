/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_cpu_matrix_h
#define ps2s_cpu_matrix_h

#include "ps2s/cpu_vector.h"

class cpu_mat_44 {
    cpu_vec_4 col0, col1, col2, col3;

public:
    inline cpu_mat_44() {}
    inline cpu_mat_44(cpu_vec_4 c0, cpu_vec_4 c1, cpu_vec_4 c2, cpu_vec_4 c3)
    {
        col0 = c0;
        col1 = c1;
        col2 = c2;
        col3 = c3;
    }

    inline void
    set_identity()
    {
        col0.set(1, 0, 0, 0);
        col1.set(0, 1, 0, 0);
        col2.set(0, 0, 1, 0);
        col3.set(0, 0, 0, 1);
    }

    inline cpu_vec_4 get_col0() const { return col0; }
    inline cpu_vec_4 get_col1() const { return col1; }
    inline cpu_vec_4 get_col2() const { return col2; }
    inline cpu_vec_4 get_col3() const { return col3; }

    inline void set_col0(const cpu_vec_xyzw& col) { col0 = col; }
    inline void set_col1(const cpu_vec_xyzw& col) { col1 = col; }
    inline void set_col2(const cpu_vec_xyzw& col) { col2 = col; }
    inline void set_col3(const cpu_vec_xyzw& col) { col3 = col; }

    inline cpu_vec_4
    operator*(const cpu_vec_4& rhs) const;

    inline cpu_mat_44
    operator*(const cpu_mat_44& rhs) const;

    inline void
    set_scale(const cpu_vec_3& scale)
    {
        set_identity();
        col0[0] = scale(0);
        col1[1] = scale(1);
        col2[2] = scale(2);
        col3[3] = 1.0f;
    }

    inline void
    set_translate(const cpu_vec_3& offsets)
    {
        set_identity();
        col3 = offsets;
    }

    void set_rotate(float angle, cpu_vec_xyz axis);

    void print() const
    {
        col0.print();
        col1.print();
        col2.print();
        col3.print();
    }

    cpu_mat_44 transpose() const;
};

inline cpu_vec_4
    cpu_mat_44::operator*(const cpu_vec_4& rhs) const
{
    cpu_vec_4 result;

    cpu_vec_4 row0(col0(0), col1(0), col2(0), col3(0));
    result[0] = row0.dot(rhs);

    cpu_vec_4 row1(col0(1), col1(1), col2(1), col3(1));
    result[1] = row1.dot(rhs);

    cpu_vec_4 row2(col0(2), col1(2), col2(2), col3(2));
    result[2] = row2.dot(rhs);

    cpu_vec_4 row3(col0(3), col1(3), col2(3), col3(3));
    result[3] = row3.dot(rhs);

    return result;
}

inline cpu_mat_44
    cpu_mat_44::operator*(const cpu_mat_44& rhs) const
{
    cpu_mat_44 result;

    result.col0 = *this * rhs.get_col0();
    result.col1 = *this * rhs.get_col1();
    result.col2 = *this * rhs.get_col2();
    result.col3 = *this * rhs.get_col3();

    return result;
}

#endif // ps2s_cpu_matrix_h
