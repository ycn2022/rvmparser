// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#ifdef HAVE_STUDIO_MESH_SERIALIZE
#include "StudioMeshSerialize.h"
#endif

#include "Store.h"


/**
 * 
 */
class  TriangulationMeshSerialize: public StudioMeshSerialize
{
public:
	TriangulationMeshSerialize(Triangulation* tri);

protected:

	virtual int32 VertexCount() override;

	virtual int32 VertexNormalCount() override;

	virtual int32 FaceCount() override;

	virtual int32 VertexColorCount() override;

	virtual int32 VertexUVCount() override;

	virtual void GetVertex(int32 index, double& x, double& y, double& z) override;
	virtual void GetVertexNormal(int32 index, float& x, float& y, float& z) override;
	virtual void GetVertexColor(int32 index, float& r, float& g, float& b) override;
	virtual void GetVertexUV(int32 index, float& u, float& v) override;
	virtual void GetFace(int32 index, int32& a, int32& b, int32& c) override;

	Triangulation* triangle = nullptr;
};
