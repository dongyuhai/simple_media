#include "sm_codec/sm_venc.h"
#include "sm_common/sm_common.h"
#include "mfxplugin.h"
#include "mfxvideo.h"
#include "mfxenc.h"
#include "mfxastructures.h"

typedef struct sm_venc_intel {
    mfxSession session;
    mfxFrameSurface1* p_in;
    mfxBitstream *p_out;
    mfxSyncPoint *p_sync;
    mfxVideoParam params;
    sm_venc_param_t in_param;
    SM_VFRAME_CALLBACK frame_callback;
    void *user_point;
#ifndef _WIN32
    CHWDevice *p_hwdev;
#endif
    int32_t in_data_size;
}sm_venc_intel_t;


sm_status_t sm_vnec_intel_pix_fmt(sm_venc_intel_t *p_ivenc)
{
    int32_t height32 = SM_ALIGN32(p_ivenc->in_param.height);
    switch (p_ivenc->in_param.pix_fmt)
    {
    case SM_VIDEO_PIX_FMT_NV12:
    case SM_VIDEO_PIX_FMT_NV21:
        p_ivenc->params.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
        p_ivenc->params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32 * 3 / 2;
        return SM_STATUS_SUCCESS;
    case SM_VIDEO_PIX_FMT_YV12:
    case SM_VIDEO_PIX_FMT_I420:
        p_ivenc->params.mfx.FrameInfo.FourCC = MFX_FOURCC_YV12;
        p_ivenc->params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32 * 3 / 2;
        return SM_STATUS_SUCCESS;
    case SM_VIDEO_PIX_FMT_YUY2:
        p_ivenc->params.mfx.FrameInfo.FourCC = MFX_FOURCC_YUY2;
        p_ivenc->params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32;
        return SM_STATUS_SUCCESS;
    case SM_VIDEO_PIX_FMT_P010:
        p_ivenc->params.mfx.FrameInfo.BitDepthLuma = 10;
        p_ivenc->params.mfx.FrameInfo.BitDepthChroma = 10;
        p_ivenc->params.mfx.FrameInfo.Shift = 1;
        p_ivenc->params.mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
        p_ivenc->params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32 * 3 / 2;
        return SM_STATUS_SUCCESS;
    case SM_VIDEO_PIX_FMT_BGRA:
        p_ivenc->params.mfx.FrameInfo.FourCC = MFX_FOURCC_RGB4;
        p_ivenc->params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32;
        return SM_STATUS_SUCCESS;
    case SM_VIDEO_PIX_FMT_RGBA:
        p_ivenc->params.mfx.FrameInfo.FourCC = MFX_FOURCC_BGR4;
        p_ivenc->params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32;
        return SM_STATUS_SUCCESS;
    }
    return SM_STATUS_NOT_SUPOORT;
}
uint16_t sm_venc_intel_get_default_biterate(int width, int height, sm_fraction_t *p_fps)
{
    uint16_t biterate = 0;
    if ((width * height) <= (720 * 480)) {
        biterate = 1024;
    }
    else if ((width * height) <= (1280 * 720)) {
        biterate = 2048;
    }
    else if ((width * height) <= (1920 * 1080)) {
        biterate = 4096;
    }
    else if ((width * height) <= (3840 * 2160)) {
        biterate = 16384;
    }
    else {
        biterate = 32767;
    }
    
    if ((p_fps->num / p_fps->den) > 30) {
        biterate *= 2;\

    }

    return 0;
}

int16_t sm_vnec_intel_get_h264_profile(sm_venc_profile_t profile)
{
    switch (profile)
    {
    case SM_VENC_PROFILE_H264_BASELINE:
        return MFX_PROFILE_AVC_BASELINE;
    case SM_VENC_PROFILE_H264_MAIN:
        return MFX_PROFILE_AVC_MAIN;
    case SM_VENC_PROFILE_H264_HIGH:
        return MFX_PROFILE_AVC_HIGH;
    default:
        return MFX_PROFILE_AVC_MAIN;
    }
    return 0;
}

#ifndef _WIN32
void sm_vnec_intel_set_va(mw_intel_venc_t *p_ivenc)
{
    p_ivenc->p_hwdev = CreateVAAPIDevice();
    if (NULL == p_ivenc->p_hwdev)
    {
        return;
    }
    mfxHDL hdl = NULL;
    p_ivenc->p_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    MFXVideoCORE_SetHandle(p_ivenc->session, MFX_HANDLE_VA_DISPLAY, hdl);
}
#endif

void* sm_venc_create_intel(sm_key_value_t *p_encoder, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
    mfxStatus ret;
    mfxFrameAllocRequest enc_request;
    mfxInitParam init_par;
    sm_venc_intel_t *p_ivenc;

}
sm_status_t sm_venc_destory_intel(void* handle)
{

}
sm_status_t sm_venc_encode_intel(void* handle, sm_frame_info_t *p_frame, uint32_t force_idr, int64_t pts)
{

}

sm_status_t sm_venc_set_property_intel(void* handle, sm_key_value_t *p_property)
{

}