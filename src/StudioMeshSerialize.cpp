// Fill out your copyright notice in the Description page of Project Settings.


#include "StudioMeshSerialize.h"
#include <cstring> 


StudioMeshSerialize::StudioMeshSerialize()
{
}

StudioMeshSerialize::~StudioMeshSerialize()
{
}


void StudioMeshSerialize::Deserialize(uint8* buffer, int32 size)
{

    currentPos = 0;

    int formatversion = DeserializeInt(buffer, size, currentPos);

    //第一版
    /*
int	formatversion
uint32	vertexcount
uint32	vertexnormalcount
uint32	facecount
uint32	vertexcolorcount
uint32	vertexuvcount
double[3]	vertex1
double[3]	vertex2
double[3]	...
double[3]	vertexn
float[3]	normal1
float[3]	normal2
float[3]	...
float[3]	normaln
uint32[3]	faceid1
uint32[3]	faceid2
uint32[3]	...
uint32[3]	faceidn
float[3]	vertexcolor1
float[3]	vertexcolor2
float[3]	...
float[3]	vertexcolorn
float[2]	vertexuv1
float[2]	vertexuv2
float[2]	...
float[2]	vertexuvn



    */
    if (formatversion == 1)
    {
        vertexCount = DeserializeInt(buffer, size, currentPos);
        vertexNormalCount = DeserializeInt(buffer, size, currentPos);
        faceCount = DeserializeInt(buffer, size, currentPos);
        vertexColorCount = DeserializeInt(buffer, size, currentPos);
        vertexUVCount = DeserializeInt(buffer, size, currentPos);

        if (vertexNormalCount > 0)
            EnableVertexNormal();
        if (vertexColorCount > 0)
            EnableVertexColor();
        if (vertexUVCount > 0)
            EnableVertexUV();

        for (uint32 i = 0; i < vertexCount; i++)
        {
            double x = DeserializeDouble(buffer, size, currentPos);
            double y = DeserializeDouble(buffer, size, currentPos);
            double z = DeserializeDouble(buffer, size, currentPos);

            AddVertex(x, y, z);
        }

        for (uint32 i = 0; i < vertexNormalCount; i++)
        {
            float x = DeserializeFloat(buffer, size, currentPos);
            float y = DeserializeFloat(buffer, size, currentPos);
            float z = DeserializeFloat(buffer, size, currentPos);

            SetVertexNormal(i,x, y, z);
        }

        for (uint32 i = 0; i < faceCount; i++)
        {
            int32 a = DeserializeInt(buffer, size, currentPos);
            int32 b = DeserializeInt(buffer, size, currentPos);
            int32 c = DeserializeInt(buffer, size, currentPos);

            AddFace(a, b, c);
        }

        for (uint32 i = 0; i < vertexColorCount; i++)
        {
            float r = DeserializeFloat(buffer, size, currentPos);
            float g = DeserializeFloat(buffer, size, currentPos);
            float b = DeserializeFloat(buffer, size, currentPos);

            SetVertexColor(i, r, g, b);
        }

        for (uint32 i = 0; i < vertexUVCount; i++)
        {
            float u = DeserializeFloat(buffer, size, currentPos);
            float v = DeserializeFloat(buffer, size, currentPos);

            SetVertexUV(i, u, v);
        }

    }

}

