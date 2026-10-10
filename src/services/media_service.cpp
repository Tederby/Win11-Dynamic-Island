#include "media_service.h"
#include "service_manager.h"
#include "../common/log.h"
#include <wincodec.h>
#include <wrl/client.h>

namespace DynamicIsland {
namespace Services {

namespace {

using Microsoft::WRL::ComPtr;

bool DecodeBytesToPBGRA(
    const uint8_t* data,
    size_t size,
    uint32_t& outWidth,
    uint32_t& outHeight,
    std::vector<uint8_t>& outPixels
) {
    if (!data || size == 0) return false;

    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr) || !factory) return false;

    ComPtr<IWICStream> stream;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr) || !stream) return false;

    hr = stream->InitializeFromMemory(const_cast<BYTE*>(data), static_cast<DWORD>(size));
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(
        stream.Get(),
        nullptr,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );
    if (FAILED(hr) || !decoder) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) return false;

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) return false;

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(hr)) return false;

    UINT w = 0, h = 0;
    converter->GetSize(&w, &h);
    if (w == 0 || h == 0) return false;

    UINT stride = w * 4;
    UINT bufferSize = stride * h;
    outPixels.resize(bufferSize);

    hr = converter->CopyPixels(nullptr, stride, bufferSize, outPixels.data());
    if (FAILED(hr)) return false;

    outWidth = w;
    outHeight = h;
    return true;
}

#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
struct ThumbnailFetchContext {
    MediaService* self = nullptr;
    winrt::Windows::Storage::Streams::IRandomAccessStreamReference thumbRef{nullptr};
    std::wstring trackId;
};

DWORD WINAPI ThumbnailWorkerThreadProc(LPVOID param) {
    auto* ctx = reinterpret_cast<ThumbnailFetchContext*>(param);
    if (!ctx || !ctx->self) {
        delete ctx;
        return 0;
    }

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    try {
        if (ctx->thumbRef) {
            auto stream = ctx->thumbRef.OpenReadAsync().get();
            if (stream) {
                uint32_t sz = static_cast<uint32_t>(stream.Size());
                if (sz > 0 && sz < 20 * 1024 * 1024) { // max 20MB safety limit
                    winrt::Windows::Storage::Streams::DataReader reader(stream);
                    reader.LoadAsync(sz).get();
                    std::vector<uint8_t> streamBytes(reader.UnconsumedBufferLength());
                    reader.ReadBytes(winrt::array_view<uint8_t>(streamBytes));

                    uint32_t width = 0, height = 0;
                    std::vector<uint8_t> pbgraPixels;
                    if (DecodeBytesToPBGRA(streamBytes.data(), streamBytes.size(), width, height, pbgraPixels)) {
                        ctx->self->StoreDecodedThumbnail(std::move(pbgraPixels), width, height, ctx->trackId);
                    }
                }
            }
        }
    } catch (...) {}

    ctx->self->FinishThumbnailFetch();
    delete ctx;
    CoUninitialize();
    return 0;
}
#endif

} // namespace

MediaService::MediaService(ServiceManager* manager)
    : m_manager(manager) {
    m_state.title.clear();
    m_state.artist.clear();
    m_state.isPlaying = false;
    m_state.progress = 0.0f;
}

MediaService::~MediaService() {
    Stop();
}

void MediaService::Start() {
    m_isRunning = true;
    LogInfo(L"MediaService started");
}

void MediaService::Stop() {
    m_isRunning = false;
    m_hasRealSession = false;
    m_lastFetchedTrackId.clear();
    {
        std::lock_guard<std::mutex> lock(m_thumbMutex);
        m_thumbPixels.clear();
        m_thumbWidth = 0;
        m_thumbHeight = 0;
        m_thumbVersion++;
    }
    m_state.isPlaying = false;
    m_state.title.clear();
    m_state.artist.clear();
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->SetLiveActivity(EventType::Media, false);
    }
}

bool MediaService::CopyThumbnail(std::vector<uint8_t>& outPixels, uint32_t& outW, uint32_t& outH, uint64_t& outVersion) const {
    std::lock_guard<std::mutex> lock(m_thumbMutex);
    if (m_thumbVersion == outVersion) {
        return false;
    }
    outVersion = m_thumbVersion;
    outPixels = m_thumbPixels;
    outW = m_thumbWidth;
    outH = m_thumbHeight;
    return true;
}

void MediaService::StoreDecodedThumbnail(std::vector<uint8_t> pixels, uint32_t width, uint32_t height, const std::wstring& trackId) {
    std::lock_guard<std::mutex> lock(m_thumbMutex);
    if (m_lastFetchedTrackId == trackId) {
        m_thumbPixels = std::move(pixels);
        m_thumbWidth = width;
        m_thumbHeight = height;
        m_thumbVersion++;
    }
}

