#pragma once

#include "stdafx.h"
#include "ConcurrentQueue.h"

using namespace Urho3D;

class Environment3D : public Application
{

private:
	String startupScriptPath_;
	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	TrackingQueue* trackedBallDisplacements_;
	int* controlFlags_;
	Matrix3 ballXYZtoArenaXYZ_;
	Vector3 ballXYZtoArenaYaw_;

public:
	Environment3D(Context * context);

	void Setup();
	void Configure(const char* startupScriptPath, int* controlFlags, TrackingQueue* trackingQueue);
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
	

	cv::Point3f absolutePose;
	
	int framecount_;
	float time_;
};