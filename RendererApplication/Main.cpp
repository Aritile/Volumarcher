<<<<<<< HEAD
///Demo renderer application,
///Implements a demo renderer that integrates Volumarcher with a simple rasterizer.


#include "../MiniEngine/Model/Model.h"
#include "../MiniEngine/Model/ModelLoader.h"
#include "../MiniEngine/Model/Renderer.h"
=======
#include "Display.h"
#include "pch.h"
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
#include "../MiniEngine/Core/GameCore.h"
#include "../MiniEngine/Core/SystemTime.h"
<<<<<<< HEAD
=======
#include "../MiniEngine/Core/TextRenderer.h"
#include "GameInput.h"
#include "PostEffects.h"
#include "UploadBuffer.h"
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
#include "../MiniEngine/Core/CommandContext.h"
#include "../MiniEngine/Core/RootSignature.h"
#include "../MiniEngine/Core/PipelineState.h"
#include "../MiniEngine/Core/BufferManager.h"
#include "CameraController.h"

#include "pch.h"
#include "GameInput.h"
#include "PostEffects.h"

#include "NoiseTexturePath.h"

#include "glm/glm.hpp"

//Optional defines
//#define VOLUMARCHER_NO_STB_IMAGE //Does not use stb image, you have to load noise textures manually
//#define VOLUMARCHER_NO_SKYBOX //Removes example skybox rendering
#include "Volumarcher.h"


<<<<<<< HEAD
enum class ArtisticExampleSettings
{
	Morning,
	Day,
	Night
};

static constexpr ArtisticExampleSettings StartSettings = ArtisticExampleSettings::Day;

const char* clouds[] = {
	"landscape",
	"landscape2",
	"singlecloud",
	"disney",
	"rainbow",
	"nebula",
	"kraken"
};

=======
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
using namespace GameCore;
using namespace Graphics;

class RendererApplication : public GameCore::IGameApp
{
public:
	RendererApplication()
	{
	}

	void Startup(void) override;

	void Cleanup(void) override;

	void Update(float deltaT) override;

	void InitRasterizor();

	void RenderRasterizerPass();

<<<<<<< HEAD
	void SetStyle(ArtisticExampleSettings _style);

=======
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
	void RenderScene(void) override;

private:
	std::unique_ptr<Volumarcher::VolumarcherContext> m_volumetricContext;

	glm::vec3 m_camPos{0.f};
	glm::vec3 m_camForward{1, 0, 0};
	float m_camYaw{0.f};
	float m_camPitch{0.f};
	const float m_cameraSpeed{2.f};
	const float m_cameraRotSpeed{0.7f};
	const float m_vFov{70.f};
<<<<<<< HEAD
	float m_near{0.1f};
	float m_far{100.f};

	double m_timer{0.f};
	double m_cloudOffsetTimer{0.f};

	std::vector<Volumarcher::PointLight> m_lights = std::vector<Volumarcher::PointLight>(10);

	std::chrono::steady_clock::time_point m_lastTime;
=======
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01

	//Cube/Rasterizor stuff
	StructuredBuffer vertexBuffer;
	StructuredBuffer indexBuffer;

<<<<<<< HEAD

	RootSignature g_RootSig;
	GraphicsPSO g_CubePSO;

	ModelInstance m_model;
	Math::Camera m_camera;
	std::unique_ptr<CameraController> m_cameraController;

	glm::vec3 m_sunDir;

	//--
	int32_t m_loadedCloud = 0;
	EnumVar m_cloud{"DemoApplication/Cloud", m_loadedCloud, _countof(clouds), clouds};
	BoolVar m_rotateSun{"DemoApplication/Rotate Sun", true};
	NumVar m_sunSpeed{"DemoApplication/Sun speed", 0.1f, 0, 3, 0.05f};
	BoolVar m_lightsEnabled{"DemoApplication/Spawn lights at center", false};

