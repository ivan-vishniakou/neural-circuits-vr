#pragma once

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/RenderPath.h>

#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>

#include <string>
#include <sstream>
#include <iostream>

#include "Environment3D.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "ApplicationSettings.h"

using namespace Urho3D;

Environment3D::Environment3D(Context * context) : Application(context), framecount_(0), time_(0), absolutePose(0, 0, 0), ballXYZtoArenaXYZ(Matrix3::IDENTITY), ballXYZtoArenaYaw(Vector3::FORWARD)
{
	arena_period = 30;
}


void Environment3D::Setup()
{
	// These parameters should be self-explanatory.
	// See http://urho3d.github.io/documentation/1.5/_main_loop.html
	// for a more complete list.

	/*
	engineParameters_["FullScreen"] = true;
	engineParameters_["WindowWidth"] = 1920;
	engineParameters_["WindowHeight"] = 1080;
	engineParameters_["WindowResizable"] = false;
	engineParameters_["Borderless"] = true;
	engineParameters_["WindowPositionX"] = 1920;
	engineParameters_["WindowPositionY"] = 0;
	engineParameters_["FullScreen"] = false;
	engineParameters_["VSync"] = false;
	/*/
	engineParameters_["WindowWidth"] = 1920 / 2;
	engineParameters_["WindowHeight"] = 1080 / 2;
	engineParameters_["WindowPositionX"] = 0;
	engineParameters_["WindowPositionY"] = 0;
	engineParameters_["WindowResizable"] = false;
	engineParameters_["FullScreen"] = false;
	//*/
}


