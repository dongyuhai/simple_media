#include <stdlib.h>
#include <string.h>
#include "sm_codec/sm_venc.h"
#include "sm_common/sm_common.h"
#include "sm_common/sm_log.h"
#include "mfxplugin.h"
#include "mfxvideo.h"
#include "mfxenc.h"
#include "mfxastructures.h"

typedef struct sm_venc_intel {
    mfxSession session;
    mfxFrameSurface1* p_surface;

    mfxBitstream *p_bitstream;
    mfxSyncPoint *p_sync_point;
    int32_t surface_plane_stride[4];
    uint32_t surface_plane_offset[4];
    int32_t surface_plane_height[4];
    uint32_t plane_num;
    uint32_t surface_num;
    uint32_t out_buffer_num;
    int32_t out_buffer_index;
    mfxVideoParam mfx_params;
    sm_venc_param_t in_param;
    SM_VFRAME_CALLBACK frame_callback;
    void *user_point;
#ifndef _WIN32
    CHWDevice *p_hwdev;
#endif
    uint32_t out_frame_size;
    int32_t have_get_error;
    uint32_t in_data_size;
    int32_t have_load_h265;
    uint64_t in_picture_count;
    uint64_t out_frame_count;
    sm_vcodec_extdata_t extdata;

    mfxExtBuffer * ext[1];
    mfxExtVideoSignalInfo signal_info;
}sm_venc_intel_t;


sm_status_t sm_venc_intel_process_pix_fmt(sm_venc_intel_t *p_ivenc)
{
    int32_t height32 = SM_ALIGN32(p_ivenc->in_param.height);
    switch (p_ivenc->in_param.pix_fmt)
    {
    case SM_PIX_FMT_NV12:
    case SM_PIX_FMT_NV21:
        p_ivenc->mfx_params.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
        p_ivenc->mfx_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32 * 3 / 2;
        p_ivenc->surface_plane_stride[0] = p_ivenc->mfx_params.mfx.FrameInfo.Width;
        p_ivenc->surface_plane_offset[0] = 0;
        p_ivenc->surface_plane_height[0] = p_ivenc->in_param.height;
        p_ivenc->surface_plane_stride[1] = p_ivenc->mfx_params.mfx.FrameInfo.Width;
        p_ivenc->surface_plane_offset[1] = p_ivenc->mfx_params.mfx.FrameInfo.Width*p_ivenc->in_param.height;
        p_ivenc->surface_plane_height[1] = p_ivenc->in_param.height/2;
        p_ivenc->plane_num = 2;
        return SM_STATUS_SUCCESS;
    case SM_PIX_FMT_YV12:
    case SM_PIX_FMT_I420:
        p_ivenc->mfx_params.mfx.FrameInfo.FourCC = MFX_FOURCC_YV12;
        p_ivenc->mfx_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32 * 3 / 2;
        p_ivenc->surface_plane_stride[0] = p_ivenc->mfx_params.mfx.FrameInfo.Width;
        p_ivenc->surface_plane_offset[0] = 0;
        p_ivenc->surface_plane_height[0] = p_ivenc->in_param.height;
        p_ivenc->surface_plane_stride[1] = p_ivenc->mfx_params.mfx.FrameInfo.Width/2;
        p_ivenc->surface_plane_offset[1] = p_ivenc->mfx_params.mfx.FrameInfo.Width*p_ivenc->in_param.height;
        p_ivenc->surface_plane_height[1] = p_ivenc->in_param.height/2;
        p_ivenc->surface_plane_stride[2] = p_ivenc->mfx_params.mfx.FrameInfo.Width/2;
        p_ivenc->surface_plane_offset[2] = p_ivenc->mfx_params.mfx.FrameInfo.Width*p_ivenc->in_param.height*5/4;
        p_ivenc->surface_plane_height[2] = p_ivenc->in_param.height/2;
        p_ivenc->plane_num = 3;
        return SM_STATUS_SUCCESS;
    case SM_PIX_FMT_YUY2:
        p_ivenc->mfx_params.mfx.FrameInfo.FourCC = MFX_FOURCC_YUY2;
        p_ivenc->mfx_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32;
        p_ivenc->surface_plane_stride[0] = p_ivenc->mfx_params.mfx.FrameInfo.Width*2;
        p_ivenc->surface_plane_offset[0] = 0;
        p_ivenc->surface_plane_height[0] = p_ivenc->in_param.height;
        p_ivenc->plane_num = 1;
        return SM_STATUS_SUCCESS;
    case SM_PIX_FMT_P010:
        p_ivenc->mfx_params.mfx.FrameInfo.BitDepthLuma = 10;
        p_ivenc->mfx_params.mfx.FrameInfo.BitDepthChroma = 10;
        p_ivenc->mfx_params.mfx.FrameInfo.Shift = 1;
        p_ivenc->mfx_params.mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
        p_ivenc->mfx_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32 * 3 / 2;
        p_ivenc->surface_plane_stride[0] = p_ivenc->mfx_params.mfx.FrameInfo.Width*2;
        p_ivenc->surface_plane_offset[0] = 0;
        p_ivenc->surface_plane_height[0] = p_ivenc->in_param.height;
        p_ivenc->surface_plane_stride[1] = p_ivenc->mfx_params.mfx.FrameInfo.Width*2;
        p_ivenc->surface_plane_offset[1] = p_ivenc->mfx_params.mfx.FrameInfo.Width*p_ivenc->in_param.height*2;
        p_ivenc->surface_plane_height[1] = p_ivenc->in_param.height/2;
        p_ivenc->plane_num = 2;
        return SM_STATUS_SUCCESS;
    case SM_PIX_FMT_BGRA:
        p_ivenc->mfx_params.mfx.FrameInfo.FourCC = MFX_FOURCC_RGB4;
        p_ivenc->mfx_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32;
        p_ivenc->surface_plane_stride[0] = p_ivenc->mfx_params.mfx.FrameInfo.Width * 4;
        p_ivenc->surface_plane_offset[0] = 0;
        p_ivenc->surface_plane_height[0] = p_ivenc->in_param.height;
        p_ivenc->plane_num = 1;
        return SM_STATUS_SUCCESS;
    case SM_PIX_FMT_RGBA:
        p_ivenc->mfx_params.mfx.FrameInfo.FourCC = MFX_FOURCC_BGR4;
        p_ivenc->mfx_params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
        p_ivenc->in_data_size = SM_ALIGN32(p_ivenc->in_param.width) * height32;
        p_ivenc->surface_plane_stride[0] = p_ivenc->mfx_params.mfx.FrameInfo.Width * 4;
        p_ivenc->surface_plane_offset[0] = 0;
        p_ivenc->surface_plane_height[0] = p_ivenc->in_param.height;
        p_ivenc->plane_num = 1;
        return SM_STATUS_SUCCESS;
    }
    SM_LOGE("unsupport pix_fmt %d\n", p_ivenc->in_param.pix_fmt);
    return SM_STATUS_NOT_SUPOORT;
}

