// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TriangulationMeshSerialize.h"

TriangulationMeshSerialize::TriangulationMeshSerialize(Triangulation* tri)
{
	triangle = tri;
}

int32 TriangulationMeshSerialize::VertexCount()
{
	return triangle->vertices_n;
}

int32 TriangulationMeshSerialize::VertexNormalCount()
{
	return triangle->vertices_n;
}

int32 TriangulationMeshSerialize::FaceCount()
{
	return triangle->triangles_n;
}

int32 TriangulationMeshSerialize::VertexColorCount()
{
	return 0;
}

int32 TriangulationMeshSerialize::VertexUVCount()
{
	return 0;
}

void TriangulationMeshSerialize::GetVertex(int32 index, double& x, double& y, double& z)
{
	if (index < triangle->vertices_n)
	{
		x = triangle->vertices[3 * index];
		y = triangle->vertices[3 * index + 1];
		z = triangle->vertices[3 * index + 2];
	}
}

void TriangulationMeshSerialize::GetVertexNormal(int32 index, float& x, float& y, float& z)
{
	if (index < triangle->vertices_n)
	{
		x = triangle->normals[3 * index];
		y = triangle->normals[3 * index + 1];
		z = triangle->normals[3 * index + 2];
	}
}

void TriangulationMeshSerialize::GetVertexColor(int32 index, float& r, float& g, float& b)
{
}

void TriangulationMeshSerialize::GetVertexUV(int32 index, float& u, float& v)
{
}

void TriangulationMeshSerialize::GetFace(int32 index, int32& a, int32& b, int32& c)
{
	if (index < triangle->triangles_n)
	{
		a = triangle->indices[3 * index];
		b = triangle->indices[3 * index + 1];
		c = triangle->indices[3 * index + 2];
	}
}