void Environment3D::Start()
{

	ApplicationSettings& settings = ApplicationSettings::getInstance();

	// We will be needing to load resources.
	// All the resources used in this example comes with Urho3D.
	// If the engine can't find them, check the ResourcePrefixPath (see http://urho3d.github.io/documentation/1.5/_main_loop.html).
	ResourceCache* cache = GetSubsystem<ResourceCache>();


	// Let's use the default style that comes with Urho3D.
	GetSubsystem<UI>()->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	// Let's create some text to display.
	text_ = new Text(context_);
	// Text will be updated later in the E_UPDATE handler. Keep readin'.
	text_->SetText("Keys: tab = toggle mouse, AWSD = move camera, Shift = fast mode, Esc = quit.\nWait a bit to see FPS.");
	// If the engine cannot find the font, it comes with Urho3D.
	// Set the environment variables URHO3D_HOME, URHO3D_PREFIX_PATH or
	// change the engine parameter "ResourcePrefixPath" in the Setup method.
	text_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 20);
	text_->SetColor(Color(.3, 0, .3));
	text_->SetHorizontalAlignment(HA_LEFT);
	text_->SetVerticalAlignment(VA_TOP);
	GetSubsystem<UI>()->GetRoot()->AddChild(text_);
	// Let's setup a scene to render.
	scene_ = new Scene(context_);



	bool grid = false;

	arena_period = settings.pt.get<float>("VR settings.cyclic_arena_period");

	Urho3D::String worldname = settings.pt.get<std::string>("VR settings.scene_filename").c_str();

	std::vector<float> v = ini_string_to_array<float>(settings.pt.get<std::string>("VR settings.ball_xyz_to_arena_xyz"));
	assert(false && v.size == 9 && "Wrong number of parameters provided for 3x3 matrix in [VR settings.ball_xyz_to_arena_xyz]");
	float g = settings.pt.get<float>("VR settings.translation_gain");
	ballXYZtoArenaXYZ = Matrix3(v[0] * g, v[1] * g, v[2] * g, v[3] * g, v[4] * g, v[5] * g, v[6] * g, v[7] * g, v[8] * g);
	v.clear();

	v = ini_string_to_array<float>(settings.pt.get<std::string>("VR settings.ball_xyz_to_arena_yaw"));
	assert(v.size == 3 && "Wrong number of parameters provided for 3-vector in [VR settings.ball_xyz_to_arena_yaw]");
	g = settings.pt.get<float>("VR settings.rotation_gain");
	ballXYZtoArenaYaw = Vector3(v[0] * g, v[1] * g, v[2] * g);
	v.clear();

	//grid = worldname == "grid.xml";
	if (!grid) {
		Urho3D::String scene_filename = Urho3D::String("Scenes/").Append(worldname);
		XMLFile *sceneFile = cache->GetResource<XMLFile>(scene_filename);
		scene_->LoadXML(sceneFile->GetRoot());
	}
	else {


		// GRID world generation example
		scene_->CreateComponent<Octree>();
		scene_->CreateComponent<PhysicsWorld>();
		// Let's add an additional scene component for fun.
		scene_->CreateComponent<DebugRenderer>();

		Node* zoneNode = scene_->CreateChild("Zone");
		Zone* zone = zoneNode->CreateComponent<Zone>();
		// Set same volume as the Octree, set a close bluish fog and some ambient light
		zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
		zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
		zone->SetFogColor(Color(0.5f, 0.5f, 0.5f));
		zone->SetFogStart(1);
		zone->SetFogEnd(10.0f);


		Node * node = scene_->CreateChild("floor");
		node->SetScale(Vector3(200, 1, 200));
		StaticModel* object = node->CreateChild()->CreateComponent<StaticModel>();
		object->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
		object->SetMaterial(cache->GetResource<Material>("Materials/Arena.xml"));
		object->SetCastShadows(true);

		node = scene_->CreateChild("ceiling");
		node->SetScale(Vector3(200, 1, 200));
		node->SetPosition(Vector3(0, 1, 0));
		node->SetRotation(Quaternion(180.0, 0.0, 0.0));
		object = node->CreateChild()->CreateComponent<StaticModel>();
		object->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
		object->SetMaterial(cache->GetResource<Material>("Materials/Arena.xml"));
		object->SetCastShadows(true);


		Light* light;
		Node* lightNode;
		float b = 10.0;
		int gridstep = 7.5;
		float r = 5.0;//25.0; for gridstep 15 and 30

		for (int i = -10; i < 10; i++) {
			for (int ii = -10; ii < 9; ii++) {

				lightNode = scene_->CreateChild("Light");
				lightNode->SetPosition(Vector3(i*gridstep, 0.5, ii*gridstep));
				light = lightNode->CreateComponent<Light>();
				light->SetLightType(LIGHT_POINT);
				light->SetRange(r);
				light->SetBrightness(b);
				//light->SetBrightness(std::max(b, float(0.0)));
				b *= -1.0;
				light->SetColor(Color(1.0, 1.0, 1.0, 1));

			}
		}



		cameraNode_ = scene_->CreateChild("Viewpoint");

		Urho3D::Node* left_cam_node = cameraNode_->CreateChild("RightCamera");
		left_cam_node->Rotate(Quaternion(-45.0f, Vector3::UP));
		left_camera_ = left_cam_node->CreateComponent<Camera>();
		left_camera_->SetFov(100.0f);
		left_camera_->SetFarClip(2000);

		Urho3D::Node* right_cam_node = cameraNode_->CreateChild("LeftCamera");
		right_cam_node->Rotate(Quaternion(+45.0f, Vector3::UP));
		right_camera_ = right_cam_node->CreateComponent<Camera>();
		right_camera_->SetFov(100.0f);
		right_camera_->SetFarClip(2000);

		// END GRID
	}

	cameraNode_ = scene_->GetChild("Viewpoint", true);
	left_camera_ = scene_->GetChild("LeftCamera", true)->GetComponent<Urho3D::Camera>();
	right_camera_ = scene_->GetChild("RightCamera", true)->GetComponent<Urho3D::Camera>();
	// We need a camera from which the viewport can render.

	GetSubsystem<Engine>()->SetMaxFps(400);
	GetSubsystem<Engine>()->SetMaxInactiveFps(400);

	std::cout << GetSubsystem<Engine>()->GetMaxInactiveFps() << std::endl;
	// Now we setup the viewport. Of course, you can have more than one!
	Graphics* graphics = GetSubsystem<Graphics>();
	Renderer* renderer = GetSubsystem<Renderer>();
	renderer->SetNumViewports(2);
	SharedPtr<Viewport> left_viewport(new Viewport(context_, scene_, right_camera_, IntRect(0, 0, graphics->GetWidth() / 2, graphics->GetHeight())));
	renderer->SetViewport(0, left_viewport);
	SharedPtr<Viewport> right_viewport(new Viewport(context_, scene_, left_camera_, IntRect(graphics->GetWidth() / 2, 0, graphics->GetWidth(), graphics->GetHeight())));
	renderer->SetViewport(1, right_viewport);


	if (!settings.pt.get<bool>("VR settings.invert_color")) {
		left_viewport->GetRenderPath()->Append(cache->GetResource<XMLFile>("PostProcess/Dither.xml"));
	}
	else {
		left_viewport->GetRenderPath()->Append(cache->GetResource<XMLFile>("PostProcess/DitherINV.xml"));
	}


	// We subscribe to the events we'd like to handle.
	// In this example we will be showing what most of them do,
	// but in reality you would only subscribe to the events
	// you really need to handle.
	// These are sort of subscribed in the order in which the engine
	// would send the events. Read each handler method's comment for
	// details.
	SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Environment3D, HandleBeginFrame));
	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Environment3D, HandleKeyDown));
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Environment3D, HandleUpdate));
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Environment3D, HandlePostUpdate));
	SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Environment3D, HandleRenderUpdate));
	SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Environment3D, HandlePostRenderUpdate));
	SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Environment3D, HandleEndFrame));

	SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(Environment3D, HandleClientConnected));

	GetSubsystem<Input>()->SetMouseVisible(true);

	// World saving
	/*
	File file(context_);
	if (!file.Open("grid_small.xml", FILE_WRITE))
	ErrorExit("Could not open output file");
	scene_->SaveXML(file);
	*/
	auto* network = GetSubsystem<Network>();
	network->StartServer(2345);
}

