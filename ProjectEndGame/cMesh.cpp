#include "cMesh.h"

#include <glm/glm.hpp>		// cross product (I hope)

cMesh::cMesh()
{
	this->numberOfVertices = 0;
	this->numberOfTriangles = 0;
	this->numberOfIndices = 0;

	this->pVertices = NULL;
	this->pTriangles = NULL;
	this->pIndices = NULL;
	//
	this->maxExtent = 0.0f;
	this->scaleForUnitBBox = 1.0f;
	return;
}


cMesh::~cMesh()
{
	// Delete everything...
	delete [] this->pVertices;
	delete [] this->pTriangles;

	return;
}

void cMesh::CalculateExtents(void)
{
	// Assume 1st vertex is both max and min
	this->minXYZ.x = this->pVertices[0].x;
	this->minXYZ.y = this->pVertices[0].y;
	this->minXYZ.z = this->pVertices[0].z;
	this->maxXYZ.x = this->pVertices[0].x;
	this->maxXYZ.y = this->pVertices[0].y;
	this->maxXYZ.z = this->pVertices[0].z;

	for ( int index = 0; index != this->numberOfVertices; index++ )
	{
		if ( this->pVertices[index].x < this->minXYZ.x )
		{
			this->minXYZ.x = this->pVertices[index].x;
		}
		if ( this->pVertices[index].x > this->maxXYZ.x )
		{
			this->maxXYZ.x = this->pVertices[index].x;
		}
		// y...
		if ( this->pVertices[index].y < this->minXYZ.y )
		{
			this->minXYZ.y = this->pVertices[index].y;
		}
		if ( this->pVertices[index].y > this->maxXYZ.y )
		{
			this->maxXYZ.y = this->pVertices[index].y;
		}
		// z...
		if ( this->pVertices[index].z < this->minXYZ.z )
		{
			this->minXYZ.z = this->pVertices[index].z;
		}
		if ( this->pVertices[index].z > this->maxXYZ.z )
		{
			this->maxXYZ.z = this->pVertices[index].z;
		}

	}//for ( int index...

	this->maxExtentXYZ.x = this->maxXYZ.x - this->minXYZ.x;
	this->maxExtentXYZ.y = this->maxXYZ.y - this->minXYZ.y;
	this->maxExtentXYZ.z = this->maxXYZ.z - this->minXYZ.z;

	this->maxExtent = this->maxExtentXYZ.x;
	if ( this->maxExtent < this->maxExtentXYZ.y )
	{	// Y is bigger
		this->maxExtent = this->maxExtentXYZ.y; 
	}
	if ( this->maxExtent < this->maxExtentXYZ.z )
	{	// Z is bigger
		this->maxExtent = this->maxExtentXYZ.z;
	}
	//
	this->scaleForUnitBBox = 1.0f / this->maxExtent;

	return;
}


//// Takes an indexed model and makes just a vertex array model
//void cMesh::FlattenIndexedModel(void)
//{
//	// 1. Make a copy of the original vertices
//	// 2. Using the triangle array, replace the original data
//
//	int origNumVertices = this->numberOfVertices;
//
////	cVertex_xyz_rgb vertOrig[MAX_VERTEX_ARRAY_SIZE]; // Stack
//
////	cVertex_xyz_rgb_n* pVertOrig = new cVertex_xyz_rgb_n[this->numberOfVertices]; // Heap
////	sVertex_xyz_rgba_n_uv2_bt* pVertOrig = new sVertex_xyz_rgba_n_uv2_bt[this->numberOfVertices]; // Heap
//	sVertex_xyz_rgba_n_uv2_bt_4Bones* pVertOrig = new sVertex_xyz_rgba_n_uv2_bt_4Bones[this->numberOfVertices]; // Heap
//
//	for ( int index = 0; index < origNumVertices; index++ )
//	{
//		pVertOrig[index] = this->pVertices[index];
//	}
//	// We now have a copy of the original vertices
//
//	// Adjust the original vertex array
//	delete [] this->pVertices;		// GOTCHA!
//
////	int numberOfVertsTOTALGUESS = this->numberOfVertices * 4;
////	this->pVertices = new cVertex_xyz_rgb[numberOfVertsTOTALGUESS];
//
//	// Actually, the number of vertices would be 
//	//	the number of triangles x 3...
//	int numberOfVertsNeeded = this->numberOfTriangles * 3;
//	numberOfVertsNeeded += 30;	// Add a few, just in case...
//	//this->pVertices = new cVertex_xyz_rgb_n[numberOfVertsNeeded];
//	//this->pVertices = new sVertex_xyz_rgba_n_uv2_bt[numberOfVertsNeeded];
//	this->pVertices = new sVertex_xyz_rgba_n_uv2_bt_4Bones[numberOfVertsNeeded];
//
//	unsigned int triIndex = 0;
//	int vertIndex = 0;
//	for ( ; triIndex < this->numberOfTriangles; 
//		  triIndex++, vertIndex += 3 )
//	{
//		// 3 4 132 80 
//		int triVert0_index = this->pTriangles[triIndex].vertex_ID_0;
//		int triVert1_index = this->pTriangles[triIndex].vertex_ID_1;
//		int triVert2_index = this->pTriangles[triIndex].vertex_ID_2;
//
//		//cVertex_xyz_rgb_n V0 = pVertOrig[triVert0_index];
//		//cVertex_xyz_rgb_n V1 = pVertOrig[triVert1_index];
//		//cVertex_xyz_rgb_n V2 = pVertOrig[triVert2_index];
//		//sVertex_xyz_rgba_n_uv2_bt V0 = pVertOrig[triVert0_index];
//		//sVertex_xyz_rgba_n_uv2_bt V1 = pVertOrig[triVert1_index];
//		//sVertex_xyz_rgba_n_uv2_bt V2 = pVertOrig[triVert2_index];
//		sVertex_xyz_rgba_n_uv2_bt_4Bones  V0 = pVertOrig[triVert0_index];
//		sVertex_xyz_rgba_n_uv2_bt_4Bones  V1 = pVertOrig[triVert1_index];
//		sVertex_xyz_rgba_n_uv2_bt_4Bones  V2 = pVertOrig[triVert2_index];
//
//		this->pVertices[vertIndex + 0] = V0;
//		this->pVertices[vertIndex + 1] = V1;
//		this->pVertices[vertIndex + 2] = V2;
//	}//for ( ; 
//	
//	// Update the number of vertices
//	// HACK: -3 YIKES!!! 
//	this->numberOfVertices = vertIndex - 3;	
//
//	return;
//}

