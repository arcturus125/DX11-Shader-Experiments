//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here
#include "ColourRGBA.h" 

#include <sstream>
#include <memory>
#include <iostream>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

//********************
// Available post-processes
enum class PostProcess
{
	None,
	PyramidBlur,
	Retro,
	Spiral,
};

auto gCurrentPostProcess = PostProcess::None;

//********************


// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 1.5f;  // Radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // Units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = true;


// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gStarsMesh;
Mesh* gGroundMesh;
Mesh* gCubeMesh;
Mesh* gCrateMesh;
Mesh* gLightMesh;

Model* gStars;
Model* gGround;
Model* gCube;
Model* gCrate;

Camera* gCamera;


// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
struct Light
{
	Model*   model;
	CVector3 colour;
	float    strength;
};
Light gLights[NUM_LIGHTS];


// Additional light information
CVector3 gAmbientColour = { 0.3f, 0.3f, 0.4f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.3f, 0.3f, 0.4f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbitRadius = 20.0f;
const float gLightOrbitSpeed = 0.7f;



//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants (settings) that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constants (settings) that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--

//**************************
PostProcessingConstants gPostProcessingConstants;       // As above, but constants (settings) for each post-process
ID3D11Buffer*           gPostProcessingConstantBuffer; // --"--
//**************************


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab
ID3D11Resource*           gStarsDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gGroundDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCrateDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCrateDiffuseSpecularMapSRV = nullptr;
ID3D11Resource*           gCubeDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCubeDiffuseSpecularMapSRV = nullptr;

ID3D11Resource*           gLightDiffuseMap = nullptr;
ID3D11ShaderResourceView* gLightDiffuseMapSRV = nullptr;



//****************************
// Post processing textures

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D*          gSceneTexture      = nullptr; // 2d texture
ID3D11RenderTargetView*   gSceneRenderTarget = nullptr; // a reference to the above texture that can be rendered to
ID3D11ShaderResourceView* gSceneTextureSRV   = nullptr; // a reference to the above texture that can be passed to shaders

// a texture used for most post processing
ID3D11Texture2D*          gPostProcessTexture      = nullptr;
ID3D11RenderTargetView*   gPostProcessRenderTarget = nullptr;
ID3D11ShaderResourceView* gPostProcessTextureSRV   = nullptr;

// a secondary post processing texture used for bloom
ID3D11Texture2D*          gBloomTexture      = nullptr; 
ID3D11RenderTargetView*   gBloomRenderTarget = nullptr; 
ID3D11ShaderResourceView* gBloomTextureSRV   = nullptr; 


// Additional textures used for specific post-processes
ID3D11Resource*           gNoiseMap = nullptr;
ID3D11ShaderResourceView* gNoiseMapSRV = nullptr;
ID3D11Resource*           gBurnMap = nullptr;
ID3D11ShaderResourceView* gBurnMapSRV = nullptr;
ID3D11Resource*           gDistortMap = nullptr;
ID3D11ShaderResourceView* gDistortMapSRV = nullptr;


//****************************


float blurStrength = 50;
float blurCurve = 0.03f;
float timer = 0; 
float bitColour = 90;
float pixelSize = 10;

bool Tint;
bool Blur;
bool GaussianBlur;
bool Underwater;
bool Retro;
bool Bloom;



//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success

float Min(float f1, float f2, float f3)
{
	float fMin = f1;
	if (f2 < fMin)
	{
		fMin = f2;
	}
	if (f3 < fMin)
	{
		fMin = f3;
	}
	return fMin;
}
float Max(float f1, float f2, float f3)
{
	float fMax = f1;
	if (f2 > fMax)
	{
		fMax = f2;
	}
	if (f3 > fMax)
	{
		fMax = f3;
	}
	return fMax;
}

// Convert an RGB colour to a HSL colour
CVector3 RGBToHSL(CVector3 rgb)
{
	float fR = rgb.x;
	float fB = rgb.y;
	float fG = rgb.z;

	float min = Min(fR, fB, fG);
	float max = Max(fR, fB, fG);

	float H;
	float S;
	float L;

	L = 50 * (max + min);
	if (min == max)
	{
		S = H = 0; 
		return CVector3(H, S, L);
	}
	else if (L < 50)
	{
		S = 100 * (max - min) / (max + min);
	}
	else
	{
		S = 100 * (max - min) / (2.0 - max - min);
	}
	if (max == fR) H = 60 * (fG - fB) / (max - min);
	else if (max == fG) H = 60 * (fB - fR) / (max - min) + 120;
	else if (max == fB) H = 60 * (fR - fG) / (max - min) + 240;

	if (H < 0) H = H + 360;

	return CVector3(H, S, L);
}
CVector3 HSLToRGB(CVector3 hsl)
{
	int H = hsl.x; 
	int S = hsl.y; 
	int L = hsl.z;

	// Fill in the correct code here for question 7 (advanced)
	float St = S / 100.0f;
	float Lt = L / 100.0f;

	// calculate C
	float C = (1.0f - fabs(2.0f * Lt - 1.0f)) * St;

	//calculate X
	float X = C * (1.0f - fabs(fmodf(H / 60.0f, 2.0f) - 1.0f));

	//calculate m
	float m = Lt - (C / 2.0f);

	float Rt = 0;
	float Gt = 0;
	float Bt = 0;

	if (H >= 0 && H < 60)
	{
		Rt = C;
		Gt = X;
		Bt = 0;
	}
	else if (H >= 60 && H < 120)
	{
		Rt = X;
		Gt = C;
		Bt = 0;
	}
	else if (H >= 120 && H < 180)
	{
		Rt = 0;
		Gt = C;
		Bt = X;
	}
	else if (H >= 180 && H < 240)
	{
		Rt = 0;
		Gt = X;
		Bt = C;
	}
	else if (H >= 240 && H < 300)
	{
		Rt = X;
		Gt = 0;
		Bt = C;
	}
	else if (H >= 300 && H < 360)
	{
		Rt = C;
		Gt = 0;
		Bt = X;
	}


	float R = (Rt + m);
	float G = (Gt + m);
	float B = (Bt + m);

	return CVector3(R,G,B);

}


CVector3 tintColour =  RGBToHSL({ 0, 1, 1 });
CVector3 tintColour2 = RGBToHSL({ 1, 1, 0 });


bool InitGeometry()
{
	////--------------- Load meshes ---------------////

	// Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
	try
	{
		gStarsMesh  = new Mesh("Stars.x");
		gGroundMesh = new Mesh("Hills.x");
		gCubeMesh   = new Mesh("Cube.x");
		gCrateMesh  = new Mesh("CargoContainer.x");
		gLightMesh  = new Mesh("Light.x");
	}
	catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
	{
		gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
		return false;
	}


	////--------------- Load / prepare textures & GPU states ---------------////

	// Load textures and create DirectX objects for them
	// The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
	// texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
	// The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
	if (!LoadTexture("Stars.jpg",                &gStarsDiffuseSpecularMap,  &gStarsDiffuseSpecularMapSRV) ||
		!LoadTexture("GrassDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
		!LoadTexture("StoneDiffuseSpecular.dds", &gCubeDiffuseSpecularMap,   &gCubeDiffuseSpecularMapSRV) ||
		!LoadTexture("CargoA.dds",               &gCrateDiffuseSpecularMap,  &gCrateDiffuseSpecularMapSRV) ||
		!LoadTexture("Flare.jpg",                &gLightDiffuseMap,          &gLightDiffuseMapSRV) ||
		!LoadTexture("Noise.png",                &gNoiseMap,   &gNoiseMapSRV) ||
		!LoadTexture("Burn.png",                 &gBurnMap,    &gBurnMapSRV) ||
		!LoadTexture("Distort.png",              &gDistortMap, &gDistortMapSRV))
	{
		gLastError = "Error loading textures";
		return false;
	}


	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}


	////--------------- Prepare shaders and constant buffers to communicate with them ---------------////

	// Load the shaders required for the geometry we will use (see Shader.cpp / .h)
	if (!LoadShaders())
	{
		gLastError = "Error loading shaders";
		return false;
	}

	// Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
	// These allow us to pass data from CPU to shaders such as lighting information or matrices
	// See the comments above where these variable are declared and also the UpdateScene function
	gPerFrameConstantBuffer       = CreateConstantBuffer(sizeof(gPerFrameConstants));
	gPerModelConstantBuffer       = CreateConstantBuffer(sizeof(gPerModelConstants));
	gPostProcessingConstantBuffer = CreateConstantBuffer(sizeof(gPostProcessingConstants));
	if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr || gPostProcessingConstantBuffer == nullptr)
	{
		gLastError = "Error creating constant buffers";
		return false;
	}



	//********************************************
	//**** Create Scene Texture

	// We will render the scene to this texture instead of the back-buffer (screen), then we post-process the texture onto the screen
	// This is exactly the same code we used in the graphics module when we were rendering the scene onto a cube using a texture

	// Using a helper function to load textures from files above. Here we create the scene texture manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D11_TEXTURE2D_DESC sceneTextureDesc = {};
	sceneTextureDesc.Width = gViewportWidth;  // Full-screen post-processing - use full screen size for texture
	sceneTextureDesc.Height = gViewportHeight;
	sceneTextureDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	sceneTextureDesc.ArraySize = 1;
	sceneTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	sceneTextureDesc.SampleDesc.Count = 1;
	sceneTextureDesc.SampleDesc.Quality = 0;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gPostProcessTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gBloomTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}

	// We created the scene texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTexture, NULL, &gSceneRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateRenderTargetView(gPostProcessTexture, NULL, &gPostProcessRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateRenderTargetView(gBloomTexture, NULL, &gBloomRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc = {};
	srDesc.Format = sceneTextureDesc.Format;
	srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTexture, &srDesc, &gSceneTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateShaderResourceView(gPostProcessTexture, &srDesc, &gPostProcessTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateShaderResourceView(gBloomTexture, &srDesc, &gBloomTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}


	return true;
}
bool InitScene()
{
	Tint = false;
	Blur = false;
	GaussianBlur = false;
	Underwater = false;
	Retro = false;

	////--------------- Set up scene ---------------////

	gStars  = new Model(gStarsMesh);
	gGround = new Model(gGroundMesh);
	gCube   = new Model(gCubeMesh);
	gCrate  = new Model(gCrateMesh);

	// Initial positions
	gCube->SetPosition({ 42, 5, -10 });
	gCube->SetRotation({ 0.0f, ToRadians(-110.0f), 0.0f });
	gCube->SetScale(1.5f);
	gCrate->SetPosition({ -10, 0, 90 });
	gCrate->SetRotation({ 0.0f, ToRadians(40.0f), 0.0f });
	gCrate->SetScale(6.0f);
	gStars->SetScale(8000.0f);


	// Light set-up - using an array this time
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gLights[i].model = new Model(gLightMesh);
	}

	gLights[0].colour = { 0.8f, 0.8f, 1.0f };
	gLights[0].strength = 10;
	gLights[0].model->SetPosition({ 30, 10, 0 });
	gLights[0].model->SetScale(pow(gLights[0].strength, 0.7f)); // Convert light strength into a nice value for the scale of the light - equation is ad-hoc.

	gLights[1].colour = { 1.0f, 0.8f, 0.2f };
	gLights[1].strength = 40;
	gLights[1].model->SetPosition({ -70, 30, 100 });
	gLights[1].model->SetScale(pow(gLights[1].strength, 0.7f));


	////--------------- Set up camera ---------------////

	gCamera = new Camera();
	gCamera->SetPosition({ 25, 18, -45 });
	gCamera->SetRotation({ ToRadians(10.0f), ToRadians(7.0f), 0.0f });

	return true;
}
void ReleaseResources()
{
	ReleaseStates();

	if (gSceneTextureSRV)              gSceneTextureSRV->Release();
	if (gSceneRenderTarget)            gSceneRenderTarget->Release();
	if (gSceneTexture)                 gSceneTexture->Release();

	if (gDistortMapSRV)                gDistortMapSRV->Release();
	if (gDistortMap)                   gDistortMap->Release();
	if (gBurnMapSRV)                   gBurnMapSRV->Release();
	if (gBurnMap)                      gBurnMap->Release();
	if (gNoiseMapSRV)                  gNoiseMapSRV->Release();
	if (gNoiseMap)                     gNoiseMap->Release();

	if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV->Release();
	if (gLightDiffuseMap)              gLightDiffuseMap->Release();
	if (gCrateDiffuseSpecularMapSRV)   gCrateDiffuseSpecularMapSRV->Release();
	if (gCrateDiffuseSpecularMap)      gCrateDiffuseSpecularMap->Release();
	if (gCubeDiffuseSpecularMapSRV)    gCubeDiffuseSpecularMapSRV->Release();
	if (gCubeDiffuseSpecularMap)       gCubeDiffuseSpecularMap->Release();
	if (gGroundDiffuseSpecularMapSRV)  gGroundDiffuseSpecularMapSRV->Release();
	if (gGroundDiffuseSpecularMap)     gGroundDiffuseSpecularMap->Release();
	if (gStarsDiffuseSpecularMapSRV)   gStarsDiffuseSpecularMapSRV->Release();
	if (gStarsDiffuseSpecularMap)      gStarsDiffuseSpecularMap->Release();

	if (gPostProcessingConstantBuffer)  gPostProcessingConstantBuffer->Release();
	if (gPerModelConstantBuffer)        gPerModelConstantBuffer->Release();
	if (gPerFrameConstantBuffer)        gPerFrameConstantBuffer->Release();

	ReleaseShaders();

	// See note in InitGeometry about why we're not using unique_ptr and having to manually delete
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		delete gLights[i].model;  gLights[i].model = nullptr;
	}
	delete gCamera;  gCamera = nullptr;
	delete gCrate;   gCrate = nullptr;
	delete gCube;    gCube = nullptr;
	delete gGround;  gGround = nullptr;
	delete gStars;   gStars = nullptr;

	delete gLightMesh;   gLightMesh = nullptr;
	delete gCrateMesh;   gCrateMesh = nullptr;
	delete gCubeMesh;    gCubeMesh = nullptr;
	delete gGroundMesh;  gGroundMesh = nullptr;
	delete gStarsMesh;   gStarsMesh = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	gPerFrameConstants.cameraMatrix = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix = camera->ViewMatrix();
	gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
	gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

	// Indicate that the constant buffer we just updated is for use in the vertex shader (VS), geometry shader (GS) and pixel shader (PS)
	gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
	gD3DContext->GSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);


	////--------------- Render ordinary models ---------------///

	// Select which shaders to use next
	gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);

	// Render lit models, only change textures for each onee
	gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

	gD3DContext->PSSetShaderResources(0, 1, &gGroundDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gGround->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCrateDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCrate->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCubeDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCube->Render();


	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Using a pixel shader that tints the texture - don't need a tint on the sky so set it to white
	gPerModelConstants.objectColour = { 1, 1, 1 };

	// Stars point inwards
	gD3DContext->RSSetState(gCullNoneState);

	// Render sky
	gD3DContext->PSSetShaderResources(0, 1, &gStarsDiffuseSpecularMapSRV);
	gStars->Render();



	////--------------- Render lights ---------------////

	// Select which shaders to use next (actually same as before, so we could skip this)
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Select the texture and sampler to use in the pixel shader
	gD3DContext->PSSetShaderResources(0, 1, &gLightDiffuseMapSRV); // First parameter must match texture slot number in the shaer

	// States - additive blending, read-only depth buffer and no culling (standard set-up for blending)
	gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// Render all the lights in the array
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gPerModelConstants.objectColour = gLights[i].colour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		gLights[i].model->Render();
	}




}