/**
* Good place to get rid of any system resources that requires the
* engine still initialized. You could do the rest in the destructor,
* but there's no need, this method will get called when the engine stops,
* for whatever reason (short of a segfault).
*/
void Environment3D::Stop()
{
}

/**
* Every frame's life must begin somewhere. Here it is.
*/
void Environment3D::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
	// We really don't have anything useful to do here for this example.
	// Probably shouldn't be subscribing to events we don't care about.
}

/**
* Input from keyboard is handled here. I'm assuming that Input, if
* available, will be handled before E_UPDATE.
*/
void Environment3D::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;
	int key = eventData[P_KEY].GetInt();
	if (key == KEY_ESCAPE)
		engine_->Exit();
	if (key == KEY_TAB)    // toggle mouse cursor when pressing tab
	{
		GetSubsystem<Input>()->SetMouseVisible(!GetSubsystem<Input>()->IsMouseVisible());
		GetSubsystem<Input>()->SetMouseGrabbed(!GetSubsystem<Input>()->IsMouseGrabbed());
	}
	if (key == KEY_R)
	{
		cameraNode_->SetPosition(Vector3(0, 0.2, 0));
		cameraNode_->SetRotation(Quaternion(0, Vector3::UP));
		file_logger->info("RESET");
	}
}

