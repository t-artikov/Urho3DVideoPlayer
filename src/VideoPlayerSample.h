#pragma once
#include "Sample.h"

namespace Urho3D
{
	class Node;
	class Scene;
}

class VideoPlayerSample : public Sample
{
	URHO3D_OBJECT(VideoPlayerSample, Sample);

public:
	VideoPlayerSample(Context* context);
	virtual void Setup();
	virtual void Start();

private:
	void CreateScene();
	void CreateInstructions();
	void SetupViewport();
	void SubscribeToEvents();
	void MoveCamera(float timeStep);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
};