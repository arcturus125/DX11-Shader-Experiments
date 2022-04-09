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
	
	float width;
	float height;
	SceneTexture.GetDimensions(width, height);
	
	
	//float2 noiseScaleInUVSpace = { width / gNoiseScale.x, height / gNoiseScale.y };
	
	float2 samplePos =
	{
	//  {               section coords             }
		round((input.uv.x * width) / gNoiseScale.x) / (width / gNoiseScale.x),
		round((input.uv.y * height) / gNoiseScale.y) / (height / gNoiseScale.y)
	};
	
	
	//round((input.uv.x * width) / gNoiseScale.x) / width / gNoiseScale.x,
	//round((input.uv.y * height) / gNoiseScale.y) / height / gNoiseScale.y
	
	
	
	
	
	
	// Sample a pixel from the scene texture
	float3 colour = SceneTexture.Sample(PointSample, samplePos).rgb;
	// Got the RGB from the scene texture, set alpha to 1 for final output
	return float4(colour, 1.0f);
}