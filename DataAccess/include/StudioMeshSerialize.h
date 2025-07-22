// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <cstdint>


using uint8 = uint8_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;

/**
 * 
 */
class  StudioMeshSerialize
{
public:
	StudioMeshSerialize();
	~StudioMeshSerialize();

	virtual void Deserialize(uint8* buffer, int32 size);

	virtual void Serialize(uint8*& buffer, int32& size);


protected:


	virtual void AddVertex(double x, double y, double z);

	virtual void SetVertexNormal(int32 vid, float x, float y, float z);

	virtual void SetVertexColor(int32 vid, float r, float g, float b);

	virtual void SetVertexUV(int32 vid, float u, float v);

	virtual void AddFace(int32 a, int32 b, int32 c);

	virtual void EnableVertexNormal();
	virtual void EnableVertexColor();
	virtual void EnableVertexUV();

	virtual int32 VertexCount();

	virtual int32 VertexNormalCount();

	virtual int32 FaceCount();

	virtual int32 VertexColorCount();

	virtual int32 VertexUVCount();

	virtual void GetVertex(int32 index, double& x, double& y, double& z);
	virtual void GetVertexNormal(int32 index, float& x, float& y, float& z);
	virtual void GetVertexColor(int32 index, float& r, float& g, float& b);
	virtual void GetVertexUV(int32 index, float& u, float& v);
	virtual void GetFace(int32 index, int32& a, int32& b, int32& c);

	virtual int FormatVersion();

	bool DeserializeBool(uint8* buffer, int size, int& pos);
	uint32 DeserializeUInt32(uint8* buffer, int size, int& pos);
	int32 DeserializeInt(uint8* buffer, int size, int& pos);
	int64 DeserializeInt64(uint8* buffer, int size, int& pos);
	float DeserializeFloat(uint8* buffer, int size, int& pos);
	double DeserializeDouble(uint8* buffer, int size, int& pos);

	void SerializeUInt32(uint8* buffer, int size, int& pos, uint32 value);

	void SerializeBool(uint8* buffer, int size, int& pos, bool value);

	void SerializeInt(uint8* buffer, int size, int& pos, int value);
	void SerializeInt64(uint8* buffer, int size, int& pos, int64 value);
	void SerializeFloat(uint8* buffer, int size, int& pos, float value);
	void SerializeDouble(uint8* buffer, int size, int& pos, double value);

	const int sizeofUInt32 = sizeof(uint32);
	const int sizeofInt = sizeof(int32);
	const int sizeofInt64 = sizeof(int64);
	const int sizeofFloat = sizeof(float);
	const int sizeofDouble = sizeof(double);

	int currentPos = 0;

	int error = 0;

	uint32 vertexCount = 0;
	uint32 vertexNormalCount = 0;
	uint32 faceCount = 0;
	uint32 vertexColorCount = 0;
	uint32 vertexUVCount = 0;

};



/**
 *
 */
class  DMesh3CommonSerialize : public StudioMeshSerialize
{
public:
	DMesh3CommonSerialize();
	~DMesh3CommonSerialize();

	virtual void Deserialize(uint8* buffer, int32 size) override;

	virtual void Serialize(uint8*& buffer, int32& size) override;


protected:

	bool bPreserveDataLayout = false; //< Preserve the data layout, i.e. external vertex/triangle/edge indices are still valid after roundtrip serialization.
	bool bCompactData = true; //< Remove any holes or padding in the data layout, and discard/recompute any redundant data. 
	//我们只用false,不持久化。持久化Dmesh3速度很慢
	bool bUseCompression = false; //< Compress all data buffers to minimize memory footprint.


};