//**************************



//this function calls the Draw() function to render the quad, but also switches the textures.
void RenderAndReset()
{
	// ########### render to texture ############
	//update buffers before rendering (so that shaders get up to date values)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	// draw the quad -- run the shaders
	gD3DContext->Draw(4, 0);
	// unbind SRV, ready for next render
	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	//###########################################

	// copy post process texture back to the scene texture
	gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);
	gD3DContext->PSSetShader(gCopy_PostProcess, nullptr, 0);
	gD3DContext->PSSetShaderResources(0, 1, &gPostProcessTextureSRV);

	// ########### render to texture ############ (same as above)
	//update buffers before rendering (so that shaders get up to date values)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	// draw the quad
	gD3DContext->Draw(4, 0);
	// unbind SRV, ready for next render
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	//###########################################
}

// Run any scene post-processing steps
void PostProcessing(float frameTime)
{
	timer += frameTime;

	// Using special vertex shader than creates its own data for a full screen quad
	gD3DContext->VSSetShader(gFullScreenQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, ignore depth buffer and culling
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gNoDepthBufferState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// No need to set vertex/index buffer (see fullscreen quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	gPostProcessingConstants.blurBellcurveStrength = blurCurve;
	gPostProcessingConstants.blurRadius = blurStrength;
	
	// Prepare custom settings for current post-process
	if (Tint)
	{
		// choose render target
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		//shader settings
		gPostProcessingConstants.tintColour = HSLToRGB(tintColour);
		gPostProcessingConstants.tintColour2 = HSLToRGB(tintColour2);

		//shader
		gD3DContext->PSSetShader(gTintPostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);


		RenderAndReset();
	}
	if (GaussianBlur)
	{
		// the first pass - blur horizontally
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		// blur Horizontally shader
		gPostProcessingConstants.blurBellcurveStrength = blurCurve;
		gPostProcessingConstants.blurRadius = blurStrength;

		//blur
		gD3DContext->PSSetShader(gGaussianBlurH_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);


		RenderAndReset();

		// the second pass - blur vertically
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		// blur Vertically shader
		gD3DContext->PSSetShader(gGaussianBlurV_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV); // use postprocess texture as input
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

		RenderAndReset();

	}
	if (Blur)
	{
		// choose render target
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		// blur settings
		gPostProcessingConstants.blurBellcurveStrength = blurCurve;
		gPostProcessingConstants.blurRadius = blurStrength;

		// blur
		gD3DContext->PSSetShader(gBlur_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

		RenderAndReset();
	}
	if (Underwater)
	{
		// choose render target
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		// underwater settings
		gPostProcessingConstants.waterTintColour = { 0, 1, 1 };
		gPostProcessingConstants.waterTintColour2 = { 0, 0.5, 1 };
		gPostProcessingConstants.hWave = timer;
		gPostProcessingConstants.vWave = timer / 2;

		// underwater shader
		gD3DContext->PSSetShader(gUnderwater_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

		RenderAndReset();

	}
	if (Retro)
	{
		// choose render target
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		//shader settings
		gPostProcessingConstants.noiseScale = { pixelSize, pixelSize };

		//pixellate
		gD3DContext->PSSetShader(gPixellate_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

		RenderAndReset();


		// choose render target
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

		gPostProcessingConstants.bitColour = bitColour;
		//bitColour
		gD3DContext->PSSetShader(gBitColour_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

		RenderAndReset();

	}
	if (Bloom)
	{
		// render to the bloom texture
		gD3DContext->OMSetRenderTargets(1, &gBloomRenderTarget, gDepthStencil); //render to bloom texture
																				//   ^
		gPostProcessingConstants.brightFilterThreshold = 0.7f;					//   |
		//shader																//   |
		gD3DContext->PSSetShader(gBrightFilter_PostProcess, nullptr, 0);		//   |
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);				// render from scene
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);


			// ########### Draw and reset - no copy  ############
			UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
			gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
			gD3DContext->Draw(4, 0);
			ID3D11ShaderResourceView* nullSRV = nullptr;
			gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
			//###########################################


		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);	// render to post process
																						//  ^
		// blur Horizontally shader														//  |
		gPostProcessingConstants.blurBellcurveStrength = blurCurve;						//  |
		gPostProcessingConstants.blurRadius = blurStrength;								//  |
																						//  |
		//blur																			//  |
		gD3DContext->PSSetShader(gGaussianBlurH_PostProcess, nullptr, 0);				//  |
		gD3DContext->PSSetShaderResources(0, 1, &gBloomTextureSRV);						// render from bloom
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);


			// ########### Draw and reset - no copy  ############
			UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
			gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
			gD3DContext->Draw(4, 0);
			gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
			//###########################################

		gD3DContext->OMSetRenderTargets(1, &gBloomRenderTarget, gDepthStencil);		// render to bloom texture
																					//  ^
		// blur Vertically shader													//  |
		gD3DContext->PSSetShader(gGaussianBlurV_PostProcess, nullptr, 0);			//  |
		gD3DContext->PSSetShaderResources(0, 1, &gPostProcessTextureSRV);			// render from  post process
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

			// ########### Draw and reset - no copy  ############
			UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
			gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
			gD3DContext->Draw(4, 0);
			gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
			//###########################################

		// combine
		gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);		// render to postProcess

		// combine shader											
		gD3DContext->PSSetShader(gCombine_PostProcess, nullptr, 0);
		gD3DContext->PSSetShaderResources(0, 1, &gBloomTextureSRV);							// combine textures from
		gD3DContext->PSSetShaderResources(1, 1, &gSceneTextureSRV);							//  bloom and scene
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler);



		RenderAndReset();
	}



	else if (gCurrentPostProcess == PostProcess::Spiral)
	{
		gD3DContext->PSSetShader(gNoise_PostProcess, nullptr, 0);

		static float wiggle = 0.0f;
		const float wiggleSpeed = 1.0f;

		// Set and increase the amount of spiral - use a tweaked cos wave to animate
		gPostProcessingConstants.spiralLevel = ((1.0f - cos(wiggle)) * 4.0f );
		wiggle += wiggleSpeed * frameTime;
	}
	else if (gCurrentPostProcess == PostProcess::PyramidBlur)
	{
	// choose render target
	gD3DContext->OMSetRenderTargets(1, &gPostProcessRenderTarget, gDepthStencil);

	gD3DContext->PSSetShader(gPyramidBlur_PostProcess, nullptr, 0);
	gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);
	gD3DContext->PSSetSamplers(0, 1, &gPointSampler);

	RenderAndReset();
	}

	//copy to back buffer
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);
	gD3DContext->PSSetShader(gCopy_PostProcess, nullptr, 0);
	gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV);

	gD3DContext->Draw(4, 0);


	// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
}

