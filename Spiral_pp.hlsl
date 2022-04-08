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
	// Get vector from screen centre to pixel UV
	const float2 centreUV = float2(0.5f, 0.5f);
	float2 centreOffsetUV = 0/*FILTER - not 0, read comment*/ - centreUV;
	float centreDistance = length(centreOffsetUV); // Distance of pixel from screen centre
	
	// Get sin and cos of spiral amount, increasing with distance from centre
	float s, c;
	sincos( centreDistance * gSpiralLevel * gSpiralLevel, s, c );
	
	// Create a (2D) rotation matrix and apply to the vector - i.e. rotate the
	// vector around the centre by the spiral amount
	matrix<float,2,2> rot2D = { c, s,
	                           -s, 0/*FILTER - not 0, it's a 2D rotation matrix...?*/ };
	float2 rotatedOffsetUV = mul(centreOffsetUV, rot2D);

	// Sample texture at new position (centre UV + rotated UV offset)
    float3 outputColour = SceneTexture.Sample( PointSample, 0/*FILTER - not 0, read the comment*/ ).rgb;

	return float4(outputColour, 1.0f);
}