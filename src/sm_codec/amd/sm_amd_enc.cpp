#include <stdlib.h>
#include <string.h>
#include "sm_codec/sm_venc.h"
#include "sm_common/sm_common.h"
#include "sm_common/sm_log.h"

#include <d3d9.h>
#include <d3d11.h>
#include "core/Platform.h"
#include "core/Factory.h"
#include "components/VideoEncoderVCE.h"
#include "components/VideoEncoderHEVC.h"
#ifdef AMF_CORE_STATIC
extern "C"
{
    extern AMF_CORE_LINK AMF_RESULT AMF_CDECL_CALL AMFInit(amf_uint64 version, amf::AMFFactory **ppFactory);
}
#endif


typedef struct sm_venc_amd {
    sm_venc_param_t in_params;
    SM_VFRAME_CALLBACK frame_callback;
    void *user_point;
    amf::AMFContextPtr					p_context;
    amf::AMFComponentPtr				p_encoder;
    amf::AMF_MEMORY_TYPE				mem_type_in;
    amf::AMFSurfacePtr					surface_ptr;

//     int32_t surface_plane_stride[4];
//     int32_t surface_plane_offset[4];
//     int32_t surface_plane_height[4];
//     uint32_t plane_num;
//     uint32_t surface_num;
//     uint32_t out_buffer_num;
//     int32_t out_buffer_index;
// 
//     uint32_t out_frame_size;
//     int32_t have_get_error;
//     uint32_t in_data_size;
//     int32_t have_load_h265;
//     uint64_t in_picture_count;
//     uint64_t out_frame_count;
    sm_vcodec_extdata_t extdata;
}sm_venc_amd_t;

amf_handle			g_amf_dll = NULL;
amf::AMFFactory*	g_amf_factory;
amf::AMFDebug*		g_amf_debug;
amf::AMFTrace*		g_amf_trace;
amf_uint64			g_amf_version;
amf_long			g_amf_ref;

sm_status_t init()
{
#ifndef AMF_CORE_STATIC
    if (g_amf_dll == NULL) {
        InterlockedIncrement(&g_amf_ref);
        return SM_STATUS_FAIL;
    }
    g_amf_dll = LoadLibrary(AMF_DLL_NAMEA);
    if (g_amf_dll == NULL) {
        return SM_STATUS_FAIL;
    }

    AMFInit_Fn init_fun = (AMFInit_Fn)::GetProcAddress((HMODULE)g_amf_dll, AMF_INIT_FUNCTION_NAME);// amf_get_proc_address(g_amf_dll, AMF_INIT_FUNCTION_NAME);
    if (init_fun == NULL) {
        return SM_STATUS_FAIL;
    }

    AMF_RESULT t_res = init_fun(AMF_FULL_VERSION, &g_amf_factory);
    if (t_res != AMF_OK) {
        return SM_STATUS_FAIL;
    }

    AMFQueryVersion_Fn version_fun = (AMFQueryVersion_Fn)::GetProcAddress((HMODULE)g_amf_dll, AMF_QUERY_VERSION_FUNCTION_NAME); //amf_get_proc_address(g_amf_dll, AMF_QUERY_VERSION_FUNCTION_NAME);
    if (version_fun == NULL) {
        return SM_STATUS_FAIL;
    }
    if (version_fun(&g_amf_version) != AMF_OK) {
        return SM_STATUS_FAIL;
    }

#else
    if (AMFInit(AMF_FULL_VERSION, &g_amf_factory) != AMF_OK) {
        return SM_STATUS_FAIL;
    }
    g_amf_version = AMF_FULL_VERSION;
#endif
    g_amf_factory->GetTrace(&g_amf_trace);
    g_amf_factory->GetDebug(&g_amf_debug);

    InterlockedIncrement(&g_amf_ref);
    return SM_STATUS_SUCCESS;
}

sm_status_t deinit()
{
    if (g_amf_dll == NULL) {
        return SM_STATUS_FAIL;
    }
    InterlockedDecrement(&g_amf_ref);
    if (g_amf_ref == 0) {
        FreeLibrary((HMODULE)g_amf_dll);
        g_amf_dll = NULL;
        g_amf_factory = NULL;
        g_amf_debug = NULL;
        g_amf_trace = NULL;
    }
    return SM_STATUS_SUCCESS;
}


