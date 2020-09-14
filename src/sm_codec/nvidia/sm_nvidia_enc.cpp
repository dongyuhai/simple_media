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

#define SET_VER(configStruct, type) {configStruct.version = type##_VER;}

typedef NVENCSTATUS(NVENCAPI *MYPROC)(NV_ENCODE_API_FUNCTION_LIST*);
#ifndef _WIN32
typedef void* HINSTANCE;
#endif

typedef struct sm_venc_nvidia {
    sm_venc_param_t in_param;
    SM_VFRAME_CALLBACK frame_callback;
    void *user_point;
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
    CUresult ret;
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
        SM_LOGE("cuCtxCreate fail\n", smmajor, smminor);
        return SM_STATUS_FAIL;
    }

    if (cuCtxPopCurrent(&context) != CUDA_SUCCESS) {
        SM_LOGE("cuCtxPopCurrent fail\n", smmajor, smminor);
        return SM_STATUS_FAIL;
    }
    /*ret = cuCtxPushCurrent((CUcontext)g_p_device[device_id]);//#*
    if (ret != CUDA_SUCCESS) {
    return -9;
    }*/
    return SM_STATUS_SUCCESS;
}


HANDLE sm_venc_create_nvidia(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
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
    memcpy(&(p_nvenc->in_param), p_param, sizeof(p_nvenc->in_param));
    p_nvenc->frame_callback = frame_callback;
    p_nvenc->user_point = user_point;

    return NULL;
}

sm_status_t sm_venc_encode_nvidia(HANDLE handle, sm_picture_info_t *p_picture, int32_t force_idr)
{
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