	struct ArtSettings
	{
		NumVar m_worldDensity{"Volumarcher/Look/Cloud Density scale", 1.f, 0.f, 50.f, 0.1f};
		NumVar m_eccentricity{"Volumarcher/Look/Eccentricity ", 0.2f, 0.f, 1.f, 0.05f};
		BoolVar m_ambientEnabled{"Volumarcher/Look/Ambient Light", true};
		NumVar m_colorR{"Volumarcher/Look/Cloud color R", 1.f, 0.f, 1.f, 0.05f};
		NumVar m_colorG{"Volumarcher/Look/Cloud color G", 1.f, 0.f, 1.f, 0.05f};
		NumVar m_colorB{"Volumarcher/Look/Cloud color B", 1.f, 0.f, 1.f, 0.05f};
		NumVar m_dWindSpeed{"Volumarcher/Look/Detail Wind speed", 0.3f, 0.f, 1.f, 0.02f};
		NumVar m_sWindSpeed{"Volumarcher/Look/Structural Wind speed", 0.f, 0.f, 5.f, 0.05f};
		NumVar m_frequency{"Volumarcher/Look/Noise frequency", 1.5f, 0.1f, 4.f, 0.1f};
	} m_artSettings;

	struct TuningSettings
	{
		NumVar reproject{"Volumarcher/Quality/Reprojection amount", 1.f, 0.f, 1.f, 0.1f};
		IntVar resolutionDivide{"Volumarcher/Quality/Resolution scale", 2, 1, 8};
		IntVar baseSamples{"Volumarcher/Quality/BaseSampleCount", 128, 0, 512};
		IntVar realDirect{"Volumarcher/Quality/Real Direct Light samples", 2, 0, 128};
		IntVar fakeDirect{"Volumarcher/Quality/Cached Direct Light samples", 32, 0, 128};
		IntVar cacheUpdateSize{"Volumarcher/Quality/LightCache update chunk size", 64, 0, 512, 8};
		IntVar ambient{"Volumarcher/Quality/Ambient Light samples", 64, 0, 128};
		IntVar lightCacheSize{"Volumarcher/Quality/Light Cache VoxelSize", 2, 0, 8};
		NumVar renderDist{"Volumarcher/Quality/High quality Target Distance", 30, 0, 100};
		NumVar renderDistLow{"Volumarcher/Quality/Low quality Target Distance", 80.f, 0, 100};
		NumVar lightRenderDist{"Volumarcher/Quality/Target Light renderDistance", 60, -1, 100};
	} m_tuningSettings;
};

//Example integration with an existing pass, not relevant to Volumarcher
#pragma region Rasterizer

void RendererApplication::InitRasterizor()
{
	Renderer::Initialize();

	m_model = Renderer::LoadModel(L"./assets/models/biglandscape.gltf", false);
	m_model.Resize(50.0f);
	m_model.GetTransform().SetTranslation({0, -7, 0});

	m_camera.SetZRange(m_near, m_far);
	m_camera.SetFOV(glm::radians(m_vFov));
	m_cameraController.reset(new FlyingFPSCamera(m_camera, Vector3(kYUnitVector)));
}

#pragma endregion
=======
	RootSignature g_RootSig;
	GraphicsPSO g_CubePSO;
};

#pragma region Cube

//Cube code generated by ChatGPT
struct Vertex
{
	float position[3];
	float normal[3];
};

