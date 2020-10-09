#include <stdlib.h>
#include <string.h>
#include "sm_codec/sm_venc.h"
#include "sm_common/sm_common.h"
#include "sm_common/sm_log.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif 
#include "dynlink_cuda.h"
#include "nvEncodeAPI.h"
#include "sm_nvidia.h"
#define NV_MAX_DEVICE_NUM 4
#define NV_OUT_BITSTREAM_BUFFER_SIZE (2 * 1024 * 1024)

#define NVENC_SET_VER(configStruct, type) {configStruct.version = type##_VER;}

typedef NVENCSTATUS(NVENCAPI *MYPROC)(NV_ENCODE_API_FUNCTION_LIST*);
#ifndef _WIN32
typedef void* HINSTANCE;
#endif
typedef struct sm_nvidia_io_buf {
    int32_t is_encoding;
    CUdeviceptr device_ptr;
    NV_ENC_REGISTERED_PTR registered_ptr;
    NV_ENC_INPUT_PTR input_ptr;
    NV_ENC_OUTPUT_PTR output_ptr;
    HANDLE  output_completion_event;
}sm_nvidia_io_buf_t;
typedef struct sm_venc_nvidia {
    sm_venc_param_t in_params;
    SM_VFRAME_CALLBACK frame_callback;
    void *user_point;

    HANDLE session;
    NV_ENC_CONFIG config;
    NV_ENC_INITIALIZE_PARAMS nvenc_params;
    NV_ENC_BUFFER_FORMAT nvenc_pix_format;
    sm_nvidia_io_buf_t *p_io_buf;
    uint32_t io_buf_num;
    uint32_t input_frame_count;
    uint32_t output_frame_count;

    int32_t nvenc_plane_stride[4];
    int32_t nvenc_plane_height[4];
    uint32_t plane_num;
    sm_vcodec_extdata_t extdata;
}sm_venc_nvidia_t;

NV_ENCODE_API_FUNCTION_LIST *g_p_nvenc_api = NULL;
void *g_p_device[NV_MAX_DEVICE_NUM] = { NULL, NULL, NULL, NULL };

