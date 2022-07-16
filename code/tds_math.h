#ifndef TDS_MATH_H

#include "tds_types.h"
#include <math.h>

#define TDS_PI32 3.14159265359f

inline r32
ToRadians(r32 Degrees)
{
	r32 Result;

	Result = Degrees * (TDS_PI32 / 180.0f);

	return (Result);
}

union v3
{
    struct
    {
        r32 X;
        r32 Y;
        r32 Z;
    };
    r32 Elements[3];

    v3
    operator-(v3 Input)
    {
        v3 Result;

        Result.X = Elements[0] - Input.Elements[0];
        Result.Y = Elements[1] - Input.Elements[1];
        Result.Z = Elements[2] - Input.Elements[2];

        return (Result);
    }

    r32
    operator*(v3 Input)
    {
        r32 Result;

        Result = (X * Input.X) + (Y * Input.Y) + (Z * Input.Z);

        return (Result);
    }
};

inline v3
Vec3(r32 X, r32 Y, r32 Z)
{
    v3 Result;

    Result.Elements[0] = X;
    Result.Elements[1] = Y;
    Result.Elements[2] = Z;

    return(Result);
}

union v2
{
    struct
    {
        r32 X;
        r32 Y;
    };
    r32 Elements[2];
};

inline v2
v2_(r32 X, r32 Y)
{
    v2 Result;

    Result.Elements[0] = X;
    Result.Elements[1] = Y;

    return(Result);
}

inline v2
NormalizeV2(v2 Input)
{
    v2 Result;

    r32 VectorLength = sqrt(Input.X*Input.X + Input.Y*Input.Y);
    Result.X = (Input.X / VectorLength);
    Result.Y = (Input.Y / VectorLength);

    return(Result);
}

inline v3
NormalizeV3(v3 Input)
{
    v3 Result;

    r32 VectorLength = sqrt((Input.X*Input.X) + (Input.Y*Input.Y) + (Input.Z*Input.Z));
    Result.X = (Input.X / VectorLength);
    Result.Y = (Input.Y / VectorLength);
    Result.Z = (Input.Z / VectorLength);

    return(Result);
}

inline v3
Cross(v3 A, v3 B)
{
    v3 Result;

    Result.X = A.Y * B.Z - A.Z * B.Y;
    Result.Y = A.Z * B.X - A.X * B.Z;
    Result.Z = A.X * B.Y - A.Y * B.X;

    return(Result);
}

struct m4
{
   r32 Elements[4][4];

   m4
   operator*(m4 Input)
   {
       m4 Result;

       for (u8 ResultColumnIndex=0;
            ResultColumnIndex < 4;
            ++ResultColumnIndex)
       {
           for (u8 RowIndex=0;
                RowIndex < 4;
                ++RowIndex)
           {
               r32 RowColSum = 0.0f;
               for (u8 ColumnIndex=0;
                    ColumnIndex < 4;
                    ++ColumnIndex)
               {
                   RowColSum += Input.Elements[ColumnIndex][ResultColumnIndex]*Elements[RowIndex][ColumnIndex];
               }
               Result.Elements[RowIndex][ResultColumnIndex] = RowColSum;
           }
       }

       return(Result);
   }
};

struct m3
{
   r32 Elements[3][3];
};

/*
m4
InverseM4(m4 Input)
{
    // TODO(caleb): write me
}
*/

inline m3
Mat3(r32 Value)
{
    m3 Result;
    for (u8 RowIndex=0;
         RowIndex < 3;
         ++RowIndex)
    {
        for (u8 ColumnIndex=0;
             ColumnIndex < 3;
             ++ColumnIndex)
        {
            if (ColumnIndex == RowIndex)
            {
                Result.Elements[ColumnIndex][RowIndex] = Value;
            }
            else
            {
                Result.Elements[ColumnIndex][RowIndex] = 0.0f;
            }
        }
    }
    return (Result);
}

