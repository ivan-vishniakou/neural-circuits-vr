#pragma once

#include <opencv2\core.hpp>

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

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>

#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>

#include "spdlog/spdlog.h"
#include "ApplicationSettings.h"
#include <boost/lockfree/queue.hpp>
#include <string>

using namespace Urho3D;

class Environment3D : public Application
{
public:

	Environment3D(Context * context);

	void Setup();
	void Start();
	void Stop();
	void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
	void HandleKeyDown(StringHash eventType, VariantMap& eventData);

	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
	void HandleEndFrame(StringHash eventType, VariantMap& eventData);

	void HandleClientConnected(StringHash eventType, VariantMap& eventData);

	void setControlsObject(int* pControlFlags, TrackingQueue* pTrackingQueue);

	Matrix3 ballXYZtoArenaXYZ;
	Vector3 ballXYZtoArenaYaw;
	cv::Point3f absolutePose;
	TrackingQueue* trackedBallDisplacements;
	int* controlFlags;
	int framecount_;
	float time_;
	SharedPtr<Text> text_;
	SharedPtr<Scene> scene_;
	SharedPtr<Node> arena_;
	SharedPtr<Node> cameraNode_;
	Node* ball_node;
	Camera* left_camera_;
	Camera* right_camera_;
	Node* white_light;
	Node* black_light;
	std::shared_ptr<spdlog::logger> file_logger = spdlog::get("pose");

	int arena_period;
};