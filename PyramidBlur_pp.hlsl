#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
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
	
	float w = 1 / width;
	float h = 1 / height;
	
	
	
	float3 colour = 0;
	int i = 0;
	int num = blurRadius;
	int scale = blurBellcurveStrength;
	
	for (int y = -blurRadius; y <= blurRadius; y++)
	{
		for (int x = -blurRadius; x <= blurRadius; x++)
		{
			float2 pos = input.uv + float2(w * x, h * y);
			float dist = distance(pos * scale, input.uv * scale);
			colour += SceneTexture.Sample(PointSample, pos).rgb * (num - dist);
			
			
			i += (num - dist);
		}
	}
	colour /= i;
	
	return float4(colour, 1.0f);
}