#define IS_SHADER
#include "Common.hlsli"
#include "TransparencyVS.hlsli"

Texture2D          OpaqueDepthTexture : register(CUSTOM_TEXTURE_0_REGISTER);
RWTexture2D<uint>  CountTexture       : register(CUSTOM_UAV_0_REGISTER);
RWTexture2D<uint2> FragmentTexture    : register(CUSTOM_UAV_1_REGISTER);

void psmain(VSOutput input)
{
    // Test fragment against opaque depth
    {
        const float opaqueDepth = OpaqueDepthTexture.Load(int3(input.position.xy, 0)).r;
        clip(input.position.z < opaqueDepth ? 1.0f : -1.0f);
    }

    // Find the next fragment index in the bucket
    const uint2 bucketIndex = (uint2)input.position.xy;
    uint nextBucketFragmentIndex = 0;
    InterlockedAdd(CountTexture[bucketIndex], 1U, nextBucketFragmentIndex);

    // Ignore the fragment if the bucket is already full
    if(nextBucketFragmentIndex >= BUFFER_BUCKETS_SIZE_PER_PIXEL)
    {
        clip(-1.0f);
    }

    // Add the fragment to the bucket
    uint2 textureFragmentIndex = bucketIndex;
    textureFragmentIndex.y *= BUFFER_BUCKETS_SIZE_PER_PIXEL;
    textureFragmentIndex.y += nextBucketFragmentIndex;
    FragmentTexture[textureFragmentIndex] = uint2(PackColor(float4(input.color, g_Globals.meshOpacity)), asuint(input.position.z));
}
