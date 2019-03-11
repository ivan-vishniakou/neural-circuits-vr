#pragma once

#include "stdafx.h"
#include "Environment3D.h"
#include "ConfigFile.h"
#include "ConfigManager.h"

using namespace Urho3D;

Environment3D::Environment3D(Context * context) : Application(context), framecount_(0), time_(0), absolutePose(0, 0, 0), ballXYZtoArenaXYZ_(Matrix3::ZERO), ballXYZtoArenaYaw_(Vector3::FORWARD)
{
	//arena_period = 30;
}


void Environment3D::Setup()
{
	// These parameters should be self-explanatory.
	// See http://urho3d.github.io/documentation/1.5/_main_loop.html
	// for a more complete list.

	ConfigFile::RegisterObject(context_);
	ConfigManager::RegisterObject(context_);

	String configurationFile = GetSubsystem<FileSystem>()->GetProgramDir() + "/Config/vr.cfg";
	ConfigManager* configManager = new ConfigManager(context_, configurationFile);
	context_->RegisterSubsystem(configManager);
	
	// Load from config file
	engineParameters_["FullScreen"] = GetSubsystem<ConfigManager>()->GetBool("engine", "FullScreen", false);
	engineParameters_["WindowWidth"] = GetSubsystem<ConfigManager>()->GetUInt("engine", "WindowWidth", 1920/2);
	engineParameters_["WindowHeight"] = GetSubsystem<ConfigManager>()->GetUInt("engine", "WindowHeight", 1080/2);
	engineParameters_["WindowPositionX"] = GetSubsystem<ConfigManager>()->GetUInt("engine", "WindowPositionX", 1000);
	engineParameters_["WindowPositionY"] = GetSubsystem<ConfigManager>()->GetUInt("engine", "WindowPositionY", 50);
	engineParameters_["WindowResizable"] = GetSubsystem<ConfigManager>()->GetBool("engine", "WindowResizable", false);
	engineParameters_["Borderless"] = GetSubsystem<ConfigManager>()->GetBool("engine", "Borderless", false);
	engineParameters_["VSync"] = GetSubsystem<ConfigManager>()->GetBool("engine", "VSync", false);

	engine_->SetMinFps(GetSubsystem<ConfigManager>()->GetUInt("engine", "minFps", 120));
	engine_->SetMaxFps(GetSubsystem<ConfigManager>()->GetUInt("engine", "maxFps", 240));
	engine_->SetMaxInactiveFps(GetSubsystem<ConfigManager>()->GetUInt("engine", "maxInactiveFps", 240));

	ballXYZtoArenaXYZ_ = GetSubsystem<ConfigManager>()->GetMatrix3("transforms", "ballXYZtoArenaXYZ", Matrix3::ZERO);
	ballXYZtoArenaYaw_ = GetSubsystem<ConfigManager>()->GetVector3("transforms", "ballXYZtoArenaYaw", Vector3::FORWARD);
}  


void Environment3D::Configure(char * startupScriptPath, int * controlFlags, TrackingQueue * trackingQueue) {
	startupScriptPath_ = String(startupScriptPath);
	controlFlags_ = controlFlags;
	trackedBallDisplacements_ = trackingQueue;
	std::cout << *controlFlags_ << std::endl;
}


void Environment3D::Start()
{
	// We will be needing to load resources.
	// All the resources used in this example comes with Urho3D.
	// If the engine can't find them, check the ResourcePrefixPath (see http://urho3d.github.io/documentation/1.5/_main_loop.html).
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	context_->RegisterSubsystem(new Script(context_));
	scene_ = new Scene(context_);

	ScriptInstance* arenaScript = scene_->CreateComponent<ScriptInstance>();
	arenaScript->CreateObject(cache->GetResource<ScriptFile>(startupScriptPath_), "Arena");

	arenaScript->Execute("void Initialize()");
	cameraNode_ = scene_->GetChild("Subject");
	
	SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Environment3D, HandleBeginFrame));
	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Environment3D, HandleKeyDown));
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Environment3D, HandleUpdate));
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Environment3D, HandlePostUpdate));
	SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Environment3D, HandleRenderUpdate));
	SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Environment3D, HandlePostRenderUpdate));
	SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Environment3D, HandleEndFrame));
	SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(Environment3D, HandleClientConnected));

	GetSubsystem<Input>()->SetMouseVisible(true);

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
		//file_logger->info("RESET");
	}
}

/**
* Your non-rendering logic should be handled here.
* This could be moving objects, checking collisions and reaction, etc.
*/
void  Environment3D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
	//framecount_++;
	//time_ += timeStep;
	//timeStep = 0.005;
	// Movement speed as world units per second
	float MOVE_SPEED = 10.0f;
	// Mouse sensitivity as degrees per pixel
	const float MOUSE_SENSITIVITY = 0.1f;

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
	/*
	while (trackedBallDisplacements->try_pop(delta)) {
		tmp = Vector3(delta[0], delta[1], delta[2]);
		cameraNode_->Translate(ballXYZtoArenaXYZ * tmp);
		cameraNode_->Yaw(ballXYZtoArenaYaw.DotProduct(tmp));
	}*/

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
	if (*controlFlags_ & 1) engine_->Exit();
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
	//Vector3 cam_pos = cameraNode_->GetPosition();
	//file_logger->info("{} {} {} {} {} {}", absolutePose.x, absolutePose.y, absolutePose.z, cam_pos.x_, cam_pos.z_, cameraNode_->GetRotation().EulerAngles().y_);
	/*
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
	}*/
}
/*
void Environment3D::setControlsObject(int * pControlFlags, TrackingQueue* pTrackingQueue)
{
	controlFlags = pControlFlags;
	trackedBallDisplacements = pTrackingQueue;
}
*/

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