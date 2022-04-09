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


// See this desmos graph i made to test this function
// https://www.desmos.com/calculator/p1s5w5wkjc
float gaussianCurve(float x)
{
	//eulers number
	const float e = 2.71828182846;
	const float pi = 3.142f;
	// standard deviation
	float c = blurBellcurveStrength;
	
	float cSquared = c * c;
	
	return (1 / sqrt(2 * pi * cSquared)) * pow(e, -(x * x) / 2 * cSquared);

}



// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	float width;
	float height;
	SceneTexture.GetDimensions(width, height);
	
	float w = 1 / width; // width of each pixel, in UV units
	float h = 1 / height; // height of each pixel, in UV units
	
	
	
	float3 colour = 0;  //SceneTexture.Sample(PointSample, input.uv).rgb;
	
	//int i = 0;
	//for (int x = -blurRadius; x <= blurRadius; x++)
	//{
	//	float2 pos = input.uv + float2(w * x, 0);
	//	float multiplier = gaussianCurveTest(x);
	//	colour += (SceneTexture.Sample(PointSample, pos).rgb * multiplier);
	//	i += multiplier;
	//}
	//colour /= i;
	
	int m = 0;
	double half_size = (blurRadius - 1) / 2.0;
	for (int i = 0; i < blurRadius; ++i)
	{
		double x = i - half_size;
		float2 pos = input.uv + float2(w * x, 0);
		float multiplier = gaussianCurve(x);
		colour += (SceneTexture.Sample(PointSample, pos).rgb * multiplier);
		m += multiplier;

	}
	colour /= m;
	
	
	
	return float4(colour, 1.0f);
}