static const Vertex cubeVertices[] =
{
	// Front face (-Z)
	{{-1, -1, -1}, {0, 0, -1}},
	{{1, -1, -1}, {0, 0, -1}},
	{{1, 1, -1}, {0, 0, -1}},
	{{-1, 1, -1}, {0, 0, -1}},

	// Back face (+Z)
	{{-1, -1, 1}, {0, 0, 1}},
	{{1, -1, 1}, {0, 0, 1}},
	{{1, 1, 1}, {0, 0, 1}},
	{{-1, 1, 1}, {0, 0, 1}},

	// Left face (-X)
	{{-1, -1, -1}, {-1, 0, 0}},
	{{-1, 1, -1}, {-1, 0, 0}},
	{{-1, 1, 1}, {-1, 0, 0}},
	{{-1, -1, 1}, {-1, 0, 0}},

	// Right face (+X)
	{{1, -1, -1}, {1, 0, 0}},
	{{1, 1, -1}, {1, 0, 0}},
	{{1, 1, 1}, {1, 0, 0}},
	{{1, -1, 1}, {1, 0, 0}},

	// Top face (+Y)
	{{-1, 1, -1}, {0, 1, 0}},
	{{1, 1, -1}, {0, 1, 0}},
	{{1, 1, 1}, {0, 1, 0}},
	{{-1, 1, 1}, {0, 1, 0}},

	// Bottom face (-Y)
	{{-1, -1, -1}, {0, -1, 0}},
	{{1, -1, -1}, {0, -1, 0}},
	{{1, -1, 1}, {0, -1, 0}},
	{{-1, -1, 1}, {0, -1, 0}},
};

static const uint16_t cubeIndices[] =
{
	// Front face
	0, 1, 2, 0, 2, 3,
	// Back face
	4, 5, 6, 4, 6, 7,
	// Left face
	8, 9, 10, 8, 10, 11,
	// Right face
	12, 13, 14, 12, 14, 15,
	// Top face
	16, 17, 18, 16, 18, 19,
	// Bottom face
	20, 21, 22, 20, 22, 23
};

struct MatrixBuffer
{
	glm::mat4 MVP;
};

#include "CompiledShaders/PixelShader.h"
#include "CompiledShaders/VertexShader.h"