void MediaService::FinishThumbnailFetch() {
    m_isFetchingThumbnail = false;
}

#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
void MediaService::TriggerAsyncThumbnailFetch(winrt::Windows::Storage::Streams::IRandomAccessStreamReference thumbRef, const std::wstring& trackId) {
    if (!thumbRef) {
        std::lock_guard<std::mutex> lock(m_thumbMutex);
        m_thumbPixels.clear();
        m_thumbWidth = 0;
        m_thumbHeight = 0;
        m_thumbVersion++;
        return;
    }

    if (m_isFetchingThumbnail.exchange(true)) {
        return; // Already in progress
    }

    auto* ctx = new ThumbnailFetchContext();
    ctx->self = this;
    ctx->thumbRef = thumbRef;
    ctx->trackId = trackId;

    HANDLE hThread = CreateThread(nullptr, 0, ThumbnailWorkerThreadProc, ctx, 0, nullptr);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        m_isFetchingThumbnail = false;
        delete ctx;
    }
}
#endif

void MediaService::Poll() {
    if (!m_isRunning) return;

#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    try {
        if (!m_sessionManager) {
            m_sessionManager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        }
        if (m_sessionManager) {
            winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession session = nullptr;
            auto sessions = m_sessionManager.GetSessions();
            uint32_t sessionCount = sessions ? sessions.Size() : 0;
            for (uint32_t i = 0; i < sessionCount; ++i) {
                auto s = sessions.GetAt(i);
                if (s) {
                    auto pb = s.GetPlaybackInfo();
                    if (pb && pb.PlaybackStatus() == winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                        session = s;
                        break;
                    }
                }
            }
            if (!session) {
                session = m_sessionManager.GetCurrentSession();
            }

            m_currentSession = session;
            if (session) {
                auto props = session.TryGetMediaPropertiesAsync().get();
                auto info = session.GetPlaybackInfo();
                auto timeline = session.GetTimelineProperties();

                std::wstring title = props ? props.Title().c_str() : L"";
                std::wstring artist = props ? props.Artist().c_str() : L"";
                bool isPlaying = (info && info.PlaybackStatus() == winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);

                float prog = 0.0f;
                if (timeline) {
                    auto total = timeline.EndTime().count();
                    auto pos = timeline.Position().count();
                    if (total > 0) {
                        prog = static_cast<float>(pos) / static_cast<float>(total);
                        if (prog > 1.0f) prog = 1.0f;
                    }
                }

                if (!title.empty()) {
                    m_hasRealSession = true;
                    m_state.title = title;
                    m_state.artist = artist;
                    m_state.isPlaying = isPlaying;
                    if (prog > 0.0f) m_state.progress = prog;

                    std::wstring currentTrackId = title + L" - " + artist;
                    if (currentTrackId != m_lastFetchedTrackId) {
                        m_lastFetchedTrackId = currentTrackId;
                        TriggerAsyncThumbnailFetch(props.Thumbnail(), currentTrackId);
                    }
                } else {
                    m_state.isPlaying = false;
                }

                if (m_manager) {
                    m_manager->UpdateMediaState(m_state);
                }
                return;
            } else {
                // No session active
                if (m_hasRealSession || m_state.isPlaying || !m_state.title.empty()) {
                    m_hasRealSession = false;
                    m_state.isPlaying = false;
                    m_state.title.clear();
                    m_state.artist.clear();
                    m_state.progress = 0.0f;
                    m_lastFetchedTrackId.clear();
                    {
                        std::lock_guard<std::mutex> lock(m_thumbMutex);
                        m_thumbPixels.clear();
                        m_thumbWidth = 0;
                        m_thumbHeight = 0;
                        m_thumbVersion++;
                    }
                    if (m_manager) {
                        m_manager->UpdateMediaState(m_state);
                    }
                }
                return;
            }
        }
    } catch (...) {
        if (m_hasRealSession || m_state.isPlaying) {
            m_hasRealSession = false;
            m_state.isPlaying = false;
            if (m_manager) {
                m_manager->UpdateMediaState(m_state);
            }
        }
        return;
    }
#endif

    // Fallback playback progression when no active WinRT session
    if (m_state.isPlaying) {
        m_state.progress += 0.0025f;
        if (m_state.progress >= 1.0f) {
            m_state.progress = 0.0f;
        }
        if (m_manager) {
            m_manager->UpdateMediaState(m_state);
        }
    }
}

void MediaService::Play() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TryPlayAsync(); } catch (...) {}
    }
#endif
    m_state.isPlaying = true;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Pause() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TryPauseAsync(); } catch (...) {}
    }
#endif
    m_state.isPlaying = false;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Next() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TrySkipNextAsync(); } catch (...) {}
    }
#endif
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Previous() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TrySkipPreviousAsync(); } catch (...) {}
    }
#endif
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

} // namespace Services
} // namespace DynamicIsland
