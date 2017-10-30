#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderSurface.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "VideoPlayerSample.h"
#include "VideoPlayer.h"

URHO3D_DEFINE_APPLICATION_MAIN(VideoPlayerSample)

VideoPlayerSample::VideoPlayerSample(Context* context) :
	Sample(context)
{
	context->RegisterFactory<VideoPlayer>();
}

void VideoPlayerSample::Start()
{
	Sample::Start();
	CreateScene();
	CreateInstructions();
	SetupViewport();
	SubscribeToEvents();
	Sample::InitMouseMode(MM_RELATIVE);
}

void VideoPlayerSample::CreateScene()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	scene_ = new Scene(context_);

	scene_->CreateComponent<Octree>();
	Node* zoneNode = scene_->CreateChild("Zone");
	Zone* zone = zoneNode->CreateComponent<Zone>();
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	Node* lightNode = scene_->CreateChild("DirectionalLight");
	lightNode->SetDirection(Vector3(0.5f, -1.0f, 0.5f));
	Light* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetColor(Color(0.5f, 0.5f, 0.5f));
	light->SetSpecularIntensity(1.0f);

	for (int y = -2; y <= 2; ++y)
	{
		for (int x = -2; x <= 2; ++x)
		{
			Node* floorNode = scene_->CreateChild("FloorTile");
			floorNode->SetPosition(Vector3(x * 21.5f, -0.5f, y * 21.5f));
			floorNode->SetScale(Vector3(20.0f, 1.0f, 20.f));
			StaticModel* floorObject = floorNode->CreateComponent<StaticModel>();
			floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
			floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
		}
	}

	{
		Node* boxNode = scene_->CreateChild("ScreenBox");
		boxNode->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
		boxNode->SetScale(Vector3(27.67f, 16.0f, 0.5f));
		StaticModel* boxObject = boxNode->CreateComponent<StaticModel>();
		boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		boxObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

		// Screen
		{
			Node* screenNode = scene_->CreateChild("Screen");
			screenNode->SetPosition(Vector3(0.0f, 10.0f, -0.27f));
			screenNode->SetRotation(Quaternion(-90.0f, 0.0f, 0.0f));
			screenNode->SetScale(Vector3(26.67f, 0.0f, 15.0f));
			StaticModel* screenObject = screenNode->CreateComponent<StaticModel>();
			screenObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
			
			VideoPlayer* videoPlayer = screenNode->CreateComponent<VideoPlayer>();
			videoPlayer->SetSource("Data/Videos/bunny.mp4");
			videoPlayer->SetGenerateMipmaps(true);
			videoPlayer->Play();
			
			SharedPtr<Material> screenMaterial(new Material(context_));
			screenMaterial->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffUnlit.xml"));
			screenMaterial->SetTexture(TU_DIFFUSE, videoPlayer->GetTexture());
			screenMaterial->SetDepthBias(BiasParameters(-0.00001f, 0.0f));
			screenObject->SetMaterial(screenMaterial);
		}

		// Sky
		{
			Node* skyNode = scene_->CreateChild("Sky");
			skyNode->SetScale(500.0f);
			Skybox* skybox = skyNode->CreateComponent<Skybox>();
			skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));

			VideoPlayer* videoPlayer = skyNode->CreateComponent<VideoPlayer>();
			videoPlayer->SetSource("Data/Videos/sky.mp4");
			videoPlayer->SetVolume(0);
			videoPlayer->SetLoop(true);
			videoPlayer->Play();

			SharedPtr<Material> skyMaterial(new Material(context_));
			skyMaterial->SetTechnique(0, cache->GetResource<Technique>("Techniques/SkyboxPanoramic.xml"));
			skyMaterial->SetCullMode(Urho3D::CULL_CW);
			skyMaterial->SetTexture(TU_DIFFUSE, videoPlayer->GetTexture());
			skybox->SetMaterial(skyMaterial);
		}
	}

	cameraNode_ = scene_->CreateChild("Camera");
	Camera* camera = cameraNode_->CreateComponent<Camera>();
	camera->SetFarClip(300.0f);
	cameraNode_->SetPosition(Vector3(0.0f, 7.0f, -30.0f));
}

void VideoPlayerSample::CreateInstructions()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();

	Text* instructionText = ui->GetRoot()->CreateChild<Text>();
	instructionText->SetText("Use WASD keys and mouse/touch to move");
	instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

	instructionText->SetHorizontalAlignment(HA_CENTER);
	instructionText->SetVerticalAlignment(VA_CENTER);
	instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 3);
}

void VideoPlayerSample::SetupViewport()
{
	Renderer* renderer = GetSubsystem<Renderer>();
	SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
	renderer->SetViewport(0, viewport);
}

void VideoPlayerSample::MoveCamera(float timeStep)
{
	if (GetSubsystem<UI>()->GetFocusElement())
		return;

	Input* input = GetSubsystem<Input>();

	const float MOVE_SPEED = 20.0f;
	const float MOUSE_SENSITIVITY = 0.1f;

	IntVector2 mouseMove = input->GetMouseMove();
	yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
	pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
	pitch_ = Clamp(pitch_, -90.0f, 90.0f);

	cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

	if (input->GetKeyDown(KEY_W))
		cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_S))
		cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_A))
		cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_D))
		cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void VideoPlayerSample::SubscribeToEvents()
{
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VideoPlayerSample, HandleUpdate));
}

void VideoPlayerSample::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;
	float timeStep = eventData[P_TIMESTEP].GetFloat();
	MoveCamera(timeStep);
}