sm_status_t sm_venc_intel_create_session(sm_venc_intel_t *p_ivenc)
{
    mfxInitParam init_param;
    mfxStatus ret;
    memset(&init_param, 0, sizeof(init_param));
    init_param.Version.Major = 1;
    init_param.Version.Minor = 3;
    init_param.GPUCopy = MFX_GPUCOPY_DEFAULT;
    init_param.Implementation = MFX_IMPL_HARDWARE_ANY;
    ret = MFXInitEx(init_param, &(p_ivenc->session));
    if (MFX_ERR_NONE != ret) {
        SM_LOGE("MFXInitEx fail, ret %d\n", ret);
        return SM_STATUS_FAIL;
    }
    return SM_STATUS_SUCCESS;
}

int16_t sm_venc_intel_get_h264_profile(sm_vcodec_profile_t profile)
{
    switch (profile)
    {
    case SM_VCODEC_PROFILE_H264_BASELINE:
        return MFX_PROFILE_AVC_BASELINE;
    case SM_VCODEC_PROFILE_H264_MAIN:
        return MFX_PROFILE_AVC_MAIN;
    case SM_VCODEC_PROFILE_H264_HIGH:
        return MFX_PROFILE_AVC_HIGH;
    default:
        return MFX_PROFILE_AVC_MAIN;
    }
    return 0;
}