void cMesh::CalculateNormals(void)
{
	// Go through each triangle...
	// ...calculate normal per vertex (3 vertices)
	// ...ADD this normal to the corresponding vertex

	// 2nd pass, 
	// go through all the vertices and "normalize" them
	for ( int vertIndex = 0; vertIndex != this->numberOfVertices; vertIndex++ )
	{
		this->pVertices[vertIndex].nx = 0.0f;
		this->pVertices[vertIndex].ny = 0.0f;
		this->pVertices[vertIndex].nz = 0.0f;
	}

	for ( int triIndex = 0; triIndex != this->numberOfTriangles; triIndex++ )
	{
		cTriangle curTri = this->pTriangles[triIndex];

		// Calculate normal for each vertex
		glm::vec3 vertA = glm::vec3( this->pVertices[ curTri.vertex_ID_0 ].x, 
									 this->pVertices[ curTri.vertex_ID_0 ].y, 
									 this->pVertices[ curTri.vertex_ID_0 ].z );

		glm::vec3 vertB = glm::vec3( this->pVertices[ curTri.vertex_ID_1 ].x, 
									 this->pVertices[ curTri.vertex_ID_1 ].y, 
									 this->pVertices[ curTri.vertex_ID_1 ].z );

		glm::vec3 vertC = glm::vec3( this->pVertices[ curTri.vertex_ID_2 ].x, 
									 this->pVertices[ curTri.vertex_ID_2 ].y, 
									 this->pVertices[ curTri.vertex_ID_2 ].z );

		// Cross of A-B and A-C (normal at vertex A)
		glm::vec3 normVec0 = glm::normalize( glm::cross( vertB - vertA, vertC - vertA ) );

		// Cross of B-A and B-C (normal at vertex B)
		glm::vec3 normVec1 = glm::normalize( glm::cross( vertA - vertB, vertC - vertB ) );

		// Cross of C-A and C-B (normal at vertex C)
		glm::vec3 normVec2 = glm::normalize( glm::cross( vertA - vertC, vertB - vertC ) );

		// Add the values to the current normals (vert A)
		this->pVertices[ curTri.vertex_ID_0 ].nx += normVec0.x;
		this->pVertices[ curTri.vertex_ID_0 ].ny += normVec0.y;
		this->pVertices[ curTri.vertex_ID_0 ].nz += normVec0.z;

		// Add the values to the current normals (vert B)
		this->pVertices[ curTri.vertex_ID_1 ].nx += normVec1.x;
		this->pVertices[ curTri.vertex_ID_1 ].ny += normVec1.y;
		this->pVertices[ curTri.vertex_ID_1 ].nz += normVec1.z;

		// Add the values to the current normals (vert C)
		this->pVertices[ curTri.vertex_ID_2 ].nx += normVec2.x;
		this->pVertices[ curTri.vertex_ID_2 ].ny += normVec2.y;
		this->pVertices[ curTri.vertex_ID_2 ].nz += normVec2.z;
	}//

	// 2nd pass: normalize the normals
	for ( int vertIndex = 0; vertIndex != this->numberOfVertices; vertIndex++ )
	{
		glm::vec3 norm = glm::vec3( this->pVertices[vertIndex].nx, 
								    this->pVertices[vertIndex].ny, 
									this->pVertices[vertIndex].nz );
		// It's value DIV length
		glm::normalize( norm );
		// divide the xyz by the length of the vector

		this->pVertices[vertIndex].nx = norm.x;
		this->pVertices[vertIndex].ny = norm.y;
		this->pVertices[vertIndex].nz = norm.z;
	}

	return;
}


