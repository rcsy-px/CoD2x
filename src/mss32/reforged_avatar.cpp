#include "drawing.h"
#include "reforged_avatar.h"
#include "shared.h"
#include "../shared/cod2_client.h"
#include "../shared/cod2_dvars.h"
#include <wininet.h>
#include <wincodec.h>
#include <d3d9.h>
#include <future>
#include <regex>
#include <vector>
#include <cmath>
#include <cstring>

namespace {
struct Avatar { std::string ref; std::vector<unsigned char> pixels; };
std::future<Avatar> request;
Avatar cached;
std::string requested, map;
materialHandle_t* material=nullptr;
void* uploadedTexture=nullptr;
std::string uploadedRef;
DWORD checkedAt=0, retryAt=0;
unsigned attempts=0;
bool validRef(const std::string& ref) {
    static const std::regex pattern("([0-9]{17,20}/(a_)?[a-f0-9]{32}|default/[0-5])");
    return ref.size()<=55 && std::regex_match(ref,pattern);
}
std::vector<unsigned char> png(const std::string& ref) {
    HINTERNET internet=InternetOpenA("ReforgedAvatar/1",INTERNET_OPEN_TYPE_PRECONFIG,nullptr,nullptr,0);
    if(!internet)return {};
    DWORD timeout=4000;
    InternetSetOptionA(internet,INTERNET_OPTION_CONNECT_TIMEOUT,&timeout,sizeof(timeout));
    InternetSetOptionA(internet,INTERNET_OPTION_SEND_TIMEOUT,&timeout,sizeof(timeout));
    InternetSetOptionA(internet,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(timeout));
    std::string url="https://cdn.discordapp.com/";
    url+=ref.compare(0,8,"default/")==0?"embed/avatars/"+ref.substr(8)+".png":"avatars/"+ref+".png?size=256";
    HINTERNET response=InternetOpenUrlA(internet,url.c_str(),nullptr,0,INTERNET_FLAG_SECURE|INTERNET_FLAG_NO_AUTO_REDIRECT|INTERNET_FLAG_NO_COOKIES|INTERNET_FLAG_NO_AUTH|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_RELOAD,0);
    std::vector<unsigned char> bytes;
    if(response) {
        DWORD status=0,size=sizeof(status);
        if(HttpQueryInfoA(response,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&status,&size,nullptr)&&status==200) {
            unsigned char buffer[8192];DWORD got=0;
            while(InternetReadFile(response,buffer,sizeof(buffer),&got)&&got) {
                if(bytes.size()+got>262144){bytes.clear();break;}
                bytes.insert(bytes.end(),buffer,buffer+got);
            }
        }
        InternetCloseHandle(response);
    }
    InternetCloseHandle(internet);
    return bytes;
}
Avatar download(std::string ref) {
    Avatar result;result.ref=ref;
    if(!validRef(ref))return result;
    auto data=png(ref);if(data.empty())return result;
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(initialized))return result;
    IWICImagingFactory* factory=nullptr;IWICStream* stream=nullptr;
    IWICBitmapDecoder* decoder=nullptr;IWICBitmapFrameDecode* frame=nullptr;
    IWICBitmapScaler* scaler=nullptr;IWICFormatConverter* converter=nullptr;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr))hr=factory->CreateStream(&stream);
    if(SUCCEEDED(hr))hr=stream->InitializeFromMemory(data.data(),data.size());
    if(SUCCEEDED(hr))hr=factory->CreateDecoderFromStream(stream,nullptr,WICDecodeMetadataCacheOnDemand,&decoder);
    if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);
    UINT w=0,h=0;
    if(SUCCEEDED(hr))hr=frame->GetSize(&w,&h);
    if(w==0||h==0||w>256||h>256)hr=E_INVALIDARG;
    if(SUCCEEDED(hr))hr=factory->CreateBitmapScaler(&scaler);
    if(SUCCEEDED(hr))hr=scaler->Initialize(frame,256,256,WICBitmapInterpolationModeFant);
    if(SUCCEEDED(hr))hr=factory->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr))hr=converter->Initialize(scaler,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(hr)) {
        result.pixels.resize(256*256*4);
        hr=converter->CopyPixels(nullptr,256*4,result.pixels.size(),result.pixels.data());
        if(FAILED(hr))result.pixels.clear();
    }
    if(converter)converter->Release();
    if(scaler)scaler->Release();
    if(frame)frame->Release();
    if(decoder)decoder->Release();
    if(stream)stream->Release();
    if(factory)factory->Release();
    CoUninitialize();
    if(result.pixels.empty())return result;
    for(int y=0;y<256;++y)for(int x=0;x<256;++x) {
        double r=std::hypot(x-127.5,y-127.5);
        double alpha=(std::max)(0.0,(std::min)(1.0,127-r));
        double ring=(std::max)(0.0,(std::min)(1.0,r-122));
        unsigned char* px=&result.pixels[(y*256+x)*4];
        const int gold[3]={87,132,156};
        for(int n=0;n<3;++n)px[n]=(unsigned char)(px[n]*(1-ring)+gold[n]*ring);
        px[3]=(unsigned char)(255*alpha);
    }
    return result;
}
}