sm_status_t sm_venc_intel_set_profile_level(sm_venc_intel_t *p_ivenc)
{
    if (SM_CODE_TYPE_H264 == p_ivenc->in_param.code_type) {
        if (SM_PIX_FMT_P010 == p_ivenc->in_param.pix_fmt) {
            SM_LOGE("h264 not support p010\n");
            return SM_STATUS_NOT_SUPOORT;
        }
        p_ivenc->mfx_params.mfx.CodecId = MFX_CODEC_AVC;
        p_ivenc->mfx_params.mfx.CodecProfile = sm_venc_intel_get_h264_profile(p_ivenc->in_param.profile);//yuy2 MFX_PROFILE_AVC_HIGH_422
        p_ivenc->mfx_params.mfx.CodecLevel = p_ivenc->in_param.level;
    }
    else if (SM_CODE_TYPE_H265 == p_ivenc->in_param.code_type) {
        p_ivenc->mfx_params.mfx.CodecId = MFX_CODEC_HEVC;
        p_ivenc->have_load_h265 = 1;
        MFXVideoUSER_Load(p_ivenc->session, &MFX_PLUGINID_HEVCE_HW, 1);
        p_ivenc->mfx_params.mfx.CodecProfile = p_ivenc->in_param.pix_fmt == SM_PIX_FMT_P010 ? MFX_PROFILE_HEVC_MAIN10 : MFX_PROFILE_HEVC_MAIN;
        p_ivenc->mfx_params.mfx.CodecLevel = p_ivenc->in_param.level;
    }
    else {
        SM_LOGE("not support code_type %d\n", p_ivenc->in_param.code_type);
        return SM_STATUS_NOT_SUPOORT;
    }
    return SM_STATUS_SUCCESS;
}
mfxU16 sm_venc_intel_bitrate(uint32_t in_bitrate)
{
    if (in_bitrate > 65535) {
        return 65535;
    }
    return (mfxU16)in_bitrate;
}