void StudioMeshSerialize::Serialize(uint8*& buffer, int32& size)
{
    int formatVersion = FormatVersion();

    vertexCount = VertexCount();
    vertexNormalCount = VertexNormalCount();
    vertexColorCount = VertexColorCount();
    vertexUVCount = VertexUVCount();
    faceCount = FaceCount();

    size = sizeofInt + 
        sizeofInt * 5 +
        vertexCount * 3 * sizeofDouble + 
        sizeofFloat * 3 * vertexNormalCount + 
        sizeofInt * 3 * faceCount +
        vertexColorCount * 3 * sizeofFloat + 
        vertexUVCount * 2 * sizeofFloat;

    buffer = new uint8[size];


    currentPos = 0;

    SerializeInt(buffer, size, currentPos, formatVersion);
    SerializeInt(buffer, size, currentPos, vertexCount);
    SerializeInt(buffer, size, currentPos, vertexNormalCount);
    SerializeInt(buffer, size, currentPos, faceCount);
    SerializeInt(buffer, size, currentPos, vertexColorCount);
    SerializeInt(buffer, size, currentPos, vertexUVCount);

    for (uint32 i = 0; i < vertexCount; i++)
    {
        double x, y, z;
        GetVertex(i, x, y, z);
        SerializeDouble(buffer, size, currentPos, x);
        SerializeDouble(buffer, size, currentPos, y);
        SerializeDouble(buffer, size, currentPos, z);
    }

    for (uint32 i = 0; i < vertexNormalCount; i++)
    {
        float x, y, z;
        GetVertexNormal(i, x, y, z);
        SerializeFloat(buffer, size, currentPos, x);
        SerializeFloat(buffer, size, currentPos, y);
        SerializeFloat(buffer, size, currentPos, z);
    }

    for (uint32 i = 0; i < faceCount; i++)
    {
        int32 a,b,c;
        GetFace(i, a, b, c);
        SerializeInt(buffer, size, currentPos, a);
        SerializeInt(buffer, size, currentPos, b);
        SerializeInt(buffer, size, currentPos, c);
    }

    for (uint32 i = 0; i < vertexColorCount; i++)
    {
        float r, g, b;
        GetVertexColor(i, r, g, b);
        SerializeFloat(buffer, size, currentPos, r);
        SerializeFloat(buffer, size, currentPos, g);
        SerializeFloat(buffer, size, currentPos, b);
    }


    for (uint32 i = 0; i < vertexUVCount; i++)
    {
        float u,v;
        GetVertexUV(i, u, v);
        SerializeFloat(buffer, size, currentPos, u);
        SerializeFloat(buffer, size, currentPos, v);
    }
}

void StudioMeshSerialize::AddVertex(double x,double y,double z)
{
}

void StudioMeshSerialize::SetVertexNormal(int32 vid, float x, float y, float z)
{
}

void StudioMeshSerialize::SetVertexColor(int32 vid, float r, float g, float b)
{
}

void StudioMeshSerialize::SetVertexUV(int32 vid, float u, float v)
{
}

void StudioMeshSerialize::AddFace(int32 a, int32 b, int32 c)
{
}

void StudioMeshSerialize::EnableVertexNormal()
{
}

void StudioMeshSerialize::EnableVertexColor()
{
}

void StudioMeshSerialize::EnableVertexUV()
{
}

int32 StudioMeshSerialize::VertexCount()
{
    return int32();
}

int32 StudioMeshSerialize::VertexNormalCount()
{
    return int32();
}

int32 StudioMeshSerialize::FaceCount()
{
    return int32();
}

int32 StudioMeshSerialize::VertexColorCount()
{
    return int32();
}

int32 StudioMeshSerialize::VertexUVCount()
{
    return int32();
}

void StudioMeshSerialize::GetVertex(int32 index, double& x, double& y, double& z)
{
}

void StudioMeshSerialize::GetVertexNormal(int32 index, float& x, float& y, float& z)
{
}

void StudioMeshSerialize::GetVertexColor(int32 index, float& r, float& g, float& b)
{
}

void StudioMeshSerialize::GetVertexUV(int32 index, float& u, float& v)
{
}

void StudioMeshSerialize::GetFace(int32 index, int32& a, int32& b, int32& c)
{
}

int StudioMeshSerialize::FormatVersion()
{
    return 1;
}

bool StudioMeshSerialize::DeserializeBool(uint8* buffer, int size, int& pos)
{
    if (pos + sizeofUInt32 > size)
        return false;
    // 通过类型转换和位运算读取 int 值
    uint32 intValue = 0;


    std::memcpy(&intValue, &buffer[pos], sizeofUInt32);

    pos += sizeofUInt32;


    return intValue == 1;
}

uint32 StudioMeshSerialize::DeserializeUInt32(uint8* buffer, int size, int& pos)
{
    if (pos + sizeofUInt32 > size)
        return 0;
    // 通过类型转换和位运算读取 int 值
    uint32 intValue = 0;


    std::memcpy(&intValue, &buffer[pos], sizeofUInt32);

    pos += sizeofUInt32;


    return intValue;
}