inline v3
MultiplyMat3ByVec3(m3 Matrix, v3 Vector)
{
    v3 Result;

    for (u8 RowIndex=0;
         RowIndex < 3;
         ++RowIndex)
    {
        u32 Sum = 0;
        for (u8 ColumnIndex=0;
             ColumnIndex < 3;
             ++ColumnIndex)
        {
            Sum += Matrix.Elements[ColumnIndex][RowIndex] * Vector.Elements[ColumnIndex];
        }
        Result.Elements[RowIndex] = Sum;
    }
    return (Result);
}


inline m3
Rotate(r32 Angle, v3 Axis)
{
    m3 Result = Mat3(1.0f);

    Axis = NormalizeV3(Axis);

    float SinTheta = sinf(ToRadians(Angle));
    float CosTheta = cosf(ToRadians(Angle));
    float CosValue = 1.0f - CosTheta;

    Result.Elements[0][0] = (Axis.X * Axis.X * CosValue) + CosTheta;
    Result.Elements[0][1] = (Axis.X * Axis.Y * CosValue) + (Axis.Z * SinTheta);
    Result.Elements[0][2] = (Axis.X * Axis.Z * CosValue) - (Axis.Y * SinTheta);

    Result.Elements[1][0] = (Axis.Y * Axis.X * CosValue) - (Axis.Z * SinTheta);
    Result.Elements[1][1] = (Axis.Y * Axis.Y * CosValue) + CosTheta;
    Result.Elements[1][2] = (Axis.Y * Axis.Z * CosValue) + (Axis.X * SinTheta);

    Result.Elements[2][0] = (Axis.Z * Axis.X * CosValue) + (Axis.Y * SinTheta);
    Result.Elements[2][1] = (Axis.Z * Axis.Y * CosValue) - (Axis.X * SinTheta);
    Result.Elements[2][2] = (Axis.Z * Axis.Z * CosValue) + CosTheta;

    return (Result);
}

inline m4
Mat4(r32 Value)
{
    m4 Result;
    for (u8 RowIndex=0;
         RowIndex < 4;
         ++RowIndex)
    {
        for (u8 ColumnIndex=0;
             ColumnIndex < 4;
             ++ColumnIndex)
        {
            if (ColumnIndex == RowIndex)
            {
                Result.Elements[ColumnIndex][RowIndex] = Value;
            }
            else
            {
                Result.Elements[ColumnIndex][RowIndex] = 0.0f;
            }
        }
    }
    return (Result);
}

inline m4
Translate(m4 Model, v3 Input)
{
    m4 Result = Model;

    Result.Elements[3][0] = Input.X;
    Result.Elements[3][1] = Input.Y;
    Result.Elements[3][2] = Input.Z;

    return(Result);
}

inline m4
LookAt(v3 Eye, v3 Center, v3 Up)
{
    m4 Result;

    v3 CameraFront = NormalizeV3(Center - Eye);
    v3 CameraRight = NormalizeV3(Cross(CameraFront, Up));
    v3 CameraUp = Cross(CameraRight, CameraFront);

    Result.Elements[0][0] = CameraRight.X;
    Result.Elements[0][1] = CameraUp.X;
    Result.Elements[0][2] = -CameraFront.X;
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = CameraRight.Y;
    Result.Elements[1][1] = CameraUp.Y;
    Result.Elements[1][2] = -CameraFront.Y;
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = CameraRight.Z;
    Result.Elements[2][1] = CameraUp.Z;
    Result.Elements[2][2] = -CameraFront.Z;
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = -(CameraRight*Eye);
    Result.Elements[3][1] = -(CameraUp*Eye);
    Result.Elements[3][2] = CameraFront*Eye;
    Result.Elements[3][3] = 1.0f;

    return (Result);
}

inline m4
Perspective(r32 FOV, r32 AspectRatio, r32 Near, r32 Far)
{
    m4 Result = Mat4(0.0f);

    r32 TanHalfFOV = tanf(FOV / 2);
    Result.Elements[0][0] = 1 / (AspectRatio * TanHalfFOV);
    Result.Elements[1][1] = 1 / (TanHalfFOV);
    Result.Elements[2][2] = -(Far + Near) / (Far - Near);
    Result.Elements[2][3] = -1.0f;
    Result.Elements[3][2] = -(2.0f * Far * Near) / (Far - Near);

    return (Result);
}

#define TDS_MATH_H
#endif
