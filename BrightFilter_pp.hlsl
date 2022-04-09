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

// Post-processing shader that copies one texture to another
float4 main(PostProcessingInput input) : SV_Target
{
	float3 colour = SceneTexture.Sample(PointSample, input.uv).rgb;
	float brightness = colour.x + colour.y + colour.z;
	if (brightness < brightFilterThreshold)
	{
		colour = (0, 0, 0);
	}
	return float4(colour, 1.0f);
}