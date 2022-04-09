//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D    SceneTexture : register(t0);
SamplerState PointSample  : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	// Sample a pixel from the scene texture
	float3 colour = SceneTexture.Sample(PointSample, input.uv).rgb;
	float x = bitColour;
	float3 newColour =
	{
		(round((colour.x * 256) / x) * x) / 256,
		(round((colour.y * 256) / x) * x) / 256,
		(round((colour.z * 256) / x) * x) / 256,
	};
	
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
	return float4(newColour, 1.0f);
}