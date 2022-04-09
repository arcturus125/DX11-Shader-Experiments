//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D BloomTexture : register(t0);
Texture2D SceneTexture : register(t1);
SamplerState PointSample  : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that copies one texture to another
float4 main(PostProcessingInput input) : SV_Target
{
	float3 colour = SceneTexture.Sample(PointSample, input.uv).rgb;
	float3 bloomcolour = BloomTexture.Sample(PointSample, input.uv).rgb;
	
	colour = colour + bloomcolour;
	return float4(colour, 1.0f);
}