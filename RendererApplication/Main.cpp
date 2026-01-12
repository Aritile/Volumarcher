///Demo renderer application,
///Implements a demo renderer that integrates Volumarcher with a simple rasterizer.


#include "../MiniEngine/Model/Model.h"
#include "../MiniEngine/Model/ModelLoader.h"
#include "../MiniEngine/Model/Renderer.h"
#include "../MiniEngine/Core/GameCore.h"
#include "../MiniEngine/Core/SystemTime.h"
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

	void SetStyle(ArtisticExampleSettings _style);

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
	float m_near{0.1f};
	float m_far{100.f};

	double m_timer{0.f};
	double m_cloudOffsetTimer{0.f};

	std::vector<Volumarcher::PointLight> m_lights = std::vector<Volumarcher::PointLight>(10);

	std::chrono::steady_clock::time_point m_lastTime;

	//Cube/Rasterizor stuff
	StructuredBuffer vertexBuffer;
	StructuredBuffer indexBuffer;


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

CREATE_APPLICATION(RendererApplication)

void RendererApplication::Startup(void)
{
	Utility::Printf("Starting Volumarcher demo\n");

	InitRasterizor();

	Utility::Printf("Creating Volumetric Context\n");
	CpuTimer startupTimer;
	startupTimer.Start();

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

	//For rasterizer pas
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");

	m_model.Update(gfxContext, _deltaTime);

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
}


void RendererApplication::RenderScene(void)
{
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
}
