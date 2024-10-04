#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h" // TODO: убрать
#include "RenderDevice.h" // TODO: убрать

namespace vkr {

#pragma region Generate mip shader VK

#if 0
RWTexture2D<float4> dstTex : register(u0); // Destination texture

cbuffer ShaderConstantData : register(b1) {

	float2 texel_size;	// 1.0 / srcTex.Dimensions
	int src_mip_level;
	// Case to filter according the parity of the dimensions in the src texture. 
	// Must be one of 0, 1, 2 or 3
	// See CSMain function bellow
	int dimension_case;
	// Ignored for now, if we want to use a different filter strategy. Current one is bi-linear filter
	int filter_option;
};

Texture2D<float4> srcTex : register(t2); // Source texture
SamplerState LinearClampSampler : register(s3); // Sampler state

// According to the dimensions of the src texture we can be in one of four cases
float3 computePixelEvenEven(float2 scrCoords);
float3 computePixelEvenOdd(float2 srcCoords);
float3 computePixelOddEven(float2 srcCoords);
float3 computePixelOddOdd(float2 srcCoords);

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	// Calculate the coordinates of the top left corner of the neighbourhood
	float2 coordInSrc = ((2 * dispatchThreadID.xy) * texel_size) + 0.5 * texel_size;

	float3 resultingPixel = float3(0.0f, 0.0f, 0.0f);
	// Get the filtered value from the src texture's neighbourhood
	// Choose the correct case according to src texture dimensions
	switch (dimension_case) {
	case 0: // Both dimension are even
		resultingPixel = computePixelEvenEven(coordInSrc);
		break;
	case 1: // width is even and height is odd
		resultingPixel = computePixelEvenOdd(coordInSrc);
		break;
	case 2: // width is odd an height is even
		resultingPixel = computePixelOddEven(coordInSrc);
		break;
	case 3: // both dimensions are odd
		resultingPixel = computePixelOddOdd(coordInSrc);
		break;
	}
	// Write the resulting color into dst texture
	dstTex[dispatchThreadID.xy] = float4(resultingPixel, 1.0f);
}