// Renderer-owned pointers cannot survive vid_restart, even on the same map.
// Keep decoded pixels/network state: the next active frame uploads without fetching.
void reforged_avatar_renderer_reset() {
    material=nullptr;
    uploadedTexture=nullptr;
    uploadedRef.clear();
    checkedAt=0;
}

// Called on the render/game thread. Only the private rfg_av00 UI image is touched.
void reforged_avatar_frame() {
    if(clientState!=CLIENT_STATE_ACTIVE){reforged_avatar_renderer_reset();map.clear();return;}
    std::string currentMap=(char*)0x0196ffa0;
    if(map!=currentMap){map=currentMap;reforged_avatar_renderer_reset();}
    if(GetTickCount()-checkedAt<250)return;
    checkedAt=GetTickCount();
    std::string ref=Dvar_GetString("ui_rf_avatar_ref");
    if(Dvar_GetInt("ui_rf_profile_linked")!=1||!validRef(ref))return;
    if(request.valid()&&request.wait_for(std::chrono::milliseconds(0))==std::future_status::ready) {
        Avatar received=request.get();
        if(!received.pixels.empty())cached=std::move(received);
    }
    if(ref!=requested) {requested=ref;attempts=0;retryAt=GetTickCount();}
    if(cached.ref!=ref&&!request.valid()&&attempts<3&&(LONG)(GetTickCount()-retryAt)>=0) {
        ++attempts;retryAt=GetTickCount()+30000;
        request=std::async(std::launch::async,download,ref);
    }
    if(cached.ref!=ref||cached.pixels.size()!=256*256*4)return;
    if(!material)material=CG_RegisterMaterial("rfg_av00",MATERIAL_TYPE_DEFAULT);
    const unsigned char* mat=(const unsigned char*)material;
    if(!mat)return;
    const char* name=*(const char* const*)mat;
    if(!name||std::strcmp(name,"rfg_av00")!=0||*(const unsigned short*)(mat+0x34)!=1)return;
    const unsigned char* table=*(const unsigned char* const*)(mat+0x3c);
    if(!table)return;
    const unsigned char* img=*(const unsigned char* const*)(table+8);
    if(!img)return;
    IDirect3DTexture9* texture=*(IDirect3DTexture9* const*)(img+4);
    if(!texture||(texture==uploadedTexture&&ref==uploadedRef))return;
    D3DSURFACE_DESC desc={};
    if(FAILED(texture->GetLevelDesc(0,&desc))||desc.Width!=256||desc.Height!=256||desc.Format!=D3DFMT_A8R8G8B8)return;
    D3DLOCKED_RECT lock={};
    if(FAILED(texture->LockRect(0,&lock,nullptr,0)))return;
    for(int y=0;y<256;++y)std::memcpy((unsigned char*)lock.pBits+y*lock.Pitch,&cached.pixels[y*1024],1024);
    texture->UnlockRect(0);uploadedTexture=texture;uploadedRef=ref;
}