void* get_dx11_device(int32_t index)
{
    void* p_dx11_device = NULL;
    HRESULT ret = S_OK;
#ifdef _WIN32
    do
    {
        IDXGIFactory* p_dxgi_factory = NULL;
        //ret = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&t_p_dxgi_factory);
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&p_dxgi_factory)) || p_dxgi_factory == NULL) {
            break;
        }

        IDXGIAdapter* p_dxgi_adapter = NULL;
        if (FAILED(p_dxgi_factory->EnumAdapters(index, &p_dxgi_adapter)) || p_dxgi_adapter == NULL) {
            p_dxgi_factory->Release();
            p_dxgi_factory = NULL;
            break;
        }

        ID3D11Device* p_d3d11_device = NULL;
        ID3D11DeviceContext* p_d3d11_device_context = NULL;
        UINT create_device_flags = 0;
        HMONITOR monitor = NULL;
        DWORD vp = 0;
        D3D_FEATURE_LEVEL d3d_feature_levels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };
        D3D_FEATURE_LEVEL d3d_feature_level = D3D_FEATURE_LEVEL_11_0;
        D3D_DRIVER_TYPE d3d_driver_type = D3D_DRIVER_TYPE_UNKNOWN;

        ret = D3D11CreateDevice(
            p_dxgi_adapter,
            d3d_driver_type,
            NULL,
            create_device_flags,
            d3d_feature_levels,
            _countof(d3d_feature_levels),
            D3D11_SDK_VERSION,
            &p_d3d11_device,
            &d3d_feature_level,
            &p_d3d11_device_context);
        // init dx11 failed to create hw dx11.1 device

        if (FAILED(ret)) {
            ret = D3D11CreateDevice(
                p_dxgi_adapter,
                d3d_driver_type,
                NULL,
                create_device_flags,
                d3d_feature_levels + 1,
                _countof(d3d_feature_levels) - 1,
                D3D11_SDK_VERSION,
                &p_d3d11_device,
                &d3d_feature_level,
                &p_d3d11_device_context);
            // init dx11 failed to create hw dx11 device
        }
        if (FAILED(ret)) {
            ret = D3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_SOFTWARE,
                NULL,
                create_device_flags,
                d3d_feature_levels,
                _countof(d3d_feature_levels),
                D3D11_SDK_VERSION,
                &p_d3d11_device,
                &d3d_feature_level,
                &p_d3d11_device_context);
            // init dx11 failed to create sw dx11.1 device
        }
        if (FAILED(ret)) {
            ret = D3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_SOFTWARE,
                NULL,
                create_device_flags,
                d3d_feature_levels + 1,
                _countof(d3d_feature_levels) - 1,
                D3D11_SDK_VERSION,
                &p_d3d11_device,
                &d3d_feature_level,
                &p_d3d11_device_context);

            // init dx11 failed to create sw dx11 device
        }

        ID3D10Multithread* p_d3d10_multi_thread = NULL;
        if (p_d3d11_device != NULL) {
            ret = p_d3d11_device->QueryInterface(
                __uuidof(ID3D10Multithread),
                reinterpret_cast<void**>(&p_d3d10_multi_thread));
            if (p_d3d10_multi_thread)
                p_d3d10_multi_thread->SetMultithreadProtected(true);
        }
        p_dx11_device = (void*)p_d3d11_device;
        if (p_dxgi_factory != NULL) {
            p_dxgi_factory->Release();
            p_dxgi_factory = NULL;
        }
    } while (false);

#endif
    return p_dx11_device;
}

