#pragma once

static const unsigned int NUMBEROFBONES = 4;
struct sVertex_xyz_rgba_n_uv2_bt_4Bones
{
	sVertex_xyz_rgba_n_uv2_bt_4Bones();
	float x, y, z, w;								// 16
	float r, g, b, a;								// 32 
	float nx, ny, nz, nw;							// 48 
	float u0, v0, u1, v1;							// 60
	float tx, ty, tz, tw;	// tangent				// 
	float bx, by, bz, bw;	// bi-normal			// 
	// For the 4 bone skinned mesh information
	float boneID[NUMBEROFBONES]; 		// New		// 
	float boneWeights[NUMBEROFBONES];	// New		// 
};