sm_status_t sm_venc_nvidia_get_nvenc_api() {
    MYPROC nvidia_api_instance;
    HINSTANCE inst_lib;
#ifndef _WIN32
    inst_lib = dlopen("libnvidia-encode.so.1", RTLD_LAZY);
#else
#if defined (_WIN64)
    inst_lib = LoadLibrary(TEXT("nvEncodeAPI64.dll"));
#else
    inst_lib = LoadLibrary(TEXT("nvEncodeAPI.dll"));
#endif

#endif
    if (NULL == inst_lib) {
        SM_LOGE("open nvidia encode lib fail\n");
        return SM_STATUS_FAIL;
    }
#ifdef _WIN32    
    nvidia_api_instance = (MYPROC)GetProcAddress(inst_lib, "NvEncodeAPICreateInstance");
#else
    nvidia_api_instance = (MYPROC)dlsym(inst_lib, "NvEncodeAPICreateInstance");
#endif
    if (NULL == nvidia_api_instance) {
        SM_LOGE("get nvidia_api_instance fail\n");
        return SM_STATUS_FAIL;
    }
    g_p_nvenc_api = (NV_ENCODE_API_FUNCTION_LIST *)malloc(sizeof(NV_ENCODE_API_FUNCTION_LIST));
    if (NULL == g_p_nvenc_api) {
        SM_LOGE("malloc g_p_nvenc_api fail\n");
        return SM_STATUS_MALLOC;
    }
    memset(g_p_nvenc_api, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
    g_p_nvenc_api->version = NV_ENCODE_API_FUNCTION_LIST_VER;
    if (NV_ENC_SUCCESS != nvidia_api_instance(g_p_nvenc_api)) {
        SM_LOGE("get nvidia_api_instance fail\n");
        free(g_p_nvenc_api);
        return SM_STATUS_FAIL;
    }
    return SM_STATUS_SUCCESS;
}

int32_t sm_venc_nvidia_get_gpu_num()
{
    CUresult ret;
    static int32_t  device_count = 0;
    static uint8_t cu_init_flag = 0;
    if (device_count) {
        return device_count;
    }
    ret = CUDA_SUCCESS;
    if (0 == cu_init_flag) {
        ret = cuInit(0, __CUDA_API_VERSION, NULL);
    }
    if (ret != CUDA_SUCCESS) {
        SM_LOGE("cuInit fail\n");
        return 0;
    }
    cu_init_flag = 1;
    ret = cuDeviceGetCount(&device_count);
    if (ret != CUDA_SUCCESS) {
        SM_LOGE("cuDeviceGetCount fail\n");
        return 0;
    }
    return device_count;
}
sm_status_t sm_venc_nvidia_get_gpu_name(int32_t device_id, char *p_gpu_name, int32_t name_len)
{
    CUdevice device;
    int32_t device_count = sm_venc_nvidia_get_gpu_num();
    if (device_id < 0) {
        SM_LOGE("device_id %d < 0\n",device_id);
        return SM_STATUS_INVALID_PARAM;
    }
    if (device_id >= device_count) {
        SM_LOGE("device_id %d >= device_count %d\n", device_id, device_count);
        return SM_STATUS_INVALID_PARAM;
    }
    if (cuDeviceGet(&device, device_id) != CUDA_SUCCESS) {
        SM_LOGE("cuDeviceGet fail\n");
        return SM_STATUS_FAIL;
    }
    if (cuDeviceGetName(p_gpu_name, name_len, device) != CUDA_SUCCESS) {
        SM_LOGE("cuDeviceGetName fail\n");
        return SM_STATUS_FAIL;
    }
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_nvidia_init(int32_t device_id)
{
    CUdevice device;
    CUcontext context;
    int32_t  device_count = 0;
    int32_t  smminor = 0, smmajor = 0;
    device_count = sm_venc_nvidia_get_gpu_num();
    // If dev is negative value, we clamp to 0
    if (device_id < 0) {
        device_id = 0;
    }
    if (device_id >= device_count) {
        SM_LOGE("device_id %d >= device_count %d\n", device_id, device_count);
        return SM_STATUS_INVALID_PARAM;
    }
    if (cuDeviceGet(&device, device_id) != CUDA_SUCCESS) {
        SM_LOGE("cuDeviceGet fail\n");
        return SM_STATUS_FAIL;
    }

    if (cuDeviceComputeCapability(&smmajor, &smminor, device_id) != CUDA_SUCCESS) {
        SM_LOGE("cuDeviceComputeCapability fail\n");
        return SM_STATUS_FAIL;
    }
    if (((smmajor << 4) + smminor) < 0x30) {
        SM_LOGE("smmajor %d smminor %d\n", smmajor, smminor);
        return SM_STATUS_FAIL;
    }

    if (cuCtxCreate((CUcontext*)(&g_p_device[device_id]), 0, device) != CUDA_SUCCESS) {
        SM_LOGE("cuCtxCreate fail\n");
        return SM_STATUS_FAIL;
    }

    if (cuCtxPopCurrent(&context) != CUDA_SUCCESS) {
        SM_LOGE("cuCtxPopCurrent fail\n");
        return SM_STATUS_FAIL;
    }
    /*if (cuCtxPushCurrent((CUcontext)g_p_device[device_id])) {
        SM_LOGE("cuCtxPushCurrent fail\n");
        return SM_STATUS_FAIL;
    }*/
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_nvidia_create_session(sm_venc_nvidia_t *p_nvenc, int32_t index)
{
    NVENCSTATUS ret;
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS open_encode_session_ex_params;
    memset(&open_encode_session_ex_params, 0, sizeof(open_encode_session_ex_params));
    NVENC_SET_VER(open_encode_session_ex_params, NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS);

    open_encode_session_ex_params.device = g_p_device[index];
    open_encode_session_ex_params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    open_encode_session_ex_params.apiVersion = NVENCAPI_VERSION;
    ret = g_p_nvenc_api->nvEncOpenEncodeSessionEx(&open_encode_session_ex_params, &(p_nvenc->session));
    if (NV_ENC_SUCCESS != ret) {
        SM_LOGE("nvEncOpenEncodeSessionEx fail, ret %d\n",ret);
        return SM_STATUS_FAIL;
    }
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_nvidia_set_profile_level(sm_venc_nvidia_t *p_nvenc)
{
    if (SM_CODE_TYPE_H265 == p_nvenc->in_params.code_type) {
        p_nvenc->nvenc_params.encodeGUID = NV_ENC_CODEC_HEVC_GUID;
    }
    else {
        p_nvenc->nvenc_params.encodeGUID = NV_ENC_CODEC_H264_GUID;
    }
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_nvidia_set_preset(sm_venc_nvidia_t *p_nvenc)
{
    if (SM_VENC_PRESET_BEST_QUALITY == p_nvenc->in_params.ext.preset) {
        p_nvenc->nvenc_params.presetGUID = NV_ENC_PRESET_HQ_GUID;
        //p_nvenc->in_params.
    }
    else if (SM_VENC_PRESET_BEST_SPEED == p_nvenc->in_params.ext.preset) {
        p_nvenc->nvenc_params.presetGUID = NV_ENC_PRESET_HP_GUID;
    }
    else {
        p_nvenc->nvenc_params.presetGUID = NV_ENC_PRESET_BD_GUID;
    }
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_nvidia_set_config(sm_venc_nvidia_t *p_nvenc)
{
    NV_ENC_PRESET_CONFIG preset_config;
    memset(&preset_config, 0, sizeof(NV_ENC_PRESET_CONFIG));
    NVENC_SET_VER(preset_config, NV_ENC_PRESET_CONFIG);
    NVENC_SET_VER(preset_config.presetCfg, NV_ENC_CONFIG);

    if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncGetEncodePresetConfig(p_nvenc->session, p_nvenc->nvenc_params.encodeGUID,
        p_nvenc->nvenc_params.presetGUID, &preset_config)) {
        printf("nvEncGetEncodePresetConfig fail\n");
        return  SM_STATUS_FAIL;
    }

    memcpy(&(p_nvenc->config), &preset_config.presetCfg, sizeof(NV_ENC_CONFIG));
    
    p_nvenc->config.gopLength = p_nvenc->in_params.gop_size;
    p_nvenc->config.mvPrecision = NV_ENC_MV_PRECISION_QUARTER_PEL;
    if (SM_CODE_TYPE_H265 == p_nvenc->in_params.code_type) {
        p_nvenc->config.encodeCodecConfig.hevcConfig.repeatSPSPPS = 1;
        p_nvenc->config.frameIntervalP = 1;//fix h265ÊÇ·ñÖ§³ÖbÖ¡
        p_nvenc->config.encodeCodecConfig.hevcConfig.idrPeriod = p_nvenc->in_params.gop_size;
        if (SM_PIX_FMT_P010 == p_nvenc->in_params.pix_fmt) {
            p_nvenc->config.profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
            p_nvenc->config.encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 2;
        }
        else {
            p_nvenc->config.profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
        }
        p_nvenc->config.encodeCodecConfig.hevcConfig.chromaFormatIDC = 1;
        p_nvenc->config.encodeCodecConfig.hevcConfig.level = p_nvenc->in_params.level * 3;
        p_nvenc->config.encodeCodecConfig.hevcConfig.sliceMode = p_nvenc->in_params.ext.slice_num > 1 ? 3 : 0;
        p_nvenc->config.encodeCodecConfig.hevcConfig.sliceModeData = p_nvenc->in_params.ext.slice_num > 1 ? p_nvenc->in_params.ext.slice_num : 0;

        p_nvenc->config.encodeCodecConfig.hevcConfig.hevcVUIParameters.videoFullRangeFlag = p_nvenc->in_params.ext.signal.yuv_is_full_range;

        if (p_nvenc->in_params.ext.signal.have_set_color) {
            p_nvenc->config.encodeCodecConfig.hevcConfig.hevcVUIParameters.colourDescriptionPresentFlag = 1;
            p_nvenc->config.encodeCodecConfig.hevcConfig.hevcVUIParameters.colourPrimaries = p_nvenc->in_params.ext.signal.color_primaries;
            p_nvenc->config.encodeCodecConfig.hevcConfig.hevcVUIParameters.transferCharacteristics = p_nvenc->in_params.ext.signal.color_trc;
            p_nvenc->config.encodeCodecConfig.hevcConfig.hevcVUIParameters.colourMatrix = p_nvenc->in_params.ext.signal.color_space;
        }
        else {
            p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.colourDescriptionPresentFlag = 0;
        }
    }
    else {
        p_nvenc->config.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
        p_nvenc->config.frameIntervalP = p_nvenc->in_params.gop_ref_size;
        p_nvenc->config.encodeCodecConfig.h264Config.idrPeriod = p_nvenc->in_params.gop_size;
        if (SM_VCODEC_PROFILE_H264_BASELINE == p_nvenc->in_params.profile) {
            p_nvenc->config.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
        }
        else if (SM_VCODEC_PROFILE_H264_HIGH == p_nvenc->in_params.profile) {
            p_nvenc->config.profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
        }
        else {
            p_nvenc->config.profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
        }
        p_nvenc->config.encodeCodecConfig.h264Config.chromaFormatIDC = 1;
        p_nvenc->config.encodeCodecConfig.h264Config.level = p_nvenc->in_params.level;
        p_nvenc->config.encodeCodecConfig.h264Config.sliceMode = p_nvenc->in_params.ext.slice_num > 1 ? 3 : 0;
        p_nvenc->config.encodeCodecConfig.h264Config.sliceModeData = p_nvenc->in_params.ext.slice_num > 1 ? p_nvenc->in_params.ext.slice_num : 0;
        p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.videoFullRangeFlag = p_nvenc->in_params.ext.signal.yuv_is_full_range;
        if (p_nvenc->in_params.ext.signal.have_set_color) {
            p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.colourDescriptionPresentFlag = 1;
            p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.colourPrimaries = p_nvenc->in_params.ext.signal.color_primaries;
            p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.transferCharacteristics = p_nvenc->in_params.ext.signal.color_trc;
            p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.colourMatrix = p_nvenc->in_params.ext.signal.color_space;

        }
        else {
            p_nvenc->config.encodeCodecConfig.h264Config.h264VUIParameters.colourDescriptionPresentFlag = 0;
        }
    }

    if (SM_VCODEC_FRAME_FILED_MODE_FIELD == p_nvenc->in_params.ext.frame_field_mode) {//iiiiiiiii
        p_nvenc->config.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FIELD;
    }
    else if (SM_VCODEC_FRAME_FILED_MODE_MBAFF == p_nvenc->in_params.ext.frame_field_mode) {
        p_nvenc->config.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_MBAFF;
    }
    else {
        p_nvenc->config.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;
    }
    p_nvenc->nvenc_params.encodeConfig = &(p_nvenc->config);
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_nvidia_set_frame_info(sm_venc_nvidia_t *p_nvenc)
{
    p_nvenc->nvenc_params.encodeWidth = p_nvenc->in_params.width;
    p_nvenc->nvenc_params.encodeHeight = p_nvenc->in_params.height;
    p_nvenc->nvenc_params.darWidth = p_nvenc->in_params.width;
    p_nvenc->nvenc_params.darHeight = p_nvenc->in_params.height;
    p_nvenc->nvenc_params.maxEncodeWidth = p_nvenc->in_params.width;
    p_nvenc->nvenc_params.maxEncodeHeight = p_nvenc->in_params.height;

    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_nvidia_set_bitrate(sm_venc_nvidia_t *p_nvenc)
{
    p_nvenc->nvenc_params.frameRateNum = p_nvenc->in_params.fps.num;
    p_nvenc->nvenc_params.frameRateDen = p_nvenc->in_params.fps.den;
    if (SM_RATECONTROL_CBR == p_nvenc->in_params.rate_control.mode) {
        p_nvenc->config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
        p_nvenc->config.rcParams.constQP.qpIntra = p_nvenc->in_params.rate_control.qpi <= 51;
        p_nvenc->config.rcParams.constQP.qpInterP = p_nvenc->in_params.rate_control.qpp <= 51;
        p_nvenc->config.rcParams.constQP.qpInterB = p_nvenc->in_params.rate_control.qpb <= 51;
    }
    else if (SM_RATECONTROL_VBR == p_nvenc->in_params.rate_control.mode) {
        p_nvenc->config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
        p_nvenc->config.rcParams.averageBitRate = p_nvenc->in_params.rate_control.target_bitrate_kbps;
        p_nvenc->config.rcParams.maxBitRate = p_nvenc->in_params.rate_control.max_bitrate_kbps;
    }
    else {
        p_nvenc->config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
        p_nvenc->config.rcParams.averageBitRate = p_nvenc->in_params.rate_control.target_bitrate_kbps;
    }
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_nvidia_set_priv(sm_venc_nvidia_t *p_nvenc)
{
#ifndef _WIN32
    p_nvenc->param.enableEncodeAsync = 0;
#else
    //p_nvenc->param.enableEncodeAsync = 1;
    NV_ENC_CAPS_PARAM caps_param;
    int asyncMode = 0;
    memset(&caps_param, 0, sizeof(NV_ENC_CAPS_PARAM));
    NVENC_SET_VER(caps_param, NV_ENC_CAPS_PARAM);
    caps_param.capsToQuery = NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;
    g_p_nvenc_api->nvEncGetEncodeCaps(p_nvenc->session, p_nvenc->nvenc_params.encodeGUID, &caps_param, (int *)&p_nvenc->nvenc_params.enableEncodeAsync);
#endif
    p_nvenc->nvenc_params.enablePTD = 1;
    p_nvenc->nvenc_params.reportSliceOffsets = 0;
    p_nvenc->nvenc_params.enableSubFrameWrite = 0;
    p_nvenc->nvenc_params.enableExternalMEHints = 0;
    return SM_STATUS_SUCCESS;
}

HANDLE sm_venc_create_nvidia(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
    NVENCSTATUS ret;
    sm_venc_nvidia_t *p_nvenc;
    if (SM_PIX_FMT_YUY2 == p_param->pix_fmt) {
        return NULL;
    }
    if (NULL == g_p_device[index]) {
        if (sm_venc_nvidia_init(index)) {
            SM_LOGE("device[%d] init cuda fail\n", 0);
            return NULL;
        }
    }
    if (NULL == g_p_nvenc_api) {
        if (sm_venc_nvidia_get_nvenc_api()) {
            return NULL;
        }
    }
    p_nvenc = (sm_venc_nvidia_t *)malloc(sizeof(sm_venc_nvidia_t));
    if (NULL == p_nvenc) {
        SM_LOGE("malloc p_nvenc fail\n");
        return NULL;
    }
    memset(p_nvenc, 0, sizeof(sm_venc_nvidia_t));
    memcpy(&(p_nvenc->in_params), p_param, sizeof(p_nvenc->in_params));
    p_nvenc->frame_callback = frame_callback;
    p_nvenc->user_point = user_point;

    NVENC_SET_VER(p_nvenc->nvenc_params, NV_ENC_INITIALIZE_PARAMS);

    if (sm_venc_nvidia_create_session(p_nvenc, index) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_nvidia_set_profile_level(p_nvenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_nvidia_set_preset(p_nvenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_nvidia_set_config(p_nvenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_nvidia_set_frame_info(p_nvenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }

    if (sm_venc_nvidia_set_bitrate(p_nvenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_nvidia_set_priv(p_nvenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }

    //p_nvenc->io_buf_num = p_nvenc->in_params.gop_ref_size;

    ret = g_p_nvenc_api->nvEncInitializeEncoder(p_nvenc->session, &(p_nvenc->nvenc_params));
    if (NV_ENC_SUCCESS != ret) {
        printf("nvEncInitializeEncoder fail ret %d\n",ret);
        goto fail;
    }
    return (HANDLE)p_nvenc;
fail:
    if (p_nvenc->session) {
        g_p_nvenc_api->nvEncDestroyEncoder(p_nvenc->session);
    }
    if (p_nvenc) {
        free(p_nvenc);
    }
    return NULL;
}

int32_t sm_venc_nvidia_create_io_buf(sm_venc_nvidia_t *p_nvenc)
{
    NV_ENC_CREATE_INPUT_BUFFER cache_input_buffer;
    NV_ENC_REGISTER_RESOURCE register_resource;
    uint32_t frame_size = 0;
    int32_t i;
    memset(&cache_input_buffer, 0, sizeof(cache_input_buffer));
    NVENC_SET_VER(cache_input_buffer, NV_ENC_CREATE_INPUT_BUFFER);

    memset(&register_resource, 0, sizeof(register_resource));
    NVENC_SET_VER(register_resource, NV_ENC_REGISTER_RESOURCE);
    register_resource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    register_resource.width = cache_input_buffer.width = p_nvenc->nvenc_params.encodeWidth;
    register_resource.height = cache_input_buffer.height = p_nvenc->nvenc_params.encodeHeight;
    cache_input_buffer.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;//NV_ENC_MEMORY_HEAP_VID;
    if (SM_PIX_FMT_NV12 == p_nvenc->in_params.pix_fmt) {
        register_resource.bufferFormat = cache_input_buffer.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
        register_resource.pitch = register_resource.width;
        frame_size = register_resource.width * register_resource.height * 3 / 2;
        p_nvenc->plane_num = 2;
        p_nvenc->nvenc_plane_height[0] = p_nvenc->in_params.height;
        p_nvenc->nvenc_plane_height[1] = p_nvenc->in_params.height/2;
        p_nvenc->nvenc_plane_stride[0] = p_nvenc->in_params.width;
        p_nvenc->nvenc_plane_stride[1] = p_nvenc->in_params.width;
    }
    else if (SM_PIX_FMT_YV12 == p_nvenc->in_params.pix_fmt) {
        register_resource.bufferFormat = cache_input_buffer.bufferFmt = NV_ENC_BUFFER_FORMAT_YV12;
        register_resource.pitch = register_resource.width;
        frame_size = register_resource.width * register_resource.height * 3 / 2;
        p_nvenc->plane_num = 3;
        p_nvenc->nvenc_plane_height[0] = p_nvenc->in_params.height;
        p_nvenc->nvenc_plane_height[1] = p_nvenc->in_params.height / 2;
        p_nvenc->nvenc_plane_height[2] = p_nvenc->in_params.height / 2;
        p_nvenc->nvenc_plane_stride[0] = p_nvenc->in_params.width;
        p_nvenc->nvenc_plane_stride[1] = p_nvenc->in_params.width / 2;
        p_nvenc->nvenc_plane_stride[2] = p_nvenc->in_params.width / 2;
    }
    else if (SM_PIX_FMT_I420 == p_nvenc->in_params.pix_fmt) {
        register_resource.bufferFormat = cache_input_buffer.bufferFmt = NV_ENC_BUFFER_FORMAT_IYUV;
        register_resource.pitch = register_resource.width;
        frame_size = register_resource.width * register_resource.height * 3 / 2;
        p_nvenc->plane_num = 3;
        p_nvenc->nvenc_plane_height[0] = p_nvenc->in_params.height;
        p_nvenc->nvenc_plane_height[1] = p_nvenc->in_params.height / 2;
        p_nvenc->nvenc_plane_height[2] = p_nvenc->in_params.height / 2;
        p_nvenc->nvenc_plane_stride[0] = p_nvenc->in_params.width;
        p_nvenc->nvenc_plane_stride[1] = p_nvenc->in_params.width / 2;
        p_nvenc->nvenc_plane_stride[2] = p_nvenc->in_params.width / 2;
    }
    else if (SM_PIX_FMT_RGBA == p_nvenc->in_params.pix_fmt) {//ABGR
        register_resource.bufferFormat = cache_input_buffer.bufferFmt = NV_ENC_BUFFER_FORMAT_ABGR;
        register_resource.pitch = register_resource.width * 4;
        frame_size = register_resource.width * register_resource.height * 4;
        p_nvenc->plane_num = 1;
        p_nvenc->nvenc_plane_height[0] = p_nvenc->in_params.height;
        p_nvenc->nvenc_plane_stride[0] = p_nvenc->in_params.width*4;
    }
    else if (SM_PIX_FMT_BGRA == p_nvenc->in_params.pix_fmt) {//ARGB
        register_resource.bufferFormat = cache_input_buffer.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
        register_resource.pitch = register_resource.width * 4;
        frame_size = register_resource.width * register_resource.height * 4;
        p_nvenc->plane_num = 1;
        p_nvenc->nvenc_plane_height[0] = p_nvenc->in_params.height;
        p_nvenc->nvenc_plane_stride[0] = p_nvenc->in_params.width * 4;
    }
    else if (SM_PIX_FMT_P010 == p_nvenc->in_params.pix_fmt) {
        register_resource.bufferFormat = cache_input_buffer.bufferFmt = NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
        register_resource.pitch = register_resource.width * 2;
        frame_size = register_resource.width * register_resource.height * 3;
        p_nvenc->plane_num = 2;
        p_nvenc->nvenc_plane_height[0] = p_nvenc->in_params.height;
        p_nvenc->nvenc_plane_height[1] = p_nvenc->in_params.height / 2;
        p_nvenc->nvenc_plane_stride[0] = p_nvenc->in_params.width*2;
        p_nvenc->nvenc_plane_stride[1] = p_nvenc->in_params.width*2;
    }
    else {
        SM_LOGE("not support pix_fmt %d\n", p_nvenc->in_params.pix_fmt);
        return -1;
    }
    p_nvenc->nvenc_pix_format = cache_input_buffer.bufferFmt;
    p_nvenc->io_buf_num = p_nvenc->in_params.gop_ref_size;
    p_nvenc->p_io_buf = (sm_nvidia_io_buf_t*)malloc(p_nvenc->io_buf_num * sizeof(sm_nvidia_io_buf_t));
    if (NULL == p_nvenc->p_io_buf) {
        printf("malloc p_io_buf fail\n");
        return -1;
    }
    memset(p_nvenc->p_io_buf, 0, p_nvenc->io_buf_num * sizeof(sm_nvidia_io_buf_t));
    for (i = 0; i < p_nvenc->io_buf_num; i++) {
        NV_ENC_CREATE_BITSTREAM_BUFFER create_bitstream_buffer;
            if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncCreateInputBuffer(p_nvenc->session, &cache_input_buffer)) {
                printf("create input buffer fail\n");
                return -1;
            }
            p_nvenc->p_io_buf[i].input_ptr = cache_input_buffer.inputBuffer;

        memset(&create_bitstream_buffer, 0, sizeof(create_bitstream_buffer));
        NVENC_SET_VER(create_bitstream_buffer, NV_ENC_CREATE_BITSTREAM_BUFFER);

        create_bitstream_buffer.size = NV_OUT_BITSTREAM_BUFFER_SIZE;
        create_bitstream_buffer.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;// NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

        if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncCreateBitstreamBuffer(p_nvenc->session, &create_bitstream_buffer)) {
            printf("create output buffer fail\n");
            return -1;
        }
        p_nvenc->p_io_buf[i].output_ptr = create_bitstream_buffer.bitstreamBuffer;
        if (p_nvenc->nvenc_params.enableEncodeAsync) {
            NV_ENC_EVENT_PARAMS event_params;
            memset(&event_params, 0, sizeof(event_params));
            NVENC_SET_VER(event_params, NV_ENC_EVENT_PARAMS);
#if defined(_WIN32)
            event_params.completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
            event_param.completionEvent = NULL;
            return 0;
#endif
            if (g_p_nvenc_api->nvEncRegisterAsyncEvent(p_nvenc->session, &event_params)) {
                return -1;
            }

            p_nvenc->p_io_buf[i].output_completion_event = event_params.completionEvent;
        }
        else {
            p_nvenc->p_io_buf[i].output_completion_event = NULL;
        }
    }
    return 0;
}

sm_frame_type_t sm_venc_nvidia_frametype(NV_ENC_PIC_TYPE frame_type)
{
    if (NV_ENC_PIC_TYPE_IDR == frame_type) {
        return SM_FRAME_TYPE_IDR;
    }
    else if (NV_ENC_PIC_TYPE_I == frame_type) {
        return SM_FRAME_TYPE_I;
    }
    else if (NV_ENC_PIC_TYPE_P == frame_type) {
        return SM_FRAME_TYPE_P;
    }
    else if (NV_ENC_PIC_TYPE_B == frame_type) {
        return SM_FRAME_TYPE_B;
    }
    return SM_FRAME_TYPE_UNKNOWN;
}

int32_t sm_venc_nvidia_get_io_buf_index(sm_venc_nvidia_t *p_nvenc)
{
    int32_t index;
    index = p_nvenc->input_frame_count % p_nvenc->io_buf_num;
    if (p_nvenc->p_io_buf[index].is_encoding) {
        NV_ENC_LOCK_BITSTREAM lock_bitstream;
        if (p_nvenc->p_io_buf[index].output_completion_event) {
#if defined(_WIN32)
            WaitForSingleObject(p_nvenc->p_io_buf[index].output_completion_event, INFINITE);
#endif
        }
        memset(&lock_bitstream, 0, sizeof(lock_bitstream));
        NVENC_SET_VER(lock_bitstream, NV_ENC_LOCK_BITSTREAM);
        lock_bitstream.outputBitstream = p_nvenc->p_io_buf[index].output_ptr;
        lock_bitstream.doNotWait = false;

        if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncLockBitstream(p_nvenc->session, &lock_bitstream)) {
            SM_LOGE("nvEncLockBitstream fail");
            return -1;
        }
        if (p_nvenc->frame_callback) {
            sm_frame_info_t frame_info;
            frame_info.frame_type = sm_venc_nvidia_frametype(lock_bitstream.pictureType);
            frame_info.pts = lock_bitstream.outputTimeStamp;
            frame_info.p_frame = (uint8_t*)lock_bitstream.bitstreamBufferPtr;
            frame_info.frame_len = lock_bitstream.bitstreamSizeInBytes;
            p_nvenc->frame_callback(p_nvenc->user_point, &frame_info);
        }
        if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncUnlockBitstream(p_nvenc->session, p_nvenc->p_io_buf[index].output_ptr)) {
            SM_LOGE("nvEncUnlockBitstream fail");
            return -1;
        }
        p_nvenc->p_io_buf[index].is_encoding = 0;
    }
    return index;
}

sm_status_t nvidia_fill_frame(sm_venc_nvidia_t *p_nvenc, sm_picture_info_t *p_picture, int index)
{
    NV_ENC_LOCK_INPUT_BUFFER lock_input_buffer;
    uint8_t *p_dst;
    memset(&lock_input_buffer, 0, sizeof(lock_input_buffer));
    NVENC_SET_VER(lock_input_buffer, NV_ENC_LOCK_INPUT_BUFFER);

    lock_input_buffer.inputBuffer = p_nvenc->p_io_buf[index].input_ptr;
    if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncLockInputBuffer(p_nvenc->session, &lock_input_buffer)) {
        SM_LOGE("nvEncLockInputBuffer fail");
        return SM_STATUS_FAIL;
    }
    p_dst = (uint8_t *)lock_input_buffer.bufferDataPtr;
    for (int i = 0; i < p_nvenc->plane_num; i++) {
        uint8_t *p_src = p_picture->p_plane[i];
        int32_t copy_stride = 0;
        if (p_nvenc->nvenc_plane_stride[i] == p_picture->plane_stride[i]) {
            memcpy(p_dst, p_src, p_picture->plane_stride[i] * p_nvenc->nvenc_plane_height[i]);
            p_dst += p_picture->plane_stride[i] * p_nvenc->nvenc_plane_height[i];
            continue;
        }
        else if (p_nvenc->nvenc_plane_stride[i] > p_picture->plane_stride[i]) {
            copy_stride = p_picture->plane_stride[i];
        }
        else {
            copy_stride = p_nvenc->nvenc_plane_stride[i];
        }
        for (int32_t j = 0; j < p_nvenc->nvenc_plane_height[i]; j++) {
            memcpy(p_dst, p_src, copy_stride);
            p_dst += p_nvenc->nvenc_plane_stride[i];
            p_src += p_picture->plane_stride[i];
        }
    }
    if (NV_ENC_SUCCESS != g_p_nvenc_api->nvEncUnlockInputBuffer(p_nvenc->session, p_nvenc->p_io_buf[index].input_ptr)) {
        SM_LOGE("nvEncUnlockInputBuffer fail");
        return SM_STATUS_FAIL;
    }
    return SM_STATUS_SUCCESS;
}
int32_t nvidia_enc_frame(sm_venc_nvidia_t *p_nvenc, int32_t index, int64_t pts, int32_t force_idr)
{
    NV_ENC_PIC_PARAMS pic_param;
    NVENCSTATUS ret;
    memset(&pic_param, 0, sizeof(pic_param));
    NVENC_SET_VER(pic_param, NV_ENC_PIC_PARAMS);

    //pic_param.frameIdx = p_nvenc->in_frame_count;
    pic_param.inputBuffer = p_nvenc->p_io_buf[index].input_ptr;
    pic_param.bufferFmt = p_nvenc->nvenc_pix_format;
    pic_param.inputWidth = p_nvenc->in_params.width;
    pic_param.inputHeight = p_nvenc->in_params.height;
    pic_param.outputBitstream = p_nvenc->p_io_buf[index].output_ptr;
    pic_param.completionEvent = p_nvenc->p_io_buf[index].output_completion_event;
    pic_param.inputTimeStamp = pts;//p_nvenc->in_frame_count * 1000000 / 30;// +1000000;
    if (NV_ENC_PARAMS_FRAME_FIELD_MODE_FIELD == p_nvenc->config.frameFieldMode) {//iiiiiiiiiii
        if (SM_VCODEC_PICSTRUCT_TFF == p_nvenc->in_params.ext.pic_struct) {
            pic_param.pictureStruct = NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM;
        }
        else {
            pic_param.pictureStruct = NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP;
        }
    }
    else {
        pic_param.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    }

    pic_param.qpDeltaMap = NULL;
    pic_param.qpDeltaMapSize = 0;
    if (force_idr) {
        pic_param.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
    }

    ret = g_p_nvenc_api->nvEncEncodePicture(p_nvenc->session, &pic_param);
    if (ret == NV_ENC_ERR_NEED_MORE_INPUT) {
        return 1;
    }
    else if (ret == NV_ENC_SUCCESS) {
        return 0;
    }
    SM_LOGE("nvEncEncodePicture fail ret[%d]", ret);
    return -1;
}

sm_status_t sm_venc_encode_nvidia(HANDLE handle, sm_picture_info_t *p_picture, int32_t force_idr)
{
    int32_t io_buf_index = -1;
    sm_venc_nvidia_t *p_nvenc = (sm_venc_nvidia_t *)handle;
    if ((NULL == p_nvenc) || (NULL == p_nvenc->session)) {
        return SM_STATUS_INVALID_PARAM;
    }

    if (NULL == p_nvenc->p_io_buf) {
        if (sm_venc_nvidia_create_io_buf(p_nvenc) < 0) {
            return SM_STATUS_FAIL;
        }
    }
    io_buf_index = sm_venc_nvidia_get_io_buf_index(p_nvenc);
    if (io_buf_index < 0) {
        printf("have no frame\n");
        return SM_STATUS_FAIL;
    }
    if (nvidia_fill_frame(p_nvenc, p_picture, io_buf_index) < 0) {
        return SM_STATUS_FAIL;
    }
    if (nvidia_enc_frame(p_nvenc, io_buf_index, p_picture->pts, force_idr) < 0) {
        return SM_STATUS_FAIL;
    }
    p_nvenc->p_io_buf[io_buf_index].is_encoding = 1;
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_destory_nvidia(HANDLE handle)
{
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_set_property_nvidia(HANDLE handle, sm_key_value_t *p_property)
{
    return SM_STATUS_SUCCESS;
}