//**************************



// Rendering the scene
void RenderScene(float frameTime)
{
	//// Common settings ////

	// Set up the light information in the constant buffer
	// Don't send to the GPU yet, the function RenderSceneFromCamera will do that
	gPerFrameConstants.light1Colour   = gLights[0].colour * gLights[0].strength;
	gPerFrameConstants.light1Position = gLights[0].model->Position();
	gPerFrameConstants.light2Colour   = gLights[1].colour * gLights[1].strength;
	gPerFrameConstants.light2Position = gLights[1].model->Position();

	gPerFrameConstants.ambientColour  = gAmbientColour;
	gPerFrameConstants.specularPower  = gSpecularPower;
	gPerFrameConstants.cameraPosition = gCamera->Position();

	gPerFrameConstants.viewportWidth  = static_cast<float>(gViewportWidth);
	gPerFrameConstants.viewportHeight = static_cast<float>(gViewportHeight);

	gPerFrameConstants.frameTime = frameTime;



	////--------------- Main scene rendering ---------------////

	// Set the target for rendering and select the main depth buffer.
	// If using post-processing then render to the scene texture, otherwise to the usual back buffer
	// Also clear the render target to a fixed colour and the depth buffer to the far distance
	if (gCurrentPostProcess != PostProcess::None
		|| Tint
		|| Blur
		|| GaussianBlur
		|| Underwater
		|| Retro
		|| Bloom)
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gSceneRenderTarget, &gBackgroundColor.r);
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
	}
	gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setup the viewport to the size of the main window
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	// Render the scene from the main camera
	RenderSceneFromCamera(gCamera);


	////--------------- Scene completion ---------------////

	// Run any post-processing steps
	if (gCurrentPostProcess != PostProcess::None || Tint
		|| Blur
		|| GaussianBlur
		|| Underwater
		|| Retro
		|| Bloom)  
		PostProcessing(frameTime);

	// When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
	// Set first parameter to 1 to lock to vsync
	gSwapChain->Present(lockFPS ? 1 : 0, 0);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
	tintColour.x += frameTime * 20;
	if (tintColour.x > 360) tintColour.x = 0;
	tintColour2.x += (frameTime*50);
	if (tintColour2.x > 360) tintColour2.x = 0;


	// Select post process on keys
	if (KeyHit(Key_1))   Tint = !Tint;
	if (KeyHit(Key_F1))  Blur = !Blur;
	if (KeyHit(Key_2))   GaussianBlur = !GaussianBlur;
	if (KeyHit(Key_3))   Underwater = !Underwater;
	if (KeyHit(Key_4))   Retro = !Retro;
	if (KeyHit(Key_5))   Bloom = !Bloom;


	//if (KeyHit(Key_5))  gCurrentPostProcess = PostProcess::Spiral;
	if (KeyHit(Key_0))  gCurrentPostProcess = PostProcess::None;

	if (KeyHeld(Key_Comma)) blurStrength--;
	if (KeyHeld(Key_Period)) blurStrength++;
	if (blurStrength < 5) blurStrength = 5;

	if (KeyHeld(Key_K)) blurCurve /= 1.1f;
	if (KeyHeld(Key_L)) blurCurve *= 1.1f;


	if (KeyHeld(Key_V)) bitColour--;
	if (bitColour < 1) bitColour = 1;
	if (KeyHeld(Key_B)) bitColour++;



	if (KeyHeld(Key_F)) pixelSize--;
	if (pixelSize < 1) pixelSize = 1;
	if (KeyHeld(Key_G)) pixelSize++;

	// Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float lightRotate = 0.0f;
	static bool go = true;
	gLights[0].model->SetPosition({ 20 + cos(lightRotate) * gLightOrbitRadius, 10, 20 + sin(lightRotate) * gLightOrbitRadius });
	if (go)  lightRotate -= gLightOrbitSpeed * frameTime;
	if (KeyHit(Key_L))  go = !go;

	// Control of camera
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);

	// Toggle FPS limiting
	if (KeyHit(Key_P))  lockFPS = !lockFPS;

	// Show frame time / FPS in the window title //
	const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
	static float totalFrameTime = 0;
	static int frameCount = 0;
	totalFrameTime += frameTime;
	++frameCount;
	if (totalFrameTime > fpsUpdateTime)
	{
		// Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
		float avgFrameTime = totalFrameTime / frameCount;
		std::ostringstream frameTimeMs;
		frameTimeMs.precision(2);
		frameTimeMs << std::fixed << avgFrameTime * 1000;
		std::string windowTitle = "CO3303 Week 13: Full Screen Post Processing - Frame Time: " + frameTimeMs.str() +
			"ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
		SetWindowTextA(gHWnd, windowTitle.c_str());
		totalFrameTime = 0;
		frameCount = 0;
	}
}