void RendererApplication::InitRasterizor()
{
	g_RootSig.Reset(1, 0);
	g_RootSig[0].InitAsConstants(0, sizeof(MatrixBuffer) / sizeof(float)); // Camera mat
	g_RootSig.Finalize(L"CubeRootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	g_CubePSO.SetRootSignature(g_RootSig);
	g_CubePSO.SetInputLayout(_countof(layout), layout);
	g_CubePSO.SetRasterizerState({
		D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, true, -1, -1.f, 0, true, false, false, 0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	});
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	depthStencilDesc.StencilEnable = false;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	D3D12_DEPTH_STENCILOP_DESC stencilDesc{};
	depthStencilDesc.BackFace = stencilDesc;
	depthStencilDesc.FrontFace = stencilDesc;
	g_CubePSO.SetDepthStencilState(depthStencilDesc);
	D3D12_BLEND_DESC blendState;
	blendState.IndependentBlendEnable = false;
	blendState.RenderTarget[0].BlendEnable = false;
	blendState.RenderTarget[0].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
	g_CubePSO.SetBlendState(blendState);
	g_CubePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	g_CubePSO.SetRenderTargetFormat(DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_D32_FLOAT);
	g_CubePSO.SetVertexShader(g_pVertexShader, sizeof(g_pVertexShader));
	g_CubePSO.SetPixelShader(g_pPixelShader, sizeof(g_pPixelShader));


	g_CubePSO.Finalize();

	vertexBuffer.Create(L"Cube vertices", _countof(cubeVertices), sizeof(cubeVertices[0]), &cubeVertices);
	indexBuffer.Create(L"Cube Indices", _countof(cubeIndices), sizeof(cubeIndices[0]), &cubeIndices);
}

#pragma endregion

>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01

CREATE_APPLICATION(RendererApplication)

void RendererApplication::Startup(void)
{
	Utility::Printf("Starting Volumarcher demo\n");

	InitRasterizor();

	Utility::Printf("Creating Volumetric Context\n");
	CpuTimer startupTimer;
	startupTimer.Start();
<<<<<<< HEAD

	///Create a volumarcher context,

	//Noise used for adding detail red channel is low frequency green is high frequency, use supplied textures if unsure
	Volumarcher::DetailNoise detailNoise(NOISE_TEXTURES);

	//Texture array for blue noise, use supplied textures
	Volumarcher::BlueNoise blueNoise(BLUENOISE_TEXTURES);

	//settings of your own camera, this needs to be the same as your geometry pass
	Volumarcher::CameraSettings cameraSettings{m_near, m_far, m_vFov};
	m_volumetricContext = std::make_unique<Volumarcher::VolumarcherContext>(detailNoise, blueNoise, cameraSettings);

	//Load a world grid of clouds
	std::string cloud = clouds[m_cloud];
	auto result [[maybe_unused]] = m_volumetricContext->LoadGrid("./assets/" + cloud + ".vdb", 0.3f,
	                                                             glm::vec3(0.f, 5.f, 0.f));
	assert(result == Volumarcher::Result::Succeeded);

	m_camPos = {0, 0, 0};
	SetStyle(StartSettings);
=======
	Volumarcher::CameraSettings cameraSettings{0.01f, 50.f, m_vFov};
	m_volumetricContext = std::make_unique<Volumarcher::VolumetricContext>(cameraSettings);

	m_volumetricContext->LoadGrid("../assets/disney.vdb", glm::vec3(6.f), glm::vec3(0), 6.f);

>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01

	PostEffects::BloomEnable = true;
	PostEffects::EnableHDR = true;
	PostEffects::EnableAdaptation = false;
	startupTimer.Stop();

	Utility::Printf("VolumetricContext Startup Time: %fs\n", startupTimer.GetTime());
}

void RendererApplication::Cleanup(void)
{
	Renderer::Shutdown();
}


void RendererApplication::Update(const float _deltaTime)
{
	ScopedTimer _prof(L"Update State");

<<<<<<< HEAD
	//For rasterizer pas
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");

	m_model.Update(gfxContext, _deltaTime);
=======
	m_volumetricContext->Update(_deltaTime);

	if (GameInput::IsFirstPressed(GameInput::DigitalInput::kKey_tab))
	{
		GameCore::g_mouseLocked = !GameCore::g_mouseLocked;
		static int cursorPos[2]{0, 0};
	}
	if (GameCore::g_mouseLocked)
	{
		//Rotation
		m_camYaw -= GameInput::GetAnalogInput(GameInput::kAnalogMouseX) * m_cameraRotSpeed;
		m_camPitch -= GameInput::GetAnalogInput(GameInput::kAnalogMouseY) * m_cameraRotSpeed;
		m_camPitch = glm::clamp(m_camPitch, -89.99f, 89.99f);
		m_camRot = glm::angleAxis(m_camYaw, glm::vec3(0, 1, 0)) * glm::angleAxis(m_camPitch, glm::vec3(1, 0, 0));
	}

>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01

	gfxContext.Finish();

	//Updates internal timer of volumarcher for animated effects/noise
	m_volumetricContext->Update(_deltaTime);

	//This is somewhat costly to call every frame, but fine enough for this demo
	if (m_lightsEnabled)
	{
		if (rand() / static_cast<float>(RAND_MAX) > 0.9f)
		{
			m_lights[rand() % m_lights.size()] = {
				glm::vec3((rand() / static_cast<float>(RAND_MAX) - 0.5f) * 10,
				          (rand() / static_cast<float>(RAND_MAX) - 0.5f) * 10 + 5,
				          (rand() / static_cast<float>(RAND_MAX) - 0.5f) * 10),
				(rand() / static_cast<float>(RAND_MAX)) * 10, glm::vec3(10, 10, 25)
			};
			m_volumetricContext->SetPointLights(m_lights);
		}
	}
	else
	{
		m_volumetricContext->SetPointLights({});
	}


	{
		//Example camera input

		if (GameInput::IsFirstPressed(GameInput::DigitalInput::kKey_tab))
		{
			GameCore::g_mouseLocked = !GameCore::g_mouseLocked;
			static int cursorPos[2]{0, 0};
		}

		m_cameraController->Update(_deltaTime);
		auto camPos = m_camera.GetPosition();
		m_camPos = {camPos.GetX(), camPos.GetY(), camPos.GetZ()};
		auto camForward = m_camera.GetForwardVec();
		m_camForward = {camForward.GetX(), camForward.GetY(), camForward.GetZ()};
	}

	{
		// Some example clouds to load
		if (m_loadedCloud != m_cloud)
		{
			m_loadedCloud = m_cloud;
			std::string cloud = clouds[m_cloud];

			if (cloud == "nebula")
			{
				m_volumetricContext->SetPointLights({
					{
						glm::vec3(2, 10, 2
						),
						20.f, glm::vec3(1, 1, 1) * 50.f
					},
					{
						glm::vec3(3, 20, -3
						),
						25.f, glm::vec3(1, 1, 1) * 30.f
					}
				});
			}
			else
			{
				m_volumetricContext->SetPointLights({});
			}
			auto result [[maybe_unused]] = m_volumetricContext->LoadGrid(
				"./assets/" + cloud + ".vdb", 0.3f, glm::vec3(0.f, 5.f, 0.f));
			assert(result == Volumarcher::Result::Succeeded);
		}
	}
	{
		// Some example environments
		//Style settings
		if (GameInput::IsFirstReleased(GameInput::kKey_1)) SetStyle(ArtisticExampleSettings::Morning);
		else if (GameInput::IsFirstReleased(GameInput::kKey_2)) SetStyle(ArtisticExampleSettings::Day);
		else if (GameInput::IsFirstReleased(GameInput::kKey_3)) SetStyle(ArtisticExampleSettings::Night);
	}

	//Rotate sun visualisation
	if (m_rotateSun)
	{
		m_timer += _deltaTime * m_sunSpeed;

		auto envSettings = m_volumetricContext->GetEnvironmentSettings();
		//float sunPitch = cos(m_timer) * 0.5 + 0.5;
		//envSettings.sunDirection = normalize(glm::vec3(sin(m_timer * 0.27) * (1 - sunPitch), -sunPitch,
		//                                               cos(m_timer * 0.27)) * ((1 - sunPitch)));
		envSettings.sunDirection = normalize(glm::vec3(sin(m_timer) * 0.2f, cos(m_timer) - 0.8, sin(m_timer) * 4.75));

		m_sunDir = {-envSettings.sunDirection.x, -envSettings.sunDirection.y, -envSettings.sunDirection.z};

		envSettings.ambientLight = m_artSettings.m_ambientEnabled
			                           ? m_volumetricContext->GetSkyBackgroundAmbient(m_sunDir)
			                           : glm::vec3(0.f);
		envSettings.sunLight = m_volumetricContext->GetSkyBackgroundSunlight(m_sunDir);


		m_volumetricContext->SetEnvironmentSettings(envSettings);
	}


	//EngineTuning UI for changing variables
	Volumarcher::QualitySettings settings{
		1920 / m_tuningSettings.resolutionDivide,
		1080 / m_tuningSettings.resolutionDivide,
		m_tuningSettings.reproject,
		m_tuningSettings.baseSamples,
		m_tuningSettings.realDirect,
		m_tuningSettings.fakeDirect,
		m_tuningSettings.cacheUpdateSize,
		m_tuningSettings.ambient,
		static_cast<uint>(m_tuningSettings.lightCacheSize),
		m_tuningSettings.renderDist,
		m_tuningSettings.renderDistLow,
		m_tuningSettings.lightRenderDist
	};

	//This sets settings, if some settings update it updates grids cache or recalculate lighting, so do not change settings frequently
	m_volumetricContext->SetQualitySettingsAndUpdateGrid(settings);

	m_cloudOffsetTimer += _deltaTime * m_artSettings.m_sWindSpeed;
	if (m_artSettings.m_sWindSpeed == 0) m_cloudOffsetTimer = 0;

	Volumarcher::ArtisticSettings artSettings{

		m_artSettings.m_worldDensity,
		m_artSettings.m_eccentricity,
		glm::vec3(m_artSettings.m_colorR, m_artSettings.m_colorG, m_artSettings.m_colorB),
		glm::vec2(1, 0.5f) * static_cast<float>(m_cloudOffsetTimer),
		glm::vec3(1, 0, 0.5) * static_cast<float>(m_artSettings.m_dWindSpeed),
		m_artSettings.m_frequency
	};

	m_volumetricContext->SetCloudLookSettings(artSettings);
}

<<<<<<< HEAD
//Renders a landscape to showcase integration with existing passes, Not relevant to Volumarcher

void RendererApplication::RenderRasterizerPass()
{
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Rasterizer Graphics Pass");

	GlobalConstants globals;
	globals.ViewProjMatrix = m_camera.GetViewProjMatrix();
	globals.CameraPos = m_camera.GetPosition();
	globals.SunDirection = {m_sunDir.x, m_sunDir.y, m_sunDir.z};

	auto sky = m_volumetricContext->GetSkyBackgroundSunlight(m_sunDir);
	globals.SunIntensity = Vector3(sky.x, sky.y, sky.z) * 0.075f;

	// Begin rendering depth
	gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	gfxContext.ClearDepth(g_SceneDepthBuffer);

	Renderer::MeshSorter sorter(Renderer::MeshSorter::kDefault);
	D3D12_VIEWPORT viewport = {
		0, 0, static_cast<float>(g_SceneColorBuffer.GetWidth()), static_cast<float>(g_SceneColorBuffer.GetHeight()),
		0.f, 1.f
	};
	D3D12_RECT scissor = {
		0, 0, static_cast<LONG>(g_SceneColorBuffer.GetWidth()), static_cast<LONG>(g_SceneColorBuffer.GetHeight())
	};
	sorter.SetCamera(m_camera);
	sorter.SetViewport(viewport);
	sorter.SetScissor(scissor);
	sorter.SetDepthStencilTarget(g_SceneDepthBuffer);
	sorter.AddRenderTarget(g_SceneColorBuffer);

	m_model.Render(sorter);


	sorter.Sort();
	{
		ScopedTimer _prof(L"Depth Pre-Pass", gfxContext);
		sorter.RenderMeshes(Renderer::MeshSorter::kZPass, gfxContext, globals);
	}
	{
		ScopedTimer _outerprof(L"Main Render", gfxContext);

		gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		gfxContext.ClearColor(g_SceneColorBuffer);

		gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
		gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV_DepthReadOnly());
		gfxContext.SetViewportAndScissor(viewport, scissor);

		sorter.RenderMeshes(Renderer::MeshSorter::kOpaque, gfxContext, globals);
	}
	gfxContext.Finish();
}

