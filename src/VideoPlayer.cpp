#include "VideoPlayer.h"
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsImpl.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#define URHO3D_LOGGING
#include <Urho3D/IO/Log.h>

#include <mfmediaengine.h>
#include <mfapi.h>
#include <d3d11.h>

#ifndef URHO3D_D3D11
	#error "Direct3D11 should be used"
#endif

#define RETURN_ON_ERROR(expr) {                                                \
    HRESULT _errorCode = expr;                                                 \
    if(FAILED(_errorCode)) {                                                   \
        URHO3D_LOGERRORF("VideoPlayer error in '%s' 0x%x", #expr, _errorCode); \
        return;                                                                \
    }                                                                          \
}

class MediaEngineNotify : public IMFMediaEngineNotify
{
public:
	MediaEngineNotify() : 
		refCount(1)
	{}

	ULONG AddRef()
	{
		return InterlockedIncrement(&refCount);
	}

	ULONG Release()
	{
		ULONG count = InterlockedDecrement(&refCount);
		if (count == 0) delete this;
		return count;
	}

	HRESULT QueryInterface(REFIID iid, void** ppv)
	{
		if (!ppv) return E_POINTER;

		if (iid == __uuidof(IUnknown))
		{
			*ppv = static_cast<IUnknown*>(this);
		}
		else if (iid == __uuidof(IMFMediaEngineNotify))
		{
			*ppv = static_cast<IMFMediaEngineNotify*>(this);
		}
		else
		{
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE EventNotify(DWORD event, DWORD_PTR param1, DWORD param2)
	{
		switch(event)
		{
			case MF_MEDIA_ENGINE_EVENT_ERROR:
				URHO3D_LOGERRORF("VideoPlayer error 0x%x", param2);
				break;
		}
		return S_OK;
	}
private:
	ULONG refCount;
};

static CComPtr<IMFDXGIDeviceManager> GetDxgiManager(Urho3D::Context* context)
{
	static CComPtr<IMFDXGIDeviceManager> dxgiManager;
	if(!dxgiManager)
	{
		ID3D11Device* dxDevice = context->GetSubsystem<Urho3D::Graphics>()->GetImpl()->GetDevice();
		CComQIPtr<ID3D10Multithread> dxMultithread = dxDevice;
		dxMultithread->SetMultithreadProtected(TRUE);
		UINT resetToken = 0;
		MFCreateDXGIDeviceManager(&resetToken, &dxgiManager);
		dxgiManager->ResetDevice(dxDevice, resetToken);
	}
	return dxgiManager;
}

VideoPlayer::VideoPlayer(Urho3D::Context* context) :
	Urho3D::Component(context),
	texture_(new Urho3D::Texture2D(context)),
	generateMipmaps_(false)
{
	RETURN_ON_ERROR(MFStartup(MF_VERSION));
	CComPtr<IMFMediaEngineClassFactory> mediaEngineFactory;
	RETURN_ON_ERROR(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mediaEngineFactory)));

	CComPtr<IMFAttributes> attributes;
	RETURN_ON_ERROR(MFCreateAttributes(&attributes, 3));
	RETURN_ON_ERROR(attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, new MediaEngineNotify()));
	RETURN_ON_ERROR(attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_R8G8B8A8_UNORM));
	RETURN_ON_ERROR(attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, GetDxgiManager(context)));
	RETURN_ON_ERROR(mediaEngineFactory->CreateInstance(0, attributes, &mediaEngine_));

	SubscribeToEvent(Urho3D::E_RENDERSURFACEUPDATE, URHO3D_HANDLER(VideoPlayer, HandleRenderSurfaceUpdate));
}

Urho3D::String VideoPlayer::GetSource() const
{
	return source_;
}

void VideoPlayer::SetSource(const Urho3D::String& value)
{
	if(!mediaEngine_) return;
	source_ = value;
	RETURN_ON_ERROR(mediaEngine_->SetSource(const_cast<wchar_t*>(Urho3D::WString(value).CString())));
}

double VideoPlayer::GetVolume() const
{
	if(!mediaEngine_) return 0;
	return mediaEngine_->GetVolume();
}

void VideoPlayer::SetVolume(double value)
{
	if(!mediaEngine_) return;
	mediaEngine_->SetVolume(value);
}

bool VideoPlayer::GetLoop() const
{
	if (!mediaEngine_) return false;
	return mediaEngine_->GetLoop() == TRUE;
}

void VideoPlayer::SetLoop(bool value)
{
	if(!mediaEngine_) return;
	mediaEngine_->SetLoop(true);
}

bool VideoPlayer::GetGenerateMipmaps() const
{
	return generateMipmaps_;
}

void VideoPlayer::SetGenerateMipmaps(bool value)
{
	generateMipmaps_ = value;
}

void VideoPlayer::Play()
{
	if(!mediaEngine_) return;
	mediaEngine_->Play();
}

void VideoPlayer::Pause()
{
	if(!mediaEngine_) return;
	mediaEngine_->Pause();
}

void VideoPlayer::Stop()
{
	if(!mediaEngine_) return;
	mediaEngine_->Pause();
	mediaEngine_->SetCurrentTime(0);
}

Urho3D::Texture2D* VideoPlayer::GetTexture() const
{
	return texture_;
}

void VideoPlayer::HandleRenderSurfaceUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
	LONGLONG pts;
	if(mediaEngine_->OnVideoStreamTick(&pts) != S_OK) return;

	ResizeTexture();
	Urho3D::RenderSurface* renderSurface = texture_->GetRenderSurface();
	if(!renderSurface || !renderSurface->IsUpdateQueued()) return;

	RECT rect = {0, 0, texture_->GetWidth(), texture_->GetHeight()};
	MFARGB borderColor = {0, 0, 0, 255};
	ID3D11Texture2D* dxTexture = (ID3D11Texture2D*)texture_->GetGPUObject();
	RETURN_ON_ERROR(mediaEngine_->TransferVideoFrame(dxTexture, nullptr, &rect, &borderColor));
	if(generateMipmaps_) texture_->RegenerateLevels();
	renderSurface->ResetUpdateQueued();
}

void VideoPlayer::ResizeTexture()
{
	DWORD w = 0;
	DWORD h = 0;
	mediaEngine_->GetNativeVideoSize(&w, &h);
	bool hasMipmaps = texture_->GetLevels() != 1;
	if(texture_->GetWidth() == w && texture_->GetHeight() == h && hasMipmaps == generateMipmaps_) return;
	texture_->SetNumLevels(generateMipmaps_ ? 0 : 1);
	texture_->SetSize(w, h, Urho3D::Graphics::GetRGBAFormat(), Urho3D::TEXTURE_RENDERTARGET);
	texture_->SetFilterMode(generateMipmaps_ ? Urho3D::TextureFilterMode::FILTER_TRILINEAR : Urho3D::TextureFilterMode::FILTER_BILINEAR);
	texture_->UnsubscribeFromEvent(Urho3D::E_RENDERSURFACEUPDATE); // VideoPlayer::HandleRenderSurfaceUpdate is used instead of the default handler
}

VideoPlayer::~VideoPlayer()
{
	RETURN_ON_ERROR(MFShutdown());
}