int32 StudioMeshSerialize::DeserializeInt(uint8* buffer, int size, int& pos)
{
    if (pos + sizeofInt > size)
        return 0;
    // 通过类型转换和位运算读取 int 值
    int32 intValue = 0;


    std::memcpy(&intValue, &buffer[pos], sizeofInt);

    pos += sizeofInt;

    //for (int i = 0; i < sizeofInt; ++i) {
    //    intValue |= static_cast<int32>(buffer[i + pos]) << (i * 8);
    //}

	return intValue;
}

int64 StudioMeshSerialize::DeserializeInt64(uint8* buffer, int size, int& pos)
{
    if (pos + sizeofInt64 > size)
        return 0;
    // 通过类型转换和位运算读取 int 值
    int64 intValue = 0;


    std::memcpy(&intValue, &buffer[pos], sizeofInt64);


    pos += sizeofInt64;

    //for (int i = 0; i < sizeofInt64; ++i) {
    //    intValue |= static_cast<int64>(buffer[i + pos]) << (i * 8);
    //}

    return intValue;
}

float StudioMeshSerialize::DeserializeFloat(uint8* buffer, int size, int& pos)
{
    if (pos + sizeofFloat > size)
        return 0;
    // 创建一个 float 类型的变量
    float floatValue;

    // 使用 memcpy 将字节数据拷贝到 float 变量中
    std::memcpy(&floatValue, &buffer[pos], sizeofFloat);


    pos += sizeofFloat;

    return floatValue;
}

double StudioMeshSerialize::DeserializeDouble(uint8* buffer, int size, int& pos)
{
    if (pos + sizeofDouble > size)
        return 0;
    // 创建一个 double 类型的变量
    double doubleValue;

    // 使用 memcpy 将字节数据拷贝到 double 变量中
    std::memcpy(&doubleValue, &buffer[pos], sizeofDouble);

    pos += sizeofDouble;

    return doubleValue;
}

void StudioMeshSerialize::SerializeUInt32(uint8* buffer, int size, int& pos, uint32 value)
{
    if (pos + sizeofUInt32 > size)
        return;

    std::memcpy(&buffer[pos], &value, sizeofUInt32);

    pos += sizeofUInt32;
}

void StudioMeshSerialize::SerializeBool(uint8* buffer, int size, int& pos, bool value)
{
    if (pos + sizeofUInt32 > size)
        return;

    uint32 u32val = value ? 1 : 0;

    std::memcpy(&buffer[pos], &u32val, sizeofUInt32);

    pos += sizeofUInt32;
}

void StudioMeshSerialize::SerializeInt(uint8* buffer, int size, int& pos, int value)
{
    if (pos + sizeofInt > size)
        return;

    std::memcpy(&buffer[pos], &value, sizeofInt);

    pos += sizeofInt;
}

void StudioMeshSerialize::SerializeInt64(uint8* buffer, int size, int& pos, int64 value)
{
    if (pos + sizeofInt64 > size)
        return;

    std::memcpy(&buffer[pos], &value, sizeofInt64);

    pos += sizeofInt64;
}

void StudioMeshSerialize::SerializeFloat(uint8* buffer, int size, int& pos, float value)
{
    if (pos + sizeofFloat > size)
        return;

    std::memcpy(&buffer[pos], &value, sizeofFloat);

    pos += sizeofFloat;
}

void StudioMeshSerialize::SerializeDouble(uint8* buffer, int size, int& pos, double value)
{
    if (pos + sizeofDouble > size)
        return;

    std::memcpy(&buffer[pos], &value, sizeofDouble);

    pos += sizeofDouble;
}


DMesh3CommonSerialize::DMesh3CommonSerialize()
{
}

DMesh3CommonSerialize::~DMesh3CommonSerialize()
{
}

void DMesh3CommonSerialize::Deserialize(uint8* buffer, int32 size)
{
}