void RendererApplication::SetStyle(ArtisticExampleSettings _style)
{
	//Some examples of environments

	Volumarcher::ArtisticSettings cloudLook;
	Volumarcher::EnvironmentSettings environmentLook;


	//Examples of some artistic settings
	switch (_style)
	{
	case ArtisticExampleSettings::Day:
		{
			cloudLook.cloudColor = glm::vec3(1.f);
			cloudLook.eccentricity = 0.3f;
			cloudLook.noiseFrequency = 0.2;
			cloudLook.noiseWind = glm::vec3(1.f, 0, 0.5f) * 0.15f;

			environmentLook.sunDirection = glm::normalize(glm::vec3(0.5f, -2.f, 0.5f));

			environmentLook.ambientLight = m_volumetricContext->GetSkyBackgroundAmbient(-environmentLook.sunDirection);

			environmentLook.sunLight = m_volumetricContext->GetSkyBackgroundSunlight(-environmentLook.sunDirection);

			break;
		}
	case ArtisticExampleSettings::Morning:
		{
			cloudLook.cloudColor = glm::vec3(1.f);
			cloudLook.eccentricity = 0.3f;
			cloudLook.noiseFrequency = 0.2;
			cloudLook.noiseWind = glm::vec3(1.f, 0, 0.5f) * 0.1f;

			environmentLook.sunDirection = glm::normalize(glm::vec3(-0.5f, -0.2f, -0.5f));
			environmentLook.ambientLight = m_volumetricContext->GetSkyBackgroundAmbient(-environmentLook.sunDirection);
			environmentLook.sunLight = m_volumetricContext->GetSkyBackgroundSunlight(-environmentLook.sunDirection);
			break;
		}
	case ArtisticExampleSettings::Night:
		{
			cloudLook.cloudColor = glm::vec3(0.95f, 0.1f, 1.f) * 0.01f;
			cloudLook.eccentricity = 0.2f;
			cloudLook.noiseFrequency = 0.1;
			cloudLook.noiseWind = glm::vec3(0.f);

			environmentLook.sunDirection = glm::normalize(glm::vec3(0.2f, 0.f, 0.5f));
			environmentLook.ambientLight = m_volumetricContext->GetSkyBackgroundAmbient(-environmentLook.sunDirection);
			environmentLook.sunLight = m_volumetricContext->GetSkyBackgroundSunlight(-environmentLook.sunDirection);

			break;
		}
	}
	m_sunDir = -environmentLook.sunDirection;

	m_volumetricContext->SetCloudLookSettings(cloudLook);
	m_volumetricContext->SetEnvironmentSettings(environmentLook);
=======

void RendererApplication::RenderRasterizerPass()
{
	GraphicsContext& graphicsContext = GraphicsContext::Begin(L"Graphics Pass");
	graphicsContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	graphicsContext.TransitionResource(g_SceneDepthBuffer,
	                                   D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	graphicsContext.ClearColor(g_SceneColorBuffer);
	graphicsContext.ClearDepth(g_SceneDepthBuffer);
	graphicsContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
	graphicsContext.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
	graphicsContext.SetRootSignature(g_RootSig);
	graphicsContext.SetPipelineState(g_CubePSO);

	graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBuffer.GetGpuVirtualAddress();
	vbv.SizeInBytes = vertexBuffer.GetBufferSize();
	vbv.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBuffer.GetGpuVirtualAddress();
	ibv.SizeInBytes = indexBuffer.GetBufferSize();
	ibv.Format = DXGI_FORMAT_R16_UINT;

	graphicsContext.SetVertexBuffer(0, vbv);
	graphicsContext.SetIndexBuffer(ibv);

	glm::vec3 camDir = m_camRot * glm::vec3(0, 0, 1);
	glm::mat4 view = glm::lookAtRH(m_camPos, m_camPos + camDir, glm::vec3(0, 1, 0));
	static const glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(m_vFov), (16.f / 9.f), 50.f, 0.01f);

	MatrixBuffer constants{projection * view * glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 0, -2))};
	graphicsContext.SetConstantArray(0, sizeof(MatrixBuffer) / sizeof(uint32_t), &constants);

	graphicsContext.DrawIndexed(_countof(cubeIndices));

	graphicsContext.Finish();
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
}


void RendererApplication::RenderScene(void)
{
<<<<<<< HEAD
#if 1

	//Example pass to show integration with existing geometry pass, this draws a cube and outputs int g_SceneColorBuffer and g_SceneDepthBuffer
	RenderRasterizerPass();

	//Optional skybox
	m_volumetricContext->RenderSkyBackground(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, g_SceneDepthBuffer,
	                                         m_camForward);
#else
	auto& context = GraphicsContext::Begin(L"ClearScreen");
	float color[4] = { 0.f, 0.f, 0.01f, 1.f };
	context.ClearColor(g_SceneColorBuffer, color);
	context.Finish();
#endif

	//Render actual volumetrics
	m_volumetricContext->RenderVolumetrics(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, g_SceneDepthBuffer,
	                                       m_camPos,
	                                       m_camForward);
=======
	RenderRasterizerPass();
	m_volumetricContext->Render(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, g_SceneDepthBuffer, m_camPos,
	                            m_camRot);
>>>>>>> 57591406dec9de690cf8c3265bfe9e668a263d01
}
