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


float gaussianCurve(float c, float x)
{
	//eulers number
	const float e = 2.71828182846;
	
	float a = 1;
	float b = 0;
	
	//float fraction = -(((x - b) * (x - b)) / (2 * c * c));
	float fraction = pow(x - b, 2) / pow(2 * c, 2);
	
	return pow(a * e, -fraction);

}


// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	float width;
	float height;
	SceneTexture.GetDimensions(width, height);
	
	float w = 1 / width; // width of each pixel, in UV units
	float h = 1 / height; // height of each pixel, in UV units
	
	
	
	float3 colour = 0;
	int num = blurRadius;
	int scale = blurBellcurveStrength;
	int i = 1;
	
	for (int x = -blurRadius; x <= blurRadius; x++)
	{
		float2 pos = input.uv + float2(0, h * x);
		float multiplier = gaussianCurve(blurBellcurveStrength,1/x);
		colour += SceneTexture.Sample(PointSample, pos).rgb * multiplier;
		i += multiplier;
	}
	colour /= i;
	
	
	return float4(colour, 1.0f);
}