sm_status_t sm_venc_intel_set_bitrate(sm_venc_intel_t *p_ivenc)
{

    p_ivenc->mfx_params.mfx.InitialDelayInKB = 0;
    p_ivenc->out_frame_size = p_ivenc->in_param.width * p_ivenc->in_param.height / 2;
    if (p_ivenc->out_frame_size / 1024 <= 512) {//fixme
        p_ivenc->out_frame_size = 512 * 1024;
    }
    if (SM_RATECONTROL_VBR == p_ivenc->in_param.rate_control.mode) {
        p_ivenc->mfx_params.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        p_ivenc->mfx_params.mfx.MaxKbps = sm_venc_intel_bitrate(p_ivenc->in_param.rate_control.max_bitrate_kbps);
        p_ivenc->mfx_params.mfx.TargetKbps = sm_venc_intel_bitrate(p_ivenc->in_param.rate_control.target_bitrate_kbps);
        p_ivenc->in_param.rate_control.max_bitrate_kbps = p_ivenc->mfx_params.mfx.MaxKbps;
        p_ivenc->in_param.rate_control.target_bitrate_kbps = p_ivenc->mfx_params.mfx.TargetKbps;
    }
    else if (SM_RATECONTROL_CQP == p_ivenc->in_param.rate_control.mode) {
        p_ivenc->mfx_params.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        p_ivenc->mfx_params.mfx.QPI = p_ivenc->in_param.rate_control.qpi;
        p_ivenc->mfx_params.mfx.QPP = p_ivenc->in_param.rate_control.qpp;
        p_ivenc->mfx_params.mfx.QPB = p_ivenc->in_param.rate_control.qpb;

        p_ivenc->out_frame_size = p_ivenc->in_param.width * p_ivenc->in_param.height * 2;
        if ((p_ivenc->out_frame_size / 1024) <= 640) {//fixme
            p_ivenc->out_frame_size = 640 * 1024;
        }
    }
    else {//cbr
        p_ivenc->mfx_params.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        p_ivenc->mfx_params.mfx.MaxKbps = sm_venc_intel_bitrate(p_ivenc->in_param.rate_control.max_bitrate_kbps);

        p_ivenc->in_param.rate_control.mode = SM_RATECONTROL_CBR;
        p_ivenc->in_param.rate_control.max_bitrate_kbps = p_ivenc->mfx_params.mfx.MaxKbps;
        p_ivenc->in_param.rate_control.target_bitrate_kbps = p_ivenc->mfx_params.mfx.TargetKbps;
    }
    p_ivenc->mfx_params.mfx.BufferSizeInKB = p_ivenc->out_frame_size / 1024;

    p_ivenc->mfx_params.mfx.FrameInfo.FrameRateExtN = p_ivenc->in_param.fps.num;
    p_ivenc->mfx_params.mfx.FrameInfo.FrameRateExtD = p_ivenc->in_param.fps.den;
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_intel_set_gop(sm_venc_intel_t *p_ivenc)
{
    p_ivenc->mfx_params.mfx.GopRefDist = p_ivenc->in_param.gop_ref_size;
    p_ivenc->mfx_params.mfx.GopPicSize = p_ivenc->in_param.gop_size;//60;
    p_ivenc->mfx_params.mfx.NumRefFrame = 0;
    p_ivenc->mfx_params.mfx.IdrInterval = (SM_CODE_TYPE_H265 == p_ivenc->in_param.code_type) ? 1 : 0;

    p_ivenc->mfx_params.mfx.GopOptFlag = MFX_GOP_CLOSED;
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_intel_set_preset(sm_venc_intel_t *p_ivenc)
{

    if (SM_VENC_PRESET_BEST_QUALITY == p_ivenc->in_param.ext.preset) {
        p_ivenc->mfx_params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_QUALITY;
    }
    else if (SM_VENC_PRESET_BEST_SPEED == p_ivenc->in_param.ext.preset) {
        p_ivenc->mfx_params.mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
    }
    else {
        p_ivenc->mfx_params.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
        p_ivenc->in_param.ext.preset = SM_VENC_PRESET_BALANCED;
    }
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_intel_set_signal_info(sm_venc_intel_t *p_ivenc)
{
    p_ivenc->signal_info.Header.BufferId = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
    p_ivenc->signal_info.Header.BufferSz = sizeof(p_ivenc->signal_info);
    p_ivenc->signal_info.VideoFormat = 0;
    p_ivenc->signal_info.VideoFullRange = p_ivenc->in_param.ext.yuv_is_full_range;

    if ((SM_VCODEC_COLOR_PRI_COUNT == p_ivenc->in_param.ext.color_primaries) &&
        (SM_VCODEC_COLOR_TRC_COUNT == p_ivenc->in_param.ext.color_trc) &&
        (SM_VCODEC_COLOR_SPACE_COUNT == p_ivenc->in_param.ext.color_space)) {
        p_ivenc->signal_info.ColourDescriptionPresent = 0;
    }
    else {
        p_ivenc->signal_info.ColourDescriptionPresent = 1;
        p_ivenc->signal_info.ColourPrimaries = p_ivenc->in_param.ext.color_primaries;
        p_ivenc->signal_info.TransferCharacteristics = p_ivenc->in_param.ext.color_trc;
        p_ivenc->signal_info.MatrixCoefficients = p_ivenc->in_param.ext.color_space;
    }
    p_ivenc->ext[0] = &(p_ivenc->signal_info.Header);
    p_ivenc->mfx_params.ExtParam = p_ivenc->ext;
    p_ivenc->mfx_params.NumExtParam = 1;
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_intel_set_frame_info(sm_venc_intel_t *p_ivenc)
{
    if (SM_VCODEC_PICSTRUCT_TFF == p_ivenc->in_param.ext.pic_struct) {
        p_ivenc->mfx_params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
    }
    else if (SM_VCODEC_PICSTRUCT_BFF == p_ivenc->in_param.ext.pic_struct) {
        p_ivenc->mfx_params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_FIELD_BFF;
    }
    else {
        p_ivenc->mfx_params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        p_ivenc->in_param.ext.pic_struct = SM_VCODEC_PICSTRUCT_PROGRESSIVE;
    }

    p_ivenc->mfx_params.mfx.FrameInfo.Width = SM_ALIGN16(p_ivenc->in_param.width);
    p_ivenc->mfx_params.mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE == p_ivenc->mfx_params.mfx.FrameInfo.PicStruct) ? SM_ALIGN16(p_ivenc->in_param.height) : SM_ALIGN32(p_ivenc->in_param.height);
    p_ivenc->mfx_params.mfx.FrameInfo.CropX = 0;
    p_ivenc->mfx_params.mfx.FrameInfo.CropY = 0;
    p_ivenc->mfx_params.mfx.FrameInfo.CropW = p_ivenc->in_param.width;
    p_ivenc->mfx_params.mfx.FrameInfo.CropH = p_ivenc->in_param.height;

    p_ivenc->mfx_params.mfx.NumSlice = p_ivenc->in_param.ext.slice_num;
    return SM_STATUS_SUCCESS;
}
sm_status_t sm_venc_intel_set_priv(sm_venc_intel_t *p_ivenc)
{
    //p_ivenc->mfx_params.AsyncDepth = p_param->intel_async_depth ? p_param->intel_async_depth : 1;//4
    p_ivenc->out_buffer_num = (p_ivenc->mfx_params.AsyncDepth == 1) ? 1 : (p_ivenc->mfx_params.AsyncDepth - 1);//p_ivenc->parms.AsyncDepth - 1;
    p_ivenc->mfx_params.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    return SM_STATUS_SUCCESS;
}
#ifndef _WIN32
void sm_venc_intel_set_va(mw_intel_venc_t *p_ivenc)
{
    mfxHDL hdl = NULL;
    p_ivenc->p_hwdev = CreateVAAPIDevice();
    if (NULL == p_ivenc->p_hwdev)
    {
        return;
    }
    p_ivenc->p_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    MFXVideoCORE_SetHandle(p_ivenc->session, MFX_HANDLE_VA_DISPLAY, hdl);
}
#endif

HANDLE sm_venc_create_intel(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
    mfxStatus ret;
    mfxFrameAllocRequest enc_request;
    sm_venc_intel_t *p_ivenc;
    p_ivenc = (sm_venc_intel_t*)malloc(sizeof(sm_venc_intel_t));
    if (NULL == p_ivenc) {
        SM_LOGE("malloc sm_venc_intel_t fail\n");
        return NULL;
    }
    memset(p_ivenc, 0, sizeof(sm_venc_intel_t));
    memcpy(&(p_ivenc->in_param), p_param, sizeof(p_ivenc->in_param));
    p_ivenc->frame_callback = frame_callback;
    p_ivenc->user_point = user_point;

    if (sm_venc_intel_create_session(p_ivenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_intel_process_pix_fmt(p_ivenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_intel_set_bitrate(p_ivenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_intel_set_gop(p_ivenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_intel_set_signal_info(p_ivenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
    if (sm_venc_intel_set_frame_info(p_ivenc) != SM_STATUS_SUCCESS) {
        goto fail;
    }
#ifndef _WIN32
    sm_venc_intel_set_va(p_ivenc);
#endif
    ret = MFXVideoENCODE_Query(p_ivenc->session, &p_ivenc->mfx_params, &p_ivenc->mfx_params);
    if (ret < MFX_ERR_NONE) {
        SM_LOGE("query mfx_params fail %d\n", ret);
        goto fail;
    }
    if (ret > MFX_ERR_NONE) {
        SM_LOGE("query mfx_params have warn %d\n", ret);
    }
    memset(&enc_request, 0, sizeof(enc_request));
    ret = MFXVideoENCODE_QueryIOSurf(p_ivenc->session, &p_ivenc->mfx_params, &enc_request);
    if (MFX_ERR_NONE != ret) {
        SM_LOGE("query IOSurf fail\n");
        goto fail;
    }
    p_ivenc->surface_num = enc_request.NumFrameSuggested;

    ret = MFXVideoENCODE_Init(p_ivenc->session, &p_ivenc->mfx_params);
    if ((MFX_ERR_NONE != ret) && (MFX_WRN_PARTIAL_ACCELERATION != ret) && (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM != ret) && (MFX_WRN_VALUE_NOT_CHANGED != ret)) {
        SM_LOGE("encoder init fail\n");
        goto fail;
    }
    SM_LOGE("create mfx encoder %p\n", p_ivenc);
    return p_ivenc;
fail:
    if (p_ivenc->have_load_h265) {
        MFXVideoUSER_UnLoad(p_ivenc->session, &MFX_PLUGINID_HEVCE_HW);
    }
    if (p_ivenc->session) {
        MFXClose(p_ivenc->session);
    }
    if (p_ivenc) {
        free(p_ivenc);
    }
    
    return NULL;
}

sm_status_t sm_venc_intel_create_io(sm_venc_intel_t *p_ivenc)
{
    int32_t i;
    //p_surface
    if (NULL == p_ivenc->p_surface) {
        p_ivenc->p_surface = (mfxFrameSurface1*)malloc(p_ivenc->surface_num * sizeof(mfxFrameSurface1));
    }
    if (NULL == p_ivenc->p_surface) {
        SM_LOGE("suface malloc fail\n");
        return SM_STATUS_MALLOC;
    }
    memset(p_ivenc->p_surface, 0, p_ivenc->surface_num * sizeof(mfxFrameSurface1));
    //p_bitstream
    if (NULL == p_ivenc->p_bitstream) {
        p_ivenc->p_bitstream = (mfxBitstream*)malloc(p_ivenc->out_buffer_num * sizeof(mfxBitstream));
    }
    if (NULL == p_ivenc->p_bitstream) {
        SM_LOGE("malloc p_bitstream fail\n");
        return SM_STATUS_MALLOC;
    }
    memset(p_ivenc->p_bitstream, 0, p_ivenc->out_buffer_num * sizeof(mfxBitstream));
    //p_sync_point
    if (NULL == p_ivenc->p_sync_point) {
        p_ivenc->p_sync_point = (mfxSyncPoint *)malloc(p_ivenc->out_buffer_num * sizeof(mfxSyncPoint));
    }
    if (NULL == p_ivenc->p_sync_point) {
        SM_LOGE("malloc p_sync fail\n");
        return SM_STATUS_MALLOC;
    }
    memset(p_ivenc->p_sync_point, 0, p_ivenc->out_buffer_num * sizeof(mfxSyncPoint));
    //p_surface
    for (i = 0; i < p_ivenc->surface_num; i++) {
        memcpy(&p_ivenc->p_surface[i].Info, &(p_ivenc->mfx_params.mfx.FrameInfo), sizeof(mfxFrameInfo));
        p_ivenc->p_surface[i].Data.Y = (mfxU8 *)malloc(p_ivenc->in_data_size);//need fix
        if (NULL == p_ivenc->p_surface[i].Data.Y) {
            SM_LOGE("in frame malloc fail\n");
            return SM_STATUS_MALLOC;
        }
    }
    //p_bitstream
    for (i = 0; i < p_ivenc->out_buffer_num; i++) {
        if (NULL == p_ivenc->p_bitstream[i].Data) {
            p_ivenc->p_bitstream[i].Data = (mfxU8*)malloc(p_ivenc->out_frame_size);
            p_ivenc->p_bitstream[i].MaxLength = p_ivenc->out_frame_size;
        }
        if (NULL == p_ivenc->p_bitstream[i].Data) {
            SM_LOGE("out frame malloc fail\n");
            return SM_STATUS_MALLOC;
        }
    }
    return SM_STATUS_SUCCESS;
}

int32_t sm_venc_intel_get_surface_index(sm_venc_intel_t *p_ivenc)
{
    int32_t index = -1;
    for (index = 0; index < p_ivenc->surface_num; index++) {
        if (0 == p_ivenc->p_surface[index].Data.Locked) {
            break;
        }
    }
    if (index >= p_ivenc->surface_num) {
        SM_LOGE("have no free surface\n");
        p_ivenc->have_get_error = 1;
        return -1;
    }
    return index;
}

sm_frame_type_t sm_venc_intel_frametype(uint16_t mfx_frame_type)
{
    if ((MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF) == mfx_frame_type) {
        return SM_FRAME_TYPE_IDR;
    }
    else if ((MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF) == mfx_frame_type) {
        return SM_FRAME_TYPE_I;
    }
    else if ((MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF) == mfx_frame_type) {
        return SM_FRAME_TYPE_P;
    }
    else if (MFX_FRAMETYPE_B == (mfx_frame_type & MFX_FRAMETYPE_B)) {
        return SM_FRAME_TYPE_B;
    }
    else {
        return SM_FRAME_TYPE_UNKNOWN;
    }
    return SM_FRAME_TYPE_UNKNOWN;
}
int32_t sm_venc_intel_output(sm_venc_intel_t *p_ivenc)
{
    mfxStatus ret;
    int32_t index = -1;
    ret = MFXVideoCORE_SyncOperation(p_ivenc->session, p_ivenc->p_sync_point[p_ivenc->out_buffer_index], 300000);
    if (MFX_ERR_NONE == ret) {
        if (p_ivenc->frame_callback) {
            sm_frame_info_t frame_info;
            frame_info.frame_type = sm_venc_intel_frametype(p_ivenc->p_bitstream[p_ivenc->out_buffer_index].FrameType);
            frame_info.pts = p_ivenc->p_bitstream[p_ivenc->out_buffer_index].TimeStamp;
            frame_info.frame_len = p_ivenc->p_bitstream[p_ivenc->out_buffer_index].DataLength;
            frame_info.p_frame = (uint8_t*)p_ivenc->p_bitstream[p_ivenc->out_buffer_index].Data;
            p_ivenc->frame_callback(p_ivenc->user_point, &frame_info);
        }
        index = p_ivenc->out_buffer_index;
        p_ivenc->out_buffer_index = (p_ivenc->out_buffer_index + 1) % p_ivenc->out_buffer_num;
    }
    else {
        SM_LOGE("MFXVideoCORE_SyncOperation fail %d\n", ret);
    }
    return index;
}
int32_t sm_venc_intel_get_out_buffer_index(sm_venc_intel_t *p_ivenc)
{
    int index = -1;
    int i = p_ivenc->out_buffer_index;
    do {
        if (NULL == p_ivenc->p_sync_point[i]) {
            index = i;
            break;
        }
        i++;
        if (i >= p_ivenc->out_buffer_num) {
            i = 0;
        }
    } while (i != p_ivenc->out_buffer_index);
    if (index < 0) {
        index = sm_venc_intel_output(p_ivenc);
    }
    if (index >= 0) {
        p_ivenc->p_bitstream[index].DataOffset = 0;
        p_ivenc->p_bitstream[index].DataLength = 0;
        p_ivenc->p_sync_point[index] = NULL;
    }
    return index;
}
void sm_venc_intel_fill_surface(sm_venc_intel_t *p_ivenc, sm_picture_info_t *p_picture, int32_t index)
{    
    for (int32_t i = 0; i < p_ivenc->plane_num; i++) {
        uint8_t *p_src = p_picture->p_plane[i];
        uint8_t *p_dst = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[i];
        int32_t copy_stride = 0;
        if (p_ivenc->surface_plane_stride[i] == p_picture->plane_stride[i]) {
            memcpy(p_dst, p_src, p_picture->plane_stride[i] * p_ivenc->surface_plane_height[i]);
            continue;
        }
        else if(p_ivenc->surface_plane_stride[i] > p_picture->plane_stride[i]){
            copy_stride = p_picture->plane_stride[i];
        }
        else {
            copy_stride = p_ivenc->surface_plane_stride[i];
        }
        for (int32_t j = 0; i < p_ivenc->surface_plane_height[i]; j++) {
            memcpy(p_dst, p_src, copy_stride);
            p_dst += p_ivenc->surface_plane_stride[i];
            p_src += p_picture->plane_stride[i];
        }
    }
    p_ivenc->p_surface[index].Data.Pitch = p_ivenc->surface_plane_stride[0];

    switch (p_ivenc->in_param.pix_fmt)
    {
    case SM_PIX_FMT_NV12:
    case SM_PIX_FMT_NV21:
        p_ivenc->p_surface[index].Data.B = p_ivenc->p_surface[index].Data.Y;
        if (SM_PIX_FMT_NV12 == p_ivenc->in_param.pix_fmt) {
            p_ivenc->p_surface[index].Data.U = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[1];
            p_ivenc->p_surface[index].Data.V = p_ivenc->p_surface[index].Data.U + 1;
        }
        else {
            p_ivenc->p_surface[index].Data.V = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[1];
            p_ivenc->p_surface[index].Data.U = p_ivenc->p_surface[index].Data.V + 1;
        }
        break;
    case SM_PIX_FMT_YV12:
    case SM_PIX_FMT_I420:
        p_ivenc->p_surface[index].Data.B = p_ivenc->p_surface[index].Data.Y;
        if (SM_PIX_FMT_I420 == p_ivenc->in_param.pix_fmt) {
            p_ivenc->p_surface[index].Data.U = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[1];
            p_ivenc->p_surface[index].Data.V = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[2];
        }
        else {
            p_ivenc->p_surface[index].Data.V = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[1];
            p_ivenc->p_surface[index].Data.U = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[2];
        }
        break;
    case SM_PIX_FMT_YUY2:
        p_ivenc->p_surface[index].Data.B = p_ivenc->p_surface[index].Data.Y;
        p_ivenc->p_surface[index].Data.U = p_ivenc->p_surface[index].Data.Y + 1;
        p_ivenc->p_surface[index].Data.V = p_ivenc->p_surface[index].Data.U + 3;
        break;
    case SM_PIX_FMT_P010:
        p_ivenc->p_surface[index].Data.B = p_ivenc->p_surface[index].Data.Y;
        p_ivenc->p_surface[index].Data.U = p_ivenc->p_surface[index].Data.Y + p_ivenc->surface_plane_offset[1];
        p_ivenc->p_surface[index].Data.V = p_ivenc->p_surface[index].Data.U + 2;
        break;
    case SM_PIX_FMT_RGBA:
    case SM_PIX_FMT_BGRA:
    case SM_PIX_FMT_ARGB:
    case SM_PIX_FMT_ABGR:
        p_ivenc->p_surface[index].Data.G = p_ivenc->p_surface[index].Data.R + 1;
        p_ivenc->p_surface[index].Data.B = p_ivenc->p_surface[index].Data.R + 2;
        p_ivenc->p_surface[index].Data.A = p_ivenc->p_surface[index].Data.R + 3;
        break;
    default:
        return;
    }
    
}

sm_status_t sm_venc_encode_intel(HANDLE handle, sm_picture_info_t *p_picture, int32_t force_idr)
{
    mfxStatus ret;
    int32_t surface_index = -1;
    int32_t out_buffer_index = -1;
    mfxEncodeCtrl enc_ctrl;
    sm_venc_intel_t *p_ivenc = (sm_venc_intel_t *)handle;
    if ((NULL == p_ivenc) || (NULL == p_ivenc->session)) {
        return SM_STATUS_INVALID_PARAM;
    }
    if (p_ivenc->have_get_error) {
        return SM_STATUS_FAIL;
    }
    if ((NULL == p_ivenc->p_surface) || (NULL == p_ivenc->p_bitstream) || (NULL == p_ivenc->p_sync_point)) {
        sm_status_t status = sm_venc_intel_create_io(p_ivenc);
        if (SM_STATUS_SUCCESS != status) {
            p_ivenc->have_get_error = 1;
            return status;
        }
    }
    memset(&enc_ctrl, 0, sizeof(enc_ctrl));
    if (force_idr) {
        enc_ctrl.FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
    }
    else {
        enc_ctrl.FrameType = MFX_FRAMETYPE_UNKNOWN;
    }
    out_buffer_index = sm_venc_intel_get_out_buffer_index(p_ivenc);
    if (out_buffer_index < 0) {
        SM_LOGE("get out buffer fail\n");
        p_ivenc->have_get_error = 1;
        return SM_STATUS_FAIL;
    }
    surface_index = sm_venc_intel_get_surface_index(p_ivenc);
    if (surface_index < 0) {
        SM_LOGE("get surface fail\n");
        p_ivenc->have_get_error = 1;
        return SM_STATUS_FAIL;
    }
    p_ivenc->p_surface[surface_index].Data.TimeStamp = p_picture->pts;// +100000;
    p_ivenc->p_surface[surface_index].Data.FrameOrder = (mfxU32)p_ivenc->in_picture_count++;
    sm_venc_intel_fill_surface(p_ivenc, p_picture, surface_index);
    ret = MFXVideoENCODE_EncodeFrameAsync(p_ivenc->session, &enc_ctrl, &(p_ivenc->p_surface[surface_index]), &(p_ivenc->p_bitstream[out_buffer_index]), &(p_ivenc->p_sync_point[out_buffer_index]));
    if ((MFX_ERR_NONE != ret) && (MFX_ERR_MORE_DATA != ret)) {
        SM_LOGE("EncodeFrameAsync %d\n", ret);
        p_ivenc->have_get_error = 1;
        return SM_STATUS_FAIL;
    }
    return SM_STATUS_SUCCESS;
}


void sm_venc_intel_flush_encoder(sm_venc_intel_t *p_ivenc)
{
    mfxStatus ret;
    mfxEncodeCtrl enc_ctrl;
    int32_t out_index = -1;
    if (!p_ivenc->p_sync_point) {
        return;
    }
    while (1) {
        if (NULL == p_ivenc->p_sync_point[p_ivenc->out_buffer_index]) {
            return;
        }
        out_index = sm_venc_intel_output(p_ivenc);
        if (out_index < 0) {
            return;
        }
        memset(&enc_ctrl, 0, sizeof(enc_ctrl));
        enc_ctrl.FrameType = MFX_FRAMETYPE_UNKNOWN;//need idr
        ret = MFXVideoENCODE_EncodeFrameAsync(p_ivenc->session, &enc_ctrl, NULL, &(p_ivenc->p_bitstream[out_index]), &(p_ivenc->p_sync_point[out_index]));
        if ((MFX_ERR_NONE != ret) && (MFX_ERR_MORE_DATA != ret) && (MFX_ERR_NOT_ENOUGH_BUFFER != ret)) {
            SM_LOGE("EncodeFrameAsync fail ret %d\n", ret);
            return;
        }
    }
}
sm_status_t sm_venc_destory_intel(HANDLE handle)
{
    int i;
    sm_venc_intel_t *p_ivenc = (sm_venc_intel_t *)handle;
    if (NULL == p_ivenc) {
        return SM_STATUS_INVALID_PARAM;
    }
    sm_venc_intel_flush_encoder(p_ivenc);

    if (p_ivenc->session) {
        MFXVideoENCODE_Close(p_ivenc->session);
    }
    if (p_ivenc->have_load_h265) {
        MFXVideoUSER_UnLoad(p_ivenc->session, &MFX_PLUGINID_HEVCE_HW);
    }

    MFXClose(p_ivenc->session);

    if (p_ivenc->p_surface) {
        for (i = 0; i < p_ivenc->surface_num; i++) {
            if (p_ivenc->p_surface[i].Data.Y) {
                free(p_ivenc->p_surface[i].Data.Y);
            }
        }
        free(p_ivenc->p_surface);
    }
    if (p_ivenc->p_bitstream) {
        for (i = 0; i < p_ivenc->out_buffer_num; i++) {
            if (p_ivenc->p_bitstream[i].Data) {
                free(p_ivenc->p_bitstream[i].Data);
            }
        }
        free(p_ivenc->p_bitstream);
    }
    if (p_ivenc->extdata.p_extdata) {
        free(p_ivenc->extdata.p_extdata);
    }
    if (p_ivenc->p_sync_point) {
        free(p_ivenc->p_sync_point);
    }
    free(p_ivenc);
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_set_property_intel(HANDLE handle, sm_key_value_t *p_property)
{
    return SM_STATUS_SUCCESS;
}