/**
* Your non-rendering logic should be handled here.
* This could be moving objects, checking collisions and reaction, etc.
*/
void  Environment3D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
	framecount_++;
	time_ += timeStep;
	timeStep = 0.005;
	// Movement speed as world units per second
	float MOVE_SPEED = 10.0f;
	// Mouse sensitivity as degrees per pixel
	const float MOUSE_SENSITIVITY = 0.1f;

	if (time_ >= 1)
	{
		std::string str;
		str.append(" ");
		//str.append("Keys: tab = toggle mouse, AWSD = move camera, Shift = fast mode, Esc = quit.\n");
		{
			std::ostringstream ss;
			ss << framecount_;
			std::string s(ss.str());
			str.append(s.substr(0, 6));
		}
		str.append(" frames in ");
		{
			std::ostringstream ss;
			ss << time_;
			std::string s(ss.str());
			str.append(s.substr(0, 6));
		}
		str.append(" seconds = ");
		{
			std::ostringstream ss;
			ss << (float)framecount_ / time_;
			std::string s(ss.str());
			str.append(s.substr(0, 6));
		}
		str.append(" fps");

		str.append("\n CAM X = ");
		{
			std::ostringstream ss;
			ss << cameraNode_->GetPosition().x_;
			std::string s(ss.str());
			str.append(s.substr(0, 6));
		}
		str.append("\n CAM Y = ");
		{
			std::ostringstream ss;
			ss << cameraNode_->GetPosition().y_;
			std::string s(ss.str());
			str.append(s.substr(0, 6));
		}
		str.append("\n CAM Z = ");
		{
			std::ostringstream ss;
			ss << cameraNode_->GetPosition().z_;
			std::string s(ss.str());
			str.append(s.substr(0, 6));
		}

		String s(str.c_str(), str.size());
		text_->SetText(s);
		URHO3D_LOGINFO(s);     // this show how to put stuff into the log
		framecount_ = 0;
		time_ = 0;
	}

	Input* input = GetSubsystem<Input>();

	Vector3 prevPos = cameraNode_->GetPosition();
	prevPos.y_ = 0.2;
	cameraNode_->SetPosition(prevPos);

	if (input->GetKeyDown('W'))
		cameraNode_->Translate(Vector3(0, 0, 1)*MOVE_SPEED*timeStep);
	if (input->GetKeyDown('S'))
		cameraNode_->Translate(Vector3(0, 0, -1)*MOVE_SPEED*timeStep);
	if (input->GetKeyDown('A'))
		cameraNode_->Translate(Vector3(-1, 0, 0)*MOVE_SPEED*timeStep);
	if (input->GetKeyDown('D'))
		cameraNode_->Translate(Vector3(1, 0, 0)*MOVE_SPEED*timeStep);

	cv::Vec3f delta;
	Vector3 tmp;
	while (trackedBallDisplacements->try_pop(delta)) {
		tmp = Vector3(delta[0], delta[1], delta[2]);
		cameraNode_->Translate(ballXYZtoArenaXYZ * tmp);
		cameraNode_->Yaw(ballXYZtoArenaYaw.DotProduct(tmp));
	}

	/*
	if (of_mutex->try_lock()) {
	//std::cout << "MU" << std::endl;
	cv::Point3d delta(delta_pose->x, delta_pose->y, delta_pose->z);


	if (GetSubsystem<Input>()->IsMouseVisible()) {

	if (std::abs(delta.x) > 1 || std::abs(delta.y) > 1 || std::abs(delta.z) > 1) {
	std::cout << "DELTA too big " << delta.x << ", " << delta.y << ", " << delta.z <<  std::endl;
	}
	else {

	cameraNode_->Translate(Vector3((-delta.z - delta.y)*(0.7071 * 0.006 * 500), 0, 0));
	cameraNode_->Translate(Vector3(0, 0, (-delta.z + delta.y)*0.7071*0.006 * 500));
	cameraNode_->Yaw(delta.x*180.0 / 3.14 * 1.0);

	absolutePose.x += (-delta.z - delta.y)*(0.7071 * 0.006 * 500);
	absolutePose.y += (-delta.z + delta.y)*0.7071*0.006 * 500;
	absolutePose.z += delta.x*180.0 / 3.14 * 1.0;
	}
	}

	delta_pose->x -= delta.x;
	delta_pose->y -= delta.y;
	delta_pose->z -= delta.z;
	of_mutex->unlock();
	Vector3 newPos = cameraNode_->GetPosition();

	PhysicsRaycastResult result;
	PhysicsWorld * pw = scene_->GetComponent<PhysicsWorld>();
	pw->SphereCast(result, Ray(newPos, prevPos), 0.4, (newPos - prevPos).Length());
	int count = 4;
	while (result.body_ && count-- > 0) {
	newPos = result.position_ + result.normal_*0.4011;
	// +(newPos - prevPos) * rayDistance*0.5);
	pw->SphereCast(result, Ray(newPos, prevPos), 0.4, (newPos - prevPos).Length());
	}

	//Grid_world_specific
	newPos.x_ = std::fmod(newPos.x_, float(arena_period));//30
	newPos.z_ = std::fmod(newPos.z_, float(arena_period));

	cameraNode_->SetPosition(newPos);

	}
	else {
	//std::cout << "MUTEX LOCKED FOR ENV3d" << std::endl;
	}*/
	/*
	if (!GetSubsystem<Input>()->IsMouseVisible())
	{
	// Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
	IntVector2 mouseMove = input->GetMouseMove();
	static float yaw_ = 0;
	static float pitch_ = 0;
	yaw_ += MOUSE_SENSITIVITY*mouseMove.x_;
	//pitch_ += MOUSE_SENSITIVITY*mouseMove.y_;
	//pitch_ = Clamp(pitch_, -90.0f, 90.0f);
	// Reset rotation and set yaw and pitch again
	cameraNode_->SetDirection(Vector3::FORWARD);
	cameraNode_->Yaw(yaw_);
	cameraNode_->Pitch(pitch_);
	}
	*/
	if (*controlFlags & 1) engine_->Exit();
}