// In this case both dimensions (width and height) are even srcCoor are the coordinates of the top left corner of the neighbourhood in the src texture
float3 computePixelEvenEven(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 2x2 neighbourhood sampling
	const float2 neighboursCoords[2][2] = {
		{ {0.0, 0.0},          {texel_size.x, 0.0} },
		{ {0.0, texel_size.y}, {texel_size.x, texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average 1/4 = 0.25 
	const float coeficients[2][2] = {
									  { 0.25f, 0.25f },
									  { 0.25f, 0.25f }
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 2; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}

// In this case width is even and height is odd
// srcCoor are the coordinates of the top left corner of the neighbourhood in the src texture
// This neighbourhood has size 2x3 (in math matices notation)
float3 computePixelEvenOdd(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 2x3 neighbourhood sampling
	const float2 neighboursCoords[2][3] = {
		{ {0.0,          0.0}, {texel_size.x,          0.0}, {2.0 * texel_size.x, 0.0} },
		{ {0.0, texel_size.y}, {texel_size.x, texel_size.y}, {2.0 * texel_size.x, texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average. 1/4 = 0.25, 1/8 = 0.125
	const float coeficients[2][3] = {
									  { 0.125f, 0.25f, 0.125f},
									  { 0.125f, 0.25f, 0.125f}
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 3; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}

// In this case width is odd and height is even
// srcCoor are the coordinates of the top left corner of the neighbourhood in the src texture
// This neighbourhood has size 3x2 (in math matices notation)
float3 computePixelOddEven(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 3x2 neighbourhood sampling
	const float2 neighboursCoords[3][2] = {
		{ {0.0,                0.0}, {texel_size.x,               0.0f} },
		{ {0.0,       texel_size.y}, {texel_size.x,       texel_size.y} },
		{ {0.0, 2.0 * texel_size.y}, {texel_size.x, 2.0 * texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average. 1/4 = 0.25, 1/8 = 0.125
	const float coeficients[3][2] = {
									  { 0.125f, 0.125f },
									  { 0.25f,  0.25f },
									  { 0.125f, 0.125f }
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 2; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}

// In this case both width and height are odd
// srcCoor are the coordinates of the higher left corner of the neighbourhood in the src texture
// This neighbourhood has size 3x3 (in math matices notation)
float3 computePixelOddOdd(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 3x3 neighbourhood sampling	
	const float2 neighboursCoords[3][3] = {
		{ {0.0,                0.0}, {texel_size.x,                0.0}, {2.0 * texel_size.x,                0.0} },
		{ {0.0,       texel_size.y}, {texel_size.x,       texel_size.y}, {2.0 * texel_size.x,       texel_size.y} },
		{ {0.0, 2.0 * texel_size.y}, {texel_size.x, 2.0 * texel_size.y}, {2.0 * texel_size.x, 2.0 * texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average. 1/4 = 0.25, 1/8 = 0.125, 1/16 = 0.0625
	const float coeficients[3][3] = {
									  { 0.0625f, 0.125f, 0.0625f},
									  { 0.125f,  0.25f,  0.125f},
									  { 0.0625f,  0.125f, 0.0625f}
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 3; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}
#endif

// Generated with:
// dxc -spirv -fspv-reflect -T cs_6_0 -E CSMain -Fh generate_mip_shader_VK.h generate_mip_shader.hlsl
constexpr unsigned char GenerateMipShaderVK[] = {
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x1b, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x53, 0x4d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x32, 0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x64, 0x73, 0x74, 0x54, 0x65, 0x78, 0x00, 0x00, 0x05, 0x00, 0x08, 0x00, 0x05, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x44, 0x61, 0x74, 0x61, 0x00, 0x06, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x65, 0x6c, 0x5f, 0x73, 0x69, 0x7a, 0x65, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x73, 0x72, 0x63, 0x5f, 0x6d, 0x69, 0x70, 0x5f, 0x6c, 0x65, 0x76, 0x65, 0x6c, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x64, 0x69, 0x6d, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x5f, 0x63, 0x61, 0x73, 0x65, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x66, 0x69, 0x6c, 0x74, 0x65, 0x72, 0x5f, 0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x44, 0x61, 0x74, 0x61, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x32, 0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x73, 0x72, 0x63, 0x54, 0x65, 0x78, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x43, 0x6c, 0x61, 0x6d, 0x70, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x53, 0x4d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x12, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x15, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3e, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3d, 0x19, 0x00, 0x09, 0x00, 0x03, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x21, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x19, 0x00, 0x09, 0x00, 0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x24, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x26, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x27, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x28, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x29, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x30, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x31, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x32, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x33, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x03, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x34, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x37, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x38, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x39, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x3d, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x40, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x41, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x42, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x43, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x21, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x30, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x39, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x39, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x42, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x26, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x4d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x41, 0x00, 0x00, 0x00, 0x4e, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x43, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x3d, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x38, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x31, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x24, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x07, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x84, 0x00, 0x05, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x70, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x29, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0xf7, 0x00, 0x03, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x0b, 0x00, 0x60, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x62, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x54, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x55, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0x77, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0x76, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x78, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x79, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x79, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x79, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x7f, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x73, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x73, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x77, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x63, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x37, 0x00, 0x00, 0x00, 0x9a, 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x52, 0x00, 0x00, 0x00, 0x9a, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x53, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x9b, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x9b, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xa2, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xa1, 0x00, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xa2, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xa9, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xa9, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa6, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0xab, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0xac, 0x00, 0x00, 0x00, 0xab, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xad, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0xac, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0xae, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0xaf, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xb3, 0x00, 0x00, 0x00, 0xae, 0x00, 0x00, 0x00, 0xaf, 0x00, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0xb3, 0x00, 0x00, 0x00, 0xad, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0xb5, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0xb6, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xb7, 0x00, 0x00, 0x00, 0xb6, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0xb5, 0x00, 0x00, 0x00, 0xb7, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x9e, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x9b, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa2, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x3c, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x50, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x51, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0xc9, 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xcd, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xce, 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xcd, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xce, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xc9, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xd5, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xd5, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0xd7, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0xd7, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xd9, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0xda, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0xdb, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0xdd, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xde, 0x00, 0x00, 0x00, 0xdd, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xdf, 0x00, 0x00, 0x00, 0xda, 0x00, 0x00, 0x00, 0xdb, 0x00, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xdf, 0x00, 0x00, 0x00, 0xd9, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xde, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0xe1, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0xe2, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe3, 0x00, 0x00, 0x00, 0xe2, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0xe1, 0x00, 0x00, 0x00, 0xe3, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0xc9, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xca, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xca, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xce, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x65, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xe5, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0xe5, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xeb, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0xed, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0xeb, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf2, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf3, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x00, 0x00, 0xf2, 0x00, 0x00, 0x00, 0xf3, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x40, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x00, 0xed, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x4e, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xf7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xf7, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xfd, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xff, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0x05, 0x01, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0x06, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0x05, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x06, 0x01, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x02, 0x01, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x4e, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0a, 0x01, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x01, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0d, 0x01, 0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0e, 0x01, 0x00, 0x00, 0x0d, 0x01, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x0a, 0x01, 0x00, 0x00, 0x0b, 0x01, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0e, 0x01, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0x12, 0x01, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x13, 0x01, 0x00, 0x00, 0x12, 0x01, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x13, 0x01, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0xf9, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x06, 0x01, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xf7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x0d, 0x00, 0x15, 0x00, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x4d, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0xa2, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0xce, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x16, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x17, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x19, 0x01, 0x00, 0x00, 0x16, 0x01, 0x00, 0x00, 0x17, 0x01, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1a, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x63, 0x00, 0x05, 0x00, 0x1a, 0x01, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x19, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00 };

#pragma endregion

#pragma region Format

#define UNCOMPRESSED_FORMAT(Name, Type, Aspect, BytesPerTexel, BytesPerComponent, Layout, Component, ComponentOffsets) \
	{                                                                                                                  \
		"" #Name,                                                                                                      \
			FORMAT_DATA_TYPE_##Type,                                                                                   \
			FORMAT_ASPECT_##Aspect,                                                                                    \
			BytesPerTexel,                                                                                             \
			/* blockWidth= */ 1,                                                                                       \
			BytesPerComponent,                                                                                         \
			FORMAT_LAYOUT_##Layout,                                                                                    \
			FORMAT_COMPONENT_##Component,                                                                              \
			OFFSETS_##ComponentOffsets                                                                                 \
	}

#define COMPRESSED_FORMAT(Name, Type, BytesPerBlock, BlockWidth, Component) \
	{                                                                       \
		"" #Name,                                                           \
			FORMAT_DATA_TYPE_##Type,                                        \
			FORMAT_ASPECT_COLOR,                                            \
			BytesPerBlock,                                                  \
			BlockWidth,                                                     \
			-1,                                                             \
			FORMAT_LAYOUT_COMPRESSED,                                       \
			FORMAT_COMPONENT_##Component,                                   \
			OFFSETS_UNDEFINED                                               \
	}

#define OFFSETS_UNDEFINED        { -1, -1, -1, -1 }
#define OFFSETS_R(R)             { R,  -1, -1, -1 }
#define OFFSETS_RG(R, G)         { R,   G, -1, -1 }
#define OFFSETS_RGB(R, G, B)     { R,   G,  B, -1 }
#define OFFSETS_RGBA(R, G, B, A) { R,   G,  B,  A }

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 5246)
#endif

// A static registry of format descriptions.
// The order must match the order of the Format enum, so that retrieving the description for a given format can be done in constant time.
constexpr FormatDesc formatDescs[] =
{
	//                 +----------------------------------------------------------------------------------------------------------+
	//                 |                                              ,-> bytes per texel                                         |
	//                 |                                              |   ,-> bytes per component                                 |
	//                 | format name      | type     | aspect         |   |   Layout   | Components            | Offsets          |
	UNCOMPRESSED_FORMAT(UNDEFINED,          UNDEFINED, UNDEFINED,     0,  0,  UNDEFINED, UNDEFINED,              UNDEFINED),
	UNCOMPRESSED_FORMAT(R8_SNORM,           SNORM,     COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R8G8_SNORM,         SNORM,     COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
	UNCOMPRESSED_FORMAT(R8G8B8_SNORM,       SNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
	UNCOMPRESSED_FORMAT(R8G8B8A8_SNORM,     SNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
	UNCOMPRESSED_FORMAT(B8G8R8_SNORM,       SNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
	UNCOMPRESSED_FORMAT(B8G8R8A8_SNORM,     SNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

	UNCOMPRESSED_FORMAT(R8_UNORM,           UNORM,     COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R8G8_UNORM,         UNORM,     COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
	UNCOMPRESSED_FORMAT(R8G8B8_UNORM,       UNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
	UNCOMPRESSED_FORMAT(R8G8B8A8_UNORM,     UNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
	UNCOMPRESSED_FORMAT(B8G8R8_UNORM,       UNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
	UNCOMPRESSED_FORMAT(B8G8R8A8_UNORM,     UNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

	UNCOMPRESSED_FORMAT(R8_SINT,            SINT,      COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R8G8_SINT,          SINT,      COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
	UNCOMPRESSED_FORMAT(R8G8B8_SINT,        SINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
	UNCOMPRESSED_FORMAT(R8G8B8A8_SINT,      SINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
	UNCOMPRESSED_FORMAT(B8G8R8_SINT,        SINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
	UNCOMPRESSED_FORMAT(B8G8R8A8_SINT,      SINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

	UNCOMPRESSED_FORMAT(R8_UINT,            UINT,      COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R8G8_UINT,          UINT,      COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
	UNCOMPRESSED_FORMAT(R8G8B8_UINT,        UINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
	UNCOMPRESSED_FORMAT(R8G8B8A8_UINT,      UINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
	UNCOMPRESSED_FORMAT(B8G8R8_UINT,        UINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
	UNCOMPRESSED_FORMAT(B8G8R8A8_UINT,      UINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

	UNCOMPRESSED_FORMAT(R16_SNORM,          SNORM,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R16G16_SNORM,       SNORM,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
	UNCOMPRESSED_FORMAT(R16G16B16_SNORM,    SNORM,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
	UNCOMPRESSED_FORMAT(R16G16B16A16_SNORM, SNORM,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

	UNCOMPRESSED_FORMAT(R16_UNORM,          UNORM,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R16G16_UNORM,       UNORM,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
	UNCOMPRESSED_FORMAT(R16G16B16_UNORM,    UNORM,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
	UNCOMPRESSED_FORMAT(R16G16B16A16_UNORM, UNORM,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

	UNCOMPRESSED_FORMAT(R16_SINT,           SINT,      COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R16G16_SINT,        SINT,      COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
	UNCOMPRESSED_FORMAT(R16G16B16_SINT,     SINT,      COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
	UNCOMPRESSED_FORMAT(R16G16B16A16_SINT,  SINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

	UNCOMPRESSED_FORMAT(R16_UINT,           UINT,      COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R16G16_UINT,        UINT,      COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
	UNCOMPRESSED_FORMAT(R16G16B16_UINT,     UINT,      COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
	UNCOMPRESSED_FORMAT(R16G16B16A16_UINT,  UINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

	UNCOMPRESSED_FORMAT(R16_FLOAT,          FLOAT,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R16G16_FLOAT,       FLOAT,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
	UNCOMPRESSED_FORMAT(R16G16B16_FLOAT,    FLOAT,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
	UNCOMPRESSED_FORMAT(R16G16B16A16_FLOAT, FLOAT,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

	UNCOMPRESSED_FORMAT(R32_SINT,           SINT,      COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R32G32_SINT,        SINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
	UNCOMPRESSED_FORMAT(R32G32B32_SINT,     SINT,      COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
	UNCOMPRESSED_FORMAT(R32G32B32A32_SINT,  SINT,      COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

	UNCOMPRESSED_FORMAT(R32_UINT,           UINT,      COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R32G32_UINT,        UINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
	UNCOMPRESSED_FORMAT(R32G32B32_UINT,     UINT,      COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
	UNCOMPRESSED_FORMAT(R32G32B32A32_UINT,  UINT,      COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

	UNCOMPRESSED_FORMAT(R32_FLOAT,          FLOAT,     COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R32G32_FLOAT,       FLOAT,     COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
	UNCOMPRESSED_FORMAT(R32G32B32_FLOAT,    FLOAT,     COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
	UNCOMPRESSED_FORMAT(R32G32B32A32_FLOAT, FLOAT,     COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

	UNCOMPRESSED_FORMAT(S8_UINT,            UINT,      STENCIL,       1,  1,  LINEAR,    STENCIL,                  RG(-1, 0)),
	UNCOMPRESSED_FORMAT(D16_UNORM,          UNORM,     DEPTH,         2,  2,  LINEAR,    DEPTH,                    RG(0, -1)),
	UNCOMPRESSED_FORMAT(D32_FLOAT,          FLOAT,     DEPTH,         4,  4,  LINEAR,    DEPTH,                    RG(0, -1)),

	UNCOMPRESSED_FORMAT(D16_UNORM_S8_UINT,  UNORM,     DEPTH_STENCIL, 3,  2,  LINEAR,    DEPTH_STENCIL,            RG(0, 2)),
	UNCOMPRESSED_FORMAT(D24_UNORM_S8_UINT,  UNORM,     DEPTH_STENCIL, 4,  3,  LINEAR,    DEPTH_STENCIL,            RG(0, 3)),
	UNCOMPRESSED_FORMAT(D32_FLOAT_S8_UINT,  FLOAT,     DEPTH_STENCIL, 5,  4,  LINEAR,    DEPTH_STENCIL,            RG(0, 4)),

	UNCOMPRESSED_FORMAT(R8_SRGB,            SRGB,     COLOR,          1,  1,  LINEAR,    RED,                       R(0)),
	UNCOMPRESSED_FORMAT(R8G8_SRBG,          SRGB,     COLOR,          2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
	UNCOMPRESSED_FORMAT(R8G8B8_SRBG,        SRGB,     COLOR,          3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
	UNCOMPRESSED_FORMAT(R8G8B8A8_SRBG,      SRGB,     COLOR,          4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
	UNCOMPRESSED_FORMAT(B8G8R8_SRBG,        SRGB,     COLOR,          3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
	UNCOMPRESSED_FORMAT(B8G8R8A8_SRBG,      SRGB,     COLOR,          4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

	// We don't support retrieving component size or byte offsets for packed formats.
	UNCOMPRESSED_FORMAT(R10G10B10A2_UNORM,  UNORM,    COLOR,          4,  -1, PACKED,    RED_GREEN_BLUE_ALPHA,   UNDEFINED),
	UNCOMPRESSED_FORMAT(R11G11B10_FLOAT,    FLOAT,    COLOR,          4,  -1, PACKED,    RED_GREEN_BLUE,         UNDEFINED),

	// We don't support retrieving component size or byte offsets for compressed formats.
	// We don't support non-square blocks for compressed textures.
	//                +-----------------------------------------------------+
	//                |                       ,-> bytes per block           |
	//                |                       |  ,-> block width            |
	//                | format name | type    |  |   Components             |
	COMPRESSED_FORMAT(BC1_RGBA_SRGB , SRGB,   8, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC1_RGBA_UNORM, UNORM,  8, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC1_RGB_SRGB  , SRGB,   8, 4,  RED_GREEN_BLUE),
	COMPRESSED_FORMAT(BC1_RGB_UNORM , UNORM,  8, 4,  RED_GREEN_BLUE),
	COMPRESSED_FORMAT(BC2_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC2_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC3_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC3_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC4_UNORM     , UNORM,  8, 4,  RED),
	COMPRESSED_FORMAT(BC4_SNORM     , SNORM,  8, 4,  RED),
	COMPRESSED_FORMAT(BC5_UNORM     , UNORM, 16, 4,  RED_GREEN),
	COMPRESSED_FORMAT(BC5_SNORM     , SNORM, 16, 4,  RED_GREEN),
	COMPRESSED_FORMAT(BC6H_UFLOAT   , FLOAT, 16, 4,  RED_GREEN_BLUE),
	COMPRESSED_FORMAT(BC6H_SFLOAT   , FLOAT, 16, 4,  RED_GREEN_BLUE),
	COMPRESSED_FORMAT(BC7_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
	COMPRESSED_FORMAT(BC7_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),

	#undef COMPRESSED_FORMAT
	#undef UNCOMPRESSED_FORMAT
	#undef OFFSETS_UNDEFINED
	#undef OFFSETS_R
	#undef OFFSETS_RG
	#undef OFFSETS_RGB
	#undef OFFSETS_RGBA
};

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

constexpr size_t formatDescsSize = sizeof(formatDescs) / sizeof(FormatDesc);

const FormatDesc* GetFormatDescription(Format format)
{
	uint32_t formatIndex = static_cast<uint32_t>(format);
	ASSERT_MSG(format != Format::Undefined && formatIndex < formatDescsSize, "invalid format");
	return &formatDescs[formatIndex];
}

std::string ToString(Format format)
{
	uint32_t formatIndex = static_cast<uint32_t>(format);
	ASSERT_MSG(formatIndex < formatDescsSize, "invalid format");
	return formatDescs[formatIndex].name;
}

#pragma endregion

#pragma region Helper

ColorComponentFlags ColorComponentFlags::RGBA()
{
	ColorComponentFlags flags = ColorComponentFlags(COLOR_COMPONENT_R | COLOR_COMPONENT_G | COLOR_COMPONENT_B | COLOR_COMPONENT_A);
	return flags;
}

ImageUsageFlags ImageUsageFlags::SampledImage()
{
	ImageUsageFlags flags = ImageUsageFlags(IMAGE_USAGE_SAMPLED);
	return flags;
}

void VertexBinding::SetBinding(uint32_t binding)
{
	m_binding = binding;
	for (auto& elem : m_attributes) {
		elem.binding = binding;
	}
}

void VertexBinding::SetStride(uint32_t stride)
{
	m_stride = stride;
}

Result VertexBinding::GetAttribute(uint32_t index, const VertexAttribute** ppAttribute) const
{
	if (!IsIndexInRange(index, m_attributes)) {
		return ERROR_OUT_OF_RANGE;
	}
	*ppAttribute = &m_attributes[index];
	return SUCCESS;
}

uint32_t VertexBinding::GetAttributeIndex(VertexSemantic semantic) const
{
	auto it = FindIf(
		m_attributes,
		[semantic](const VertexAttribute& elem) -> bool {
			bool isMatch = (elem.semantic == semantic);
			return isMatch; });
	if (it == std::end(m_attributes)) {
		return VALUE_IGNORED;
	}
	uint32_t index = static_cast<uint32_t>(std::distance(std::begin(m_attributes), it));
	return index;
}

VertexBinding& VertexBinding::AppendAttribute(const VertexAttribute& attribute)
{
	m_attributes.push_back(attribute);

	if (m_inputRate == VertexBinding::kInvalidVertexInputRate)
		m_inputRate = attribute.inputRate;

	// Calculate offset for inserted attribute
	if (m_attributes.size() > 1)
	{
		size_t i1 = m_attributes.size() - 1;
		size_t i0 = i1 - 1;
		if (m_attributes[i1].offset == APPEND_OFFSET_ALIGNED)
		{
			uint32_t prevOffset = m_attributes[i0].offset;
			uint32_t prevSize = GetFormatDescription(m_attributes[i0].format)->bytesPerTexel;
			m_attributes[i1].offset = prevOffset + prevSize;
		}
	}
	// Size of mAttributes should be 1
	else
	{
		if (m_attributes[0].offset == APPEND_OFFSET_ALIGNED)
		{
			m_attributes[0].offset = 0;
		}
	}

	// Calculate stride
	m_stride = 0;
	for (auto& elem : m_attributes)
	{
		uint32_t size = GetFormatDescription(elem.format)->bytesPerTexel;
		m_stride += size;
	}

	return *this;
}

VertexBinding& VertexBinding::operator+=(const VertexAttribute& rhs)
{
	AppendAttribute(rhs);
	return *this;
}

Result VertexDescription::GetBinding(uint32_t index, const VertexBinding** ppBinding) const
{
	if (!IsIndexInRange(index, m_bindings))
		return ERROR_OUT_OF_RANGE;

	if (!IsNull(ppBinding))
		*ppBinding = &m_bindings[index];

	return SUCCESS;
}

const VertexBinding* VertexDescription::GetBinding(uint32_t index) const
{
	const VertexBinding* ptr = nullptr;
	GetBinding(index, &ptr);
	return ptr;
}

uint32_t VertexDescription::GetBindingIndex(uint32_t binding) const
{
	auto it = FindIf(
		m_bindings,
		[binding](const VertexBinding& elem) -> bool {
			bool isMatch = (elem.GetBinding() == binding);
			return isMatch; });
	if (it == std::end(m_bindings)) {
		return VALUE_IGNORED;
	}
	uint32_t index = static_cast<uint32_t>(std::distance(std::begin(m_bindings), it));
	return index;
}

Result VertexDescription::AppendBinding(const VertexBinding& binding)
{
	auto it = FindIf(
		m_bindings,
		[binding](const VertexBinding& elem) -> bool {
			bool isSame = (elem.GetBinding() == binding.GetBinding());
			return isSame; });
	if (it != std::end(m_bindings)) {
		return ERROR_DUPLICATE_ELEMENT;
	}
	m_bindings.push_back(binding);
	return SUCCESS;
}
#pragma endregion

#pragma region Utils

std::string ToString(DescriptorType value)
{
	switch (value)
	{
	case DescriptorType::Sampler: return "DescriptorType::Sampler"; break;
	case DescriptorType::CombinedImageSampler: return "DescriptorType::CombinedImageSampler"; break;
	case DescriptorType::SampledImage: return "DescriptorType::SampledImage"; break;
	case DescriptorType::StorageImage: return "DescriptorType::StorageImage"; break;
	case DescriptorType::UniformTexelBuffer: return "DescriptorType::UniformTexelBuffer  "; break;
	case DescriptorType::StorageTexelBuffer: return "DescriptorType::StorageTexelBuffer  "; break;
	case DescriptorType::UniformBuffer: return "DescriptorType::UniformBuffer"; break;
	case DescriptorType::RawStorageBuffer: return "DescriptorType::RawStorageBuffer"; break;
	case DescriptorType::ROStructuredBuffer: return "DescriptorType::ROStructuredBuffer"; break;
	case DescriptorType::RWStructuredBuffer: return "DescriptorType::RWStructuredBuffer"; break;
	case DescriptorType::UniformBufferDynamic: return "DescriptorType::UniformBufferDynamic"; break;
	case DescriptorType::StorageBufferDynamic: return "DescriptorType::StorageBufferDynamic"; break;
	case DescriptorType::InputAttachment: return "DescriptorType::InputAttachment"; break;

	case DescriptorType::Undefined: break;
	default: break;
	}
	return "<unknown descriptor type>";
}

std::string ToString(VertexSemantic value)
{
	switch (value) {
	case VertexSemantic::Position: return "POSITION"; break;
	case VertexSemantic::Normal: return "NORMAL"; break;
	case VertexSemantic::Color: return "COLOR"; break;
	case VertexSemantic::Tangent: return "TANGENT"; break;
	case VertexSemantic::Bitangent: return "BITANGENT"; break;
	case VertexSemantic::Texcoord: return "TEXCOORD"; break;
	case VertexSemantic::Texcoord0: return "TEXCOORD0"; break;
	case VertexSemantic::Texcoord1: return "TEXCOORD1"; break;
	case VertexSemantic::Texcoord2: return "TEXCOORD2"; break;
	case VertexSemantic::Texcoord3: return "TEXCOORD3"; break;
	case VertexSemantic::Texcoord4: return "TEXCOORD4"; break;
	case VertexSemantic::Texcoord5: return "TEXCOORD5"; break;
	case VertexSemantic::Texcoord6: return "TEXCOORD6"; break;
	case VertexSemantic::Texcoord7: return "TEXCOORD7"; break;
	case VertexSemantic::Texcoord8: return "TEXCOORD8"; break;
	case VertexSemantic::Texcoord9: return "TEXCOORD9"; break;
	case VertexSemantic::Texcoord10: return "TEXCOORD10"; break;
	case VertexSemantic::Texcoord11: return "TEXCOORD11"; break;
	case VertexSemantic::Texcoord12: return "TEXCOORD12"; break;
	case VertexSemantic::Texcoord13: return "TEXCOORD13"; break;
	case VertexSemantic::Texcoord14: return "TEXCOORD14"; break;
	case VertexSemantic::Texcoord15: return "TEXCOORD15"; break;
	case VertexSemantic::Texcoord16: return "TEXCOORD16"; break;
	case VertexSemantic::Texcoord17: return "TEXCOORD17"; break;
	case VertexSemantic::Texcoord18: return "TEXCOORD18"; break;
	case VertexSemantic::Texcoord19: return "TEXCOORD19"; break;
	case VertexSemantic::Texcoord20: return "TEXCOORD20"; break;
	case VertexSemantic::Texcoord21: return "TEXCOORD21"; break;
	case VertexSemantic::Texcoord22: return "TEXCOORD22"; break;
	case VertexSemantic::Undefined: break;
	default: break;
	}
	return "";
}

std::string ToString(IndexType value)
{
	switch (value) {
	default: break;
	case IndexType::Undefined: return "IndexType::Undefined";
	case IndexType::Uint16:    return "IndexType::Uint16";
	case IndexType::Uint32:    return "IndexType::Uint32";
	case IndexType::Uint8:     return "IndexType::Uint8";
	}
	return "<unknown IndexType>";
}

uint32_t IndexTypeSize(IndexType value)
{
	switch (value) {
	case IndexType::Uint16: return sizeof(uint16_t); break;
	case IndexType::Uint32: return sizeof(uint32_t); break;
	case IndexType::Uint8:  return sizeof(uint8_t);  break;
	case IndexType::Undefined:
	default: break;
	}
	return 0;
}

Format VertexSemanticFormat(VertexSemantic value)
{
	switch (value) {
	case VertexSemantic::Position: return Format::R32G32B32_FLOAT; break;
	case VertexSemantic::Normal: return Format::R32G32B32_FLOAT; break;
	case VertexSemantic::Color: return Format::R32G32B32_FLOAT; break;
	case VertexSemantic::Tangent: return Format::R32G32B32_FLOAT; break;
	case VertexSemantic::Bitangent: return Format::R32G32B32_FLOAT; break;
	case VertexSemantic::Texcoord: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord0: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord1: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord2: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord3: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord4: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord5: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord6: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord7: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord8: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord9: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord10: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord11: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord12: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord13: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord14: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord15: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord16: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord17: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord18: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord19: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord20: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord21: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Texcoord22: return Format::R32G32_FLOAT; break;
	case VertexSemantic::Undefined:
	default: break;
	}
	return Format::Undefined;
}

std::string ToString(const gli::target& target)
{
	switch (target) {
	case gli::TARGET_1D:         return "TARGET_1D";
	case gli::TARGET_1D_ARRAY:   return "TARGET_1D_ARRAY";
	case gli::TARGET_2D:         return "TARGET_2D";
	case gli::TARGET_2D_ARRAY:   return "TARGET_2D_ARRAY";
	case gli::TARGET_3D:         return "TARGET_3D";
	case gli::TARGET_RECT:       return "TARGET_RECT";
	case gli::TARGET_RECT_ARRAY: return "TARGET_RECT_ARRAY";
	case gli::TARGET_CUBE:       return "TARGET_CUBE";
	case gli::TARGET_CUBE_ARRAY: return "TARGET_CUBE_ARRAY";
	}
	return "TARGET_UNKNOWN";
}

std::string ToString(const gli::format& format)
{
	switch (format) {
	case gli::FORMAT_UNDEFINED: return "FORMAT_UNDEFINED";
	case gli::FORMAT_RG4_UNORM_PACK8: return "FORMAT_RG4_UNORM_PACK8";
	case gli::FORMAT_RGBA4_UNORM_PACK16: return "FORMAT_RGBA4_UNORM_PACK16";
	case gli::FORMAT_BGRA4_UNORM_PACK16: return "FORMAT_BGRA4_UNORM_PACK16";
	case gli::FORMAT_R5G6B5_UNORM_PACK16: return "FORMAT_R5G6B5_UNORM_PACK16";
	case gli::FORMAT_B5G6R5_UNORM_PACK16: return "FORMAT_B5G6R5_UNORM_PACK16";
	case gli::FORMAT_RGB5A1_UNORM_PACK16: return "FORMAT_RGB5A1_UNORM_PACK16";
	case gli::FORMAT_BGR5A1_UNORM_PACK16: return "FORMAT_BGR5A1_UNORM_PACK16";
	case gli::FORMAT_A1RGB5_UNORM_PACK16: return "FORMAT_A1RGB5_UNORM_PACK16";

	case gli::FORMAT_R8_UNORM_PACK8: return "FORMAT_R8_UNORM_PACK8";
	case gli::FORMAT_R8_SNORM_PACK8: return "FORMAT_R8_SNORM_PACK8";
	case gli::FORMAT_R8_USCALED_PACK8: return "FORMAT_R8_USCALED_PACK8";
	case gli::FORMAT_R8_SSCALED_PACK8: return "FORMAT_R8_SSCALED_PACK8";
	case gli::FORMAT_R8_UINT_PACK8: return "FORMAT_R8_UINT_PACK8";
	case gli::FORMAT_R8_SINT_PACK8: return "FORMAT_R8_SINT_PACK8";
	case gli::FORMAT_R8_SRGB_PACK8: return "FORMAT_R8_SRGB_PACK8";

	case gli::FORMAT_RG8_UNORM_PACK8: return "FORMAT_RG8_UNORM_PACK8";
	case gli::FORMAT_RG8_SNORM_PACK8: return "FORMAT_RG8_SNORM_PACK8";
	case gli::FORMAT_RG8_USCALED_PACK8: return "FORMAT_RG8_USCALED_PACK8";
	case gli::FORMAT_RG8_SSCALED_PACK8: return "FORMAT_RG8_SSCALED_PACK8";
	case gli::FORMAT_RG8_UINT_PACK8: return "FORMAT_RG8_UINT_PACK8";
	case gli::FORMAT_RG8_SINT_PACK8: return "FORMAT_RG8_SINT_PACK8";
	case gli::FORMAT_RG8_SRGB_PACK8: return "FORMAT_RG8_SRGB_PACK8";

	case gli::FORMAT_RGB8_UNORM_PACK8: return "FORMAT_RGB8_UNORM_PACK8";
	case gli::FORMAT_RGB8_SNORM_PACK8: return "FORMAT_RGB8_SNORM_PACK8";
	case gli::FORMAT_RGB8_USCALED_PACK8: return "FORMAT_RGB8_USCALED_PACK8";
	case gli::FORMAT_RGB8_SSCALED_PACK8: return "FORMAT_RGB8_SSCALED_PACK8";
	case gli::FORMAT_RGB8_UINT_PACK8: return "FORMAT_RGB8_UINT_PACK8";
	case gli::FORMAT_RGB8_SINT_PACK8: return "FORMAT_RGB8_SINT_PACK8";
	case gli::FORMAT_RGB8_SRGB_PACK8: return "FORMAT_RGB8_SRGB_PACK8";

	case gli::FORMAT_BGR8_UNORM_PACK8: return "FORMAT_BGR8_UNORM_PACK8";
	case gli::FORMAT_BGR8_SNORM_PACK8: return "FORMAT_BGR8_SNORM_PACK8";
	case gli::FORMAT_BGR8_USCALED_PACK8: return "FORMAT_BGR8_USCALED_PACK8";
	case gli::FORMAT_BGR8_SSCALED_PACK8: return "FORMAT_BGR8_SSCALED_PACK8";
	case gli::FORMAT_BGR8_UINT_PACK8: return "FORMAT_BGR8_UINT_PACK8";
	case gli::FORMAT_BGR8_SINT_PACK8: return "FORMAT_BGR8_SINT_PACK8";
	case gli::FORMAT_BGR8_SRGB_PACK8: return "FORMAT_BGR8_SRGB_PACK8";

	case gli::FORMAT_RGBA8_UNORM_PACK8: return "FORMAT_RGBA8_UNORM_PACK8";
	case gli::FORMAT_RGBA8_SNORM_PACK8: return "FORMAT_RGBA8_SNORM_PACK8";
	case gli::FORMAT_RGBA8_USCALED_PACK8: return "FORMAT_RGBA8_USCALED_PACK8";
	case gli::FORMAT_RGBA8_SSCALED_PACK8: return "FORMAT_RGBA8_SSCALED_PACK8";
	case gli::FORMAT_RGBA8_UINT_PACK8: return "FORMAT_RGBA8_UINT_PACK8";
	case gli::FORMAT_RGBA8_SINT_PACK8: return "FORMAT_RGBA8_SINT_PACK8";
	case gli::FORMAT_RGBA8_SRGB_PACK8: return "FORMAT_RGBA8_SRGB_PACK8";

	case gli::FORMAT_BGRA8_UNORM_PACK8: return "FORMAT_BGRA8_UNORM_PACK8";
	case gli::FORMAT_BGRA8_SNORM_PACK8: return "FORMAT_BGRA8_SNORM_PACK8";
	case gli::FORMAT_BGRA8_USCALED_PACK8: return "FORMAT_BGRA8_USCALED_PACK8";
	case gli::FORMAT_BGRA8_SSCALED_PACK8: return "FORMAT_BGRA8_SSCALED_PACK8";
	case gli::FORMAT_BGRA8_UINT_PACK8: return "FORMAT_BGRA8_UINT_PACK8";
	case gli::FORMAT_BGRA8_SINT_PACK8: return "FORMAT_BGRA8_SINT_PACK8";
	case gli::FORMAT_BGRA8_SRGB_PACK8: return "FORMAT_BGRA8_SRGB_PACK8";

	case gli::FORMAT_RGBA8_UNORM_PACK32: return "FORMAT_RGBA8_UNORM_PACK32";
	case gli::FORMAT_RGBA8_SNORM_PACK32: return "FORMAT_RGBA8_SNORM_PACK32";
	case gli::FORMAT_RGBA8_USCALED_PACK32: return "FORMAT_RGBA8_USCALED_PACK32";
	case gli::FORMAT_RGBA8_SSCALED_PACK32: return "FORMAT_RGBA8_SSCALED_PACK32";
	case gli::FORMAT_RGBA8_UINT_PACK32: return "FORMAT_RGBA8_UINT_PACK32";
	case gli::FORMAT_RGBA8_SINT_PACK32: return "FORMAT_RGBA8_SINT_PACK32";
	case gli::FORMAT_RGBA8_SRGB_PACK32: return "FORMAT_RGBA8_SRGB_PACK32";

	case gli::FORMAT_RGB10A2_UNORM_PACK32: return "FORMAT_RGB10A2_UNORM_PACK32";
	case gli::FORMAT_RGB10A2_SNORM_PACK32: return "FORMAT_RGB10A2_SNORM_PACK32";
	case gli::FORMAT_RGB10A2_USCALED_PACK32: return "FORMAT_RGB10A2_USCALED_PACK32";
	case gli::FORMAT_RGB10A2_SSCALED_PACK32: return "FORMAT_RGB10A2_SSCALED_PACK32";
	case gli::FORMAT_RGB10A2_UINT_PACK32: return "FORMAT_RGB10A2_UINT_PACK32";
	case gli::FORMAT_RGB10A2_SINT_PACK32: return "FORMAT_RGB10A2_SINT_PACK32";

	case gli::FORMAT_BGR10A2_UNORM_PACK32: return "FORMAT_BGR10A2_UNORM_PACK32";
	case gli::FORMAT_BGR10A2_SNORM_PACK32: return "FORMAT_BGR10A2_SNORM_PACK32";
	case gli::FORMAT_BGR10A2_USCALED_PACK32: return "FORMAT_BGR10A2_USCALED_PACK32";
	case gli::FORMAT_BGR10A2_SSCALED_PACK32: return "FORMAT_BGR10A2_SSCALED_PACK32";
	case gli::FORMAT_BGR10A2_UINT_PACK32: return "FORMAT_BGR10A2_UINT_PACK32";
	case gli::FORMAT_BGR10A2_SINT_PACK32: return "FORMAT_BGR10A2_SINT_PACK32";

	case gli::FORMAT_R16_UNORM_PACK16: return "FORMAT_R16_UNORM_PACK16";
	case gli::FORMAT_R16_SNORM_PACK16: return "FORMAT_R16_SNORM_PACK16";
	case gli::FORMAT_R16_USCALED_PACK16: return "FORMAT_R16_USCALED_PACK16";
	case gli::FORMAT_R16_SSCALED_PACK16: return "FORMAT_R16_SSCALED_PACK16";
	case gli::FORMAT_R16_UINT_PACK16: return "FORMAT_R16_UINT_PACK16";
	case gli::FORMAT_R16_SINT_PACK16: return "FORMAT_R16_SINT_PACK16";
	case gli::FORMAT_R16_SFLOAT_PACK16: return "FORMAT_R16_SFLOAT_PACK16";

	case gli::FORMAT_RG16_UNORM_PACK16: return "FORMAT_RG16_UNORM_PACK16";
	case gli::FORMAT_RG16_SNORM_PACK16: return "FORMAT_RG16_SNORM_PACK16";
	case gli::FORMAT_RG16_USCALED_PACK16: return "FORMAT_RG16_USCALED_PACK16";
	case gli::FORMAT_RG16_SSCALED_PACK16: return "FORMAT_RG16_SSCALED_PACK16";
	case gli::FORMAT_RG16_UINT_PACK16: return "FORMAT_RG16_UINT_PACK16";
	case gli::FORMAT_RG16_SINT_PACK16: return "FORMAT_RG16_SINT_PACK16";
	case gli::FORMAT_RG16_SFLOAT_PACK16: return "FORMAT_RG16_SFLOAT_PACK16";

	case gli::FORMAT_RGB16_UNORM_PACK16: return "FORMAT_RGB16_UNORM_PACK16";
	case gli::FORMAT_RGB16_SNORM_PACK16: return "FORMAT_RGB16_SNORM_PACK16";
	case gli::FORMAT_RGB16_USCALED_PACK16: return "FORMAT_RGB16_USCALED_PACK16";
	case gli::FORMAT_RGB16_SSCALED_PACK16: return "FORMAT_RGB16_SSCALED_PACK16";
	case gli::FORMAT_RGB16_UINT_PACK16: return "FORMAT_RGB16_UINT_PACK16";
	case gli::FORMAT_RGB16_SINT_PACK16: return "FORMAT_RGB16_SINT_PACK16";
	case gli::FORMAT_RGB16_SFLOAT_PACK16: return "FORMAT_RGB16_SFLOAT_PACK16";

	case gli::FORMAT_RGBA16_UNORM_PACK16: return "FORMAT_RGBA16_UNORM_PACK16";
	case gli::FORMAT_RGBA16_SNORM_PACK16: return "FORMAT_RGBA16_SNORM_PACK16";
	case gli::FORMAT_RGBA16_USCALED_PACK16: return "FORMAT_RGBA16_USCALED_PACK16";
	case gli::FORMAT_RGBA16_SSCALED_PACK16: return "FORMAT_RGBA16_SSCALED_PACK16";
	case gli::FORMAT_RGBA16_UINT_PACK16: return "FORMAT_RGBA16_UINT_PACK16";
	case gli::FORMAT_RGBA16_SINT_PACK16: return "FORMAT_RGBA16_SINT_PACK16";
	case gli::FORMAT_RGBA16_SFLOAT_PACK16: return "FORMAT_RGBA16_SFLOAT_PACK16";

	case gli::FORMAT_R32_UINT_PACK32: return "FORMAT_R32_UINT_PACK32";
	case gli::FORMAT_R32_SINT_PACK32: return "FORMAT_R32_SINT_PACK32";
	case gli::FORMAT_R32_SFLOAT_PACK32: return "FORMAT_R32_SFLOAT_PACK32";

	case gli::FORMAT_RG32_UINT_PACK32: return "FORMAT_RG32_UINT_PACK32";
	case gli::FORMAT_RG32_SINT_PACK32: return "FORMAT_RG32_SINT_PACK32";
	case gli::FORMAT_RG32_SFLOAT_PACK32: return "FORMAT_RG32_SFLOAT_PACK32";

	case gli::FORMAT_RGB32_UINT_PACK32: return "FORMAT_RGB32_UINT_PACK32";
	case gli::FORMAT_RGB32_SINT_PACK32: return "FORMAT_RGB32_SINT_PACK32";
	case gli::FORMAT_RGB32_SFLOAT_PACK32: return "FORMAT_RGB32_SFLOAT_PACK32";

	case gli::FORMAT_RGBA32_UINT_PACK32: return "FORMAT_RGBA32_UINT_PACK32";
	case gli::FORMAT_RGBA32_SINT_PACK32: return "FORMAT_RGBA32_SINT_PACK32";
	case gli::FORMAT_RGBA32_SFLOAT_PACK32: return "FORMAT_RGBA32_SFLOAT_PACK32";

	case gli::FORMAT_R64_UINT_PACK64: return "FORMAT_R64_UINT_PACK64";
	case gli::FORMAT_R64_SINT_PACK64: return "FORMAT_R64_SINT_PACK64";
	case gli::FORMAT_R64_SFLOAT_PACK64: return "FORMAT_R64_SFLOAT_PACK64";

	case gli::FORMAT_RG64_UINT_PACK64: return "FORMAT_RG64_UINT_PACK64";
	case gli::FORMAT_RG64_SINT_PACK64: return "FORMAT_RG64_SINT_PACK64";
	case gli::FORMAT_RG64_SFLOAT_PACK64: return "FORMAT_RG64_SFLOAT_PACK64";

	case gli::FORMAT_RGB64_UINT_PACK64: return "FORMAT_RGB64_UINT_PACK64";
	case gli::FORMAT_RGB64_SINT_PACK64: return "FORMAT_RGB64_SINT_PACK64";
	case gli::FORMAT_RGB64_SFLOAT_PACK64: return "FORMAT_RGB64_SFLOAT_PACK64";

	case gli::FORMAT_RGBA64_UINT_PACK64: return "FORMAT_RGBA64_UINT_PACK64";
	case gli::FORMAT_RGBA64_SINT_PACK64: return "FORMAT_RGBA64_SINT_PACK64";
	case gli::FORMAT_RGBA64_SFLOAT_PACK64: return "FORMAT_RGBA64_SFLOAT_PACK64";

	case gli::FORMAT_RG11B10_UFLOAT_PACK32: return "FORMAT_RG11B10_UFLOAT_PACK32";
	case gli::FORMAT_RGB9E5_UFLOAT_PACK32: return "FORMAT_RGB9E5_UFLOAT_PACK32";

	case gli::FORMAT_D16_UNORM_PACK16: return "FORMAT_D16_UNORM_PACK16";
	case gli::FORMAT_D24_UNORM_PACK32: return "FORMAT_D24_UNORM_PACK32";
	case gli::FORMAT_D32_SFLOAT_PACK32: return "FORMAT_D32_SFLOAT_PACK32";
	case gli::FORMAT_S8_UINT_PACK8: return "FORMAT_S8_UINT_PACK8";
	case gli::FORMAT_D16_UNORM_S8_UINT_PACK32: return "FORMAT_D16_UNORM_S8_UINT_PACK32";
	case gli::FORMAT_D24_UNORM_S8_UINT_PACK32: return "FORMAT_D24_UNORM_S8_UINT_PACK32";
	case gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64: return "FORMAT_D32_SFLOAT_S8_UINT_PACK64";

	case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return "FORMAT_RGB_DXT1_UNORM_BLOCK8";
	case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return "FORMAT_RGB_DXT1_SRGB_BLOCK8";
	case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return "FORMAT_RGBA_DXT1_UNORM_BLOCK8";
	case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return "FORMAT_RGBA_DXT1_SRGB_BLOCK8";
	case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return "FORMAT_RGBA_DXT3_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return "FORMAT_RGBA_DXT3_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return "FORMAT_RGBA_DXT5_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return "FORMAT_RGBA_DXT5_SRGB_BLOCK16";
	case gli::FORMAT_R_ATI1N_UNORM_BLOCK8: return "FORMAT_R_ATI1N_UNORM_BLOCK8";
	case gli::FORMAT_R_ATI1N_SNORM_BLOCK8: return "FORMAT_R_ATI1N_SNORM_BLOCK8";
	case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16: return "FORMAT_RG_ATI2N_UNORM_BLOCK16";
	case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16: return "FORMAT_RG_ATI2N_SNORM_BLOCK16";
	case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16: return "FORMAT_RGB_BP_UFLOAT_BLOCK16";
	case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16: return "FORMAT_RGB_BP_SFLOAT_BLOCK16";
	case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return "FORMAT_RGBA_BP_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return "FORMAT_RGBA_BP_SRGB_BLOCK16";

	case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8: return "FORMAT_RGB_ETC2_UNORM_BLOCK8";
	case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8: return "FORMAT_RGB_ETC2_SRGB_BLOCK8";
	case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK8: return "FORMAT_RGBA_ETC2_UNORM_BLOCK8";
	case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK8: return "FORMAT_RGBA_ETC2_SRGB_BLOCK8";
	case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK16: return "FORMAT_RGBA_ETC2_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK16: return "FORMAT_RGBA_ETC2_SRGB_BLOCK16";
	case gli::FORMAT_R_EAC_UNORM_BLOCK8: return "FORMAT_R_EAC_UNORM_BLOCK8";
	case gli::FORMAT_R_EAC_SNORM_BLOCK8: return "FORMAT_R_EAC_SNORM_BLOCK8";
	case gli::FORMAT_RG_EAC_UNORM_BLOCK16: return "FORMAT_RG_EAC_UNORM_BLOCK16";
	case gli::FORMAT_RG_EAC_SNORM_BLOCK16: return "FORMAT_RG_EAC_SNORM_BLOCK16";

	case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16";

	case gli::FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32: return "FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32";
	case gli::FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32: return "FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32";
	case gli::FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32: return "FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32";
	case gli::FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32: return "FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32";
	case gli::FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32: return "FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32";
	case gli::FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32: return "FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32";
	case gli::FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32: return "FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32";
	case gli::FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32: return "FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32";
	case gli::FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8: return "FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8";
	case gli::FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8: return "FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8";
	case gli::FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8: return "FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8";
	case gli::FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8: return "FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8";

	case gli::FORMAT_RGB_ETC_UNORM_BLOCK8: return "FORMAT_RGB_ETC_UNORM_BLOCK8";
	case gli::FORMAT_RGB_ATC_UNORM_BLOCK8: return "FORMAT_RGB_ATC_UNORM_BLOCK8";
	case gli::FORMAT_RGBA_ATCA_UNORM_BLOCK16: return "FORMAT_RGBA_ATCA_UNORM_BLOCK16";
	case gli::FORMAT_RGBA_ATCI_UNORM_BLOCK16: return "FORMAT_RGBA_ATCI_UNORM_BLOCK16";

	case gli::FORMAT_L8_UNORM_PACK8: return "FORMAT_L8_UNORM_PACK8";
	case gli::FORMAT_A8_UNORM_PACK8: return "FORMAT_A8_UNORM_PACK8";
	case gli::FORMAT_LA8_UNORM_PACK8: return "FORMAT_LA8_UNORM_PACK8";
	case gli::FORMAT_L16_UNORM_PACK16: return "FORMAT_L16_UNORM_PACK16";
	case gli::FORMAT_A16_UNORM_PACK16: return "FORMAT_A16_UNORM_PACK16";
	case gli::FORMAT_LA16_UNORM_PACK16: return "FORMAT_LA16_UNORM_PACK16";

	case gli::FORMAT_BGR8_UNORM_PACK32: return "FORMAT_BGR8_UNORM_PACK32";
	case gli::FORMAT_BGR8_SRGB_PACK32: return "FORMAT_BGR8_SRGB_PACK32";

	case gli::FORMAT_RG3B2_UNORM_PACK8: return "FORMAT_RG3B2_UNORM_PACK8";
	}
	return "FORMAT_UNKNOWN";
}

#pragma endregion

#pragma region Vk Utils

std::string ToString(VkResult value)
{
	return string_VkResult(value);
}

std::string ToString(VkDescriptorType value)
{
	return string_VkDescriptorType(value);	
}

std::string ToString(VkPresentModeKHR value)
{
	return string_VkPresentModeKHR(value);
}

VkAttachmentLoadOp ToVkEnum(AttachmentLoadOp value)
{
	switch (value) {
	default: break;
	case AttachmentLoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD; break;
	case AttachmentLoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR; break;
	case AttachmentLoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE; break;
	}
	return InvalidValue<VkAttachmentLoadOp>();
}

VkAttachmentStoreOp ToVkEnum(AttachmentStoreOp value)
{
	switch (value) {
	default: break;
	case AttachmentStoreOp::Store: return VK_ATTACHMENT_STORE_OP_STORE; break;
	case AttachmentStoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE; break;
	}
	return InvalidValue<VkAttachmentStoreOp>();
}

VkBlendFactor ToVkEnum(BlendFactor value)
{
	switch (value) {
	default: break;
	case BLEND_FACTOR_ZERO: return VK_BLEND_FACTOR_ZERO; break;
	case BLEND_FACTOR_ONE: return VK_BLEND_FACTOR_ONE; break;
	case BLEND_FACTOR_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
	case BLEND_FACTOR_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
	case BLEND_FACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
	case BLEND_FACTOR_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
	case BLEND_FACTOR_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; break;
	case BLEND_FACTOR_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA; break;
	case BLEND_FACTOR_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE; break;
	case BLEND_FACTOR_SRC1_COLOR: return VK_BLEND_FACTOR_SRC1_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR; break;
	case BLEND_FACTOR_SRC1_ALPHA: return VK_BLEND_FACTOR_SRC1_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA; break;
	}
	return InvalidValue<VkBlendFactor>();
}

VkBlendOp ToVkEnum(BlendOp value)
{
	switch (value) {
	default: break;
	case BLEND_OP_ADD: return VK_BLEND_OP_ADD; break;
	case BLEND_OP_SUBTRACT: return VK_BLEND_OP_SUBTRACT; break;
	case BLEND_OP_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT; break;
	case BLEND_OP_MIN: return VK_BLEND_OP_MIN; break;
	case BLEND_OP_MAX: return VK_BLEND_OP_MAX; break;

#if defined(VK_BLEND_OPERATION_ADVANCED)
	// Provdied by VK_blend_operation_advanced
	case BLEND_OP_ZERO: return  VK_BLEND_OP_ZERO_EXT; break;
	case BLEND_OP_SRC: return  VK_BLEND_OP_SRC_EXT; break;
	case BLEND_OP_DST: return  VK_BLEND_OP_DST_EXT; break;
	case BLEND_OP_SRC_OVER: return  VK_BLEND_OP_SRC_OVER_EXT; break;
	case BLEND_OP_DST_OVER: return  VK_BLEND_OP_DST_OVER_EXT; break;
	case BLEND_OP_SRC_IN: return  VK_BLEND_OP_SRC_IN_EXT; break;
	case BLEND_OP_DST_IN: return  VK_BLEND_OP_DST_IN_EXT; break;
	case BLEND_OP_SRC_OUT: return  VK_BLEND_OP_SRC_OUT_EXT; break;
	case BLEND_OP_DST_OUT: return  VK_BLEND_OP_DST_OUT_EXT; break;
	case BLEND_OP_SRC_ATOP: return  VK_BLEND_OP_SRC_ATOP_EXT; break;
	case BLEND_OP_DST_ATOP: return  VK_BLEND_OP_DST_ATOP_EXT; break;
	case BLEND_OP_XOR: return  VK_BLEND_OP_XOR_EXT; break;
	case BLEND_OP_MULTIPLY: return  VK_BLEND_OP_MULTIPLY_EXT; break;
	case BLEND_OP_SCREEN: return  VK_BLEND_OP_SCREEN_EXT; break;
	case BLEND_OP_OVERLAY: return  VK_BLEND_OP_OVERLAY_EXT; break;
	case BLEND_OP_DARKEN: return  VK_BLEND_OP_DARKEN_EXT; break;
	case BLEND_OP_LIGHTEN: return  VK_BLEND_OP_LIGHTEN_EXT; break;
	case BLEND_OP_COLORDODGE: return  VK_BLEND_OP_COLORDODGE_EXT; break;
	case BLEND_OP_COLORBURN: return  VK_BLEND_OP_COLORBURN_EXT; break;
	case BLEND_OP_HARDLIGHT: return  VK_BLEND_OP_HARDLIGHT_EXT; break;
	case BLEND_OP_SOFTLIGHT: return  VK_BLEND_OP_SOFTLIGHT_EXT; break;
	case BLEND_OP_DIFFERENCE: return  VK_BLEND_OP_DIFFERENCE_EXT; break;
	case BLEND_OP_EXCLUSION: return  VK_BLEND_OP_EXCLUSION_EXT; break;
	case BLEND_OP_INVERT: return  VK_BLEND_OP_INVERT_EXT; break;
	case BLEND_OP_INVERT_RGB: return  VK_BLEND_OP_INVERT_RGB_EXT; break;
	case BLEND_OP_LINEARDODGE: return  VK_BLEND_OP_LINEARDODGE_EXT; break;
	case BLEND_OP_LINEARBURN: return  VK_BLEND_OP_LINEARBURN_EXT; break;
	case BLEND_OP_VIVIDLIGHT: return  VK_BLEND_OP_VIVIDLIGHT_EXT; break;
	case BLEND_OP_LINEARLIGHT: return  VK_BLEND_OP_LINEARLIGHT_EXT; break;
	case BLEND_OP_PINLIGHT: return  VK_BLEND_OP_PINLIGHT_EXT; break;
	case BLEND_OP_HARDMIX: return  VK_BLEND_OP_HARDMIX_EXT; break;
	case BLEND_OP_HSL_HUE: return  VK_BLEND_OP_HSL_HUE_EXT; break;
	case BLEND_OP_HSL_SATURATION: return  VK_BLEND_OP_HSL_SATURATION_EXT; break;
	case BLEND_OP_HSL_COLOR: return  VK_BLEND_OP_HSL_COLOR_EXT; break;
	case BLEND_OP_HSL_LUMINOSITY: return  VK_BLEND_OP_HSL_LUMINOSITY_EXT; break;
	case BLEND_OP_PLUS: return  VK_BLEND_OP_PLUS_EXT; break;
	case BLEND_OP_PLUS_CLAMPED: return  VK_BLEND_OP_PLUS_CLAMPED_EXT; break;
	case BLEND_OP_PLUS_CLAMPED_ALPHA: return  VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT; break;
	case BLEND_OP_PLUS_DARKER: return  VK_BLEND_OP_PLUS_DARKER_EXT; break;
	case BLEND_OP_MINUS: return  VK_BLEND_OP_MINUS_EXT; break;
	case BLEND_OP_MINUS_CLAMPED: return  VK_BLEND_OP_MINUS_CLAMPED_EXT; break;
	case BLEND_OP_CONTRAST: return  VK_BLEND_OP_CONTRAST_EXT; break;
	case BLEND_OP_INVERT_OVG: return  VK_BLEND_OP_INVERT_OVG_EXT; break;
	case BLEND_OP_RED: return  VK_BLEND_OP_RED_EXT; break;
	case BLEND_OP_GREEN: return  VK_BLEND_OP_GREEN_EXT; break;
	case BLEND_OP_BLUE: return  VK_BLEND_OP_BLUE_EXT; break;
#endif // defined(PPX_VK_BLEND_OPERATION_ADVANCED)
	}
	return InvalidValue<VkBlendOp>();
}

VkBorderColor ToVkEnum(BorderColor value)
{
	switch (value) {
	default: break;
	case decltype(value)::FloatTransparentBlack: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	case decltype(value)::IntTransparentBlack:   return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	case decltype(value)::FloatOpaqueBlack:      return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	case decltype(value)::IntOpaqueBlack:        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	case decltype(value)::FloatOpaqueWhite:      return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	case decltype(value)::IntOpaqueWhite:        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	}
	return InvalidValue<VkBorderColor>();
}

VkBufferUsageFlags ToVkBufferUsageFlags(const BufferUsageFlags& value)
{
	VkBufferUsageFlags flags = 0;
	if (value.bits.transferSrc)                    flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (value.bits.transferDst)                    flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (value.bits.uniformTexelBuffer)             flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	if (value.bits.storageTexelBuffer)             flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	if (value.bits.uniformBuffer)                  flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (value.bits.rawStorageBuffer)               flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (value.bits.roStructuredBuffer)             flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (value.bits.rwStructuredBuffer)             flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (value.bits.indexBuffer)                    flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (value.bits.vertexBuffer)                   flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (value.bits.indirectBuffer)                 flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if (value.bits.conditionalRendering)           flags |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
	if (value.bits.transformFeedbackBuffer)        flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
	if (value.bits.transformFeedbackCounterBuffer) flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
	if (value.bits.shaderDeviceAddress)            flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
	return flags;
}

VkChromaLocation ToVkEnum(ChromaLocation value)
{
	switch (value) {
	default: break;
	case ChromaLocation::CositedEven: return VK_CHROMA_LOCATION_COSITED_EVEN;
	case ChromaLocation::Midpoint: return VK_CHROMA_LOCATION_MIDPOINT;
	}
	return InvalidValue<VkChromaLocation>();
}

VkClearColorValue ToVkClearColorValue(const float4& value)
{
	VkClearColorValue res = {};
	res.float32[0] = value[0];
	res.float32[1] = value[1];
	res.float32[2] = value[2];
	res.float32[3] = value[3];
	return res;
}

VkClearDepthStencilValue ToVkClearDepthStencilValue(const DepthStencilClearValue& value)
{
	VkClearDepthStencilValue res = {};
	res.depth = value.depth;
	res.stencil = value.stencil;
	return res;
}

VkColorComponentFlags ToVkColorComponentFlags(const ColorComponentFlags& value)
{
	VkColorComponentFlags flags = 0;
	if (value.bits.r) flags |= VK_COLOR_COMPONENT_R_BIT;
	if (value.bits.g) flags |= VK_COLOR_COMPONENT_G_BIT;
	if (value.bits.b) flags |= VK_COLOR_COMPONENT_B_BIT;
	if (value.bits.a) flags |= VK_COLOR_COMPONENT_A_BIT;
	return flags;
}

VkCompareOp ToVkEnum(CompareOp value)
{
	switch (value) {
	default: break;
	case decltype(value)::Never:          return VK_COMPARE_OP_NEVER;
	case decltype(value)::Less:           return VK_COMPARE_OP_LESS;
	case decltype(value)::Equal:          return VK_COMPARE_OP_EQUAL;
	case decltype(value)::LessOrEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
	case decltype(value)::Greater:        return VK_COMPARE_OP_GREATER;
	case decltype(value)::NotEqual:       return VK_COMPARE_OP_NOT_EQUAL;
	case decltype(value)::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case decltype(value)::Always:         return VK_COMPARE_OP_ALWAYS;
	}
	return InvalidValue<VkCompareOp>();
}

VkComponentSwizzle ToVkEnum(ComponentSwizzle value)
{
	switch (value) {
	default: break;
	case ComponentSwizzle::Identity: return VK_COMPONENT_SWIZZLE_IDENTITY; break;
	case ComponentSwizzle::Zero: return VK_COMPONENT_SWIZZLE_ZERO; break;
	case ComponentSwizzle::One: return VK_COMPONENT_SWIZZLE_ONE; break;
	case ComponentSwizzle::R: return VK_COMPONENT_SWIZZLE_R; break;
	case ComponentSwizzle::G: return VK_COMPONENT_SWIZZLE_G; break;
	case ComponentSwizzle::B: return VK_COMPONENT_SWIZZLE_B; break;
	case ComponentSwizzle::A: return VK_COMPONENT_SWIZZLE_A; break;
	}
	return InvalidValue<VkComponentSwizzle>();
}

VkComponentMapping ToVkComponentMapping(const ComponentMapping& value)
{
	VkComponentMapping res = {};
	res.r = ToVkEnum(value.r);
	res.g = ToVkEnum(value.g);
	res.b = ToVkEnum(value.b);
	res.a = ToVkEnum(value.a);
	return res;
}

VkCullModeFlagBits ToVkEnum(CullMode value)
{
	switch (value) {
	default: break;
	case CULL_MODE_NONE: return VK_CULL_MODE_NONE; break;
	case CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT; break;
	case CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT; break;
	}
	return InvalidValue<VkCullModeFlagBits>();
}

VkDescriptorBindingFlags ToVkDescriptorBindingFlags(const DescriptorBindingFlags& value)
{
	VkDescriptorBindingFlags flags = 0;
	if (value.bits.updatable) {
		flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
	}
	if (value.bits.partiallyBound) {
		flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
	}
	return flags;
}

VkDescriptorType ToVkEnum(DescriptorType value)
{
	switch (value) {

	case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER; break;
	case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
	case DescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
	case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
	case DescriptorType::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; break;
	case DescriptorType::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER; break;
	case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
	case DescriptorType::RawStorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
	case DescriptorType::ROStructuredBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
	case DescriptorType::RWStructuredBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
	case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; break;
	case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; break;
	case DescriptorType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; break;
	case DescriptorType::Undefined:
	default: break;
	}
	return InvalidValue<VkDescriptorType>();
}

VkFilter ToVkEnum(Filter value)
{
	switch (value) {
	default: break;
	case decltype(value)::Nearest: return VK_FILTER_NEAREST;
	case decltype(value)::Linear:  return VK_FILTER_LINEAR;
	}
	return InvalidValue<VkFilter>();
}

VkFormat ToVkEnum(Format value)
{
	switch (value)
	{
	// 8-bit signed normalized
	case Format::R8_SNORM: return VK_FORMAT_R8_SNORM; break;
	case Format::R8G8_SNORM: return VK_FORMAT_R8G8_SNORM; break;
	case Format::R8G8B8_SNORM: return VK_FORMAT_R8G8B8_SNORM; break;
	case Format::R8G8B8A8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM; break;
	case Format::B8G8R8_SNORM: return VK_FORMAT_B8G8R8_SNORM; break;
	case Format::B8G8R8A8_SNORM: return VK_FORMAT_B8G8R8A8_SNORM; break;

	// 8-bit unsigned normalized
	case Format::R8_UNORM: return VK_FORMAT_R8_UNORM; break;
	case Format::R8G8_UNORM: return VK_FORMAT_R8G8_UNORM; break;
	case Format::R8G8B8_UNORM: return VK_FORMAT_R8G8B8_UNORM; break;
	case Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM; break;
	case Format::B8G8R8_UNORM: return VK_FORMAT_B8G8R8_UNORM; break;
	case Format::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM; break;

	// 8-bit signed integer
	case Format::R8_SINT: return VK_FORMAT_R8_SINT; break;
	case Format::R8G8_SINT: return VK_FORMAT_R8G8_SINT; break;
	case Format::R8G8B8_SINT: return VK_FORMAT_R8G8B8_SINT; break;
	case Format::R8G8B8A8_SINT: return VK_FORMAT_R8G8B8A8_SINT; break;
	case Format::B8G8R8_SINT: return VK_FORMAT_B8G8R8_SINT; break;
	case Format::B8G8R8A8_SINT: return VK_FORMAT_B8G8R8A8_SINT; break;

	// 8-bit unsigned integer
	case Format::R8_UINT: return VK_FORMAT_R8_UINT; break;
	case Format::R8G8_UINT: return VK_FORMAT_R8G8_UINT; break;
	case Format::R8G8B8_UINT: return VK_FORMAT_R8G8B8_UINT; break;
	case Format::R8G8B8A8_UINT: return VK_FORMAT_R8G8B8A8_UINT; break;
	case Format::B8G8R8_UINT: return VK_FORMAT_B8G8R8_UINT; break;
	case Format::B8G8R8A8_UINT: return VK_FORMAT_B8G8R8A8_UINT; break;

	// 16-bit signed normalized
	case Format::R16_SNORM: return VK_FORMAT_R16_SNORM; break;
	case Format::R16G16_SNORM: return VK_FORMAT_R16G16_SNORM; break;
	case Format::R16G16B16_SNORM: return VK_FORMAT_R16G16B16_SNORM; break;
	case Format::R16G16B16A16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM; break;

	// 16-bit unsigned normalized
	case Format::R16_UNORM: return VK_FORMAT_R16_UNORM; break;
	case Format::R16G16_UNORM: return VK_FORMAT_R16G16_UNORM; break;
	case Format::R16G16B16_UNORM: return VK_FORMAT_R16G16B16_UNORM; break;
	case Format::R16G16B16A16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM; break;

	// 16-bit signed integer
	case Format::R16_SINT: return VK_FORMAT_R16_SINT; break;
	case Format::R16G16_SINT: return VK_FORMAT_R16G16_SINT; break;
	case Format::R16G16B16_SINT: return VK_FORMAT_R16G16B16_SINT; break;
	case Format::R16G16B16A16_SINT: return VK_FORMAT_R16G16B16A16_SINT; break;

	// 16-bit unsigned integer
	case Format::R16_UINT: return VK_FORMAT_R16_UINT; break;
	case Format::R16G16_UINT: return VK_FORMAT_R16G16_UINT; break;
	case Format::R16G16B16_UINT: return VK_FORMAT_R16G16B16_UINT; break;
	case Format::R16G16B16A16_UINT: return VK_FORMAT_R16G16B16A16_UINT; break;

	// 16-bit float
	case Format::R16_FLOAT: return VK_FORMAT_R16_SFLOAT; break;
	case Format::R16G16_FLOAT: return VK_FORMAT_R16G16_SFLOAT; break;
	case Format::R16G16B16_FLOAT: return VK_FORMAT_R16G16B16_SFLOAT; break;
	case Format::R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT; break;

	// 32-bit signed integer
	case Format::R32_SINT: return VK_FORMAT_R32_SINT; break;
	case Format::R32G32_SINT: return VK_FORMAT_R32G32_SINT; break;
	case Format::R32G32B32_SINT: return VK_FORMAT_R32G32B32_SINT; break;
	case Format::R32G32B32A32_SINT: return VK_FORMAT_R32G32B32A32_SINT; break;

	// 32-bit unsigned integer
	case Format::R32_UINT: return VK_FORMAT_R32_UINT; break;
	case Format::R32G32_UINT: return VK_FORMAT_R32G32_UINT; break;
	case Format::R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT; break;
	case Format::R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT; break;

	// 32-bit float
	case Format::R32_FLOAT: return VK_FORMAT_R32_SFLOAT; break;
	case Format::R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT; break;
	case Format::R32G32B32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT; break;
	case Format::R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT; break;

	// 8-bit unsigned integer stencil
	case Format::S8_UINT: return VK_FORMAT_S8_UINT; break;

	// 16-bit unsigned normalized depth
	case Format::D16_UNORM: return VK_FORMAT_D16_UNORM; break;

	// 32-bit float depth
	case Format::D32_FLOAT: return VK_FORMAT_D32_SFLOAT; break;

	// Depth/stencil combinations
	case Format::D16_UNORM_S8_UINT: return VK_FORMAT_D16_UNORM_S8_UINT; break;
	case Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT; break;
	case Format::D32_FLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT; break;

	// SRGB
	case Format::R8_SRGB: return VK_FORMAT_R8_SRGB; break;
	case Format::R8G8_SRGB: return VK_FORMAT_R8G8_SRGB; break;
	case Format::R8G8B8_SRGB: return VK_FORMAT_R8G8B8_SRGB; break;
	case Format::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB; break;
	case Format::B8G8R8_SRGB: return VK_FORMAT_B8G8R8_SRGB; break;
	case Format::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB; break;

	// 10-bit
	case Format::R10G10B10A2_UNORM: return VK_FORMAT_A2R10G10B10_UNORM_PACK32; break;

	// 11-bit R, 11-bit G, 10-bit B packed
	case Format::R11G11B10_FLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32; break;

	// Compressed images
	case Format::BC1_RGBA_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK; break;
	case Format::BC1_RGBA_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK; break;
	case Format::BC1_RGB_SRGB: return VK_FORMAT_BC1_RGB_SRGB_BLOCK; break;
	case Format::BC1_RGB_UNORM: return VK_FORMAT_BC1_RGB_UNORM_BLOCK; break;
	case Format::BC2_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK; break;
	case Format::BC2_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK; break;
	case Format::BC3_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK; break;
	case Format::BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK; break;
	case Format::BC4_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK; break;
	case Format::BC4_SNORM: return VK_FORMAT_BC4_SNORM_BLOCK; break;
	case Format::BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK; break;
	case Format::BC5_SNORM: return VK_FORMAT_BC5_SNORM_BLOCK; break;
	case Format::BC6H_UFLOAT: return VK_FORMAT_BC6H_UFLOAT_BLOCK; break;
	case Format::BC6H_SFLOAT: return VK_FORMAT_BC6H_SFLOAT_BLOCK; break;
	case Format::BC7_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK; break;
	case Format::BC7_SRGB: return VK_FORMAT_BC7_SRGB_BLOCK; break;

	case Format::Undefined:
	default: break;
	}
	return VK_FORMAT_UNDEFINED;
}

VkFrontFace ToVkEnum(FrontFace value)
{
	switch (value) {
	default: break;
	case FRONT_FACE_CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
	case FRONT_FACE_CW: return VK_FRONT_FACE_CLOCKWISE; break;
	}
	return InvalidValue<VkFrontFace>();
}

VkImageType ToVkEnum(ImageType value)
{
	switch (value)
	{
	case decltype(value)::Image1D: return VK_IMAGE_TYPE_1D;
	case decltype(value)::Image2D: return VK_IMAGE_TYPE_2D;
	case decltype(value)::Image3D: return VK_IMAGE_TYPE_3D;
	case decltype(value)::Cube:    return VK_IMAGE_TYPE_2D;
	case decltype(value)::Undefined:
	default: break;
	}
	return InvalidValue<VkImageType>();
}

VkImageUsageFlags ToVkImageUsageFlags(const ImageUsageFlags& value)
{
	VkImageUsageFlags flags = 0;
	if (value.bits.transferSrc) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (value.bits.transferDst) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (value.bits.sampled) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (value.bits.storage) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (value.bits.colorAttachment) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (value.bits.depthStencilAttachment) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (value.bits.transientAttachment) flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	if (value.bits.inputAttachment) flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	if (value.bits.fragmentDensityMap) flags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
	if (value.bits.fragmentShadingRateAttachment) flags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
	return flags;
}

VkImageViewType ToVkEnum(ImageViewType value)
{
	switch (value)
	{
	case ImageViewType::ImageView1D: return VK_IMAGE_VIEW_TYPE_1D; break;
	case ImageViewType::ImageView2D: return VK_IMAGE_VIEW_TYPE_2D; break;
	case ImageViewType::ImageView3D: return VK_IMAGE_VIEW_TYPE_3D; break;
	case ImageViewType::Cube: return VK_IMAGE_VIEW_TYPE_CUBE; break;
	case ImageViewType::ImageView1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY; break;
	case ImageViewType::ImageView2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
	case ImageViewType::CubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
	case ImageViewType::Undefined:
	default: break;
	}
	return InvalidValue<VkImageViewType>();
}

VkIndexType ToVkEnum(IndexType value)
{
	switch (value)
	{
	case IndexType::Uint16: return VK_INDEX_TYPE_UINT16; break;
	case IndexType::Uint32: return VK_INDEX_TYPE_UINT32; break;
	case IndexType::Uint8:  return VK_INDEX_TYPE_UINT8_EXT; break;
	case IndexType::Undefined:
	default: break;
	}
	return InvalidValue<VkIndexType>();
}

VkLogicOp ToVkEnum(LogicOp value)
{
	switch (value)
	{
	case LOGIC_OP_CLEAR: return VK_LOGIC_OP_CLEAR; break;
	case LOGIC_OP_AND: return VK_LOGIC_OP_AND; break;
	case LOGIC_OP_AND_REVERSE: return VK_LOGIC_OP_AND_REVERSE; break;
	case LOGIC_OP_COPY: return VK_LOGIC_OP_COPY; break;
	case LOGIC_OP_AND_INVERTED: return VK_LOGIC_OP_AND_INVERTED; break;
	case LOGIC_OP_NO_OP: return VK_LOGIC_OP_NO_OP; break;
	case LOGIC_OP_XOR: return VK_LOGIC_OP_XOR; break;
	case LOGIC_OP_OR: return VK_LOGIC_OP_OR; break;
	case LOGIC_OP_NOR: return VK_LOGIC_OP_NOR; break;
	case LOGIC_OP_EQUIVALENT: return VK_LOGIC_OP_EQUIVALENT; break;
	case LOGIC_OP_INVERT: return VK_LOGIC_OP_INVERT; break;
	case LOGIC_OP_OR_REVERSE: return VK_LOGIC_OP_OR_REVERSE; break;
	case LOGIC_OP_COPY_INVERTED: return VK_LOGIC_OP_COPY_INVERTED; break;
	case LOGIC_OP_OR_INVERTED: return VK_LOGIC_OP_OR_INVERTED; break;
	case LOGIC_OP_NAND: return VK_LOGIC_OP_NAND; break;
	case LOGIC_OP_SET: return VK_LOGIC_OP_SET; break;
	default: break;
	}
	return InvalidValue<VkLogicOp>();
}

VkPipelineStageFlagBits ToVkEnum(PipelineStage value)
{
	switch (value) {
	default: break;
	case PIPELINE_STAGE_TOP_OF_PIPE_BIT: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; break;
	case PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; break;
	}
	return InvalidValue<VkPipelineStageFlagBits>();
}

VkPolygonMode ToVkEnum(PolygonMode value)
{
	switch (value) {
	default: break;
	case POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL; break;
	case POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE; break;
	case POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT; break;
	}
	return InvalidValue<VkPolygonMode>();
}

VkPresentModeKHR ToVkEnum(PresentMode value)
{
	switch (value)
	{
	case PRESENT_MODE_FIFO: return VK_PRESENT_MODE_FIFO_KHR; break;
	case PRESENT_MODE_MAILBOX: return VK_PRESENT_MODE_MAILBOX_KHR; break;
	case PRESENT_MODE_IMMEDIATE: return VK_PRESENT_MODE_IMMEDIATE_KHR; break;
	case PRESENT_MODE_UNDEFINED:
	default: break;
	}
	return InvalidValue<VkPresentModeKHR>();
}

VkPrimitiveTopology ToVkEnum(PrimitiveTopology value)
{
	switch (value)
	{
	case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
	case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
	case PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
	case PRIMITIVE_TOPOLOGY_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
	case PRIMITIVE_TOPOLOGY_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
	case PRIMITIVE_TOPOLOGY_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
	default: break;
	}
	return InvalidValue<VkPrimitiveTopology>();
}

VkQueryType ToVkEnum(QueryType value)
{
	switch (value)
	{
	case decltype(value)::Occlusion:          return VK_QUERY_TYPE_OCCLUSION;
	case decltype(value)::Timestamp:          return VK_QUERY_TYPE_TIMESTAMP;
	case decltype(value)::PipelineStatistics: return VK_QUERY_TYPE_PIPELINE_STATISTICS;
	case decltype(value)::Undefined:
	default: break;
	}
	return InvalidValue<VkQueryType>();
}

VkSamplerAddressMode ToVkEnum(SamplerAddressMode value)
{
	switch (value)
	{
	default: break;
	case decltype(value)::Repeat:            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case decltype(value)::MirrorRepeat:      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case decltype(value)::ClampToEdge:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case decltype(value)::ClampToBorder:     return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case decltype(value)::MirrorClampToEdge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
	return InvalidValue<VkSamplerAddressMode>();
}

VkSamplerMipmapMode ToVkEnum(SamplerMipmapMode value)
{
	switch (value)
	{
	default: break;
	case decltype(value)::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case decltype(value)::Linear:  return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	return InvalidValue<VkSamplerMipmapMode>();
}

VkSamplerReductionMode vkr::ToVkEnum(SamplerReductionMode value)
{
	switch (value) {
	default: break;
	case decltype(value)::Standard:   return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
	case decltype(value)::Comparison: return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
	case decltype(value)::Minimum:    return VK_SAMPLER_REDUCTION_MODE_MIN;
	case decltype(value)::Maximum:    return VK_SAMPLER_REDUCTION_MODE_MAX;
	}
	return InvalidValue<VkSamplerReductionMode>();
}

VkSampleCountFlagBits ToVkEnum(SampleCount value)
{
	switch (value) {
	default: break;
	case SampleCount::Sample1: return VK_SAMPLE_COUNT_1_BIT; break;
	case SampleCount::Sample2: return VK_SAMPLE_COUNT_2_BIT; break;
	case SampleCount::Sample4: return VK_SAMPLE_COUNT_4_BIT; break;
	case SampleCount::Sample8: return VK_SAMPLE_COUNT_8_BIT; break;
	case SampleCount::Sample16: return VK_SAMPLE_COUNT_16_BIT; break;
	case SampleCount::Sample32: return VK_SAMPLE_COUNT_32_BIT; break;
	case SampleCount::Sample64: return VK_SAMPLE_COUNT_64_BIT; break;
	}
	return InvalidValue<VkSampleCountFlagBits>();
}

VkShaderStageFlags ToVkShaderStageFlags(const ShaderStageFlags& value)
{
	VkShaderStageFlags flags = 0;
	if (value.bits.VS) flags |= VK_SHADER_STAGE_VERTEX_BIT;
	if (value.bits.HS) flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	if (value.bits.DS) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	if (value.bits.GS) flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	if (value.bits.PS) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (value.bits.CS) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	return flags;
}

VkStencilOp ToVkEnum(StencilOp value)
{
	switch (value) {
	default: break;
	case STENCIL_OP_KEEP: return VK_STENCIL_OP_KEEP; break;
	case STENCIL_OP_ZERO: return VK_STENCIL_OP_ZERO; break;
	case STENCIL_OP_REPLACE: return VK_STENCIL_OP_REPLACE; break;
	case STENCIL_OP_INCREMENT_AND_CLAMP: return VK_STENCIL_OP_INCREMENT_AND_CLAMP; break;
	case STENCIL_OP_DECREMENT_AND_CLAMP: return VK_STENCIL_OP_DECREMENT_AND_CLAMP; break;
	case STENCIL_OP_INVERT: return VK_STENCIL_OP_INVERT; break;
	case STENCIL_OP_INCREMENT_AND_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP; break;
	case STENCIL_OP_DECREMENT_AND_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP; break;
	}
	return InvalidValue<VkStencilOp>();
}

VkTessellationDomainOrigin ToVkEnum(TessellationDomainOrigin value)
{
	switch (value) {
	default: break;
	case TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT: return VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT; break;
	case TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT: return VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT; break;
	}
	return InvalidValue<VkTessellationDomainOrigin>();
}

VkVertexInputRate ToVkEnum(VertexInputRate value)
{
	switch (value) {
	default: break;
	case VertexInputRate::Vertex: return VK_VERTEX_INPUT_RATE_VERTEX; break;
	case VertexInputRate::Instance: return VK_VERTEX_INPUT_RATE_INSTANCE; break;
	}
	return InvalidValue<VkVertexInputRate>();
}

static Result ToVkBarrier(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	bool                            isSource,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout)
{
	VkPipelineStageFlags PIPELINE_STAGE_ALL_SHADER_STAGES = {};
	if (commandType == CommandType::COMMAND_TYPE_COMPUTE) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (commandType == CommandType::COMMAND_TYPE_GRAPHICS) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	VkPipelineStageFlags PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES = {};
	if (commandType == CommandType::COMMAND_TYPE_COMPUTE) {
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (commandType == CommandType::COMMAND_TYPE_GRAPHICS) {
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	}
	else {
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	if (commandType == CommandType::COMMAND_TYPE_GRAPHICS && features.geometryShader) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
	}
	if (commandType == CommandType::COMMAND_TYPE_GRAPHICS && features.tessellationShader) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |=
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |=
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
	}

	switch (state) {
	default: return ERROR_FAILED; break;

	case ResourceState::Undefined: {
		stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_UNDEFINED;
	} break;

	case ResourceState::General: {
		stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	} break;

	case ResourceState::ConstantBuffer: {
		stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | PIPELINE_STAGE_ALL_SHADER_STAGES;
		accessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;
	case ResourceState::VertexBuffer: {
		stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | PIPELINE_STAGE_ALL_SHADER_STAGES;
		accessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case ResourceState::IndexBuffer: {
		stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		accessMask = VK_ACCESS_INDEX_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case ResourceState::RenderTarget: {
		stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	} break;

	case ResourceState::UnorderedAccess: {
		stageMask = PIPELINE_STAGE_ALL_SHADER_STAGES;
		accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	} break;

	case ResourceState::DepthStencilRead: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	} break;

	case ResourceState::DepthStencilWrite: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	} break;

	case ResourceState::DepthWriteStencilRead: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	} break;

	case ResourceState::DepthReadStencilWrite: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	} break;

	case ResourceState::NonPixelShaderResource: {
		stageMask = PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES;
		accessMask = VK_ACCESS_SHADER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} break;

	case ResourceState::ShaderResource: {
		stageMask = PIPELINE_STAGE_ALL_SHADER_STAGES;
		accessMask = VK_ACCESS_SHADER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} break;

	case ResourceState::PixelShaderResource: {
		stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessMask = VK_ACCESS_SHADER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} break;

	case ResourceState::StreamOut: {
		stageMask = VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
		accessMask = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case ResourceState::IndirectArgument: {
		stageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		accessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case ResourceState::CopySrc: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	} break;

	case ResourceState::CopyDst: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	} break;

	case ResourceState::ResolveSrc: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	} break;

	case ResourceState::ResolveDst: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	} break;

	case ResourceState::Present: {
		stageMask = isSource ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	} break;

	case ResourceState::Predication: {
		stageMask = VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
		accessMask = VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case ResourceState::RaytracingAccelerationStructure: {
		stageMask = InvalidValue<VkPipelineStageFlags>();
		accessMask = InvalidValue<VkAccessFlags>();
		layout = InvalidValue<VkImageLayout>();
	} break;

	case ResourceState::FragmentDensityMapAttachment: {
		stageMask = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
		accessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
		layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	} break;

	case ResourceState::FragmentShadingRateAttachment: {
		stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		accessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	} break;
	}

	ASSERT_MSG(stageMask != 0, "stageMask must never be 0 (we don't use synchronization2).");
	return SUCCESS;
}

Result ToVkBarrierSrc(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout)
{
	return ToVkBarrier(state, commandType, features, true, stageMask, accessMask, layout);
}

Result ToVkBarrierDst(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout)
{
	return ToVkBarrier(state, commandType, features, false, stageMask, accessMask, layout);
}

#if defined(_MSC_VER)
#	pragma warning(push, 3)
#	pragma warning(disable : 4061)
#endif

VkImageAspectFlags DetermineAspectMask(VkFormat format)
{
	switch (format) {
	// Depth
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT: {
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	} break;

	 // Stencil
	case VK_FORMAT_S8_UINT: {
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	} break;

	// Depth/stencil
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT: {
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	} break;

	// Assume everything else is color
	default: break;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

VmaMemoryUsage ToVmaMemoryUsage(MemoryUsage value)
{
	switch (value) {
	default: break;
	case MemoryUsage::GPUOnly: return VMA_MEMORY_USAGE_GPU_ONLY; break;
	case MemoryUsage::CPUOnly: return VMA_MEMORY_USAGE_CPU_ONLY; break;
	case MemoryUsage::CPUToGPU: return VMA_MEMORY_USAGE_CPU_TO_GPU; break;
	case MemoryUsage::GPUToCPU: return VMA_MEMORY_USAGE_GPU_TO_CPU; break;
	}
	return VMA_MEMORY_USAGE_UNKNOWN;
}

VkSamplerYcbcrModelConversion ToVkEnum(YcbcrModelConversion value)
{
	switch (value) {
	default: break;
	case YcbcrModelConversion::RGBIdentity: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY;
	case YcbcrModelConversion::YCBCRIdentity: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY;
	case YcbcrModelConversion::YCBCR709: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
	case YcbcrModelConversion::YCBCR601: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
	case YcbcrModelConversion::YCBCR2020: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020;
	}
	return InvalidValue<VkSamplerYcbcrModelConversion>();
}

VkSamplerYcbcrRange ToVkEnum(YcbcrRange value)
{
	switch (value) {
	default: break;
	case YcbcrRange::ITU_FULL: return VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
	case YcbcrRange::ITU_NARROW: return VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
	}
	return InvalidValue<VkSamplerYcbcrRange>();
}

#pragma endregion

#pragma region DeviceQueue

bool DeviceQueue::init(vkb::Device& vkbDevice, vkb::QueueType type)
{
	switch (type)
	{
	case vkb::QueueType::present:  CommandType = COMMAND_TYPE_PRESENT; break;
	case vkb::QueueType::graphics: CommandType = COMMAND_TYPE_GRAPHICS; break;
	case vkb::QueueType::compute:  CommandType = COMMAND_TYPE_COMPUTE; break;
	case vkb::QueueType::transfer: CommandType = COMMAND_TYPE_TRANSFER; break;
	}

	auto queueRet = vkbDevice.get_queue(type);
	if (!queueRet.has_value())
	{
		Fatal("failed to get queue: " + queueRet.error().message());
		return false;
	}
	Queue = queueRet.value();

	auto queueFamilyRet = vkbDevice.get_queue_index(type);
	if (!queueFamilyRet.has_value())
	{
		Fatal("failed to get queue index: " + queueFamilyRet.error().message());
		return false;
	}
	QueueFamily = queueFamilyRet.value();

	return true;
}

#pragma endregion

#pragma region TriMesh

TriMesh::TriMesh()
{
}

TriMesh::TriMesh(IndexType indexType)
	: mIndexType(indexType)
{
	// TODO: #514 - Remove assert when UINT8 is supported
	ASSERT_MSG(mIndexType != IndexType::Uint8, "IndexType::Uint8 unsupported in TriMesh");
}

TriMesh::TriMesh(TriMeshAttributeDim texCoordDim)
	: mTexCoordDim(texCoordDim)
{
}

TriMesh::TriMesh(IndexType indexType, TriMeshAttributeDim texCoordDim)
	: mIndexType(indexType), mTexCoordDim(texCoordDim)
{
	// TODO: #514 - Remove assert when UINT8 is supported
	ASSERT_MSG(mIndexType != IndexType::Uint8, "IndexType::Uint8 unsupported in TriMesh");
}

uint32_t TriMesh::GetCountTriangles() const
{
	uint32_t count = 0;
	if (mIndexType != IndexType::Undefined) {
		uint32_t elementSize = IndexTypeSize(mIndexType);
		uint32_t elementCount = CountU32(mIndices) / elementSize;
		count = elementCount / 3;
	}
	else {
		count = CountU32(mPositions) / 3;
	}
	return count;
}

uint32_t TriMesh::GetCountIndices() const
{
	uint32_t indexSize = IndexTypeSize(mIndexType);
	if (indexSize == 0) {
		return 0;
	}

	uint32_t count = CountU32(mIndices) / indexSize;
	return count;
}

uint32_t TriMesh::GetCountPositions() const
{
	uint32_t count = CountU32(mPositions);
	return count;
}

uint32_t TriMesh::GetCountColors() const
{
	uint32_t count = CountU32(mColors);
	return count;
}

uint32_t TriMesh::GetCountNormals() const
{
	uint32_t count = CountU32(mNormals);
	return count;
}

uint32_t TriMesh::GetCountTexCoords() const
{
	if (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_2) {
		uint32_t count = CountU32(mTexCoords) / 2;
		return count;
	}
	else if (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_3) {
		uint32_t count = CountU32(mTexCoords) / 3;
		return count;
	}
	else if (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_4) {
		uint32_t count = CountU32(mTexCoords) / 4;
		return count;
	}
	return 0;
}

uint32_t TriMesh::GetCountTangents() const
{
	uint32_t count = CountU32(mTangents);
	return count;
}

uint32_t TriMesh::GetCountBitangents() const
{
	uint32_t count = CountU32(mBitangents);
	return count;
}

uint64_t TriMesh::GetDataSizeIndices() const
{
	uint64_t size = static_cast<uint64_t>(mIndices.size());
	return size;
}

uint64_t TriMesh::GetDataSizePositions() const
{
	uint64_t size = static_cast<uint64_t>(mPositions.size() * sizeof(float3));
	return size;
}

uint64_t TriMesh::GetDataSizeColors() const
{
	uint64_t size = static_cast<uint64_t>(mColors.size() * sizeof(float3));
	return size;
}

uint64_t TriMesh::GetDataSizeNormalls() const
{
	uint64_t size = static_cast<uint64_t>(mNormals.size() * sizeof(float3));
	return size;
}

uint64_t TriMesh::GetDataSizeTexCoords() const
{
	uint64_t size = static_cast<uint64_t>(mTexCoords.size() * sizeof(float));
	return size;
}

uint64_t TriMesh::GetDataSizeTangents() const
{
	uint64_t size = static_cast<uint64_t>(mTangents.size() * sizeof(float3));
	return size;
}

uint64_t TriMesh::GetDataSizeBitangents() const
{
	uint64_t size = static_cast<uint64_t>(mBitangents.size() * sizeof(float3));
	return size;
}

const uint16_t* TriMesh::GetDataIndicesU16(uint32_t index) const
{
	if (mIndexType != IndexType::Uint16) {
		return nullptr;
	}
	uint32_t count = GetCountIndices();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(uint16_t) * index;
	const char* ptr = reinterpret_cast<const char*>(mIndices.data()) + offset;
	return reinterpret_cast<const uint16_t*>(ptr);
}

const uint32_t* TriMesh::GetDataIndicesU32(uint32_t index) const
{
	if (mIndexType != IndexType::Uint32) {
		return nullptr;
	}
	uint32_t count = GetCountIndices();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(uint32_t) * index;
	const char* ptr = reinterpret_cast<const char*>(mIndices.data()) + offset;
	return reinterpret_cast<const uint32_t*>(ptr);
}

const float3* TriMesh::GetDataPositions(uint32_t index) const
{
	if (index >= mPositions.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mPositions.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

const float3* TriMesh::GetDataColors(uint32_t index) const
{
	if (index >= mColors.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mColors.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

const float3* TriMesh::GetDataNormalls(uint32_t index) const
{
	if (index >= mNormals.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mNormals.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

const float2* TriMesh::GetDataTexCoords2(uint32_t index) const
{
	if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_2) {
		return nullptr;
	}
	uint32_t count = GetCountTexCoords();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(float2) * index;
	const char* ptr = reinterpret_cast<const char*>(mTexCoords.data()) + offset;
	return reinterpret_cast<const float2*>(ptr);
}

const float3* TriMesh::GetDataTexCoords3(uint32_t index) const
{
	if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_3) {
		return nullptr;
	}
	uint32_t count = GetCountTexCoords();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mTexCoords.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

const float4* TriMesh::GetDataTexCoords4(uint32_t index) const
{
	if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_4) {
		return nullptr;
	}
	uint32_t count = GetCountTexCoords();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(float4) * index;
	const char* ptr = reinterpret_cast<const char*>(mTexCoords.data()) + offset;
	return reinterpret_cast<const float4*>(ptr);
}

const float4* TriMesh::GetDataTangents(uint32_t index) const
{
	if (index >= mTangents.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float4) * index;
	const char* ptr = reinterpret_cast<const char*>(mTangents.data()) + offset;
	return reinterpret_cast<const float4*>(ptr);
}

const float3* TriMesh::GetDataBitangents(uint32_t index) const
{
	if (index >= mBitangents.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mBitangents.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

void TriMesh::AppendIndexU16(uint16_t value)
{
	const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
	mIndices.push_back(pBytes[0]);
	mIndices.push_back(pBytes[1]);
}

void TriMesh::AppendIndexU32(uint32_t value)
{
	const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
	mIndices.push_back(pBytes[0]);
	mIndices.push_back(pBytes[1]);
	mIndices.push_back(pBytes[2]);
	mIndices.push_back(pBytes[3]);
}

void TriMesh::PreallocateForTriangleCount(size_t triangleCount, bool enableColors, bool enableNormals, bool enableTexCoords, bool enableTangents)
{
	size_t vertexCount = triangleCount * 3;

	// Reserve for triangles
	switch (mIndexType)
	{
	case IndexType::Uint8:
		mIndices.reserve(vertexCount * sizeof(uint8_t));
		break;
	case IndexType::Uint16:
		mIndices.reserve(vertexCount * sizeof(uint16_t));
		break;
	case IndexType::Uint32:
		mIndices.reserve(vertexCount * sizeof(uint32_t));
		break;
	case IndexType::Undefined:
	default: return;
	}

	// Position per vertex
	mPositions.reserve(vertexCount);
	// Color per vertex
	if (enableColors) mColors.reserve(vertexCount);
	// Normal per vertex
	if (enableNormals) mNormals.reserve(vertexCount);

	// TexCoord per vertex
	if (enableTexCoords)
	{
		int32_t dimCount = (mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_4) ? 4 : ((mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_3) ? 3 : ((mTexCoordDim == TRI_MESH_ATTRIBUTE_DIM_2) ? 2 : 0));
		mTexCoords.reserve(vertexCount * dimCount);
	}
	// Tangents = 3 per triangle (NOT necessarily related to vertices!)
	if (enableTangents)
	{
		mTangents.reserve(triangleCount * 3);
		mBitangents.reserve(triangleCount * 3);
	}
}

uint32_t TriMesh::AppendTriangle(uint32_t v0, uint32_t v1, uint32_t v2)
{
	if (mIndexType == IndexType::Uint16)
	{
		mIndices.reserve(mIndices.size() + 3 * sizeof(uint16_t));
		AppendIndexU16(static_cast<uint16_t>(v0));
		AppendIndexU16(static_cast<uint16_t>(v1));
		AppendIndexU16(static_cast<uint16_t>(v2));
	}
	else if (mIndexType == IndexType::Uint32)
	{
		mIndices.reserve(mIndices.size() + 3 * sizeof(uint32_t));
		AppendIndexU32(v0);
		AppendIndexU32(v1);
		AppendIndexU32(v2);
	}
	else
	{
		Fatal("unknown index type");
		return 0;
	}
	uint32_t count = GetCountTriangles();
	return count;
}

uint32_t TriMesh::AppendPosition(const float3& value)
{
	mPositions.push_back(value);
	// Update bounding box
	uint32_t count = GetCountPositions();
	if (count > 1) {
		mBoundingBoxMin.x = std::min<float>(mBoundingBoxMin.x, value.x);
		mBoundingBoxMin.y = std::min<float>(mBoundingBoxMin.y, value.y);
		mBoundingBoxMin.z = std::min<float>(mBoundingBoxMin.z, value.z);
		mBoundingBoxMax.x = std::max<float>(mBoundingBoxMax.x, value.x);
		mBoundingBoxMax.y = std::max<float>(mBoundingBoxMax.y, value.y);
		mBoundingBoxMax.z = std::max<float>(mBoundingBoxMax.z, value.z);
	}
	else {
		mBoundingBoxMin = mBoundingBoxMax = value;
	}
	return count;
}

uint32_t TriMesh::AppendColor(const float3& value)
{
	mColors.push_back(value);
	uint32_t count = GetCountColors();
	return count;
}

uint32_t TriMesh::AppendTexCoord(const float2& value)
{
	if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_2)
	{
		Fatal("unknown tex coord dim");
		return 0;
	}
	mTexCoords.reserve(mTexCoords.size() + 2);
	mTexCoords.push_back(value.x);
	mTexCoords.push_back(value.y);
	uint32_t count = GetCountTexCoords();
	return count;
}

uint32_t TriMesh::AppendTexCoord(const float3& value)
{
	if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_3)
	{
		Fatal("unknown tex coord dim");
		return 0;
	}
	mTexCoords.reserve(mTexCoords.size() + 3);
	mTexCoords.push_back(value.x);
	mTexCoords.push_back(value.y);
	mTexCoords.push_back(value.z);
	uint32_t count = GetCountTexCoords();
	return count;
}

uint32_t TriMesh::AppendTexCoord(const float4& value)
{
	if (mTexCoordDim != TRI_MESH_ATTRIBUTE_DIM_4)
	{
		Fatal("unknown tex coord dim");
		return 0;
	}
	mTexCoords.reserve(mTexCoords.size() + 3);
	mTexCoords.push_back(value.x);
	mTexCoords.push_back(value.y);
	mTexCoords.push_back(value.z);
	mTexCoords.push_back(value.w);
	uint32_t count = GetCountTexCoords();
	return count;
}

uint32_t TriMesh::AppendNormal(const float3& value)
{
	mNormals.push_back(value);
	uint32_t count = GetCountNormals();
	return count;
}

uint32_t TriMesh::AppendTangent(const float4& value)
{
	mTangents.push_back(value);
	uint32_t count = GetCountTangents();
	return count;
}

uint32_t TriMesh::AppendBitangent(const float3& value)
{
	mBitangents.push_back(value);
	uint32_t count = GetCountBitangents();
	return count;
}

Result TriMesh::GetTriangle(uint32_t triIndex, uint32_t& v0, uint32_t& v1, uint32_t& v2) const
{
	if (mIndexType == IndexType::Undefined) return ERROR_GEOMETRY_NO_INDEX_DATA;

	uint32_t triCount = GetCountTriangles();
	if (triIndex >= triCount) return ERROR_OUT_OF_RANGE;

	const uint8_t* pData = mIndices.data();
	uint32_t elementSize = IndexTypeSize(mIndexType);

	if (mIndexType == IndexType::Uint16)
	{
		size_t          offset = 3 * triIndex * elementSize;
		const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(pData + offset);
		v0 = static_cast<uint32_t>(pIndexData[0]);
		v1 = static_cast<uint32_t>(pIndexData[1]);
		v2 = static_cast<uint32_t>(pIndexData[2]);
	}
	else if (mIndexType == IndexType::Uint32)
	{
		size_t          offset = 3 * triIndex * elementSize;
		const uint32_t* pIndexData = reinterpret_cast<const uint32_t*>(pData + offset);
		v0 = static_cast<uint32_t>(pIndexData[0]);
		v1 = static_cast<uint32_t>(pIndexData[1]);
		v2 = static_cast<uint32_t>(pIndexData[2]);
	}

	return SUCCESS;
}

Result TriMesh::GetVertexData(uint32_t vtxIndex, TriMeshVertexData* pVertexData) const
{
	uint32_t vertexCount = GetCountPositions();
	if (vtxIndex >= vertexCount) return ERROR_OUT_OF_RANGE;

	const float3* pPosition = GetDataPositions(vtxIndex);
	const float3* pColor = GetDataColors(vtxIndex);
	const float3* pNormal = GetDataNormalls(vtxIndex);
	const float2* pTexCoord2 = GetDataTexCoords2(vtxIndex);
	const float4* pTangent = GetDataTangents(vtxIndex);
	const float3* pBitangent = GetDataBitangents(vtxIndex);

	pVertexData->position = *pPosition;

	if (!IsNull(pColor)) {
		pVertexData->color = *pColor;
	}

	if (!IsNull(pNormal)) {
		pVertexData->normal = *pNormal;
	}

	if (!IsNull(pTexCoord2)) {
		pVertexData->texCoord = *pTexCoord2;
	}

	if (!IsNull(pTangent)) {
		pVertexData->tangent = *pTangent;
	}
	if (!IsNull(pBitangent)) {
		pVertexData->bitangent = *pBitangent;
	}

	return SUCCESS;
}

void TriMesh::AppendIndexAndVertexData(
	std::vector<uint32_t>& indexData,
	const std::vector<float>& vertexData,
	const uint32_t            expectedVertexCount,
	const TriMeshOptions& options,
	TriMesh& mesh)
{
	IndexType indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;

	// Verify expected vertex count
	size_t vertexCount = (vertexData.size() * sizeof(float)) / sizeof(TriMeshVertexData);
	ASSERT_MSG(vertexCount == expectedVertexCount, "unexpected vertex count");

	// Get base pointer to vertex data
	const char* pData = reinterpret_cast<const char*>(vertexData.data());

	if (indexType != IndexType::Undefined) {
		for (size_t i = 0; i < vertexCount; ++i) {
			const TriMeshVertexData* pVertexData = reinterpret_cast<const TriMeshVertexData*>(pData + (i * sizeof(TriMeshVertexData)));

			mesh.AppendPosition(pVertexData->position * options.mScale);

			if (options.mEnableVertexColors || options.mEnableObjectColor) {
				float3 color = options.mEnableObjectColor ? options.mObjectColor : pVertexData->color;
				mesh.AppendColor(color);
			}

			if (options.mEnableNormals) {
				mesh.AppendNormal(pVertexData->normal);
			}

			if (options.mEnableTexCoords) {
				mesh.AppendTexCoord(pVertexData->texCoord * options.mTexCoordScale);
			}

			if (options.mEnableTangents) {
				mesh.AppendTangent(pVertexData->tangent);
				mesh.AppendBitangent(pVertexData->bitangent);
			}
		}

		size_t triCount = indexData.size() / 3;
		for (size_t triIndex = 0; triIndex < triCount; ++triIndex) {
			uint32_t v0 = indexData[3 * triIndex + 0];
			uint32_t v1 = indexData[3 * triIndex + 1];
			uint32_t v2 = indexData[3 * triIndex + 2];
			mesh.AppendTriangle(v0, v1, v2);
		}
	}
	else {
		for (size_t i = 0; i < indexData.size(); ++i) {
			uint32_t                 vi = indexData[i];
			const TriMeshVertexData* pVertexData = reinterpret_cast<const TriMeshVertexData*>(pData + (vi * sizeof(TriMeshVertexData)));

			mesh.AppendPosition(pVertexData->position * options.mScale);

			if (options.mEnableVertexColors) {
				mesh.AppendColor(pVertexData->color);
			}

			if (options.mEnableNormals) {
				mesh.AppendNormal(pVertexData->normal);
			}

			if (options.mEnableTexCoords) {
				mesh.AppendTexCoord(pVertexData->texCoord);
			}

			if (options.mEnableTangents) {
				mesh.AppendTangent(pVertexData->tangent);
				mesh.AppendBitangent(pVertexData->bitangent);
			}
		}
	}
}

TriMesh TriMesh::CreatePlane(TriMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options)
{
	const float    hs = size.x / 2.0f;
	const float    ht = size.y / 2.0f;
	const float    ds = size.x / static_cast<float>(usegs);
	const float    dt = size.y / static_cast<float>(vsegs);
	const uint32_t uverts = usegs + 1;
	const uint32_t vverts = vsegs + 1;

	std::vector<float> vertexData;
	for (uint32_t j = 0; j < vverts; ++j)
	{
		for (uint32_t i = 0; i < uverts; ++i)
		{
			float s = static_cast<float>(i) * ds / size.x;
			float t = static_cast<float>(j) * dt / size.y;
			float u = options.mTexCoordScale.x * s;
			float v = options.mTexCoordScale.y * t;

			// float3 position  = float3(s - hx, 0, t - hz);
			float3 position = float3(0);
			switch (plane)
			{
			case TRI_MESH_PLANE_POSITIVE_X:
			case TRI_MESH_PLANE_NEGATIVE_X:
			case TRI_MESH_PLANE_POSITIVE_Z:
			case TRI_MESH_PLANE_NEGATIVE_Z:
			default: {
				Fatal("unknown plane orientation");
			} break;

			case TRI_MESH_PLANE_POSITIVE_Y: {
				position = float3(s * size.x - hs, 0, t * size.y - ht);
			} break;

			case TRI_MESH_PLANE_NEGATIVE_Y: {
				position = float3((1.0f - s) * size.x - hs, 0, (1.0f - t) * size.y - ht);
			} break;
			}

			float3 color = float3(u, v, 0);
			float3 normal = float3(0, 1, 0);
			float2 texcoord = float2(u, v);
			float4 tangent = float4(0.0f, 0.0f, 0.0f, 1.0f);
			float3 bitangent = glm::cross(normal, float3(tangent));

			vertexData.push_back(position.x);
			vertexData.push_back(position.y);
			vertexData.push_back(position.z);
			vertexData.push_back(color.r);
			vertexData.push_back(color.g);
			vertexData.push_back(color.b);
			vertexData.push_back(normal.x);
			vertexData.push_back(normal.y);
			vertexData.push_back(normal.z);
			vertexData.push_back(texcoord.x);
			vertexData.push_back(texcoord.y);
			vertexData.push_back(tangent.x);
			vertexData.push_back(tangent.y);
			vertexData.push_back(tangent.z);
			vertexData.push_back(tangent.w);
			vertexData.push_back(bitangent.x);
			vertexData.push_back(bitangent.y);
			vertexData.push_back(bitangent.z);
		}
	}

	std::vector<uint32_t> indexData;
	for (uint32_t i = 1; i < uverts; ++i)
	{
		for (uint32_t j = 1; j < vverts; ++j)
		{
			uint32_t i0 = i - 1;
			uint32_t i1 = i;
			uint32_t j0 = j - 1;
			uint32_t j1 = j;
			uint32_t v0 = i1 * vverts + j0;
			uint32_t v1 = i1 * vverts + j1;
			uint32_t v2 = i0 * vverts + j1;
			uint32_t v3 = i0 * vverts + j0;

			switch (plane)
			{
			case TRI_MESH_PLANE_POSITIVE_X:
			case TRI_MESH_PLANE_NEGATIVE_X:
			case TRI_MESH_PLANE_POSITIVE_Z:
			case TRI_MESH_PLANE_NEGATIVE_Z:
			default: {
				Fatal("unknown plane orientation");
			} break;

			case TRI_MESH_PLANE_POSITIVE_Y: {
				indexData.push_back(v0);
				indexData.push_back(v1);
				indexData.push_back(v2);

				indexData.push_back(v0);
				indexData.push_back(v2);
				indexData.push_back(v3);
			} break;

			case TRI_MESH_PLANE_NEGATIVE_Y: {
				indexData.push_back(v0);
				indexData.push_back(v2);
				indexData.push_back(v1);

				indexData.push_back(v0);
				indexData.push_back(v3);
				indexData.push_back(v2);
			} break;
			}
		}
	}

	IndexType     indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
	TriMesh             mesh = TriMesh(indexType, texCoordDim);

	uint32_t expectedVertexCount = uverts * vverts;
	AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

	return mesh;

	/*
	const float hx = size.x / 2.0f;
	const float hz = size.y / 2.0f;
	std::vector<float> vertexData = {
		// position       // vertex color     // normal           // texcoord   // tangent                // bitangent
		-hx, 0.0f, -hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
		-hx, 0.0f,  hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
		 hx, 0.0f,  hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
		 hx, 0.0f, -hz,   0.7f, 0.7f, 0.7f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
	};

	std::vector<uint32_t> indexData = {
		0, 1, 2,
		0, 2, 3
	};

	IndexType     indexType   = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
	TriMesh             mesh        = TriMesh(indexType, texCoordDim);

	AppendIndexAndVertexData(indexData, vertexData, 4, options, mesh);

	return mesh;
	*/
}

TriMesh TriMesh::CreateCube(const float3& size, const TriMeshOptions& options)
{
	const float hx = size.x / 2.0f;
	const float hy = size.y / 2.0f;
	const float hz = size.z / 2.0f;

	std::vector<float> vertexData = {
		// position      // vertex colors    // normal           // texcoords   // tangents               // bitangents
		 hx,  hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  0  -Z side
		 hx, -hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  1
		-hx, -hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  2
		-hx,  hy, -hz,    1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,  -1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  3

		-hx,  hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  4  +Z side
		-hx, -hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  5
		 hx, -hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  6
		 hx,  hy,  hz,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  7

		-hx,  hy, -hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  8  -X side
		-hx, -hy, -hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  //  9
		-hx, -hy,  hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 10
		-hx,  hy,  hz,   -0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 11

		 hx,  hy,  hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 12  +X side
		 hx, -hy,  hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 13
		 hx, -hy, -hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 14
		 hx,  hy, -hz,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f,-1.0f, 1.0f,   0.0f,-1.0f, 0.0f,  // 15

		-hx, -hy,  hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 16  -Y side
		-hx, -hy, -hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 17
		 hx, -hy, -hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 18
		 hx, -hy,  hz,    1.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,-1.0f,  // 19

		-hx,  hy, -hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 20  +Y side
		-hx,  hy,  hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 21
		 hx,  hy,  hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 22
		 hx,  hy, -hz,    0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 23
	};

	std::vector<uint32_t> indexData = {
		0,  1,  2, // -Z side
		0,  2,  3,

		4,  5,  6, // +Z side
		4,  6,  7,

		8,  9, 10, // -X side
		8, 10, 11,

		12, 13, 14, // +X side
		12, 14, 15,

		16, 17, 18, // -X side
		16, 18, 19,

		20, 21, 22, // +X side
		20, 22, 23,
	};

	IndexType     indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
	TriMesh             mesh = TriMesh(indexType, texCoordDim);

	AppendIndexAndVertexData(indexData, vertexData, 24, options, mesh);

	return mesh;
}

TriMesh TriMesh::CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options)
{
	constexpr float kPi = glm::pi<float>();
	constexpr float kTwoPi = 2.0f * kPi;

	const uint32_t uverts = usegs + 1;
	const uint32_t vverts = vsegs + 1;

	float dt = kTwoPi / static_cast<float>(usegs);
	float dp = kPi / static_cast<float>(vsegs);

	std::vector<float> vertexData;
	for (uint32_t i = 0; i < uverts; ++i)
	{
		for (uint32_t j = 0; j < vverts; ++j)
		{
			float theta = static_cast<float>(i) * dt;
			float phi = static_cast<float>(j) * dp;
			float u = options.mTexCoordScale.x * theta / kTwoPi;
			float v = options.mTexCoordScale.y * phi / kPi;
			float3 P = SphericalToCartesian(theta, phi);
			float3 position = radius * P;
			float3 color = float3(u, v, 0);
			float3 normal = normalize(position);
			float2 texcoord = float2(u, v);
			float4 tangent = float4(-SphericalTangent(theta, phi), 1.0);
			float3 bitangent = glm::cross(normal, float3(tangent));

			vertexData.push_back(position.x);
			vertexData.push_back(position.y);
			vertexData.push_back(position.z);
			vertexData.push_back(color.r);
			vertexData.push_back(color.g);
			vertexData.push_back(color.b);
			vertexData.push_back(normal.x);
			vertexData.push_back(normal.y);
			vertexData.push_back(normal.z);
			vertexData.push_back(texcoord.x);
			vertexData.push_back(texcoord.y);
			vertexData.push_back(tangent.x);
			vertexData.push_back(tangent.y);
			vertexData.push_back(tangent.z);
			vertexData.push_back(tangent.w);
			vertexData.push_back(bitangent.x);
			vertexData.push_back(bitangent.y);
			vertexData.push_back(bitangent.z);
		}
	}

	std::vector<uint32_t> indexData;
	for (uint32_t i = 1; i < uverts; ++i) {
		for (uint32_t j = 1; j < vverts; ++j) {
			uint32_t i0 = i - 1;
			uint32_t i1 = i;
			uint32_t j0 = j - 1;
			uint32_t j1 = j;
			uint32_t v0 = i1 * vverts + j0;
			uint32_t v1 = i1 * vverts + j1;
			uint32_t v2 = i0 * vverts + j1;
			uint32_t v3 = i0 * vverts + j0;

			indexData.push_back(v0);
			indexData.push_back(v1);
			indexData.push_back(v2);

			indexData.push_back(v0);
			indexData.push_back(v2);
			indexData.push_back(v3);
		}
	}

	IndexType     indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
	TriMesh             mesh = TriMesh(indexType, texCoordDim);

	uint32_t expectedVertexCount = uverts * vverts;
	AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

	return mesh;
}

Result TriMesh::CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options, TriMesh* pTriMesh)
{
	if (IsNull(pTriMesh)) return ERROR_UNEXPECTED_NULL_ARGUMENT;

	/*Timer timer; // TODO: замер времени
	ASSERT_MSG(timer.Start() == TIMER_RESULT_SUCCESS, "timer start failed");
	double fnStartTime = timer.SecondsSinceStart();*/

	// Determine index type and tex coord dim
	IndexType indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	TriMeshAttributeDim texCoordDim = options.mEnableTexCoords ? TRI_MESH_ATTRIBUTE_DIM_2 : TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;

	// Create new mesh
	*pTriMesh = TriMesh(indexType, texCoordDim);

	const std::vector<float3> colors = {
		{1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{1.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 1.0f},
		{0.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
	};

	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;

	FileStream objStream;
	if (!objStream.Open(path.string().c_str())) return ERROR_GEOMETRY_FILE_LOAD_FAILED;

	std::string warn;
	std::string err;
	std::istream istr(&objStream);
	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &istr, nullptr, true);
	if (!loaded || !err.empty()) return ERROR_GEOMETRY_FILE_LOAD_FAILED;

	size_t numShapes = shapes.size();
	if (numShapes == 0) return ERROR_GEOMETRY_FILE_NO_DATA;

	// Preallocate based on the total number of triangles.
	size_t totalTriangles = 0;
	for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx)
	{
		totalTriangles += shapes[shapeIdx].mesh.indices.size() / 3;
	}
	pTriMesh->PreallocateForTriangleCount(totalTriangles,
		/* enableColors= */ (options.mEnableVertexColors || options.mEnableObjectColor),
		options.mEnableNormals,
		options.mEnableTexCoords,
		options.mEnableTangents);

	// Build geometry
	for (size_t shapeIdx = 0; shapeIdx < numShapes; ++shapeIdx)
	{
		const tinyobj::shape_t& shape = shapes[shapeIdx];
		const tinyobj::mesh_t& shapeMesh = shape.mesh;

		size_t numTriangles = shapeMesh.indices.size() / 3;
		for (size_t triIdx = 0; triIdx < numTriangles; ++triIdx)
		{
			size_t triVtxIdx0 = triIdx * 3 + 0;
			size_t triVtxIdx1 = triIdx * 3 + 1;
			size_t triVtxIdx2 = triIdx * 3 + 2;

			// Index data
			const tinyobj::index_t& dataIdx0 = shapeMesh.indices[triVtxIdx0];
			const tinyobj::index_t& dataIdx1 = shapeMesh.indices[triVtxIdx1];
			const tinyobj::index_t& dataIdx2 = shapeMesh.indices[triVtxIdx2];

			// Vertex data
			TriMeshVertexData vtx0 = {};
			TriMeshVertexData vtx1 = {};
			TriMeshVertexData vtx2 = {};

			// Pick a face color
			float3 faceColor = colors[triIdx % colors.size()];
			vtx0.color = faceColor;
			vtx1.color = faceColor;
			vtx2.color = faceColor;

			// Vertex positions
			{
				int i0 = 3 * dataIdx0.vertex_index + 0;
				int i1 = 3 * dataIdx0.vertex_index + 1;
				int i2 = 3 * dataIdx0.vertex_index + 2;
				vtx0.position = float3(attrib.vertices[static_cast<size_t>(i0)], attrib.vertices[static_cast<size_t>(i1)], attrib.vertices[static_cast<size_t>(i2)]);

				i0 = 3 * dataIdx1.vertex_index + 0;
				i1 = 3 * dataIdx1.vertex_index + 1;
				i2 = 3 * dataIdx1.vertex_index + 2;
				vtx1.position = float3(attrib.vertices[static_cast<size_t>(i0)], attrib.vertices[static_cast<size_t>(i1)], attrib.vertices[static_cast<size_t>(i2)]);

				i0 = 3 * dataIdx2.vertex_index + 0;
				i1 = 3 * dataIdx2.vertex_index + 1;
				i2 = 3 * dataIdx2.vertex_index + 2;
				vtx2.position = float3(attrib.vertices[static_cast<size_t>(i0)], attrib.vertices[static_cast<size_t>(i1)], attrib.vertices[static_cast<size_t>(i2)]);
			}

			// Normals
			if ((dataIdx0.normal_index != -1) && (dataIdx1.normal_index != -1) && (dataIdx2.normal_index != -1))
			{
				int i0 = 3 * dataIdx0.normal_index + 0;
				int i1 = 3 * dataIdx0.normal_index + 1;
				int i2 = 3 * dataIdx0.normal_index + 2;
				vtx0.normal = float3(attrib.normals[static_cast<size_t>(i0)], attrib.normals[static_cast<size_t>(i1)], attrib.normals[static_cast<size_t>(i2)]);

				i0 = 3 * dataIdx1.normal_index + 0;
				i1 = 3 * dataIdx1.normal_index + 1;
				i2 = 3 * dataIdx1.normal_index + 2;
				vtx1.normal = float3(attrib.normals[static_cast<size_t>(i0)], attrib.normals[static_cast<size_t>(i1)], attrib.normals[static_cast<size_t>(i2)]);

				i0 = 3 * dataIdx2.normal_index + 0;
				i1 = 3 * dataIdx2.normal_index + 1;
				i2 = 3 * dataIdx2.normal_index + 2;
				vtx2.normal = float3(attrib.normals[static_cast<size_t>(i0)], attrib.normals[static_cast<size_t>(i1)], attrib.normals[static_cast<size_t>(i2)]);
			}

			// Texture coordinates
			if ((dataIdx0.texcoord_index != -1) && (dataIdx1.texcoord_index != -1) && (dataIdx2.texcoord_index != -1)) {
				int i0 = 2 * dataIdx0.texcoord_index + 0;
				int i1 = 2 * dataIdx0.texcoord_index + 1;
				vtx0.texCoord = float2(attrib.texcoords[static_cast<size_t>(i0)], attrib.texcoords[static_cast<size_t>(i1)]);

				i0 = 2 * dataIdx1.texcoord_index + 0;
				i1 = 2 * dataIdx1.texcoord_index + 1;
				vtx1.texCoord = float2(attrib.texcoords[static_cast<size_t>(i0)], attrib.texcoords[static_cast<size_t>(i1)]);

				i0 = 2 * dataIdx2.texcoord_index + 0;
				i1 = 2 * dataIdx2.texcoord_index + 1;
				vtx2.texCoord = float2(attrib.texcoords[static_cast<size_t>(i0)], attrib.texcoords[static_cast<size_t>(i1)]);

				// Scale tex coords
				vtx0.texCoord *= options.mTexCoordScale;
				vtx1.texCoord *= options.mTexCoordScale;
				vtx2.texCoord *= options.mTexCoordScale;

				if (options.mInvertTexCoordsV) {
					vtx0.texCoord.y = 1.0f - vtx0.texCoord.y;
					vtx1.texCoord.y = 1.0f - vtx1.texCoord.y;
					vtx2.texCoord.y = 1.0f - vtx2.texCoord.y;
				}
			}

			float3 pos0 = (vtx0.position * options.mScale) + options.mTranslate;
			float3 pos1 = (vtx1.position * options.mScale) + options.mTranslate;
			float3 pos2 = (vtx2.position * options.mScale) + options.mTranslate;

			uint32_t triVtx0 = pTriMesh->AppendPosition(pos0) - 1;
			uint32_t triVtx1 = pTriMesh->AppendPosition(pos1) - 1;
			uint32_t triVtx2 = pTriMesh->AppendPosition(pos2) - 1;

			if (options.mEnableVertexColors || options.mEnableObjectColor)
			{
				if (options.mEnableObjectColor)
				{
					vtx0.color = options.mObjectColor;
					vtx1.color = options.mObjectColor;
					vtx2.color = options.mObjectColor;
				}
				pTriMesh->AppendColor(vtx0.color);
				pTriMesh->AppendColor(vtx1.color);
				pTriMesh->AppendColor(vtx2.color);
			}

			if (options.mEnableNormals)
			{
				pTriMesh->AppendNormal(vtx0.normal);
				pTriMesh->AppendNormal(vtx1.normal);
				pTriMesh->AppendNormal(vtx2.normal);
			}

			if (options.mEnableTexCoords)
			{
				pTriMesh->AppendTexCoord(vtx0.texCoord);
				pTriMesh->AppendTexCoord(vtx1.texCoord);
				pTriMesh->AppendTexCoord(vtx2.texCoord);
			}

			if (options.mEnableTangents)
			{
				float3 edge1 = vtx1.position - vtx0.position;
				float3 edge2 = vtx2.position - vtx0.position;
				float2 duv1 = vtx1.texCoord - vtx0.texCoord;
				float2 duv2 = vtx2.texCoord - vtx0.texCoord;
				float  r = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x);

				float3 tangent = float3(
					((edge1.x * duv2.y) - (edge2.x * duv1.y)) * r,
					((edge1.y * duv2.y) - (edge2.y * duv1.y)) * r,
					((edge1.z * duv2.y) - (edge2.z * duv1.y)) * r);

				float3 bitangent = float3(
					((edge1.x * duv2.x) - (edge2.x * duv1.x)) * r,
					((edge1.y * duv2.x) - (edge2.y * duv1.x)) * r,
					((edge1.z * duv2.x) - (edge2.z * duv1.x)) * r);

				tangent = glm::normalize(tangent - vtx0.normal * glm::dot(vtx0.normal, tangent));
				float w = 1.0f;

				pTriMesh->AppendTangent(float4(-tangent, w));
				pTriMesh->AppendTangent(float4(-tangent, w));
				pTriMesh->AppendTangent(float4(-tangent, w));
				pTriMesh->AppendBitangent(-bitangent);
				pTriMesh->AppendBitangent(-bitangent);
				pTriMesh->AppendBitangent(-bitangent);
			}

			if (indexType != IndexType::Undefined)
			{
				if (options.mInvertWinding)
				{
					pTriMesh->AppendTriangle(triVtx0, triVtx2, triVtx1);
				}
				else {
					pTriMesh->AppendTriangle(triVtx0, triVtx1, triVtx2);
				}
			}
		}
	}

	//double fnEndTime = timer.SecondsSinceStart();
	//float  fnElapsed = static_cast<float>(fnEndTime - fnStartTime);
	//Print("Created mesh from OBJ file: " + path + " (" + FloatString(fnElapsed) + " seconds, " + numShapes + " shapes, " + totalTriangles + " triangles)");

	return SUCCESS;
}

TriMesh TriMesh::CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options)
{
	TriMesh mesh;
	CHECKED_CALL(CreateFromOBJ(path, options, &mesh));
	return mesh;
}

#pragma endregion

#pragma region WireMesh

WireMesh::WireMesh(IndexType indexType)
	: mIndexType(indexType)
{
	// TODO: #514 - Remove assert when UINT8 is supported
	ASSERT_MSG(mIndexType != IndexType::Uint8, "IndexType::Uint8 unsupported in WireMesh");
}

uint32_t WireMesh::GetCountEdges() const
{
	uint32_t count = 0;
	if (mIndexType != IndexType::Undefined) {
		uint32_t elementSize = IndexTypeSize(mIndexType);
		uint32_t elementCount = CountU32(mIndices) / elementSize;
		count = elementCount / 2;
	}
	else {
		count = CountU32(mPositions) / 2;
	}
	return count;
}

uint32_t WireMesh::GetCountIndices() const
{
	uint32_t indexSize = IndexTypeSize(mIndexType);
	if (indexSize == 0) {
		return 0;
	}

	uint32_t count = CountU32(mIndices) / indexSize;
	return count;
}

uint32_t WireMesh::GetCountPositions() const
{
	uint32_t count = CountU32(mPositions);
	return count;
}

uint32_t WireMesh::GetCountColors() const
{
	uint32_t count = CountU32(mColors);
	return count;
}

uint64_t WireMesh::GetDataSizeIndices() const
{
	uint64_t size = static_cast<uint64_t>(mIndices.size());
	return size;
}

uint64_t WireMesh::GetDataSizePositions() const
{
	uint64_t size = static_cast<uint64_t>(mPositions.size() * sizeof(float3));
	return size;
}

uint64_t WireMesh::GetDataSizeColors() const
{
	uint64_t size = static_cast<uint64_t>(mColors.size() * sizeof(float3));
	return size;
}

const uint16_t* WireMesh::GetDataIndicesU16(uint32_t index) const
{
	if (mIndexType != IndexType::Uint16) {
		return nullptr;
	}
	uint32_t count = GetCountIndices();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(uint16_t) * index;
	const char* ptr = reinterpret_cast<const char*>(mIndices.data()) + offset;
	return reinterpret_cast<const uint16_t*>(ptr);
}

const uint32_t* WireMesh::GetDataIndicesU32(uint32_t index) const
{
	if (mIndexType != IndexType::Uint32) {
		return nullptr;
	}
	uint32_t count = GetCountIndices();
	if (index >= count) {
		return nullptr;
	}
	size_t      offset = sizeof(uint32_t) * index;
	const char* ptr = reinterpret_cast<const char*>(mIndices.data()) + offset;
	return reinterpret_cast<const uint32_t*>(ptr);
}

const float3* WireMesh::GetDataPositions(uint32_t index) const
{
	if (index >= mPositions.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mPositions.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

const float3* WireMesh::GetDataColors(uint32_t index) const
{
	if (index >= mColors.size()) {
		return nullptr;
	}
	size_t      offset = sizeof(float3) * index;
	const char* ptr = reinterpret_cast<const char*>(mColors.data()) + offset;
	return reinterpret_cast<const float3*>(ptr);
}

void WireMesh::AppendIndexU16(uint16_t value)
{
	const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
	mIndices.push_back(pBytes[0]);
	mIndices.push_back(pBytes[1]);
}

void WireMesh::AppendIndexU32(uint32_t value)
{
	const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(&value);
	mIndices.push_back(pBytes[0]);
	mIndices.push_back(pBytes[1]);
	mIndices.push_back(pBytes[2]);
	mIndices.push_back(pBytes[3]);
}

uint32_t WireMesh::AppendEdge(uint32_t v0, uint32_t v1)
{
	if (mIndexType == IndexType::Uint16)
	{
		ASSERT_MSG(v0 <= UINT16_MAX, "v0 is out of range for index type UINT16");
		ASSERT_MSG(v1 <= UINT16_MAX, "v1 is out of range for index type UINT16");
		mIndices.reserve(mIndices.size() + 2 * sizeof(uint16_t));
		AppendIndexU16(static_cast<uint16_t>(v0));
		AppendIndexU16(static_cast<uint16_t>(v1));
	}
	else if (mIndexType == IndexType::Uint32)
	{
		mIndices.reserve(mIndices.size() + 2 * sizeof(uint32_t));
		AppendIndexU32(v0);
		AppendIndexU32(v1);
	}
	else
	{
		Fatal("unknown index type");
		return 0;
	}
	uint32_t count = GetCountEdges();
	return count;
}

uint32_t WireMesh::AppendPosition(const float3& value)
{
	mPositions.push_back(value);
	uint32_t count = GetCountPositions();
	if (count > 0) {
		mBoundingBoxMin.x = std::min<float>(mBoundingBoxMin.x, value.x);
		mBoundingBoxMin.y = std::min<float>(mBoundingBoxMin.y, value.y);
		mBoundingBoxMin.z = std::min<float>(mBoundingBoxMin.z, value.z);
		mBoundingBoxMax.x = std::min<float>(mBoundingBoxMax.x, value.x);
		mBoundingBoxMax.y = std::min<float>(mBoundingBoxMax.y, value.y);
		mBoundingBoxMax.z = std::min<float>(mBoundingBoxMax.z, value.z);
	}
	else {
		mBoundingBoxMin = mBoundingBoxMax = value;
	}
	return count;
}

uint32_t WireMesh::AppendColor(const float3& value)
{
	mColors.push_back(value);
	uint32_t count = GetCountColors();
	return count;
}

Result WireMesh::GetEdge(uint32_t triIndex, uint32_t& v0, uint32_t& v1) const
{
	if (mIndexType == IndexType::Undefined) {
		return ERROR_GEOMETRY_NO_INDEX_DATA;
	}

	uint32_t triCount = GetCountEdges();
	if (triIndex >= triCount) {
		return ERROR_OUT_OF_RANGE;
	}

	const uint8_t* pData = mIndices.data();
	uint32_t       elementSize = IndexTypeSize(mIndexType);

	if (mIndexType == IndexType::Uint16) {
		size_t          offset = 2 * triIndex * elementSize;
		const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(pData + offset);
		v0 = static_cast<uint32_t>(pIndexData[0]);
		v1 = static_cast<uint32_t>(pIndexData[1]);
	}
	else if (mIndexType == IndexType::Uint32) {
		size_t          offset = 2 * triIndex * elementSize;
		const uint32_t* pIndexData = reinterpret_cast<const uint32_t*>(pData + offset);
		v0 = static_cast<uint32_t>(pIndexData[0]);
		v1 = static_cast<uint32_t>(pIndexData[1]);
	}

	return SUCCESS;
}

Result WireMesh::GetVertexData(uint32_t vtxIndex, WireMeshVertexData* pVertexData) const
{
	uint32_t vertexCount = GetCountPositions();
	if (vtxIndex >= vertexCount) {
		return ERROR_OUT_OF_RANGE;
	}

	const float3* pPosition = GetDataPositions(vtxIndex);
	const float3* pColor = GetDataColors(vtxIndex);

	pVertexData->position = *pPosition;

	if (!IsNull(pColor)) {
		pVertexData->color = *pColor;
	}

	return SUCCESS;
}

void WireMesh::AppendIndexAndVertexData(
	std::vector<uint32_t>& indexData,
	const std::vector<float>& vertexData,
	const uint32_t            expectedVertexCount,
	const WireMeshOptions& options,
	WireMesh& mesh)
{
	IndexType indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;

	// Verify expected vertex count
	size_t vertexCount = (vertexData.size() * sizeof(float)) / sizeof(WireMeshVertexData);
	ASSERT_MSG(vertexCount == expectedVertexCount, "unexpected vertex count");

	// Get base pointer to vertex data
	const char* pData = reinterpret_cast<const char*>(vertexData.data());

	if (indexType != IndexType::Undefined) {
		for (size_t i = 0; i < vertexCount; ++i) {
			const WireMeshVertexData* pVertexData = reinterpret_cast<const WireMeshVertexData*>(pData + (i * sizeof(WireMeshVertexData)));

			mesh.AppendPosition(pVertexData->position * options.mScale);

			if (options.mEnableVertexColors || options.mEnableObjectColor) {
				float3 color = options.mEnableObjectColor ? options.mObjectColor : pVertexData->color;
				mesh.AppendColor(color);
			}
		}

		size_t edgeCount = indexData.size() / 2;
		for (size_t triIndex = 0; triIndex < edgeCount; ++triIndex) {
			uint32_t v0 = indexData[2 * triIndex + 0];
			uint32_t v1 = indexData[2 * triIndex + 1];
			mesh.AppendEdge(v0, v1);
		}
	}
	else {
		for (size_t i = 0; i < indexData.size(); ++i) {
			uint32_t                  vi = indexData[i];
			const WireMeshVertexData* pVertexData = reinterpret_cast<const WireMeshVertexData*>(pData + (vi * sizeof(WireMeshVertexData)));

			mesh.AppendPosition(pVertexData->position * options.mScale);

			if (options.mEnableVertexColors) {
				mesh.AppendColor(pVertexData->color);
			}
		}
	}
}

WireMesh WireMesh::CreatePlane(WireMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options)
{
	const float    hs = size.x / 2.0f;
	const float    ht = size.y / 2.0f;
	const float    ds = size.x / static_cast<float>(usegs);
	const float    dt = size.y / static_cast<float>(vsegs);
	const uint32_t uverts = usegs + 1;
	const uint32_t vverts = vsegs + 1;

	std::vector<float>    vertexData;
	std::vector<uint32_t> indexData;
	uint32_t              indexCount = 0;
	// U segemnts
	for (uint32_t i = 0; i < uverts; ++i)
	{
		float s = static_cast<float>(i) * ds / size.x;
		float t0 = 0;
		float t1 = 1;

		float3 position0 = float3(0);
		float3 position1 = float3(0);
		switch (plane)
		{
		case WIRE_MESH_PLANE_POSITIVE_X:
		case WIRE_MESH_PLANE_NEGATIVE_X:
		case WIRE_MESH_PLANE_POSITIVE_Z:
		case WIRE_MESH_PLANE_NEGATIVE_Z:
		default: {
			Fatal("unknown plane orientation");
		} break;

		case WIRE_MESH_PLANE_POSITIVE_Y: {
			position0 = float3(s * size.x - hs, 0, t0 * size.y - ht);
			position1 = float3(s * size.x - hs, 0, t1 * size.y - ht);
		} break;

		case WIRE_MESH_PLANE_NEGATIVE_Y: {
			position0 = float3((1.0f - s) * size.x - hs, 0, (1.0f - t0) * size.y - ht);
			position1 = float3((1.0f - s) * size.x - hs, 0, (1.0f - t1) * size.y - ht);
		} break;
		}

		float3 color0 = float3(s, 0, 0);
		float3 color1 = float3(s, 1, 0);

		vertexData.push_back(position0.x);
		vertexData.push_back(position0.y);
		vertexData.push_back(position0.z);
		vertexData.push_back(color0.r);
		vertexData.push_back(color0.g);
		vertexData.push_back(color0.b);

		vertexData.push_back(position1.x);
		vertexData.push_back(position1.y);
		vertexData.push_back(position1.z);
		vertexData.push_back(color1.r);
		vertexData.push_back(color1.g);
		vertexData.push_back(color1.b);

		indexData.push_back(indexCount);
		indexCount += 1;
		indexData.push_back(indexCount);
		indexCount += 1;
	}
	// V segemnts
	for (uint32_t j = 0; j < vverts; ++j)
	{
		float s0 = 0;
		float s1 = 1;
		float t = static_cast<float>(j) * dt / size.y;

		float3 position0 = float3(0);
		float3 position1 = float3(0);
		switch (plane)
		{
		case WIRE_MESH_PLANE_POSITIVE_X:
		case WIRE_MESH_PLANE_NEGATIVE_X:
		case WIRE_MESH_PLANE_POSITIVE_Z:
		case WIRE_MESH_PLANE_NEGATIVE_Z:
		default:
		{
			Fatal("unknown plane orientation");
		} break;

		case WIRE_MESH_PLANE_POSITIVE_Y: {
			position0 = float3(s0 * size.x - hs, 0, t * size.y - ht);
			position1 = float3(s1 * size.x - hs, 0, t * size.y - ht);
		} break;

		case WIRE_MESH_PLANE_NEGATIVE_Y: {
			position0 = float3((1.0f - s0) * size.x - hs, 0, (1.0f - t) * size.y - ht);
			position1 = float3((1.0f - s1) * size.x - hs, 0, (1.0f - t) * size.y - ht);
		} break;
		}

		float3 color0 = float3(0, t, 0);
		float3 color1 = float3(1, t, 0);

		vertexData.push_back(position0.x);
		vertexData.push_back(position0.y);
		vertexData.push_back(position0.z);
		vertexData.push_back(color0.r);
		vertexData.push_back(color0.g);
		vertexData.push_back(color0.b);

		vertexData.push_back(position1.x);
		vertexData.push_back(position1.y);
		vertexData.push_back(position1.z);
		vertexData.push_back(color1.r);
		vertexData.push_back(color1.g);
		vertexData.push_back(color1.b);

		indexData.push_back(indexCount);
		indexCount += 1;
		indexData.push_back(indexCount);
		indexCount += 1;
	}

	IndexType indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	WireMesh        mesh = WireMesh(indexType);

	uint32_t expectedVertexCount = 2 * (uverts + vverts);
	AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

	return mesh;
}

WireMesh WireMesh::CreateCube(const float3& size, const WireMeshOptions& options)
{
	const float hx = size.x / 2.0f;
	const float hy = size.y / 2.0f;
	const float hz = size.z / 2.0f;

	std::vector<float> vertexData = {
		// position      // vertex colors    
		 hx,  hy, -hz,    1.0f, 0.0f, 0.0f,  //  0  -Z side
		 hx, -hy, -hz,    1.0f, 0.0f, 0.0f,  //  1
		-hx, -hy, -hz,    1.0f, 0.0f, 0.0f,  //  2
		-hx,  hy, -hz,    1.0f, 0.0f, 0.0f,  //  3

		-hx,  hy,  hz,    0.0f, 1.0f, 0.0f,  //  4  +Z side
		-hx, -hy,  hz,    0.0f, 1.0f, 0.0f,  //  5
		 hx, -hy,  hz,    0.0f, 1.0f, 0.0f,  //  6
		 hx,  hy,  hz,    0.0f, 1.0f, 0.0f,  //  7

		-hx,  hy, -hz,   -0.0f, 0.0f, 1.0f,  //  8  -X side
		-hx, -hy, -hz,   -0.0f, 0.0f, 1.0f,  //  9
		-hx, -hy,  hz,   -0.0f, 0.0f, 1.0f,  // 10
		-hx,  hy,  hz,   -0.0f, 0.0f, 1.0f,  // 11

		 hx,  hy,  hz,    1.0f, 1.0f, 0.0f,  // 12  +X side
		 hx, -hy,  hz,    1.0f, 1.0f, 0.0f,  // 13
		 hx, -hy, -hz,    1.0f, 1.0f, 0.0f,  // 14
		 hx,  hy, -hz,    1.0f, 1.0f, 0.0f,  // 15

		-hx, -hy,  hz,    1.0f, 0.0f, 1.0f,  // 16  -Y side
		-hx, -hy, -hz,    1.0f, 0.0f, 1.0f,  // 17
		 hx, -hy, -hz,    1.0f, 0.0f, 1.0f,  // 18
		 hx, -hy,  hz,    1.0f, 0.0f, 1.0f,  // 19

		-hx,  hy, -hz,    0.0f, 1.0f, 1.0f,  // 20  +Y side
		-hx,  hy,  hz,    0.0f, 1.0f, 1.0f,  // 21
		 hx,  hy,  hz,    0.0f, 1.0f, 1.0f,  // 22
		 hx,  hy, -hz,    0.0f, 1.0f, 1.0f,  // 23
	};

	std::vector<uint32_t> indexData = {
		0,  1, // -Z side
		1,  2,
		2,  3,
		3,  0,

		4,  5, // +Z side
		5,  6,
		6,  7,
		7,  4,

		8,  9, // -X side
		9, 10,
	   10, 11,
	   11,  8,

		12, 13, // +X side
		13, 14,
		14, 15,
		15, 12,

		16, 17, // -X side
		17, 18,
		18, 19,
		19, 16,

		20, 21, // +X side
		21, 22,
		22, 23,
		23, 20,
	};

	IndexType indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	WireMesh        mesh = WireMesh(indexType);

	AppendIndexAndVertexData(indexData, vertexData, 24, options, mesh);

	return mesh;
}

WireMesh WireMesh::CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options)
{
	constexpr float kPi = glm::pi<float>();
	constexpr float kTwoPi = 2.0f * kPi;

	const uint32_t uverts = usegs + 1;
	const uint32_t vverts = vsegs + 1;

	float dt = kTwoPi / static_cast<float>(usegs);
	float dp = kPi / static_cast<float>(vsegs);

	std::vector<float>    vertexData;
	std::vector<uint32_t> indexData;
	uint32_t              indexCount = 0;
	// U segemnts
	for (uint32_t j = 1; j < (vverts - 1); ++j)
	{
		for (uint32_t i = 1; i < uverts; ++i)
		{
			float  theta0 = (static_cast<float>(i) - 1) * dt;
			float  theta1 = (static_cast<float>(i) - 0) * dt;
			float  phi = static_cast<float>(j) * dp;
			float  u0 = theta0 / kTwoPi;
			float  u1 = theta1 / kTwoPi;
			float  v = phi / kPi;
			float3 P0 = SphericalToCartesian(theta0, phi);
			float3 P1 = SphericalToCartesian(theta1, phi);
			float3 position0 = radius * P0;
			float3 position1 = radius * P1;
			float3 color0 = float3(u0, v, 0);
			float3 color1 = float3(u1, v, 0);

			vertexData.push_back(position0.x);
			vertexData.push_back(position0.y);
			vertexData.push_back(position0.z);
			vertexData.push_back(color0.r);
			vertexData.push_back(color0.g);
			vertexData.push_back(color0.b);

			vertexData.push_back(position1.x);
			vertexData.push_back(position1.y);
			vertexData.push_back(position1.z);
			vertexData.push_back(color1.r);
			vertexData.push_back(color1.g);
			vertexData.push_back(color1.b);

			indexData.push_back(indexCount);
			indexCount += 1;
			indexData.push_back(indexCount);
			indexCount += 1;
		}
	}
	// V segemnts
	for (uint32_t i = 0; i < (uverts - 1); ++i)
	{
		for (uint32_t j = 1; j < vverts; ++j)
		{
			float theta = static_cast<float>(i) * dt;
			float phi0 = static_cast<float>(j - 0) * dp;
			float phi1 = static_cast<float>(j - 1) * dp;
			float u = theta / kTwoPi;
			float v0 = phi0 / kPi;
			float v1 = phi1 / kPi;
			float3 P0 = SphericalToCartesian(theta, phi0);
			float3 P1 = SphericalToCartesian(theta, phi1);
			float3 position0 = radius * P0;
			float3 position1 = radius * P1;
			float3 color0 = float3(u, v0, 0);
			float3 color1 = float3(u, v1, 0);

			vertexData.push_back(position0.x);
			vertexData.push_back(position0.y);
			vertexData.push_back(position0.z);
			vertexData.push_back(color0.r);
			vertexData.push_back(color0.g);
			vertexData.push_back(color0.b);

			vertexData.push_back(position1.x);
			vertexData.push_back(position1.y);
			vertexData.push_back(position1.z);
			vertexData.push_back(color1.r);
			vertexData.push_back(color1.g);
			vertexData.push_back(color1.b);

			indexData.push_back(indexCount);
			indexCount += 1;
			indexData.push_back(indexCount);
			indexCount += 1;
		}
	}

	IndexType indexType = options.mEnableIndices ? IndexType::Uint32 : IndexType::Undefined;
	WireMesh        mesh = WireMesh(indexType);

	uint32_t expectedVertexCountU = (uverts - 1) * (vverts - 2);
	uint32_t expectedVertexCountV = (uverts - 1) * (vverts - 1);
	uint32_t expectedVertexCount = 2 * (expectedVertexCountU + expectedVertexCountV);
	AppendIndexAndVertexData(indexData, vertexData, expectedVertexCount, options, mesh);

	return mesh;
}

#pragma endregion

#pragma region Geometry

#define NOT_INTERLEAVED_MSG "cannot append interleaved data if attribute layout is not interleaved"
#define NOT_PLANAR_MSG      "cannot append planar data if attribute layout is not planar"

// -------------------------------------------------------------------------------------------------
// VertexDataProcessorBase
//     interface for all VertexDataProcessors
//     with helper functions to allow derived classes to access Geometry
// Note that the base and derived VertexDataProcessor classes do not have any data member
//     be careful when adding data members to any of these classes
//     it could create problems for multithreaded cases for multiple geometry objects
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorBase
{
public:
	// Validates the layout
	// returns false if the validation fails
	virtual bool Validate(Geometry* pGeom) = 0;
	// Updates the vertex buffer and the vertex buffer index
	// returns Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC if the sematic is invalid
	virtual Result UpdateVertexBuffer(Geometry* pGeom) = 0;
	// Fetches vertex data from vtx and append it to the geometry
	// Returns 0 if the appending fails
	virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) = 0;
	virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) = 0;
	// Gets the vertex count of the geometry
	virtual uint32_t GetVertexCount(const Geometry* pGeom) = 0;

protected:
	// Prevent from being deleted explicitly
	virtual ~VertexDataProcessorBase() {}

	// ----------------------------------------
	// Helper functions to access Geometry data
	// ----------------------------------------

	// Missing attributes will also result in NOOP.
	// returns the element count of the vertex buffer with the bufferIndex
	// 0 is returned if the bufferIndex is ignored
	template <typename U>
	uint32_t AppendDataToVertexBuffer(Geometry* pGeom, uint32_t bufferIndex, const U& data)
	{
		if (bufferIndex != VALUE_IGNORED) {
			ASSERT_MSG((bufferIndex >= 0) && (bufferIndex < pGeom->mVertexBuffers.size()), "buffer index is not valid");
			pGeom->mVertexBuffers[bufferIndex].Append(data);
			return pGeom->mVertexBuffers[bufferIndex].GetElementCount();
		}
		return 0;
	}

	void AddVertexBuffer(Geometry* pGeom, uint32_t bindingIndex)
	{
		pGeom->mVertexBuffers.push_back(Geometry::Buffer(Geometry::BUFFER_TYPE_VERTEX, GetVertexBindingStride(pGeom, bindingIndex)));
	}

	uint32_t GetVertexBufferSize(const Geometry* pGeom, uint32_t bufferIndex) const
	{
		return pGeom->mVertexBuffers[bufferIndex].GetSize();
	}

	uint32_t GetVertexBufferElementCount(const Geometry* pGeom, uint32_t bufferIndex) const
	{
		return pGeom->mVertexBuffers[bufferIndex].GetElementCount();
	}

	uint32_t GetVertexBufferElementSize(const Geometry* pGeom, uint32_t bufferIndex) const
	{
		return pGeom->mVertexBuffers[bufferIndex].GetElementSize();
	}

	uint32_t GetVertexBindingAttributeCount(const Geometry* pGeom, uint32_t bindingIndex) const
	{
		return pGeom->mCreateInfo.vertexBindings[bindingIndex].GetAttributeCount();
	}

	uint32_t GetVertexBindingStride(const Geometry* pGeom, uint32_t bindingIndex) const
	{
		return pGeom->mCreateInfo.vertexBindings[bindingIndex].GetStride();
	}

	uint32_t GetVertexBindingCount(const Geometry* pGeom) const
	{
		return pGeom->mCreateInfo.vertexBindingCount;
	}

	VertexSemantic GetVertexBindingAttributeSematic(const Geometry* pGeom, uint32_t bindingIndex, uint32_t attrIndex) const
	{
		const VertexAttribute* pAttribute = nullptr;
		Result                       ppxres = pGeom->mCreateInfo.vertexBindings[bindingIndex].GetAttribute(attrIndex, &pAttribute);
		ASSERT_MSG(ppxres == SUCCESS, "attribute not found at index=" + std::to_string(attrIndex));
		return pAttribute->semantic;
	}

	uint32_t GetPositionBufferIndex(const Geometry* pGeom) const { return pGeom->mPositionBufferIndex; }
	uint32_t GetNormalBufferIndex(const Geometry* pGeom) const { return pGeom->mNormaBufferIndex; }
	uint32_t GetColorBufferIndex(const Geometry* pGeom) const { return pGeom->mColorBufferIndex; }
	uint32_t GetTexCoordBufferIndex(const Geometry* pGeom) const { return pGeom->mTexCoordBufferIndex; }
	uint32_t GetTangentBufferIndex(const Geometry* pGeom) const { return pGeom->mTangentBufferIndex; }
	uint32_t GetBitangentBufferIndex(const Geometry* pGeom) const { return pGeom->mBitangentBufferIndex; }

	void SetPositionBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mPositionBufferIndex = index; }
	void SetNormalBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mNormaBufferIndex = index; }
	void SetColorBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mColorBufferIndex = index; }
	void SetTexCoordBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mTexCoordBufferIndex = index; }
	void SetTangentBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mTangentBufferIndex = index; }
	void SetBitangentBufferIndex(Geometry* pGeom, uint32_t index) { pGeom->mBitangentBufferIndex = index; }
};

// -------------------------------------------------------------------------------------------------
// VertexDataProcessor for Planar vertex attribute layout
//     Planar: each attribute has its own vertex input binding
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorPlanar : public VertexDataProcessorBase<T>
{
public:
	virtual bool Validate(Geometry* pGeom) override
	{
		const uint32_t vertexBindingCount = this->GetVertexBindingCount(pGeom);
		for (uint32_t i = 0; i < vertexBindingCount; ++i) {
			if (this->GetVertexBindingAttributeCount(pGeom, i) != 1) {
				Fatal("planar layout must have 1 attribute");
				return false;
			}
		}
		return true;
	}

	virtual Result UpdateVertexBuffer(Geometry* pGeom) override
	{
		// Create buffers
		const uint32_t vertexBindingCount = this->GetVertexBindingCount(pGeom);
		for (uint32_t i = 0; i < vertexBindingCount; ++i) {
			this->AddVertexBuffer(pGeom, i);
			const VertexSemantic semantic = this->GetVertexBindingAttributeSematic(pGeom, i, 0);
			switch (semantic)
			{
			case VertexSemantic::Undefined:
			case VertexSemantic::Texcoord0:
			case VertexSemantic::Texcoord1:
			case VertexSemantic::Texcoord2:
			case VertexSemantic::Texcoord3:
			case VertexSemantic::Texcoord4:
			case VertexSemantic::Texcoord5:
			case VertexSemantic::Texcoord6:
			case VertexSemantic::Texcoord7:
			case VertexSemantic::Texcoord8:
			case VertexSemantic::Texcoord9:
			case VertexSemantic::Texcoord10:
			case VertexSemantic::Texcoord11:
			case VertexSemantic::Texcoord12:
			case VertexSemantic::Texcoord13:
			case VertexSemantic::Texcoord14:
			case VertexSemantic::Texcoord15:
			case VertexSemantic::Texcoord16:
			case VertexSemantic::Texcoord17:
			case VertexSemantic::Texcoord18:
			case VertexSemantic::Texcoord19:
			case VertexSemantic::Texcoord20:
			case VertexSemantic::Texcoord21:
			case VertexSemantic::Texcoord22:
			default: return Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC;
			case VertexSemantic::Position: this->SetPositionBufferIndex(pGeom, i); break;
			case VertexSemantic::Normal: this->SetNormalBufferIndex(pGeom, i); break;
			case VertexSemantic::Color: this->SetColorBufferIndex(pGeom, i); break;
			case VertexSemantic::Tangent: this->SetTangentBufferIndex(pGeom, i); break;
			case VertexSemantic::Bitangent: this->SetBitangentBufferIndex(pGeom, i); break;
			case VertexSemantic::Texcoord: this->SetTexCoordBufferIndex(pGeom, i); break;
			}
		}
		return SUCCESS;
	}

	virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) override
	{
		const uint32_t n = this->AppendDataToVertexBuffer(pGeom, this->GetPositionBufferIndex(pGeom), vtx.position);
		ASSERT_MSG(n > 0, "position should always available");
		this->AppendDataToVertexBuffer(pGeom, this->GetNormalBufferIndex(pGeom), vtx.normal);
		this->AppendDataToVertexBuffer(pGeom, this->GetColorBufferIndex(pGeom), vtx.color);
		this->AppendDataToVertexBuffer(pGeom, this->GetTexCoordBufferIndex(pGeom), vtx.texCoord);
		this->AppendDataToVertexBuffer(pGeom, this->GetTangentBufferIndex(pGeom), vtx.tangent);
		this->AppendDataToVertexBuffer(pGeom, this->GetBitangentBufferIndex(pGeom), vtx.bitangent);
		return n;
	}

	virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) override
	{
		const uint32_t n = this->AppendDataToVertexBuffer(pGeom, this->GetPositionBufferIndex(pGeom), vtx.position);
		ASSERT_MSG(n > 0, "position should always available");
		this->AppendDataToVertexBuffer(pGeom, this->GetColorBufferIndex(pGeom), vtx.color);
		return n;
	}

	virtual uint32_t GetVertexCount(const Geometry* pGeom) override
	{
		return this->GetVertexBufferElementCount(pGeom, this->GetPositionBufferIndex(pGeom));
	}
};

// -------------------------------------------------------------------------------------------------
// VertexDataProcessor for Interleaved vertex attribute layout
//     Interleaved: only has 1 vertex input binding, data is interleaved
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorInterleaved : public VertexDataProcessorBase<T>
{
public:
	virtual bool Validate(Geometry* pGeom) override
	{
		const uint32_t vertexBindingCount = this->GetVertexBindingCount(pGeom);
		if (vertexBindingCount != 1) {
			Fatal("interleaved layout must have 1 binding");
			return false;
		}
		return true;
	}

	virtual Result UpdateVertexBuffer(Geometry* pGeom) override
	{
		ASSERT_MSG(1 == this->GetVertexBindingCount(pGeom), "there should be only 1 binding for planar");
		this->AddVertexBuffer(pGeom, kBufferIndex);
		return SUCCESS;
	}

	virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) override
	{
		uint32_t       startSize = this->GetVertexBufferSize(pGeom, kBufferIndex);
		const uint32_t attrCount = this->GetVertexBindingAttributeCount(pGeom, kBufferIndex);
		for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
			const VertexSemantic semantic = this->GetVertexBindingAttributeSematic(pGeom, kBufferIndex, attrIndex);

			switch (semantic) {
			case VertexSemantic::Undefined:
			case VertexSemantic::Texcoord0:
			case VertexSemantic::Texcoord1:
			case VertexSemantic::Texcoord2:
			case VertexSemantic::Texcoord3:
			case VertexSemantic::Texcoord4:
			case VertexSemantic::Texcoord5:
			case VertexSemantic::Texcoord6:
			case VertexSemantic::Texcoord7:
			case VertexSemantic::Texcoord8:
			case VertexSemantic::Texcoord9:
			case VertexSemantic::Texcoord10:
			case VertexSemantic::Texcoord11:
			case VertexSemantic::Texcoord12:
			case VertexSemantic::Texcoord13:
			case VertexSemantic::Texcoord14:
			case VertexSemantic::Texcoord15:
			case VertexSemantic::Texcoord16:
			case VertexSemantic::Texcoord17:
			case VertexSemantic::Texcoord18:
			case VertexSemantic::Texcoord19:
			case VertexSemantic::Texcoord20:
			case VertexSemantic::Texcoord21:
			case VertexSemantic::Texcoord22:
			default: break;
			case VertexSemantic::Position:
			{
				const uint32_t n = this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.position);
				ASSERT_MSG(n > 0, "position should always available");
			}
			break;
			case VertexSemantic::Normal: this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.normal); break;
			case VertexSemantic::Color: this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.color); break;
			case VertexSemantic::Tangent: this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.tangent); break;
			case VertexSemantic::Bitangent: this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.bitangent); break;
			case VertexSemantic::Texcoord: this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.texCoord); break;
			}
		}
		uint32_t endSize = this->GetVertexBufferSize(pGeom, kBufferIndex);

		uint32_t       bytesWritten = (endSize - startSize);
		const uint32_t vertexBufferElementSize = this->GetVertexBufferElementSize(pGeom, kBufferIndex);
		ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");

		return this->GetVertexBufferElementCount(pGeom, kBufferIndex);
	}

	virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) override
	{
		uint32_t       startSize = this->GetVertexBufferSize(pGeom, kBufferIndex);
		const uint32_t attrCount = this->GetVertexBindingAttributeCount(pGeom, kBufferIndex);
		for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
			const VertexSemantic semantic = this->GetVertexBindingAttributeSematic(pGeom, kBufferIndex, attrIndex);

			switch (semantic)
			{
			case VertexSemantic::Undefined:
			case VertexSemantic::Normal:
			case VertexSemantic::Bitangent:
			case VertexSemantic::Tangent:
			case VertexSemantic::Texcoord:
			case VertexSemantic::Texcoord0:
			case VertexSemantic::Texcoord1:
			case VertexSemantic::Texcoord2:
			case VertexSemantic::Texcoord3:
			case VertexSemantic::Texcoord4:
			case VertexSemantic::Texcoord5:
			case VertexSemantic::Texcoord6:
			case VertexSemantic::Texcoord7:
			case VertexSemantic::Texcoord8:
			case VertexSemantic::Texcoord9:
			case VertexSemantic::Texcoord10:
			case VertexSemantic::Texcoord11:
			case VertexSemantic::Texcoord12:
			case VertexSemantic::Texcoord13:
			case VertexSemantic::Texcoord14:
			case VertexSemantic::Texcoord15:
			case VertexSemantic::Texcoord16:
			case VertexSemantic::Texcoord17:
			case VertexSemantic::Texcoord18:
			case VertexSemantic::Texcoord19:
			case VertexSemantic::Texcoord20:
			case VertexSemantic::Texcoord21:
			case VertexSemantic::Texcoord22:
			default: break;
			case VertexSemantic::Position:
			{
				const uint32_t n = this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.position);
				ASSERT_MSG(n > 0, "position should always available");
			}
			break;
			case VertexSemantic::Color: this->AppendDataToVertexBuffer(pGeom, kBufferIndex, vtx.color); break;
			}
		}
		uint32_t endSize = this->GetVertexBufferSize(pGeom, kBufferIndex);

		uint32_t       bytesWritten = (endSize - startSize);
		const uint32_t vertexBufferElementSize = this->GetVertexBufferElementSize(pGeom, kBufferIndex);
		ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");

		return this->GetVertexBufferElementCount(pGeom, kBufferIndex);
	}

	virtual uint32_t GetVertexCount(const Geometry* pGeom) override
	{
		return this->GetVertexBufferElementCount(pGeom, kBufferIndex);
	}

private:
	// for VertexDataProcessorInterleaved, there is only 1 binding, so the index is always 0
	const uint32_t kBufferIndex = 0;
};

// -------------------------------------------------------------------------------------------------
// VertexDataProcessor for Position Planar vertex attribute layout
//     Position Planar: only has 2 vertex input bindings
//        - Binding 0 only has Position data
//        - Binding 1 contains all non-position data, interleaved
// -------------------------------------------------------------------------------------------------
template <typename T>
class VertexDataProcessorPositionPlanar : public VertexDataProcessorBase<T>
{
public:
	virtual bool Validate(Geometry* pGeom) override
	{
		const uint32_t vertexBindingCount = this->GetVertexBindingCount(pGeom);
		if (vertexBindingCount != 2)
		{
			Fatal("position planar layout must have 2 bindings");
			return false;
		}
		return true;
	}

	virtual Result UpdateVertexBuffer(Geometry* pGeom) override
	{
		ASSERT_MSG(2 == this->GetVertexBindingCount(pGeom), "there should be 2 binding for position planar");
		// Position
		this->AddVertexBuffer(pGeom, kPositionBufferIndex);
		// Non-Position data
		this->AddVertexBuffer(pGeom, kNonPositionBufferIndex);

		this->SetPositionBufferIndex(pGeom, kPositionBufferIndex);

		const uint32_t attributeCount = this->GetVertexBindingAttributeCount(pGeom, kNonPositionBufferIndex);
		for (uint32_t i = 0; i < attributeCount; ++i) {
			const VertexSemantic semantic = this->GetVertexBindingAttributeSematic(pGeom, kNonPositionBufferIndex, i);

			switch (semantic) {
			case VertexSemantic::Undefined:
			case VertexSemantic::Texcoord0:
			case VertexSemantic::Texcoord1:
			case VertexSemantic::Texcoord2:
			case VertexSemantic::Texcoord3:
			case VertexSemantic::Texcoord4:
			case VertexSemantic::Texcoord5:
			case VertexSemantic::Texcoord6:
			case VertexSemantic::Texcoord7:
			case VertexSemantic::Texcoord8:
			case VertexSemantic::Texcoord9:
			case VertexSemantic::Texcoord10:
			case VertexSemantic::Texcoord11:
			case VertexSemantic::Texcoord12:
			case VertexSemantic::Texcoord13:
			case VertexSemantic::Texcoord14:
			case VertexSemantic::Texcoord15:
			case VertexSemantic::Texcoord16:
			case VertexSemantic::Texcoord17:
			case VertexSemantic::Texcoord18:
			case VertexSemantic::Texcoord19:
			case VertexSemantic::Texcoord20:
			case VertexSemantic::Texcoord21:
			case VertexSemantic::Texcoord22:
			default: return Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC;
			case VertexSemantic::Position: Fatal("position should be in binding 0"); break;
			case VertexSemantic::Normal: this->SetNormalBufferIndex(pGeom, kNonPositionBufferIndex); break;
			case VertexSemantic::Color: this->SetColorBufferIndex(pGeom, kNonPositionBufferIndex); break;
			case VertexSemantic::Tangent: this->SetTangentBufferIndex(pGeom, kNonPositionBufferIndex); break;
			case VertexSemantic::Bitangent: this->SetBitangentBufferIndex(pGeom, kNonPositionBufferIndex); break;
			case VertexSemantic::Texcoord: this->SetTexCoordBufferIndex(pGeom, kNonPositionBufferIndex); break;
			}
		}

		return SUCCESS;
	}

	virtual uint32_t AppendVertexData(Geometry* pGeom, const T& vtx) override
	{
		const uint32_t n = this->AppendDataToVertexBuffer(pGeom, this->GetPositionBufferIndex(pGeom), vtx.position);
		ASSERT_MSG(n > 0, "position should always available");

		uint32_t       startSize = this->GetVertexBufferSize(pGeom, kNonPositionBufferIndex);
		const uint32_t attrCount = this->GetVertexBindingAttributeCount(pGeom, kNonPositionBufferIndex);
		for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
			const VertexSemantic semantic = this->GetVertexBindingAttributeSematic(pGeom, kNonPositionBufferIndex, attrIndex);

			switch (semantic) 
			{
			case VertexSemantic::Undefined:
			case VertexSemantic::Texcoord0:
			case VertexSemantic::Texcoord1:
			case VertexSemantic::Texcoord2:
			case VertexSemantic::Texcoord3:
			case VertexSemantic::Texcoord4:
			case VertexSemantic::Texcoord5:
			case VertexSemantic::Texcoord6:
			case VertexSemantic::Texcoord7:
			case VertexSemantic::Texcoord8:
			case VertexSemantic::Texcoord9:
			case VertexSemantic::Texcoord10:
			case VertexSemantic::Texcoord11:
			case VertexSemantic::Texcoord12:
			case VertexSemantic::Texcoord13:
			case VertexSemantic::Texcoord14:
			case VertexSemantic::Texcoord15:
			case VertexSemantic::Texcoord16:
			case VertexSemantic::Texcoord17:
			case VertexSemantic::Texcoord18:
			case VertexSemantic::Texcoord19:
			case VertexSemantic::Texcoord20:
			case VertexSemantic::Texcoord21:
			case VertexSemantic::Texcoord22:
			default: Fatal( "should not have other sematic"); break;
			case VertexSemantic::Position: Fatal("position should be in binding 0"); break;
			case VertexSemantic::Normal: this->AppendDataToVertexBuffer(pGeom, this->GetNormalBufferIndex(pGeom), vtx.normal); break;
			case VertexSemantic::Color: this->AppendDataToVertexBuffer(pGeom, this->GetColorBufferIndex(pGeom), vtx.color); break;
			case VertexSemantic::Tangent: this->AppendDataToVertexBuffer(pGeom, this->GetTangentBufferIndex(pGeom), vtx.tangent); break;
			case VertexSemantic::Bitangent: this->AppendDataToVertexBuffer(pGeom, this->GetBitangentBufferIndex(pGeom), vtx.bitangent); break;
			case VertexSemantic::Texcoord: this->AppendDataToVertexBuffer(pGeom, this->GetTexCoordBufferIndex(pGeom), vtx.texCoord); break;
			}
		}
		uint32_t endSize = this->GetVertexBufferSize(pGeom, kNonPositionBufferIndex);

		uint32_t       bytesWritten = (endSize - startSize);
		const uint32_t vertexBufferElementSize = this->GetVertexBufferElementSize(pGeom, kNonPositionBufferIndex);
		ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");
		return n;
	}

	virtual uint32_t AppendVertexData(Geometry* pGeom, const WireMeshVertexData& vtx) override
	{
		const uint32_t n = this->AppendDataToVertexBuffer(pGeom, kPositionBufferIndex, vtx.position);
		ASSERT_MSG(n > 0, "position should always available");
		uint32_t       startSize = this->GetVertexBufferSize(pGeom, kNonPositionBufferIndex);
		const uint32_t attrCount = this->GetVertexBindingAttributeCount(pGeom, kNonPositionBufferIndex);
		for (uint32_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
			const VertexSemantic semantic = this->GetVertexBindingAttributeSematic(pGeom, kNonPositionBufferIndex, attrIndex);

			switch (semantic) {
			case VertexSemantic::Undefined:
			case VertexSemantic::Normal:
			case VertexSemantic::Bitangent:
			case VertexSemantic::Tangent:
			case VertexSemantic::Texcoord:
			case VertexSemantic::Texcoord0:
			case VertexSemantic::Texcoord1:
			case VertexSemantic::Texcoord2:
			case VertexSemantic::Texcoord3:
			case VertexSemantic::Texcoord4:
			case VertexSemantic::Texcoord5:
			case VertexSemantic::Texcoord6:
			case VertexSemantic::Texcoord7:
			case VertexSemantic::Texcoord8:
			case VertexSemantic::Texcoord9:
			case VertexSemantic::Texcoord10:
			case VertexSemantic::Texcoord11:
			case VertexSemantic::Texcoord12:
			case VertexSemantic::Texcoord13:
			case VertexSemantic::Texcoord14:
			case VertexSemantic::Texcoord15:
			case VertexSemantic::Texcoord16:
			case VertexSemantic::Texcoord17:
			case VertexSemantic::Texcoord18:
			case VertexSemantic::Texcoord19:
			case VertexSemantic::Texcoord20:
			case VertexSemantic::Texcoord21:
			case VertexSemantic::Texcoord22:
			default: Fatal("should not have other sematic"); break;
			case VertexSemantic::Position: Fatal("position should be in binding 0"); break;
			case VertexSemantic::Color: this->AppendDataToVertexBuffer(pGeom, this->GetColorBufferIndex(pGeom), vtx.color); break;
			}
		}
		uint32_t endSize = this->GetVertexBufferSize(pGeom, kNonPositionBufferIndex);

		uint32_t bytesWritten = (endSize - startSize);

		const uint32_t vertexBufferElementSize = this->GetVertexBufferElementSize(pGeom, kNonPositionBufferIndex);
		ASSERT_MSG(bytesWritten == vertexBufferElementSize, "size of vertex data written does not match buffer's element size");
		return n;
	}

	virtual uint32_t GetVertexCount(const Geometry* pGeom) override
	{
		return this->GetVertexBufferElementCount(pGeom, this->GetPositionBufferIndex(pGeom));
	}

private:
	const uint32_t kPositionBufferIndex = 0;
	const uint32_t kNonPositionBufferIndex = 1;
};

static VertexDataProcessorPlanar<TriMeshVertexData>                   sVDProcessorPlanar;
static VertexDataProcessorPlanar<TriMeshVertexDataCompressed>         sVDProcessorPlanarCompressed;
static VertexDataProcessorInterleaved<TriMeshVertexData>              sVDProcessorInterleaved;
static VertexDataProcessorInterleaved<TriMeshVertexDataCompressed>    sVDProcessorInterleavedCompressed;
static VertexDataProcessorPositionPlanar<TriMeshVertexData>           sVDProcessorPositionPlanar;
static VertexDataProcessorPositionPlanar<TriMeshVertexDataCompressed> sVDProcessorPositionPlanarCompressed;

GeometryCreateInfo GeometryCreateInfo::InterleavedU8(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
	ci.indexType = IndexType::Uint8;
	ci.vertexBindingCount = 1; // Interleave attribute layout always has 1 vertex binding
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::InterleavedU16(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
	ci.indexType = IndexType::Uint16;
	ci.vertexBindingCount = 1; // Interleave attribute layout always has 1 vertex binding
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::InterleavedU32(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
	ci.indexType = IndexType::Uint32;
	ci.vertexBindingCount = 1; // Interleave attribute layout always has 1 vertex binding
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PlanarU8(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
	ci.indexType = IndexType::Uint8;
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PlanarU16(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
	ci.indexType = IndexType::Uint16;
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PlanarU32(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
	ci.indexType = IndexType::Uint32;
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PositionPlanarU8(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
	ci.indexType = IndexType::Uint8;
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PositionPlanarU16(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
	ci.indexType = IndexType::Uint16;
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PositionPlanarU32(Format format)
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
	ci.indexType = IndexType::Uint32;
	ci.AddPosition(format);
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::Interleaved()
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
	ci.indexType = IndexType::Undefined;
	ci.vertexBindingCount = 1; // Interleave attribute layout always has 1 vertex binding
	ci.AddPosition();
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::Planar()
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
	ci.indexType = IndexType::Undefined;
	ci.AddPosition();
	return ci;
}

GeometryCreateInfo GeometryCreateInfo::PositionPlanar()
{
	GeometryCreateInfo ci = {};
	ci.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR;
	ci.indexType = IndexType::Undefined;
	ci.AddPosition();
	return ci;
}

GeometryCreateInfo& GeometryCreateInfo::IndexType(vkr::IndexType indexType_)
{
	indexType = indexType_;
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::IndexTypeU8()
{
	return IndexType(IndexType::Uint8);
}

GeometryCreateInfo& GeometryCreateInfo::IndexTypeU16()
{
	return IndexType(IndexType::Uint16);
}

GeometryCreateInfo& GeometryCreateInfo::IndexTypeU32()
{
	return IndexType(IndexType::Uint32);
}

GeometryCreateInfo& GeometryCreateInfo::AddAttribute(VertexSemantic semantic, Format format)
{
	bool exists = false;
	for (uint32_t bindingIndex = 0; bindingIndex < vertexBindingCount; ++bindingIndex) {
		const VertexBinding& binding = vertexBindings[bindingIndex];
		for (uint32_t attrIndex = 0; attrIndex < binding.GetAttributeCount(); ++attrIndex) {
			const VertexAttribute* pAttribute = nullptr;
			binding.GetAttribute(attrIndex, &pAttribute);
			exists = (pAttribute->semantic == semantic);
			if (exists) {
				break;
			}
		}
		if (exists) {
			break;
		}
	}

	if (!exists) {
		uint32_t location = 0;
		for (uint32_t bindingIndex = 0; bindingIndex < vertexBindingCount; ++bindingIndex) {
			const VertexBinding& binding = vertexBindings[bindingIndex];
			location += binding.GetAttributeCount();
		}

		VertexAttribute attribute = {};
		attribute.semanticName = ToString(semantic);
		attribute.location = location;
		attribute.format = format;
		attribute.binding = VALUE_IGNORED; // Determined below
		attribute.offset = APPEND_OFFSET_ALIGNED;
		attribute.inputRate = VertexInputRate::Vertex;
		attribute.semantic = semantic;

		switch (vertexAttributeLayout) {
		case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED:
			attribute.binding = 0;
			vertexBindings[0].AppendAttribute(attribute);
			vertexBindingCount = 1;
			break;
		case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR:
			ASSERT_MSG(vertexBindingCount < MaxVertexBindings, "max vertex bindings exceeded");
			vertexBindings[vertexBindingCount].AppendAttribute(attribute);
			vertexBindings[vertexBindingCount].SetBinding(vertexBindingCount);
			vertexBindingCount += 1;
			break;
		case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR:
			if (semantic == VertexSemantic::Position) {
				attribute.binding = 0;
				vertexBindings[0].AppendAttribute(attribute);
			}
			else {
				vertexBindings[1].AppendAttribute(attribute);
				vertexBindings[1].SetBinding(1);
			}
			vertexBindingCount = 2;
			break;
		default:
			Fatal("unsupported vertex attribute layout type");
			break;
		}
	}
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::AddPosition(Format format)
{
	AddAttribute(VertexSemantic::Position, format);
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::AddNormal(Format format)
{
	AddAttribute(VertexSemantic::Normal, format);
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::AddColor(Format format)
{
	AddAttribute(VertexSemantic::Color, format);
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::AddTexCoord(Format format)
{
	AddAttribute(VertexSemantic::Texcoord, format);
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::AddTangent(Format format)
{
	AddAttribute(VertexSemantic::Tangent, format);
	return *this;
}

GeometryCreateInfo& GeometryCreateInfo::AddBitangent(Format format)
{
	AddAttribute(VertexSemantic::Bitangent, format);
	return *this;
}

uint32_t Geometry::Buffer::GetElementCount() const
{
	if (mElementSize == 0) return 0;
	size_t sizeOfData = mUsedSize;
	// round up for the case of interleaved buffers
	uint32_t count = static_cast<uint32_t>(std::ceil(static_cast<double>(sizeOfData) / static_cast<double>(mElementSize)));
	return count;
}

Result Geometry::InternalCtor()
{
	switch (mCreateInfo.vertexAttributeLayout) {
	case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED:
		mVDProcessor = &sVDProcessorInterleaved;
		mVDProcessorCompressed = &sVDProcessorInterleavedCompressed;
		break;
	case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR:
		mVDProcessor = &sVDProcessorPlanar;
		mVDProcessorCompressed = &sVDProcessorPlanarCompressed;
		break;
	case GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR:
		mVDProcessor = &sVDProcessorPositionPlanar;
		mVDProcessorCompressed = &sVDProcessorPositionPlanarCompressed;
		break;
	default:
		Fatal("unsupported vertex attribute layout type");
		return ERROR_FAILED;
	}

	if (!mVDProcessor->Validate(this)) {
		return ERROR_FAILED;
	}

	if (mCreateInfo.indexType != IndexType::Undefined)
	{
		uint32_t elementSize = IndexTypeSize(mCreateInfo.indexType);

		if (elementSize == 0)
		{
			// Shouldn't occur unless there's corruption
			Fatal("could not determine index type size");
			return ERROR_FAILED;
		}

		mIndexBuffer = Buffer(BUFFER_TYPE_INDEX, elementSize);
	}

	return mVDProcessor->UpdateVertexBuffer(this);
}

Result Geometry::Create(const GeometryCreateInfo& createInfo, Geometry* pGeometry)
{
	ASSERT_NULL_ARG(pGeometry);

	*pGeometry = Geometry();

	if (createInfo.primitiveTopology != PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	{
		Fatal("only triangle list is supported");
		return ERROR_INVALID_CREATE_ARGUMENT;
	}

	if (createInfo.vertexBindingCount == 0)
	{
		Fatal("must have at least one vertex binding");
		return ERROR_INVALID_CREATE_ARGUMENT;
	}

	pGeometry->mCreateInfo = createInfo;

	Result ppxres = pGeometry->InternalCtor();
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

Result Geometry::Create(const GeometryCreateInfo& createInfo, const TriMesh& mesh, Geometry* pGeometry)
{
	// Create geometry
	Result ppxres = Geometry::Create(createInfo, pGeometry);
	if (Failed(ppxres))
	{
		Fatal("failed creating geometry");
		return ppxres;
	}

	// Target geometry WITHOUT index data
	if (createInfo.indexType == IndexType::Undefined)
	{
		// Mesh has index data
		if (mesh.GetIndexType() != IndexType::Undefined)
		{
			// Iterate through the meshes triangles and add vertex data for each triangle vertex
			uint32_t triCount = mesh.GetCountTriangles();
			for (uint32_t triIndex = 0; triIndex < triCount; ++triIndex)
			{
				uint32_t vtxIndex0 = VALUE_IGNORED;
				uint32_t vtxIndex1 = VALUE_IGNORED;
				uint32_t vtxIndex2 = VALUE_IGNORED;
				ppxres = mesh.GetTriangle(triIndex, vtxIndex0, vtxIndex1, vtxIndex2);
				if (Failed(ppxres))
				{
					Fatal("failed getting triangle indices at triIndex=" + std::to_string(triIndex));
					return ppxres;
				}

				// First vertex
				TriMeshVertexData vertexData0 = {};
				ppxres = mesh.GetVertexData(vtxIndex0, &vertexData0);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex0=" + std::to_string(vtxIndex0));
					return ppxres;
				}
				// Second vertex
				TriMeshVertexData vertexData1 = {};
				ppxres = mesh.GetVertexData(vtxIndex1, &vertexData1);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex1=" + std::to_string(vtxIndex1));
					return ppxres;
				}
				// Third vertex
				TriMeshVertexData vertexData2 = {};
				ppxres = mesh.GetVertexData(vtxIndex2, &vertexData2);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex2=" + std::to_string(vtxIndex2));
					return ppxres;
				}

				pGeometry->AppendVertexData(vertexData0);
				pGeometry->AppendVertexData(vertexData1);
				pGeometry->AppendVertexData(vertexData2);
			}
		}
		// Mesh does not have index data
		else
		{
			// Iterate through the meshes vertx data and add it to the geometry
			uint32_t vertexCount = mesh.GetCountPositions();
			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
				TriMeshVertexData vertexData = {};
				ppxres = mesh.GetVertexData(vertexIndex, &vertexData);
				if (Failed(ppxres)) {
					Fatal("failed getting vertex data at vertexIndex=" + std::to_string(vertexIndex));
					return ppxres;
				}
				pGeometry->AppendVertexData(vertexData);
			}
		}
	}
	// Target geometry WITH index data
	else
	{
		// Mesh has index data
		if (mesh.GetIndexType() != IndexType::Undefined)
		{
			// Iterate the meshes triangles and add the vertex indices
			uint32_t triCount = mesh.GetCountTriangles();
			for (uint32_t triIndex = 0; triIndex < triCount; ++triIndex)
			{
				uint32_t v0 = VALUE_IGNORED;
				uint32_t v1 = VALUE_IGNORED;
				uint32_t v2 = VALUE_IGNORED;
				ppxres = mesh.GetTriangle(triIndex, v0, v1, v2);
				if (Failed(ppxres))
				{
					Fatal("couldn't get triangle at triIndex=" + std::to_string(triIndex));
					return ppxres;
				}
				pGeometry->AppendIndicesTriangle(v0, v1, v2);
			}

			// Iterate through the meshes vertx data and add it to the geometry
			uint32_t vertexCount = mesh.GetCountPositions();
			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				TriMeshVertexData vertexData = {};
				ppxres = mesh.GetVertexData(vertexIndex, &vertexData);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vertexIndex=" + std::to_string(vertexIndex));
					return ppxres;
				}
				pGeometry->AppendVertexData(vertexData);
			}
		}
		// Mesh does not have index data
		else
		{
			// Use every 3 vertices as a triangle and add each as an indexed triangle
			uint32_t triCount = mesh.GetCountPositions() / 3;
			for (uint32_t triIndex = 0; triIndex < triCount; ++triIndex)
			{
				uint32_t vtxIndex0 = 3 * triIndex + 0;
				uint32_t vtxIndex1 = 3 * triIndex + 1;
				uint32_t vtxIndex2 = 3 * triIndex + 2;

				// First vertex
				TriMeshVertexData vertexData0 = {};
				ppxres = mesh.GetVertexData(vtxIndex0, &vertexData0);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex0=" + std::to_string(vtxIndex0));
					return ppxres;
				}
				// Second vertex
				TriMeshVertexData vertexData1 = {};
				ppxres = mesh.GetVertexData(vtxIndex1, &vertexData1);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex1=" + std::to_string(vtxIndex1));
					return ppxres;
				}
				// Third vertex
				TriMeshVertexData vertexData2 = {};
				ppxres = mesh.GetVertexData(vtxIndex2, &vertexData2);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex2=" + std::to_string(vtxIndex2));
					return ppxres;
				}

				// Will append indices if geometry has index buffer
				pGeometry->AppendTriangle(vertexData0, vertexData1, vertexData2);
			}
		}
	}

	return SUCCESS;
}

Result Geometry::Create(const GeometryCreateInfo& createInfo, const WireMesh& mesh, Geometry* pGeometry)
{
	// Create geometry
	Result ppxres = Geometry::Create(createInfo, pGeometry);
	if (Failed(ppxres)) {
		Fatal("failed creating geometry");
		return ppxres;
	}

	// Target geometry WITHOUT index data
	if (createInfo.indexType == IndexType::Undefined) {
		// Mesh has index data
		if (mesh.GetIndexType() != IndexType::Undefined) {
			// Iterate through the meshes edges and add vertex data for each edge vertex
			uint32_t edgeCount = mesh.GetCountEdges();
			for (uint32_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
				uint32_t vtxIndex0 = VALUE_IGNORED;
				uint32_t vtxIndex1 = VALUE_IGNORED;
				ppxres = mesh.GetEdge(edgeIndex, vtxIndex0, vtxIndex1);
				if (Failed(ppxres)) {
					Fatal("failed getting triangle indices at edgeIndex=" + std::to_string(edgeIndex));
					return ppxres;
				}

				// First vertex
				WireMeshVertexData vertexData0 = {};
				ppxres = mesh.GetVertexData(vtxIndex0, &vertexData0);
				if (Failed(ppxres)) {
					Fatal("failed getting vertex data at vtxIndex0=" + std::to_string(vtxIndex0));
					return ppxres;
				}
				// Second vertex
				WireMeshVertexData vertexData1 = {};
				ppxres = mesh.GetVertexData(vtxIndex1, &vertexData1);
				if (Failed(ppxres)) {
					Fatal("failed getting vertex data at vtxIndex1=" + std::to_string(vtxIndex1));
					return ppxres;
				}

				pGeometry->AppendVertexData(vertexData0);
				pGeometry->AppendVertexData(vertexData1);
			}
		}
		// Mesh does not have index data
		else {
			// Iterate through the meshes vertx data and add it to the geometry
			uint32_t vertexCount = mesh.GetCountPositions();
			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
				WireMeshVertexData vertexData = {};
				ppxres = mesh.GetVertexData(vertexIndex, &vertexData);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vertexIndex=" + std::to_string(vertexIndex));
					return ppxres;
				}
				pGeometry->AppendVertexData(vertexData);
			}
		}
	}
	// Target geometry WITH index data
	else
	{
		// Mesh has index data
		if (mesh.GetIndexType() != IndexType::Undefined)
		{
			// Iterate the meshes edges and add the vertex indices
			uint32_t edgeCount = mesh.GetCountEdges();
			for (uint32_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex)
			{
				uint32_t v0 = VALUE_IGNORED;
				uint32_t v1 = VALUE_IGNORED;
				ppxres = mesh.GetEdge(edgeIndex, v0, v1);
				if (Failed(ppxres))
				{
					Fatal("couldn't get triangle at edgeIndex=" + std::to_string(edgeIndex));
					return ppxres;
				}
				pGeometry->AppendIndicesEdge(v0, v1);
			}

			// Iterate through the meshes vertex data and add it to the geometry
			uint32_t vertexCount = mesh.GetCountPositions();
			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				WireMeshVertexData vertexData = {};
				ppxres = mesh.GetVertexData(vertexIndex, &vertexData);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vertexIndex=" + std::to_string(vertexIndex));
					return ppxres;
				}
				pGeometry->AppendVertexData(vertexData);
			}
		}
		// Mesh does not have index data
		else {
			// Use every 2 vertices as an edge and add each as an indexed edge
			uint32_t edgeCount = mesh.GetCountPositions() / 2;
			for (uint32_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
				uint32_t vtxIndex0 = 2 * edgeIndex + 0;
				uint32_t vtxIndex1 = 2 * edgeIndex + 1;

				// First vertex
				WireMeshVertexData vertexData0 = {};
				ppxres = mesh.GetVertexData(vtxIndex0, &vertexData0);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex0=" + std::to_string(vtxIndex0));
					return ppxres;
				}
				// Second vertex
				WireMeshVertexData vertexData1 = {};
				ppxres = mesh.GetVertexData(vtxIndex1, &vertexData1);
				if (Failed(ppxres))
				{
					Fatal("failed getting vertex data at vtxIndex1=" + std::to_string(vtxIndex1));
					return ppxres;
				}

				// Will append indices if geometry has index buffer
				pGeometry->AppendEdge(vertexData0, vertexData1);
			}
		}
	}

	return SUCCESS;
}

Result Geometry::Create(const TriMesh& mesh, Geometry* pGeometry)
{
	GeometryCreateInfo createInfo    = {};
	createInfo.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
	createInfo.indexType             = mesh.GetIndexType();
	createInfo.primitiveTopology     = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	createInfo.AddPosition();

	if (mesh.HasColors())     createInfo.AddColor();
	if (mesh.HasNormals())    createInfo.AddNormal();
	if (mesh.HasTexCoords())  createInfo.AddTexCoord();
	if (mesh.HasTangents())   createInfo.AddTangent();
	if (mesh.HasBitangents()) createInfo.AddBitangent();

	Result ppxres = Create(createInfo, mesh, pGeometry);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

Result Geometry::Create(const WireMesh& mesh, Geometry* pGeometry)
{
	GeometryCreateInfo createInfo = {};
	createInfo.vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR;
	createInfo.indexType = mesh.GetIndexType();
	createInfo.primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	createInfo.AddPosition();

	if (mesh.HasColors()) createInfo.AddColor();

	Result ppxres = Create(createInfo, mesh, pGeometry);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

const VertexBinding* Geometry::GetVertexBinding(uint32_t index) const
{
	const VertexBinding* pBinding = nullptr;
	if (index < mCreateInfo.vertexBindingCount) {
		pBinding = &mCreateInfo.vertexBindings[index];
	}
	return pBinding;
}

void Geometry::SetIndexBuffer(const Geometry::Buffer& newIndexBuffer)
{
	ASSERT_MSG(newIndexBuffer.GetType() == mIndexBuffer.GetType(), "New index buffer is not the same type");
	ASSERT_MSG(newIndexBuffer.GetElementSize() == mIndexBuffer.GetElementSize(), "New index buffer does not have the same element size");
	mIndexBuffer = newIndexBuffer;
}

uint32_t Geometry::GetIndexCount() const
{
	uint32_t count = 0;
	if (mCreateInfo.indexType != IndexType::Undefined) {
		count = mIndexBuffer.GetElementCount();
	}
	return count;
}

uint32_t Geometry::GetVertexCount() const
{
	ASSERT_MSG(mVDProcessor != nullptr, "Geometry is not initialized");
	return mVDProcessor->GetVertexCount(this);
}

const Geometry::Buffer* Geometry::GetVertexBuffer(uint32_t index) const
{
	ASSERT_MSG(IsIndexInRange(index, mVertexBuffers), "Vertex buffer does not exist at index: " + std::to_string(index));
	return &mVertexBuffers[index];
}

Geometry::Buffer* Geometry::GetVertexBuffer(uint32_t index)
{
	ASSERT_MSG(IsIndexInRange(index, mVertexBuffers), "Vertex buffer does not exist at index: " + std::to_string(index));
	return &mVertexBuffers[index];
}

uint32_t Geometry::GetLargestBufferSize() const
{
	uint32_t size = mIndexBuffer.GetSize();
	for (size_t i = 0; i < mVertexBuffers.size(); ++i) {
		size = std::max(size, mVertexBuffers[i].GetSize());
	}
	return size;
}

void Geometry::AppendIndex(uint32_t idx)
{
	if (mCreateInfo.indexType == IndexType::Uint16) mIndexBuffer.Append(static_cast<uint16_t>(idx));
	else if (mCreateInfo.indexType == IndexType::Uint32) mIndexBuffer.Append(idx);
	else if (mCreateInfo.indexType == IndexType::Uint8) mIndexBuffer.Append(static_cast<uint8_t>(idx));
}

void Geometry::AppendIndicesTriangle(uint32_t idx0, uint32_t idx1, uint32_t idx2)
{
	if (mCreateInfo.indexType == IndexType::Uint16) {
		mIndexBuffer.Append(static_cast<uint16_t>(idx0));
		mIndexBuffer.Append(static_cast<uint16_t>(idx1));
		mIndexBuffer.Append(static_cast<uint16_t>(idx2));
	}
	else if (mCreateInfo.indexType == IndexType::Uint32) {
		mIndexBuffer.Append(idx0);
		mIndexBuffer.Append(idx1);
		mIndexBuffer.Append(idx2);
	}
	else if (mCreateInfo.indexType == IndexType::Uint8) {
		mIndexBuffer.Append(static_cast<uint8_t>(idx0));
		mIndexBuffer.Append(static_cast<uint8_t>(idx1));
		mIndexBuffer.Append(static_cast<uint8_t>(idx2));
	}
}

void Geometry::AppendIndicesEdge(uint32_t idx0, uint32_t idx1)
{
	if (mCreateInfo.indexType == IndexType::Uint16) {
		mIndexBuffer.Append(static_cast<uint16_t>(idx0));
		mIndexBuffer.Append(static_cast<uint16_t>(idx1));
	}
	else if (mCreateInfo.indexType == IndexType::Uint32) {
		mIndexBuffer.Append(idx0);
		mIndexBuffer.Append(idx1);
	}
	else if (mCreateInfo.indexType == IndexType::Uint8) {
		mIndexBuffer.Append(static_cast<uint8_t>(idx0));
		mIndexBuffer.Append(static_cast<uint8_t>(idx1));
	}
}

void Geometry::AppendIndicesU32(uint32_t count, const uint32_t* pIndices)
{
	if (mCreateInfo.indexType != IndexType::Uint32)
	{
		Fatal("Can't append UINT32 indices to buffer of type: " + ToString(mCreateInfo.indexType));
		return;
	}
	mIndexBuffer.Append(count, pIndices);
}

uint32_t Geometry::AppendVertexData(const TriMeshVertexData& vtx)
{
	return mVDProcessor->AppendVertexData(this, vtx);
}

uint32_t Geometry::AppendVertexData(const TriMeshVertexDataCompressed& vtx)
{
	return mVDProcessorCompressed->AppendVertexData(this, vtx);
}

uint32_t Geometry::AppendVertexData(const WireMeshVertexData& vtx)
{
	return mVDProcessor->AppendVertexData(this, vtx);
}

void Geometry::AppendTriangle(const TriMeshVertexData& vtx0, const TriMeshVertexData& vtx1, const TriMeshVertexData& vtx2)
{
	uint32_t n0 = AppendVertexData(vtx0) - 1;
	uint32_t n1 = AppendVertexData(vtx1) - 1;
	uint32_t n2 = AppendVertexData(vtx2) - 1;

	// Will only append indices if geometry has an index buffer
	AppendIndicesTriangle(n0, n1, n2);
}

void Geometry::AppendEdge(const WireMeshVertexData& vtx0, const WireMeshVertexData& vtx1)
{
	uint32_t n0 = AppendVertexData(vtx0) - 1;
	uint32_t n1 = AppendVertexData(vtx1) - 1;

	// Will only append indices if geometry has an index buffer
	AppendIndicesEdge(n0, n1);
}

#pragma endregion

#pragma region grfx util

namespace vkrUtil
{

	Format ToGrfxFormat(Bitmap::Format value)
	{
		switch (value) {
		case Bitmap::FORMAT_R_UINT32:
		case Bitmap::FORMAT_RG_UINT32:
		case Bitmap::FORMAT_RGB_UINT32:
		case Bitmap::FORMAT_RGBA_UINT32:
		case Bitmap::FORMAT_UNDEFINED:
		default: break;
		case Bitmap::FORMAT_R_UINT8: return Format::R8_UNORM; break;
		case Bitmap::FORMAT_RG_UINT8: return Format::R8G8_UNORM; break;
		case Bitmap::FORMAT_RGB_UINT8: return Format::R8G8B8_UNORM; break;
		case Bitmap::FORMAT_RGBA_UINT8: return Format::R8G8B8A8_UNORM; break;
		case Bitmap::FORMAT_R_UINT16: return Format::R16_UNORM; break;
		case Bitmap::FORMAT_RG_UINT16: return Format::R16G16_UNORM; break;
		case Bitmap::FORMAT_RGB_UINT16: return Format::R16G16B16_UNORM; break;
		case Bitmap::FORMAT_RGBA_UINT16: return Format::R16G16B16A16_UNORM; break;
		case Bitmap::FORMAT_R_FLOAT: return Format::R32_FLOAT; break;
		case Bitmap::FORMAT_RG_FLOAT: return Format::R32G32_FLOAT; break;
		case Bitmap::FORMAT_RGB_FLOAT: return Format::R32G32B32_FLOAT; break;
		case Bitmap::FORMAT_RGBA_FLOAT: return Format::R32G32B32A32_FLOAT; break;
		}
		return Format::Undefined;
	}

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4061)
#endif

	Format ToGrfxFormat(gli::format value)
	{
		switch (value) {
		case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return Format::BC1_RGB_UNORM;
		case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return Format::BC1_RGB_SRGB;
		case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return Format::BC1_RGBA_UNORM;
		case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return Format::BC1_RGBA_SRGB;
		case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return Format::BC2_SRGB;
		case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return Format::BC2_UNORM;
		case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return Format::BC3_SRGB;
		case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return Format::BC3_UNORM;
		case gli::FORMAT_R_ATI1N_UNORM_BLOCK8: return Format::BC4_UNORM;
		case gli::FORMAT_R_ATI1N_SNORM_BLOCK8: return Format::BC4_SNORM;
		case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16: return Format::BC5_UNORM;
		case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16: return Format::BC5_SNORM;
		case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16: return Format::BC6H_UFLOAT;
		case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16: return Format::BC6H_SFLOAT;
		case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return Format::BC7_UNORM;
		case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return Format::BC7_SRGB;
		default:
			return Format::Undefined;
		}
	}

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

	// -------------------------------------------------------------------------------------------------

	Result CopyBitmapToImage(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Image* pImage,
		uint32_t            mipLevel,
		uint32_t            arrayLayer,
		ResourceState stateBefore,
		ResourceState stateAfter)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pBitmap);
		ASSERT_NULL_ARG(pImage);

		Result ppxres = ERROR_FAILED;

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// This is the number of bytes we're going to copy per row.
		uint32_t rowCopySize = pBitmap->GetWidth() * pBitmap->GetPixelStride();

		// When copying from a buffer to a image/texture, D3D12 requires that the rows
		// stored in the source buffer (aka staging buffer) are aligned to 256 bytes.
		// Vulkan does not have this requirement. So for the staging buffer, we want
		// to enforce the alignment for D3D12 but not for Vulkan.
		//
		uint32_t apiRowStrideAligement = /*IsDx12(pQueue->GetDevice()->GetApi()) ? PPX_D3D12_TEXTURE_DATA_PITCH_ALIGNMENT :*/ 1;
		// The staging buffer's row stride alignemnt needs to be based off the bitmap's
		// width (i.e. the number of bytes we're going to copy) and not the bitmap's row
		// stride. The bitmap's may be padded beyond width * pixel stride.
		//
		uint32_t stagingBufferRowStride = RoundUp<uint32_t>(rowCopySize, apiRowStrideAligement);

		// Create staging buffer
		BufferPtr stagingBuffer;
		{
			uint64_t bufferSize = stagingBufferRowStride * pBitmap->GetHeight();

			BufferCreateInfo ci = {};
			ci.size = bufferSize;
			ci.usageFlags.bits.transferSrc = true;
			ci.memoryUsage = MemoryUsage::CPUToGPU;

			ppxres = pQueue->GetDevice()->CreateBuffer(ci, &stagingBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(stagingBuffer);

			// Map and copy to staging buffer
			void* pBufferAddress = nullptr;
			ppxres = stagingBuffer->MapMemory(0, &pBufferAddress);
			if (Failed(ppxres)) {
				return ppxres;
			}

			const char* pSrc = pBitmap->GetData();
			char* pDst = static_cast<char*>(pBufferAddress);
			const uint32_t srcRowStride = pBitmap->GetRowStride();
			const uint32_t dstRowStride = stagingBufferRowStride;
			for (uint32_t y = 0; y < pBitmap->GetHeight(); ++y) {
				memcpy(pDst, pSrc, rowCopySize);
				pSrc += srcRowStride;
				pDst += dstRowStride;
			}

			stagingBuffer->UnmapMemory();
		}

		// Copy info
		BufferToImageCopyInfo copyInfo = {};
		copyInfo.srcBuffer.imageWidth = pBitmap->GetWidth();
		copyInfo.srcBuffer.imageHeight = pBitmap->GetHeight();
		copyInfo.srcBuffer.imageRowStride = stagingBufferRowStride;
		copyInfo.srcBuffer.footprintOffset = 0;
		copyInfo.srcBuffer.footprintWidth = pBitmap->GetWidth();
		copyInfo.srcBuffer.footprintHeight = pBitmap->GetHeight();
		copyInfo.srcBuffer.footprintDepth = 1;
		copyInfo.dstImage.mipLevel = mipLevel;
		copyInfo.dstImage.arrayLayer = arrayLayer;
		copyInfo.dstImage.arrayLayerCount = 1;
		copyInfo.dstImage.x = 0;
		copyInfo.dstImage.y = 0;
		copyInfo.dstImage.z = 0;
		copyInfo.dstImage.width = pBitmap->GetWidth();
		copyInfo.dstImage.height = pBitmap->GetHeight();
		copyInfo.dstImage.depth = 1;

		// Copy to GPU image
		ppxres = pQueue->CopyBufferToImage(
			std::vector<BufferToImageCopyInfo>{copyInfo},
			stagingBuffer,
			pImage,
			mipLevel,
			1,
			arrayLayer,
			1,
			stateBefore,
			stateAfter);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result CreateImageFromBitmap(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Image** ppImage,
		const ImageOptions& options)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pBitmap);
		ASSERT_NULL_ARG(ppImage);

		Result ppxres = ERROR_FAILED;

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// Cap mip level count
		uint32_t maxMipLevelCount = Mipmap::CalculateLevelCount(pBitmap->GetWidth(), pBitmap->GetHeight());
		uint32_t mipLevelCount = std::min<uint32_t>(options.mMipLevelCount, maxMipLevelCount);

		// Create target image
		ImagePtr targetImage;
		{
			ImageCreateInfo ci = {};
			ci.type = ImageType::Image2D;
			ci.width = pBitmap->GetWidth();
			ci.height = pBitmap->GetHeight();
			ci.depth = 1;
			ci.format = ToGrfxFormat(pBitmap->GetFormat());
			ci.sampleCount = SampleCount::Sample1;
			ci.mipLevelCount = mipLevelCount;
			ci.arrayLayerCount = 1;
			ci.usageFlags.bits.transferDst = true;
			ci.usageFlags.bits.sampled = true;
			ci.memoryUsage = MemoryUsage::GPUOnly;
			ci.initialState = ResourceState::ShaderResource;

			ci.usageFlags.flags |= options.mAdditionalUsage;

			ppxres = pQueue->GetDevice()->CreateImage(ci, &targetImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetImage);
		}

		// Since this mipmap is temporary, it's safe to use the static pool.
		Mipmap mipmap = Mipmap(*pBitmap, mipLevelCount, /* useStaticPool= */ true);
		if (!mipmap.IsOk()) {
			return ERROR_FAILED;
		}

		// Copy mips to image
		for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
			const Bitmap* pMip = mipmap.GetMip(mipLevel);

			ppxres = CopyBitmapToImage(
				pQueue,
				pMip,
				targetImage,
				mipLevel,
				0,
				ResourceState::ShaderResource,
				ResourceState::ShaderResource);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Change ownership to reference so object doesn't get destroyed
		targetImage->SetOwnership(Ownership::Reference);

		// Assign output
		*ppImage = targetImage;

		return SUCCESS;
	}

	Result CreateImageFromBitmapGpu(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Image** ppImage,
		const ImageOptions& options)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pBitmap);
		ASSERT_NULL_ARG(ppImage);

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		Result ppxres = ERROR_FAILED;

		// Cap mip level count
		uint32_t maxMipLevelCount = Mipmap::CalculateLevelCount(pBitmap->GetWidth(), pBitmap->GetHeight());
		uint32_t mipLevelCount = std::min<uint32_t>(options.mMipLevelCount, maxMipLevelCount);

		// Create target image
		ImagePtr targetImage;
		{
			ImageCreateInfo ci = {};
			ci.type = ImageType::Image2D;
			ci.width = pBitmap->GetWidth();
			ci.height = pBitmap->GetHeight();
			ci.depth = 1;
			ci.format = ToGrfxFormat(pBitmap->GetFormat());
			ci.sampleCount = SampleCount::Sample1;
			ci.mipLevelCount = mipLevelCount;
			ci.arrayLayerCount = 1;
			ci.usageFlags.bits.transferDst = true;
			ci.usageFlags.bits.transferSrc = true; // For CS
			ci.usageFlags.bits.sampled = true;
			ci.usageFlags.bits.storage = true; // For CS
			ci.memoryUsage = MemoryUsage::GPUOnly;
			ci.initialState = ResourceState::ShaderResource;

			ci.usageFlags.flags |= options.mAdditionalUsage;

			ppxres = pQueue->GetDevice()->CreateImage(ci, &targetImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetImage);
		}

		// Copy first level mip into image
		ppxres = CopyBitmapToImage(
			pQueue,
			pBitmap,
			targetImage,
			0,
			0,
			ResourceState::ShaderResource,
			ResourceState::ShaderResource);

		if (Failed(ppxres)) {
			return ppxres;
		}

		// Transition image mips from 1 to rest to general layout
		{
			// Create a command buffer
			CommandBufferPtr cmdBuffer;
			CHECKED_CALL(pQueue->CreateCommandBuffer(&cmdBuffer));
			// Record command buffer
			CHECKED_CALL(cmdBuffer->Begin());
			cmdBuffer->TransitionImageLayout(targetImage, 1, mipLevelCount - 1, 0, 1, ResourceState::ShaderResource, ResourceState::General);
			CHECKED_CALL(cmdBuffer->End());
			// Submit to queue
			SubmitInfo submitInfo = {};
			submitInfo.commandBufferCount = 1;
			submitInfo.ppCommandBuffers = &cmdBuffer;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.ppWaitSemaphores = nullptr;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.ppSignalSemaphores = nullptr;
			submitInfo.pFence = nullptr;
			CHECKED_CALL(pQueue->Submit(&submitInfo));
		}

		// Requiered to setup compute shader
		ShaderModulePtr        computeShader;
		PipelineInterfacePtr   computePipelineInterface;
		ComputePipelinePtr     computePipeline;
		DescriptorSetLayoutPtr computeDescriptorSetLayout;
		DescriptorPoolPtr      descriptorPool;
		DescriptorSetPtr       computeDescriptorSet;
		BufferPtr              uniformBuffer;
		SamplerPtr             sampler;

		{ // Uniform buffer
			BufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.size = MINIMUM_UNIFORM_BUFFER_SIZE;
			bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
			bufferCreateInfo.memoryUsage = MemoryUsage::CPUToGPU;
			CHECKED_CALL(pQueue->GetDevice()->CreateBuffer(bufferCreateInfo, &uniformBuffer));
		}

		{ // Sampler
			SamplerCreateInfo samplerCreateInfo = {};
			CHECKED_CALL(pQueue->GetDevice()->CreateSampler(samplerCreateInfo, &sampler));
		}

		{ // Descriptors
			DescriptorPoolCreateInfo poolCreateInfo = {};
			poolCreateInfo.storageImage = 1;
			poolCreateInfo.uniformBuffer = 1;
			poolCreateInfo.sampledImage = 1;
			poolCreateInfo.sampler = 1;

			CHECKED_CALL(pQueue->GetDevice()->CreateDescriptorPool(poolCreateInfo, &descriptorPool));

			{ // Shader inputs
				DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
				layoutCreateInfo.bindings.push_back(DescriptorBinding(0, DescriptorType::StorageImage));
				layoutCreateInfo.bindings.push_back(DescriptorBinding(1, DescriptorType::UniformBuffer));
				layoutCreateInfo.bindings.push_back(DescriptorBinding(2, DescriptorType::SampledImage));
				layoutCreateInfo.bindings.push_back(DescriptorBinding(3, DescriptorType::Sampler));

				CHECKED_CALL(pQueue->GetDevice()->CreateDescriptorSetLayout(layoutCreateInfo, &computeDescriptorSetLayout));

				CHECKED_CALL(pQueue->GetDevice()->AllocateDescriptorSet(descriptorPool, computeDescriptorSetLayout, &computeDescriptorSet));

				WriteDescriptor write = {};
				write.binding = 1;
				write.type = DescriptorType::UniformBuffer;
				write.bufferOffset = 0;
				write.bufferRange = WHOLE_SIZE;
				write.buffer = uniformBuffer;
				CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));

				write = {};
				write.binding = 3;
				write.type = DescriptorType::Sampler;
				write.sampler = sampler;
				CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));
			}
		}

		// Compute pipeline
		{
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4365)
#endif
			std::vector<char> bytecode;
			bytecode = { std::begin(GenerateMipShaderVK), std::end(GenerateMipShaderVK) };
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
			
			ASSERT_MSG(!bytecode.empty(), "CS shader bytecode load failed");
			ShaderModuleCreateInfo shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(pQueue->GetDevice()->CreateShaderModule(shaderCreateInfo, &computeShader));

			PipelineInterfaceCreateInfo piCreateInfo = {};
			piCreateInfo.setCount = 1;
			piCreateInfo.sets[0].set = 0;
			piCreateInfo.sets[0].layout = computeDescriptorSetLayout;
			CHECKED_CALL(pQueue->GetDevice()->CreatePipelineInterface(piCreateInfo, &computePipelineInterface));

			ComputePipelineCreateInfo cpCreateInfo = {};
			cpCreateInfo.CS = { computeShader.Get(), "CSMain" };
			cpCreateInfo.pipelineInterface = computePipelineInterface;
			CHECKED_CALL(pQueue->GetDevice()->CreateComputePipeline(cpCreateInfo, &computePipeline));
		}

		// Prepare data for CS
		uint32_t srcCurrentWidth = pBitmap->GetWidth();
		uint32_t srcCurrentHeight = pBitmap->GetHeight();

		// For the uniform (constant) data
		struct alignas(16) ShaderConstantData
		{
			float texel_size[2]; // 1.0 / srcTex.Dimensions
			int   src_mip_level;
			// Case to filter according the parity of the dimensions in the src texture.
			// Must be one of 0, 1, 2 or 3
			// See CSMain function bellow
			int dimension_case;
			// Ignored for now, if we want to use a different filter strategy. Current one is bi-linear filter
			int filter_option;
			int unused[3];
		};

		// Generate the rest of the mips
		for (uint32_t i = 1; i < mipLevelCount; ++i)
		{
			StorageImageViewPtr storageImageView;
			SampledImageViewPtr sampledImageView;

			{ // Pass uniform data into shader
				ShaderConstantData constants;
				// Calculate current texel size
				constants.texel_size[0] = 1.0f / float(srcCurrentWidth);
				constants.texel_size[1] = 1.0f / float(srcCurrentHeight);
				// Calculate current dimension case
				// If width is even
				if ((srcCurrentWidth % 2) == 0) {
					// Test the height
					constants.dimension_case = (srcCurrentHeight % 2) == 0 ? 0 : 1;
				}
				else { // width is odd
					// Test the height
					constants.dimension_case = (srcCurrentHeight % 2) == 0 ? 2 : 3;
				}
				constants.src_mip_level = static_cast<int>(i) - 1; // We calculate mip level i with level i - 1
				constants.filter_option = 1;     // Ignored for now, defaults to bilinear
				void* pData = nullptr;
				CHECKED_CALL(uniformBuffer->MapMemory(0, &pData));
				memcpy(pData, &constants, sizeof(constants));
				uniformBuffer->UnmapMemory();
			}

			{ // Storage Image view
				StorageImageViewCreateInfo storageViewCreateInfo = StorageImageViewCreateInfo::GuessFromImage(targetImage);
				storageViewCreateInfo.mipLevel = i;
				storageViewCreateInfo.mipLevelCount = 1;

				CHECKED_CALL(pQueue->GetDevice()->CreateStorageImageView(storageViewCreateInfo, &storageImageView));

				WriteDescriptor write = {};
				write.binding = 0;
				write.type = DescriptorType::StorageImage;
				write.imageView = storageImageView;
				CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));
			}

			{ // Sampler Image View
				SampledImageViewCreateInfo sampledViewCreateInfo = SampledImageViewCreateInfo::GuessFromImage(targetImage);
				sampledViewCreateInfo.mipLevel = i - 1;
				sampledViewCreateInfo.mipLevelCount = 1;

				CHECKED_CALL(pQueue->GetDevice()->CreateSampledImageView(sampledViewCreateInfo, &sampledImageView));

				WriteDescriptor write = {};
				write.binding = 2;
				write.type = DescriptorType::SampledImage;
				write.imageView = sampledImageView;
				CHECKED_CALL(computeDescriptorSet->UpdateDescriptors(1, &write));
			}

			{ // Create a command buffer
				CommandBufferPtr cmdBuffer;
				CHECKED_CALL(pQueue->CreateCommandBuffer(&cmdBuffer));
				// Record command buffer
				CHECKED_CALL(cmdBuffer->Begin());
				cmdBuffer->BindComputeDescriptorSets(computePipelineInterface, 1, &computeDescriptorSet);
				cmdBuffer->BindComputePipeline(computePipeline);
				// Update width and height for the next iteration
				srcCurrentWidth = srcCurrentWidth > 1 ? srcCurrentWidth / 2 : 1;
				srcCurrentHeight = srcCurrentHeight > 1 ? srcCurrentHeight / 2 : 1;
				// Launch the CS once per dst size (which is half of src size)
				cmdBuffer->Dispatch(srcCurrentWidth, srcCurrentHeight, 1);
				CHECKED_CALL(cmdBuffer->End());
				// Submitt to queue
				SubmitInfo submitInfo = {};
				submitInfo.commandBufferCount = 1;
				submitInfo.ppCommandBuffers = &cmdBuffer;
				submitInfo.waitSemaphoreCount = 0;
				submitInfo.ppWaitSemaphores = nullptr;
				submitInfo.signalSemaphoreCount = 0;
				submitInfo.ppSignalSemaphores = nullptr;
				submitInfo.pFence = nullptr;

				CHECKED_CALL(pQueue->Submit(&submitInfo));
				CHECKED_CALL(pQueue->WaitIdle());
			}

			{ // Transition i-th mip back to shader resource
				// Create a command buffer
				CommandBufferPtr cmdBuffer;
				CHECKED_CALL(pQueue->CreateCommandBuffer(&cmdBuffer));
				// Record into command buffer
				CHECKED_CALL(cmdBuffer->Begin());
				cmdBuffer->TransitionImageLayout(targetImage, i, 1, 0, 1, ResourceState::General, ResourceState::ShaderResource);
				CHECKED_CALL(cmdBuffer->End());
				// Submitt to queue
				SubmitInfo submitInfo = {};
				submitInfo.commandBufferCount = 1;
				submitInfo.ppCommandBuffers = &cmdBuffer;
				submitInfo.waitSemaphoreCount = 0;
				submitInfo.ppWaitSemaphores = nullptr;
				submitInfo.signalSemaphoreCount = 0;
				submitInfo.ppSignalSemaphores = nullptr;
				submitInfo.pFence = nullptr;
				CHECKED_CALL(pQueue->Submit(&submitInfo));
				CHECKED_CALL(pQueue->WaitIdle());
			}
		}

		// Change ownership to reference so object doesn't get destroyed
		targetImage->SetOwnership(Ownership::Reference);

		// Assign output
		*ppImage = targetImage;

		return SUCCESS;
	}

	bool IsDDSFile(const std::filesystem::path& path)
	{
		return (std::strstr(path.string().c_str(), ".dds") != nullptr || std::strstr(path.string().c_str(), ".ktx") != nullptr);
	}

	struct MipLevel
	{
		uint32_t width;
		uint32_t height;
		uint32_t bufferWidth;
		uint32_t bufferHeight;
		uint32_t srcRowStride;
		uint32_t dstRowStride;
		size_t   offset;
	};

	Result CreateImageFromCompressedImage(
		Queue* pQueue,
		const gli::texture& image,
		Image** ppImage,
		const ImageOptions& options)
	{
		Result ppxres;

		Print("Target type: " + ToString(image.target()) + "\n");
		Print("Format: " + ToString(image.format()) + "\n");
		Print("Swizzles: " + std::to_string(image.swizzles()[0]) + std::string(", ") + std::to_string(image.swizzles()[1]) + ", " + std::to_string(image.swizzles()[2]) + ", " + std::to_string(image.swizzles()[3]) + "\n");
		Print("Layer information:\n"
			+ std::string("\tBase layer: ") + std::to_string(image.base_layer()) + "\n"
			+ std::string("\tMax layer: ") + std::to_string(image.max_layer()) + "\n"
			+ std::string("\t# of layers: ") + std::to_string(image.layers()) + "\n");
		Print("Face information:\n"
			+ std::string("\tBase face: ") + std::to_string(image.base_face()) + "\n"
			+ std::string("\tMax face: ") + std::to_string(image.max_face()) + "\n"
			+ std::string("\t# of faces: ") + std::to_string(image.faces()) + "\n");
		Print("Level information:\n"
			+ std::string("\tBase level: ") + std::to_string(image.base_level()) + "\n"
			+ std::string("\tMax level: ") + std::to_string(image.max_level()) + "\n"
			+ std::string("\t# of levels: ") + std::to_string(image.levels()) + "\n");
		Print("Image extents by level:\n");
		for (gli::texture::size_type level = 0; level < image.levels(); level++)
		{
			Print("\textent(level == " + std::to_string(level) + "): [" + std::to_string(image.extent(level)[0]) + ", " + std::to_string(image.extent(level)[1]) + ", " + std::to_string(image.extent(level)[2]) + "]\n");
		}
		Print("Total image size (bytes): " + std::to_string(image.size()) + "\n");
		Print("Image size by level:");
		for (gli::texture::size_type i = 0; i < image.levels(); i++)
		{
			Print("\tsize(level == " + std::to_string(i) + "): " + std::to_string(image.size(i)) + "\n");
		}
		//Print("Image data pointer: " + image.data() + "\n");

		ASSERT_MSG((image.target() == gli::TARGET_2D), "Expecting a 2D DDS image.");

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// Cap mip level count
		const Format format = ToGrfxFormat(image.format());
		const uint32_t     maxMipLevelCount = std::min<uint32_t>(options.mMipLevelCount, static_cast<uint32_t>(image.levels()));
		const uint32_t     imageWidth = static_cast<uint32_t>(image.extent(0)[0]);
		const uint32_t     imageHeight = static_cast<uint32_t>(image.extent(0)[1]);

		// Row stride and texture offset alignment to handle DX's requirements
		const uint32_t rowStrideAlignment =/* IsDx12(pQueue->GetDevice()->GetApi()) ? D3D12_TEXTURE_DATA_PITCH_ALIGNMENT :*/ 1;
		const uint32_t offsetAlignment = /*IsDx12(pQueue->GetDevice()->GetApi()) ? D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT :*/ 1;
		const uint32_t bytesPerTexel = GetFormatDescription(format)->bytesPerTexel;
		const uint32_t blockWidth = GetFormatDescription(format)->blockWidth;

		// Create staging buffer
		BufferPtr stagingBuffer;
		Print("Storage size for image: " + std::to_string(image.size()) + " bytes");
		Print(std::string("Is image compressed: ") + (gli::is_compressed(image.format()) ? "YES" : "NO"));

		BufferCreateInfo bufci = {};
		bufci.size = 0;
		bufci.usageFlags.bits.transferSrc = true;
		bufci.memoryUsage = MemoryUsage::CPUToGPU;

		// Compute each mipmap level size and alignments.
		// This step filters out levels too small to match minimal alignment.
		std::vector<MipLevel> levelSizes;
		for (gli::texture::size_type level = 0; level < maxMipLevelCount; level++)
		{
			MipLevel ls;
			ls.width = static_cast<uint32_t>(image.extent(level)[0]);
			ls.height = static_cast<uint32_t>(image.extent(level)[1]);
			// Stop when mipmaps are becoming too small to respect the format alignment.
			// The DXT* format documentation says texture sizes must be a multiple of 4.
			// For some reason, tools like imagemagick can generate mipmaps with a size < 4.
			// We need to ignore those.
			if (ls.width < blockWidth || ls.height < blockWidth) {
				break;
			}

			// If the DDS file contains textures which size is not a multiple of 4, something is wrong.
			// Since imagemagick can create invalid mipmap levels, I'd assume it can also create invalid textures with non-multiple-of-4 sizes. Asserting to catch those.
			if (ls.width % blockWidth != 0 || ls.height % blockWidth != 0)
			{
				Error("Compressed textures width & height must be a multiple of the block size.");
				return ERROR_IMAGE_INVALID_FORMAT;
			}

			// Compute pitch for this format.
			// See https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
			const uint32_t blockRowByteSize = (bytesPerTexel * blockWidth) / (blockWidth * blockWidth);
			const uint32_t rowStride = (ls.width * blockRowByteSize);

			ls.bufferWidth = ls.width;
			ls.bufferHeight = ls.height;
			ls.srcRowStride = rowStride;
			ls.dstRowStride = RoundUp<uint32_t>(ls.srcRowStride, rowStrideAlignment);

			ls.offset = bufci.size;
			bufci.size += (image.size(level) / ls.srcRowStride) * ls.dstRowStride;
			bufci.size = RoundUp<uint64_t>(bufci.size, offsetAlignment);
			levelSizes.emplace_back(std::move(ls));
		}
		const uint32_t mipmapLevelCount = CountU32(levelSizes);
		ASSERT_MSG(mipmapLevelCount > 0, "Requested texture size too small for the chosen format.");

		ppxres = pQueue->GetDevice()->CreateBuffer(bufci, &stagingBuffer);
		if (Failed(ppxres)) {
			return ppxres;
		}
		SCOPED_DESTROYER.AddObject(stagingBuffer);

		// Map and copy to staging buffer
		void* pBufferAddress = nullptr;
		ppxres = stagingBuffer->MapMemory(0, &pBufferAddress);
		if (Failed(ppxres)) {
			return ppxres;
		}

		for (size_t level = 0; level < mipmapLevelCount; level++)
		{
			auto& ls = levelSizes[level];

			const char* pSrc = static_cast<const char*>(image.data(0, 0, level));
			char* pDst = static_cast<char*>(pBufferAddress) + ls.offset;
			for (uint32_t row = 0; row * ls.srcRowStride < image.size(level); row++)
			{
				const char* pSrcRow = pSrc + row * ls.srcRowStride;
				char* pDstRow = pDst + row * ls.dstRowStride;
				memcpy(pDstRow, pSrcRow, ls.srcRowStride);
			}
		}

		stagingBuffer->UnmapMemory();

		// Create target image
		ImagePtr targetImage;
		{
			ImageCreateInfo ci = {};
			ci.type = ImageType::Image2D;
			ci.width = imageWidth;
			ci.height = imageHeight;
			ci.depth = 1;
			ci.format = format;
			ci.sampleCount = SampleCount::Sample1;
			ci.mipLevelCount = mipmapLevelCount;
			ci.arrayLayerCount = 1;
			ci.usageFlags.bits.transferDst = true;
			ci.usageFlags.bits.sampled = true;
			ci.memoryUsage = MemoryUsage::GPUOnly;

			ci.usageFlags.flags |= options.mAdditionalUsage;

			ppxres = pQueue->GetDevice()->CreateImage(ci, &targetImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetImage);
		}

		std::vector<BufferToImageCopyInfo> copyInfos(mipmapLevelCount);
		for (gli::texture::size_type level = 0; level < mipmapLevelCount; level++)
		{
			auto& ls = levelSizes[level];
			auto& copyInfo = copyInfos[level];

			// Copy info
			copyInfo.srcBuffer.imageWidth = ls.bufferWidth;
			copyInfo.srcBuffer.imageHeight = ls.bufferHeight;
			copyInfo.srcBuffer.imageRowStride = ls.dstRowStride;
			copyInfo.srcBuffer.footprintOffset = ls.offset;
			copyInfo.srcBuffer.footprintWidth = ls.bufferWidth;
			copyInfo.srcBuffer.footprintHeight = ls.bufferHeight;
			copyInfo.srcBuffer.footprintDepth = 1;
			copyInfo.dstImage.mipLevel = static_cast<uint32_t>(level);
			copyInfo.dstImage.arrayLayer = 0;
			copyInfo.dstImage.arrayLayerCount = 1;
			copyInfo.dstImage.x = 0;
			copyInfo.dstImage.y = 0;
			copyInfo.dstImage.z = 0;
			copyInfo.dstImage.width = ls.width;
			copyInfo.dstImage.height = ls.height;
			copyInfo.dstImage.depth = 1;
		}

		// Copy to GPU image
		ppxres = pQueue->CopyBufferToImage(
			copyInfos,
			stagingBuffer,
			targetImage,
			ALL_SUBRESOURCES,
			ResourceState::Undefined,
			ResourceState::ShaderResource);
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Change ownership to reference so object doesn't get destroyed
		targetImage->SetOwnership(Ownership::Reference);

		// Assign output
		*ppImage = targetImage;

		return SUCCESS;
	}

	// -------------------------------------------------------------------------------------------------

	Result CreateImageFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Image** ppImage,
		const ImageOptions& options,
		bool                         useGpu)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppImage);

		//ScopedTimer timer("Image creation from file '" + path.string() + "'"); // TODO: сделать таймер

		Result ppxres;
		if (Bitmap::IsBitmapFile(path))
		{
			// Load bitmap
			Bitmap bitmap;
			ppxres = Bitmap::LoadFile(path, &bitmap);
			if (Failed(ppxres)) 
			{
				return ppxres;
			}

			if (useGpu)
			{
				ppxres = CreateImageFromBitmapGpu(pQueue, &bitmap, ppImage, options);
			}
			else {
				ppxres = CreateImageFromBitmap(pQueue, &bitmap, ppImage, options);
			}

			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		else if (IsDDSFile(path))
		{
			// Generate a bitmap out of a DDS
			gli::texture image = gli::load(path.string().c_str());
			if (image.empty()) {
				return Result::ERROR_IMAGE_FILE_LOAD_FAILED;
			}
			Print("Successfully loaded compressed image: " + path.string());
			ppxres = CreateImageFromCompressedImage(pQueue, image, ppImage, options);
		}
		else
		{
			ppxres = Result::ERROR_IMAGE_FILE_LOAD_FAILED;
		}

		if (ppxres != Result::SUCCESS)
		{
			Error("Failed to create image from image file: " + path.string());
		}
		return SUCCESS;
	}

	Result CopyBitmapToTexture(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Texture* pTexture,
		uint32_t            mipLevel,
		uint32_t            arrayLayer,
		ResourceState stateBefore,
		ResourceState stateAfter)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pBitmap);
		ASSERT_NULL_ARG(pTexture);

		Result ppxres = CopyBitmapToImage(
			pQueue,
			pBitmap,
			pTexture->GetImage(),
			mipLevel,
			arrayLayer,
			stateBefore,
			stateAfter);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result CreateTextureFromBitmap(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Texture** ppTexture,
		const TextureOptions& options)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pBitmap);
		ASSERT_NULL_ARG(ppTexture);

		Result ppxres = ERROR_FAILED;

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// Cap mip level count
		uint32_t maxMipLevelCount = Mipmap::CalculateLevelCount(pBitmap->GetWidth(), pBitmap->GetHeight());
		uint32_t mipLevelCount = std::min<uint32_t>(options.mMipLevelCount, maxMipLevelCount);

		// Create target texture
		TexturePtr targetTexture;
		{
			TextureCreateInfo ci = {};
			ci.image = nullptr;
			ci.imageType = ImageType::Image2D;
			ci.width = pBitmap->GetWidth();
			ci.height = pBitmap->GetHeight();
			ci.depth = 1;
			ci.imageFormat = ToGrfxFormat(pBitmap->GetFormat());
			ci.sampleCount = SampleCount::Sample1;
			ci.mipLevelCount = mipLevelCount;
			ci.arrayLayerCount = 1;
			ci.usageFlags.bits.transferDst = true;
			ci.usageFlags.bits.sampled = true;
			ci.memoryUsage = MemoryUsage::GPUOnly;
			ci.initialState = options.mInitialState;
			ci.RTVClearValue = {0, 0, 0, 0};
			ci.DSVClearValue = { 1.0f, 0xFF };
			ci.sampledImageViewType = ImageViewType::Undefined;
			ci.sampledImageViewFormat = Format::Undefined;
			ci.sampledImageYcbcrConversion = options.mYcbcrConversion;
			ci.renderTargetViewFormat = Format::Undefined;
			ci.depthStencilViewFormat = Format::Undefined;
			ci.storageImageViewFormat = Format::Undefined;
			ci.ownership = Ownership::Reference;

			ci.usageFlags.flags |= options.mAdditionalUsage;

			ppxres = pQueue->GetDevice()->CreateTexture(ci, &targetTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetTexture);
		}

		// Since this mipmap is temporary, it's safe to use the static pool.
		Mipmap mipmap = Mipmap(*pBitmap, mipLevelCount, /* useStaticPool= */ true);
		if (!mipmap.IsOk()) {
			return ERROR_FAILED;
		}

		// Copy mips to texture
		for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel)
		{
			const Bitmap* pMip = mipmap.GetMip(mipLevel);

			ppxres = CopyBitmapToTexture(
				pQueue,
				pMip,
				targetTexture,
				mipLevel,
				0,
				options.mInitialState,
				options.mInitialState);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Change ownership to reference so object doesn't get destroyed
		targetTexture->SetOwnership(Ownership::Reference);

		// Assign output
		*ppTexture = targetTexture;

		return SUCCESS;
	}

	Result CreateTextureFromMipmap(
		Queue* pQueue,
		const Mipmap* pMipmap,
		Texture** ppTexture,
		const TextureOptions& options)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pMipmap);
		ASSERT_NULL_ARG(ppTexture);

		Result ppxres = ERROR_FAILED;

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// Cap mip level count
		auto pMip0 = pMipmap->GetMip(0);

		// Create target texture
		TexturePtr targetTexture;
		{
			TextureCreateInfo ci = {};
			ci.image = nullptr;
			ci.imageType = ImageType::Image2D;
			ci.width = pMip0->GetWidth();
			ci.height = pMip0->GetHeight();
			ci.depth = 1;
			ci.imageFormat = ToGrfxFormat(pMip0->GetFormat());
			ci.sampleCount = SampleCount::Sample1;
			ci.mipLevelCount = pMipmap->GetLevelCount();
			ci.arrayLayerCount = 1;
			ci.usageFlags.bits.transferDst = true;
			ci.usageFlags.bits.sampled = true;
			ci.memoryUsage = MemoryUsage::GPUOnly;
			ci.initialState = options.mInitialState;
			ci.RTVClearValue = {0, 0, 0, 0};
			ci.DSVClearValue = { 1.0f, 0xFF };
			ci.sampledImageViewType = ImageViewType::Undefined;
			ci.sampledImageViewFormat = Format::Undefined;
			ci.sampledImageYcbcrConversion = options.mYcbcrConversion;
			ci.renderTargetViewFormat = Format::Undefined;
			ci.depthStencilViewFormat = Format::Undefined;
			ci.storageImageViewFormat = Format::Undefined;
			ci.ownership = Ownership::Reference;

			ci.usageFlags.flags |= options.mAdditionalUsage;

			ppxres = pQueue->GetDevice()->CreateTexture(ci, &targetTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetTexture);
		}

		// Copy mips to texture
		for (uint32_t mipLevel = 0; mipLevel < pMipmap->GetLevelCount(); ++mipLevel)
		{
			const Bitmap* pMip = pMipmap->GetMip(mipLevel);

			ppxres = CopyBitmapToTexture(
				pQueue,
				pMip,
				targetTexture,
				mipLevel,
				0,
				options.mInitialState,
				options.mInitialState);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Change ownership to reference so object doesn't get destroyed
		targetTexture->SetOwnership(Ownership::Reference);

		// Assign output
		*ppTexture = targetTexture;

		return SUCCESS;
	}

	// -------------------------------------------------------------------------------------------------

	Result CreateTextureFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Texture** ppTexture,
		const TextureOptions& options)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppTexture);

		//ScopedTimer timer("Texture creation from image file '" + path.string() + "'"); // TODO: сделать таймер

		// Load bitmap
		Bitmap bitmap;
		Result ppxres = Bitmap::LoadFile(path, &bitmap);
		if (Failed(ppxres)) {
			return ppxres;
		}
		return CreateTextureFromBitmap(pQueue, &bitmap, ppTexture, options);
	}

	// -------------------------------------------------------------------------------------------------

	struct SubImage
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bufferOffset = 0;
	};

	SubImage CalcSubimageCrossHorizontalLeft(
		uint32_t     subImageIndex,
		uint32_t     imageWidth,
		uint32_t     imageHeight,
		Format format)
	{
		uint32_t cellPixelsX = imageWidth / 4;
		uint32_t cellPixelsY = imageHeight / 3;
		uint32_t cellX = 0;
		uint32_t cellY = 0;
		switch (subImageIndex) {
		default: break;

		case 0: {
			cellX = 1;
			cellY = 0;
		} break;

		case 1: {
			cellX = 0;
			cellY = 1;
		} break;

		case 2: {
			cellX = 1;
			cellY = 1;
		} break;

		case 3: {
			cellX = 2;
			cellY = 1;
		} break;

		case 4: {
			cellX = 3;
			cellY = 1;
		} break;

		case 5: {
			cellX = 1;
			cellY = 2;

		} break;
		}

		uint32_t pixelStride = GetFormatDescription(format)->bytesPerTexel;
		uint32_t pixelOffsetX = cellX * cellPixelsX * pixelStride;
		uint32_t pixelOffsetY = cellY * cellPixelsY * imageWidth * pixelStride;

		SubImage subImage = {};
		subImage.width = cellPixelsX;
		subImage.height = cellPixelsY;
		subImage.bufferOffset = pixelOffsetX + pixelOffsetY;

		return subImage;
	}

	Result CreateIBLTexturesFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Texture** ppIrradianceTexture,
		Texture** ppEnvironmentTexture)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppIrradianceTexture);
		ASSERT_NULL_ARG(ppEnvironmentTexture);

		auto fileBytes = LoadFile(path);
		if (!fileBytes.has_value())
		{
			return ERROR_IMAGE_FILE_LOAD_FAILED;
		}

		auto is = std::istringstream(std::string(fileBytes.value().data(), fileBytes.value().size()));
		if (!is.good())
		{
			return ERROR_IMAGE_FILE_LOAD_FAILED;
		}

		std::filesystem::path irrFile;
		is >> irrFile;

		std::filesystem::path envFile;
		is >> envFile;

		uint32_t baseWidth = 0;
		is >> baseWidth;

		uint32_t baseHeight = 0;
		is >> baseHeight;

		uint32_t levelCount = 0;
		is >> levelCount;

		if (irrFile.empty() || envFile.empty() || (baseWidth == 0) || (baseHeight == 0) || (levelCount == 0)) {
			return ERROR_IMAGE_FILE_LOAD_FAILED;
		}

		// Create irradiance texture - does not require mip maps
		std::filesystem::path irrFilePath = path.parent_path() / irrFile;
		Result                ppxres;
		{
			//ScopedTimer timer("Texture creation from file '" + irrFilePath.string() + "'"); // TODO: сделать таймер
			ppxres = CreateTextureFromFile(pQueue, irrFilePath, ppIrradianceTexture);
		}
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Load IBL environment map - this is stored as a bitmap on disk
		std::filesystem::path envFilePath = path.parent_path() / envFile;
		//ScopedTimer           timer("Texture creation from mipmap file '" + envFilePath.string() + "'");// TODO: сделать таймер
		Mipmap                mipmap = {};
		ppxres = Mipmap::LoadFile(envFilePath, baseWidth, baseHeight, &mipmap, levelCount);
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Create environment texture
		return CreateTextureFromMipmap(pQueue, &mipmap, ppEnvironmentTexture);
	}

	Result CreateCubeMapFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		const CubeMapCreateInfo* pCreateInfo,
		Image** ppImage,
		const ImageUsageFlags& additionalImageUsage)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppImage);
		//ScopedTimer timer("Cubemap creation from file '" + path.string() + "'");// TODO: сделать таймер

		// Load bitmap
		Bitmap bitmap;
		Result ppxres = Bitmap::LoadFile(path, &bitmap);
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Scoped destroy
		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// Create staging buffer
		BufferPtr stagingBuffer;
		{
			uint64_t bitmapFootprintSize = bitmap.GetFootprintSize();

			BufferCreateInfo ci = {};
			ci.size = bitmapFootprintSize;
			ci.usageFlags.bits.transferSrc = true;
			ci.memoryUsage = MemoryUsage::CPUToGPU;

			ppxres = pQueue->GetDevice()->CreateBuffer(ci, &stagingBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(stagingBuffer);

			// Map and copy to staging buffer
			void* pBufferAddress = nullptr;
			ppxres = stagingBuffer->MapMemory(0, &pBufferAddress);
			if (Failed(ppxres)) {
				return ppxres;
			}
			std::memcpy(pBufferAddress, bitmap.GetData(), bitmapFootprintSize);
			stagingBuffer->UnmapMemory();
		}

		// Target format
		Format targetFormat = Format::R8G8B8A8_UNORM;

		ASSERT_MSG(bitmap.GetWidth() * 3 == bitmap.GetHeight() * 4, "cubemap texture dimension must be a multiple of 4x3");
		// Calculate subImage to use for target image dimensions
		SubImage tmpSubImage = CalcSubimageCrossHorizontalLeft(0, bitmap.GetWidth(), bitmap.GetHeight(), targetFormat);

		ASSERT_MSG(tmpSubImage.width == tmpSubImage.height, "cubemap face width != height");
		// Create target image
		ImagePtr targetImage;
		{
			ImageCreateInfo ci = {};
			ci.type = ImageType::Cube;
			ci.width = tmpSubImage.width;
			ci.height = tmpSubImage.height;
			ci.depth = 1;
			ci.format = targetFormat;
			ci.sampleCount = SampleCount::Sample1;
			ci.mipLevelCount = 1;
			ci.arrayLayerCount = 6;
			ci.usageFlags.bits.transferDst = true;
			ci.usageFlags.bits.sampled = true;
			ci.memoryUsage = MemoryUsage::GPUOnly;

			ci.usageFlags.flags |= additionalImageUsage.flags;

			ppxres = pQueue->GetDevice()->CreateImage(ci, &targetImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetImage);
		}

		// Copy to GPU image
		//
		{
			uint32_t faces[6] = {
				pCreateInfo->posX,
				pCreateInfo->negX,
				pCreateInfo->posY,
				pCreateInfo->negY,
				pCreateInfo->posZ,
				pCreateInfo->negZ,
			};

			std::vector<BufferToImageCopyInfo> copyInfos(6);
			for (uint32_t arrayLayer = 0; arrayLayer < 6; ++arrayLayer) {
				uint32_t subImageIndex = faces[arrayLayer];
				SubImage subImage = CalcSubimageCrossHorizontalLeft(subImageIndex, bitmap.GetWidth(), bitmap.GetHeight(), targetFormat);

				// Copy info
				BufferToImageCopyInfo& copyInfo = copyInfos[arrayLayer];
				copyInfo.srcBuffer.imageWidth = bitmap.GetWidth();
				copyInfo.srcBuffer.imageHeight = bitmap.GetHeight();
				copyInfo.srcBuffer.imageRowStride = bitmap.GetRowStride();
				copyInfo.srcBuffer.footprintOffset = subImage.bufferOffset;
				copyInfo.srcBuffer.footprintWidth = subImage.width;
				copyInfo.srcBuffer.footprintHeight = subImage.height;
				copyInfo.srcBuffer.footprintDepth = 1;
				copyInfo.dstImage.mipLevel = 0;
				copyInfo.dstImage.arrayLayer = arrayLayer;
				copyInfo.dstImage.arrayLayerCount = 1;
				copyInfo.dstImage.x = 0;
				copyInfo.dstImage.y = 0;
				copyInfo.dstImage.z = 0;
				copyInfo.dstImage.width = subImage.width;
				copyInfo.dstImage.height = subImage.height;
				copyInfo.dstImage.depth = 1;
			}

			ppxres = pQueue->CopyBufferToImage(
				copyInfos,
				stagingBuffer,
				targetImage,
				ALL_SUBRESOURCES,
				ResourceState::Undefined,
				ResourceState::ShaderResource);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Change ownership to reference so object doesn't get destroyed
		targetImage->SetOwnership(Ownership::Reference);

		// Assign output
		*ppImage = targetImage;

		return SUCCESS;
	}

	// -------------------------------------------------------------------------------------------------

	Result CreateMeshFromGeometry(
		Queue* pQueue,
		const Geometry* pGeometry,
		Mesh** ppMesh)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pGeometry);
		ASSERT_NULL_ARG(ppMesh);

		ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());

		// Create staging buffer
		BufferPtr stagingBuffer;
		{
			uint32_t biggestBufferSize = pGeometry->GetLargestBufferSize();

			BufferCreateInfo ci = {};
			ci.size = biggestBufferSize;
			ci.usageFlags.bits.transferSrc = true;
			ci.memoryUsage = MemoryUsage::CPUToGPU;

			Result ppxres = pQueue->GetDevice()->CreateBuffer(ci, &stagingBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(stagingBuffer);
		}

		// Create target mesh
		MeshPtr targetMesh;
		{
			MeshCreateInfo ci = MeshCreateInfo(*pGeometry);

			Result ppxres = pQueue->GetDevice()->CreateMesh(ci, &targetMesh);
			if (Failed(ppxres)) {
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetMesh);
		}

		// Copy geometry data to mesh
		{
			// Copy info
			BufferToBufferCopyInfo copyInfo = {};

			// Index buffer
			if (pGeometry->GetIndexType() != IndexType::Undefined)
			{
				const Geometry::Buffer* pGeoBuffer = pGeometry->GetIndexBuffer();
				ASSERT_NULL_ARG(pGeoBuffer);

				uint32_t geoBufferSize = pGeoBuffer->GetSize();

				Result ppxres = stagingBuffer->CopyFromSource(geoBufferSize, pGeoBuffer->GetData());
				if (Failed(ppxres)) {
					return ppxres;
				}

				copyInfo.size = geoBufferSize;

				// Copy to GPU buffer
				ppxres = pQueue->CopyBufferToBuffer(&copyInfo, stagingBuffer, targetMesh->GetIndexBuffer(), ResourceState::IndexBuffer, ResourceState::IndexBuffer);
				if (Failed(ppxres))
				{
					return ppxres;
				}
			}

			// Vertex buffers
			uint32_t vertexBufferCount = pGeometry->GetVertexBufferCount();
			for (uint32_t i = 0; i < vertexBufferCount; ++i)
			{
				const Geometry::Buffer* pGeoBuffer = pGeometry->GetVertexBuffer(i);
				ASSERT_NULL_ARG(pGeoBuffer);

				uint32_t geoBufferSize = pGeoBuffer->GetSize();

				Result ppxres = stagingBuffer->CopyFromSource(geoBufferSize, pGeoBuffer->GetData());
				if (Failed(ppxres)) {
					return ppxres;
				}

				copyInfo.size = geoBufferSize;

				BufferPtr targetBuffer = targetMesh->GetVertexBuffer(i);

				// Copy to GPU buffer
				ppxres = pQueue->CopyBufferToBuffer(&copyInfo, stagingBuffer, targetBuffer, ResourceState::VertexBuffer, ResourceState::VertexBuffer);
				if (Failed(ppxres)) {
					return ppxres;
				}
			}
		}

		// Change ownership to reference so object doesn't get destroyed
		targetMesh->SetOwnership(Ownership::Reference);

		// Assign output
		*ppMesh = targetMesh;

		return SUCCESS;
	}

	Result CreateMeshFromTriMesh(
		Queue* pQueue,
		const TriMesh* pTriMesh,
		Mesh** ppMesh)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pTriMesh);
		ASSERT_NULL_ARG(ppMesh);

		Result ppxres = ERROR_FAILED;

		Geometry geo;
		ppxres = Geometry::Create(*pTriMesh, &geo);
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = CreateMeshFromGeometry(pQueue, &geo, ppMesh);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result CreateMeshFromWireMesh(
		Queue* pQueue,
		const WireMesh* pWireMesh,
		Mesh** ppMesh)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pWireMesh);
		ASSERT_NULL_ARG(ppMesh);

		Result ppxres = ERROR_FAILED;

		Geometry geo;
		ppxres = Geometry::Create(*pWireMesh, &geo);
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = CreateMeshFromGeometry(pQueue, &geo, ppMesh);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result CreateMeshFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Mesh** ppMesh,
		const TriMeshOptions& options)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppMesh);

		TriMesh mesh = TriMesh::CreateFromOBJ(path, options);

		Result ppxres = CreateMeshFromTriMesh(pQueue, &mesh, ppMesh);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

} // namespace vkrUtil

#pragma endregion

} // namespace vkr