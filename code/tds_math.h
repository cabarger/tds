#ifndef TDS_MATH_H

#include "tds_types.h"

union v3
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
    };
    r32 m_[3];
};

inline v3
v3_(r32 x, r32 y, r32 z)
{
    v3 Result;

    Result.m_[0] = x;
    Result.m_[1] = y;
    Result.m_[2] = z;

    return(Result);
}


union v2
{
    struct
    {
        r32 x;
        r32 y;
    };
    r32 m_[2];
};

#define TDS_MATH_H
#endif