/**
* Anything in the non-rendering logic that requires a second pass,
* it might be well suited to be handled here.
*/
void Environment3D::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	// We really don't have anything useful to do here for this example.
	// Probably shouldn't be subscribing to events we don't care about.
	/*DebugRenderer * dbgRenderer = scene_->GetComponent<DebugRenderer>();
	if (dbgRenderer)
	{
	// Draw navmesh data
	light->DrawDebugGeometry(dbgRenderer, true);
	camera_->DrawDebugGeometry(dbgRenderer, true);
	}
	*/
}

/**
* If you have any details you want to change before the viewport is
* rendered, try putting it here.
* See http://urho3d.github.io/documentation/1.32/_rendering.html
* for details on how the rendering pipeline is setup.
*/
void  Environment3D::HandleRenderUpdate(StringHash eventType, VariantMap & eventData)
{
	// We really don't have anything useful to do here for this example.
	// Probably shouldn't be subscribing to events we don't care about.
}

/**
* After everything is rendered, there might still be things you wish
* to add to the rendering. At this point you cannot modify the scene,
* only post rendering is allowed. Good for adding things like debug
* artifacts on screen or brush up lighting, etc.
*/
void  Environment3D::HandlePostRenderUpdate(StringHash eventType, VariantMap & eventData)
{

}

/**
* All good things must come to an end.
*/
void  Environment3D::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
	// We really don't have anything useful to do here for this example.
	// Probably shouldn't be subscribing to events we don't care about.
	Vector3 cam_pos = cameraNode_->GetPosition();
	file_logger->info("{} {} {} {} {} {}", absolutePose.x, absolutePose.y, absolutePose.z, cam_pos.x_, cam_pos.z_, cameraNode_->GetRotation().EulerAngles().y_);
	auto* network = GetSubsystem<Network>();
	if (network->IsServerRunning())
	{
		const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();

		if (connections.Size() > 0) {
			Urho3D::VectorBuffer data;
			Urho3D::String msg = "P";
			msg.Append(Urho3D::String(absolutePose.x));
			msg.Append(", ");
			msg.Append(Urho3D::String(absolutePose.y));
			msg.Append(", ");
			msg.Append(Urho3D::String(absolutePose.z));
			msg.Append(", ");

			msg.Append(Urho3D::String(cam_pos.x_));
			msg.Append(", ");
			msg.Append(Urho3D::String(cam_pos.z_));
			msg.Append(", ");
			msg.Append(Urho3D::String(cameraNode_->GetRotation().EulerAngles().y_));
			msg.Append("\n");

			data.WriteString(msg);
			network->BroadcastMessage(654, false, true, data);
		}
	}
}

void Environment3D::setControlsObject(int * pControlFlags, TrackingQueue* pTrackingQueue)
{
	controlFlags = pControlFlags;
	trackedBallDisplacements = pTrackingQueue;
}


void Environment3D::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
	using namespace ClientConnected;

	// When a client connects, assign to scene to begin scene replication
	auto* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	newConnection->SetScene(scene_);

	// Then create a controllable object for that client
	//Node* newObject = CreateControllableObject();
	//serverObjects_[newConnection] = newObject;

	// Finally send the object's node ID using a remote event
	//VariantMap remoteEventData;
	//remoteEventData[P_ID] = newObject->GetID();
	//newConnection->SendRemoteEvent(E_CLIENTOBJECTID, true, remoteEventData);
}