void* get_dx9_device(int32_t index, bool b_dx9ex)
{
    void* p_dx9_device = NULL;

#ifdef _WIN32

    IDirect3D9Ex*	p_d3d9_ex = NULL;
    IDirect3D9*		p_d3d9 = NULL;

    IDirect3DDevice9Ex*		p_d3d9_device_ex = NULL;
    IDirect3DDevice9*		p_d3d9_device = NULL;

    HRESULT ret = S_OK;
    if (b_dx9ex) {
        if (Direct3DCreate9Ex(D3D_SDK_VERSION, &p_d3d9_ex) != S_OK)
            return p_d3d9_ex;
        else
            p_d3d9 = p_d3d9_ex;
    }
    else
        p_d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    D3DADAPTER_IDENTIFIER9 t_adapter_identifier = { 0 };
    if (p_d3d9->GetAdapterIdentifier(index, 0, &t_adapter_identifier) != S_OK) {
        p_d3d9->Release();
        return p_dx9_device;
    }

    D3DPRESENT_PARAMETERS               d3dpresent_parameters;
    ZeroMemory(&d3dpresent_parameters, sizeof(d3dpresent_parameters));

    d3dpresent_parameters.BackBufferWidth = 1;
    d3dpresent_parameters.BackBufferHeight = 1;
    d3dpresent_parameters.Windowed = TRUE;
    d3dpresent_parameters.SwapEffect = D3DSWAPEFFECT_COPY;

    d3dpresent_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpresent_parameters.hDeviceWindow = GetDesktopWindow();
    d3dpresent_parameters.Flags = D3DPRESENTFLAG_VIDEO;
    d3dpresent_parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    D3DCAPS9    ddCaps;
    ZeroMemory(&ddCaps, sizeof(ddCaps));
    
    if (p_d3d9->GetDeviceCaps((UINT)index, D3DDEVTYPE_HAL, &ddCaps); != S_OK) {
        p_d3d9->Release();
        return p_dx9_device;
    }

    DWORD       vp = 0;
    if (ddCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    if (b_dx9ex) {
        ret = p_d3d9_ex->CreateDeviceEx(
            index,
            D3DDEVTYPE_HAL,
            d3dpresent_parameters.hDeviceWindow,
            vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
            &d3dpresent_parameters,
            NULL,
            &p_d3d9_device_ex
        );
        if (ret == S_OK)
            p_d3d9_device = p_d3d9_device_ex;
    }
    else {
        ret = p_d3d9->CreateDevice(
            index,
            D3DDEVTYPE_HAL,
            d3dpresent_parameters.hDeviceWindow,
            vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
            &d3dpresent_parameters,
            &p_d3d9_device
        );
    }

    p_dx9_device = (void*)p_d3d9_device;

#endif

    return p_dx9_device;
}
sm_status_t create_encoder_init_mem(sm_venc_amd_t* p_avenc, int32_t index)
{

}

HANDLE sm_venc_create_amd(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
    sm_venc_amd_t* p_avenc = (sm_venc_amd_t *)malloc(sizeof(sm_venc_amd_t));
    if (NULL == p_avenc) {
        SM_LOGE("malloc p_nvenc fail\n");
        return NULL;
    }
    memset(p_avenc, 0, sizeof(sm_venc_amd_t));
    memcpy(&(p_avenc->in_params), p_param, sizeof(p_avenc->in_params));
    p_avenc->frame_callback = frame_callback;
    p_avenc->user_point = user_point;


    if (g_amf_factory->CreateContext(&p_avenc->p_context) != AMF_OK || p_avenc->p_context == NULL) {
        return NULL;
    }

    if (create_encoder_init_mem(p_avenc, index) != SM_STATUS_SUCCESS)
        return NULL;

    if (g_amf_factory->CreateComponent(p_avenc->p_context, AMFVideoEncoder_HEVC, &p_avenc->p_encoder) != AMF_OK || p_avenc->p_encoder == NULL) {
        return NULL;
    }
    //create_encoder_h265_init_props(g_amf_factory, p_param, frame_callback, user_ptr);
    //create_encoder_h265_init_props(g_amf_factory, p_param, frame_callback, user_ptr);

    //determine_encoder_mem(p_param, frame_callback, user_ptr);
}

sm_status_t sm_venc_encode_amd(HANDLE handle, sm_picture_info_t *p_picture, int32_t force_idr)
{

}

sm_status_t sm_venc_destory_amd(HANDLE handle)
{

}

sm_status_t sm_venc_set_property_amd(HANDLE handle, sm_key_value_t *p_property)
{

}