glm::vec3 cMesh::CalculateClosestPointOnMesh( glm::vec3 pointToTest )
{
	// Assume the closest point is the 1st corner of the 1st triangle...
	// (we have to start somewhere)
	glm::vec3 closestXYZSoFar;
	closestXYZSoFar.x = this->pVertices[ this->pTriangles[0].vertex_ID_0 ].x;
	closestXYZSoFar.y = this->pVertices[ this->pTriangles[0].vertex_ID_0 ].y;
	closestXYZSoFar.z = this->pVertices[ this->pTriangles[0].vertex_ID_0 ].z;

	cTriangle curTriangle;
	glm::vec3 triCorners[3];

	glm::vec3 closestXYZCurrent;	// The current one

	// For each triangle in the mesh information...
	for ( unsigned int triIndex = 0; triIndex != this->numberOfTriangles; triIndex++ )
	{
		curTriangle = this->pTriangles[triIndex];

		triCorners[0].x = this->pVertices[ curTriangle.vertex_ID_0 ].x;
		triCorners[0].y = this->pVertices[ curTriangle.vertex_ID_0 ].y;
		triCorners[0].z = this->pVertices[ curTriangle.vertex_ID_0 ].z;

		triCorners[1].x = this->pVertices[ curTriangle.vertex_ID_1 ].x;
		triCorners[1].y = this->pVertices[ curTriangle.vertex_ID_1 ].y;
		triCorners[1].z = this->pVertices[ curTriangle.vertex_ID_1 ].z;

		triCorners[2].x = this->pVertices[ curTriangle.vertex_ID_2 ].x;
		triCorners[2].y = this->pVertices[ curTriangle.vertex_ID_2 ].y;
		triCorners[2].z = this->pVertices[ curTriangle.vertex_ID_2 ].z;


		closestXYZCurrent = this->ClosestPtPointTriangle( pointToTest, triCorners[0], triCorners[1], triCorners[2] );

		// Is this the closest point so far? 
		if ( glm::distance( pointToTest, closestXYZCurrent ) < glm::distance( pointToTest, closestXYZSoFar) )
		{
			closestXYZSoFar = closestXYZCurrent;
		}
	}//for ( unsigned int triIndex = 0

	return closestXYZSoFar;
}


glm::vec3 cMesh::ClosestPtPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
    glm::vec3 /*Vector*/ ab = b - a;
    glm::vec3 /*Vector*/ ac = c - a;
    glm::vec3 /*Vector*/ bc = c - b;

    // Compute parametric position s for projection P' of P on AB,
    // P' = A + s*AB, s = snom/(snom+sdenom)
 //   float snom = Dot(p - a, ab), sdenom = Dot(p - b, a - b);
	float snom = glm::dot( p - a, ab );
	float sdenom = glm::dot(p - b, a - b);

    // Compute parametric position t for projection P' of P on AC,
    // P' = A + t*AC, s = tnom/(tnom+tdenom)
//    float tnom = Dot(p - a, ac), tdenom = Dot(p - c, a - c);
    float tnom = glm::dot(p - a, ac);
	float tdenom = glm::dot(p - c, a - c);

    if (snom <= 0.0f && tnom <= 0.0f) return a; // Vertex region early out

    // Compute parametric position u for projection P' of P on BC,
    // P' = B + u*BC, u = unom/(unom+udenom)
//    float unom = Dot(p - b, bc), udenom = Dot(p - c, b - c);
    float unom = glm::dot(p - b, bc);
	float udenom = glm::dot(p - c, b - c);

    if (sdenom <= 0.0f && unom <= 0.0f) return b; // Vertex region early out
    if (tdenom <= 0.0f && udenom <= 0.0f) return c; // Vertex region early out


    // P is outside (or on) AB if the triple scalar product [N PA PB] <= 0
//    Vector n = Cross(b - a, c - a);
	glm::vec3 /*Vector*/ n = glm::cross( b - a, c - a);
//    float vc = Dot(n, Cross(a - p, b - p));

    float vc = glm::dot(n, glm::cross(a - p, b - p));

    // If P outside AB and within feature region of AB,
    // return projection of P onto AB
    if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f)
        return a + snom / (snom + sdenom) * ab;

    // P is outside (or on) BC if the triple scalar product [N PB PC] <= 0
//    float va = Dot(n, Cross(b - p, c - p));
    float va = glm::dot(n, glm::cross(b - p, c - p));


    // If P outside BC and within feature region of BC,
    // return projection of P onto BC
    if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f)
        return b + unom / (unom + udenom) * bc;

    // P is outside (or on) CA if the triple scalar product [N PC PA] <= 0
//    float vb = Dot(n, Cross(c - p, a - p));
    float vb = glm::dot(n, glm::cross(c - p, a - p));

   // If P outside CA and within feature region of CA,
    // return projection of P onto CA
    if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f)
        return a + tnom / (tnom + tdenom) * ac;

    // P must project inside face region. Compute Q using barycentric coordinates
    float u = va / (va + vb + vc);
    float v = vb / (va + vb + vc);
    float w = 1.0f - u - v; // = vc / (va + vb + vc)
    return u * a + v * b + w * c;
}