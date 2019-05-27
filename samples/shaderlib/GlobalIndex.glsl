/*
	Helper functions for compute shader
*/

#include "Math.glsl"


// global linear index
int    GetGlobalIndexSize ();
int    GetGlobalIndex ();		// 0..size-1
float  GetGlobalIndexUNorm ();	//  0..1
float  GetGlobalIndexSNorm ();	// -1..1

// local linear index
int    GetLocalIndexSize ();
int    GetLocalIndex ();		// 0..size-1
float  GetLocalIndexUNorm ();	//  0..1
float  GetLocalIndexSNorm ();	// -1..1

// group linear index
int    GetGroupIndexSize ();
int    GetGroupIndex ();		// 0..size-1
float  GetGroupIndexUNorm ();	//  0..1
float  GetGroupIndexSNorm ();	// -1..1

// global coordinate in 3D
int3    GetGlobalSize ();
int3    GetGlobalCoord ();		// 0..size-1
float3  GetGlobalCoordUNorm ();	//  0..1
float3  GetGlobalCoordSNorm ();	// -1..1

// local coordinate in 3D
int3    GetLocalSize ();
int3    GetLocalCoord ();		// 0..size-1
float3  GetLocalCoordUNorm ();	//  0..1
float3  GetLocalCoordSNorm ();	// -1..1

// group coordinate in 3D
int3    GetGroupSize ();
int3    GetGroupCoord ();		// 0..size-1
float3  GetGroupCoordUNorm ();	//  0..1
float3  GetGroupCoordSNorm ();	// -1..1

// global normalized coordinate in 2D with correction
float2  GetGlobalCoordUNormCorrected ();	//  0..1
float2  GetGlobalCoordSNormCorrected ();	// -1..1


//-----------------------------------------------------------------------------


// local linear index
int GetLocalIndex ()
{
	return int( gl_LocalInvocationIndex );
}


// global coordinate in 3D
int3 GetGlobalCoord ()
{
	return int3( gl_GlobalInvocationID );
}

int3 GetGlobalSize ()
{
	return GetGroupSize() * GetLocalSize();
}


// local coordinate in 3D
int3 GetLocalCoord ()
{
	return int3( gl_LocalInvocationID );
}

int3 GetLocalSize ()
{
	return int3( gl_WorkGroupSize );
}


// group coordinate in 3D
int3 GetGroupCoord ()
{
	return int3( gl_WorkGroupID );
}

int3 GetGroupSize ()
{
	return int3( gl_NumWorkGroups );
}

// global linear index
int GetGlobalIndex ()
{
	int3 coord = GetGlobalCoord();
	int3 size  = GetGlobalSize();
	return coord.x + (coord.y * size.x) + (coord.z * size.x * size.y);
}

int GetGlobalIndexSize ()
{
	int3 size  = GetGlobalSize();
	return size.x * size.y * size.z;
}

float GetGlobalIndexUNorm ()
{
	return float(GetGlobalIndex()) / float(GetGlobalIndexSize()-1);
}

float GetGlobalIndexSNorm ()
{
	return GetGlobalIndexUNorm() * 2.0 - 1.0;
}


// local linear index
int GetLocalIndexSize ()
{
	int3 size  = GetLocalSize();
	return size.x * size.y * size.z;
}

float GetLocalIndexUNorm ()
{
	return float(GetLocalIndex()) / float(GetLocalIndexSize()-1);
}

float GetLocalIndexSNorm ()
{
	return GetLocalIndexUNorm() * 2.0 - 1.0;
}


// group linear index
int GetGroupIndex ()
{
	int3 coord = GetGroupCoord();
	int3 size  = GetGroupSize();
	return coord.x + (coord.y * size.x) + (coord.z * size.x * size.y);
}

int GetGroupIndexSize ()
{
	int3 size  = GetGroupSize();
	return size.x * size.y * size.z;
}

float GetGroupIndexUNorm ()
{
	return float(GetGroupIndex()) / float(GetGroupIndexSize()-1);
}

float GetGroupIndexSNorm ()
{
	return GetGroupIndexUNorm() * 2.0 - 1.0;
}


// global coordinate in 3D
float3 GetGlobalCoordUNorm ()
{
	return float3(GetGlobalCoord()) / float3(GetGlobalSize()-1);
}

float3 GetGlobalCoordSNorm ()
{
	return GetGlobalCoordUNorm() * 2.0 - 1.0;
}


// local coordinate in 3D
float3 GetLocalCoordUNorm ()
{
	return float3(GetLocalCoord()) / float3(GetLocalSize()-1);
}

float3 GetLocalCoordSNorm ()
{
	return GetLocalCoordUNorm() * 2.0 - 1.0;
}


// group coordinate in 3D
float3 GetGroupCoordUNorm ()
{
	return float3(GetGroupCoord()) / float3(GetGroupSize()-1);
}

float3 GetGroupCoordSNorm ()
{
	return GetGroupCoordUNorm() * 2.0 - 1.0;
}


// global normalized coordinate in 2D with correction
float2 GetGlobalCoordUNormCorrected ()
{
	float2	size = float2(GetGlobalSize().xy - 1);
	return float2(GetGlobalCoord()) / Max( size.x, size.y );
}

float2 GetGlobalCoordSNormCorrected ()
{
	float2	hsize	= float2(GetGlobalSize().xy - 1) * 0.5;
	float	msize	= Max( hsize.x, hsize.y );
	return (float2(GetGlobalCoord()) - hsize) / msize;
}