void DMesh3CommonSerialize::Serialize(uint8*& buffer, int32& size)
{
    vertexCount = (uint32)VertexCount();
    vertexNormalCount = (uint32)VertexNormalCount();
    vertexColorCount = (uint32)VertexColorCount();
    vertexUVCount = (uint32)VertexUVCount();
    faceCount = (uint32)FaceCount();

    size = 0;

    size += sizeofUInt32 * 3;

    size += sizeofUInt32 + sizeofUInt32 + (int)vertexCount * 3 * sizeofDouble;

    size += sizeofUInt32;
    if (vertexNormalCount > 0)
    {
        size += sizeofUInt32 + sizeofUInt32 + (int)vertexNormalCount * 3 * sizeofFloat;
    }

    size += sizeofUInt32;
    if (vertexColorCount > 0)
    {
        size += sizeofUInt32 + sizeofUInt32 + (int)vertexColorCount * 3 * sizeofFloat;
    }

    size += sizeofUInt32;
    if (vertexUVCount > 0)
    {
        size += sizeofUInt32 + sizeofUInt32 + (int)vertexUVCount * 2 * sizeofFloat;
    }

    size += sizeofUInt32 + sizeofUInt32 + (int)faceCount * 3 * sizeofInt;

    size += sizeofUInt32;

    size += sizeofInt;

    size += sizeofUInt32;

    buffer = new uint8[size];


    currentPos = 0;

    try
    {

        SerializeBool(buffer, size, currentPos, bPreserveDataLayout);
        SerializeBool(buffer, size, currentPos, bCompactData);
        SerializeBool(buffer, size, currentPos, bUseCompression);


        SerializeUInt32(buffer, size, currentPos, vertexCount);
        SerializeBool(buffer, size, currentPos, false);
        for (int i = 0; i < vertexCount; i++)
        {
            double x = 0, y = 0, z = 0;
            GetVertex(i, x, y, z);
            SerializeDouble(buffer, size, currentPos, x);
            SerializeDouble(buffer, size, currentPos, y);
            SerializeDouble(buffer, size, currentPos, z);
        }


        SerializeBool(buffer, size, currentPos, vertexNormalCount > 0);
        if (vertexNormalCount > 0)
        {
            SerializeUInt32(buffer, size, currentPos, vertexNormalCount);
            SerializeBool(buffer, size, currentPos, false);
            for (int i = 0; i < vertexNormalCount; i++)
            {
                float x = 0, y = 0, z = 0;
                GetVertexNormal(i, x, y, z);
                SerializeFloat(buffer, size, currentPos, x);
                SerializeFloat(buffer, size, currentPos, y);
                SerializeFloat(buffer, size, currentPos, z);
            }
        }


        SerializeBool(buffer, size, currentPos, vertexColorCount > 0);
        if (vertexColorCount > 0)
        {
            SerializeUInt32(buffer, size, currentPos, vertexColorCount);
            SerializeBool(buffer, size, currentPos, false);
            for (int i = 0; i < vertexColorCount; i++)
            {
                float r = 0, g = 0, b = 0;
                GetVertexColor(i, r, g, b);
                SerializeFloat(buffer, size, currentPos, r);
                SerializeFloat(buffer, size, currentPos, g);
                SerializeFloat(buffer, size, currentPos, b);
            }
        }


        SerializeBool(buffer, size, currentPos, vertexUVCount > 0);
        if (vertexUVCount > 0)
        {
            SerializeUInt32(buffer, size, currentPos, vertexUVCount);
            SerializeBool(buffer, size, currentPos, false);
            for (int i = 0; i < vertexUVCount; i++)
            {
                float u = 0, v = 0;
                GetVertexUV(i, u, v);
                SerializeFloat(buffer, size, currentPos, u);
                SerializeFloat(buffer, size, currentPos, v);
            }
        }

        SerializeUInt32(buffer, size, currentPos, faceCount);
        SerializeBool(buffer, size, currentPos, false);
        for (int i = 0; i < faceCount; i++)
        {
            int a = 0, b = 0, c = 0;
            GetFace(i, a, b, c);
            SerializeInt(buffer, size, currentPos, a);
            SerializeInt(buffer, size, currentPos, b);
            SerializeInt(buffer, size, currentPos, c);
        }



        SerializeBool(buffer, size, currentPos, false);
        SerializeInt(buffer, size, currentPos, 0);
        SerializeBool(buffer, size, currentPos, false);

    }
    catch (...)
    {
        if (buffer)
        {
            delete[] buffer;
            buffer = nullptr;
        }
        size = 0;
        return;
    }

    return;
}
