#pragma once

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <atlbase.h>

struct IMFMediaEngine;

class VideoPlayer : public Urho3D::Component
{
	URHO3D_OBJECT(VideoPlayer, Urho3D::Object);
public:
	VideoPlayer(Urho3D::Context* context);

	Urho3D::String GetSource() const;
	void SetSource(const Urho3D::String& value);
	void Play();
	void Pause();
	void Stop();
	
	double GetVolume() const;
	void SetVolume(double value);
	bool GetLoop() const;
	void SetLoop(bool value);
	bool GetGenerateMipmaps() const;
	void SetGenerateMipmaps(bool value);

	Urho3D::Texture2D* GetTexture() const;

	~VideoPlayer();

private:
	void HandleRenderSurfaceUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
	void ResizeTexture();
	Urho3D::String source_;
	bool generateMipmaps_;
	Urho3D::SharedPtr<Urho3D::Texture2D> texture_;
	CComPtr<IMFMediaEngine> mediaEngine_;
};