#include "stdlib.h"
#include "sm_codec/sm_venc.h"
#include "intel/sm_intel.h"
typedef struct sm_venc {
    HANDLE venc_handle;
    HANDLE(*create)(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point);
    sm_status_t(*destory)(HANDLE handle);
    sm_status_t(*encode)(HANDLE handle, sm_picture_info_t *p_picture, int32_t force_idr);
    sm_status_t(*set_property)(HANDLE handle, sm_key_value_t *p_property);
}sm_venc_t;
uint32_t sm_venc_get_default_bitrate(sm_venc_param_t *p_param)
{
    uint32_t bitrate = p_param->width*p_param->height/512;
    if (bitrate < 128) {
        bitrate = 128;
    }
    if ((p_param->fps.num / p_param->fps.den) < 30) {
        bitrate /= 2;
    }
    else if ((p_param->fps.num / p_param->fps.den) > 100) {
        bitrate *= 2;
    }
    return bitrate;
}

sm_status_t sm_venc_check_ext_param(sm_venc_param_t *p_param)
{
    p_param->ext.slice_num = p_param->ext.slice_num ? p_param->ext.slice_num : 1;
    if (p_param->ext.preset >= SM_VENC_PRESET_COUNT || p_param->ext.preset <= SM_VENC_PRESET_UNKNOWN) {
        p_param->ext.preset = SM_VENC_PRESET_BALANCED;
    }
    if (p_param->ext.pic_struct >= SM_VCODEC_PICSTRUCT_COUNT || p_param->ext.pic_struct < SM_VCODEC_PICSTRUCT_PROGRESSIVE) {
        p_param->ext.pic_struct = SM_VCODEC_PICSTRUCT_PROGRESSIVE;
    }
    if (!p_param->ext.signal.have_set_color) {
        return SM_STATUS_SUCCESS;
    }
    if (p_param->ext.signal.color_primaries >= SM_VCODEC_COLOR_PRI_COUNT || p_param->ext.signal.color_primaries < SM_VCODEC_COLOR_PRI_RESERVED0) {
        p_param->ext.signal.color_primaries = SM_VCODEC_COLOR_PRI_RESERVED0;
    }
    if (p_param->ext.signal.color_trc >= SM_VCODEC_COLOR_TRC_COUNT || p_param->ext.signal.color_trc < SM_VCODEC_COLOR_TRC_RESERVED0) {
        p_param->ext.signal.color_trc = SM_VCODEC_COLOR_TRC_RESERVED0;
    }
    if (p_param->ext.signal.color_space >= SM_VCODEC_COLOR_SPACE_COUNT || p_param->ext.signal.color_space <=SM_VCODEC_COLOR_SPACE_RGB) {
        p_param->ext.signal.color_space = SM_VCODEC_COLOR_SPACE_RGB;
    }
    return SM_STATUS_SUCCESS;
}

sm_status_t sm_venc_check_param(sm_venc_param_t *p_param)
{
    if ((p_param->code_type != SM_CODE_TYPE_H264) && (p_param->code_type != SM_CODE_TYPE_H265)) {
        return SM_STATUS_NOT_SUPOORT;
    }

    if (p_param->pix_fmt <= SM_PIX_FMT_UNKNOWN || p_param->pix_fmt >= SM_PIX_FMT_COUNT){
        return SM_STATUS_NOT_SUPOORT;
    }
    if (p_param->width <= 0) {
        return SM_STATUS_NOT_SUPOORT;
    }
    if (p_param->height <= 0) {
        return SM_STATUS_NOT_SUPOORT;
    }

    p_param->fps.den = (p_param->fps.den > 0) ? p_param->fps.den : 1;
    if (p_param->fps.num <= 0){
        p_param->fps.num = p_param->fps.den * 30;
    }
    else if (p_param->fps.num / p_param->fps.den > 1000) {
        p_param->fps.num = 1000 * p_param->fps.den;
    }

    if (SM_RATECONTROL_CBR == p_param->rate_control.mode) {
        if ((0 == p_param->rate_control.target_bitrate_kbps) && (0 == p_param->rate_control.max_bitrate_kbps)) {
            p_param->rate_control.target_bitrate_kbps = sm_venc_get_default_bitrate(p_param);
        }
        else if(0 == p_param->rate_control.target_bitrate_kbps){
            p_param->rate_control.target_bitrate_kbps = p_param->rate_control.max_bitrate_kbps * 2 / 3;
        }
    }
    else if (SM_RATECONTROL_VBR == p_param->rate_control.mode) {
        if ((0 == p_param->rate_control.target_bitrate_kbps) && (0 == p_param->rate_control.max_bitrate_kbps)) {
            p_param->rate_control.target_bitrate_kbps = sm_venc_get_default_bitrate(p_param);
            p_param->rate_control.max_bitrate_kbps = p_param->rate_control.target_bitrate_kbps * 3 / 2;
        }
        else if (0 == p_param->rate_control.target_bitrate_kbps) {
            p_param->rate_control.target_bitrate_kbps = p_param->rate_control.max_bitrate_kbps * 2 / 3;
        }
        else if(0 == p_param->rate_control.max_bitrate_kbps){
            p_param->rate_control.max_bitrate_kbps = p_param->rate_control.target_bitrate_kbps * 3 / 2;
        }
    }
    else if (SM_RATECONTROL_CQP == p_param->rate_control.mode) {
        if ((0 == p_param->rate_control.qpi) && (0 == p_param->rate_control.qpp) && (0 == p_param->rate_control.qpb)) {
            p_param->rate_control.qpi = p_param->rate_control.qpp = p_param->rate_control.qpb = 22;
        }
        else if (0 == p_param->rate_control.qpi) {
            p_param->rate_control.qpi = p_param->rate_control.qpp ? p_param->rate_control.qpp : p_param->rate_control.qpb;
        }
        else if (0 == p_param->rate_control.qpb) {
            p_param->rate_control.qpi = p_param->rate_control.qpp ? p_param->rate_control.qpp : p_param->rate_control.qpb;
        }
        else if (0 == p_param->rate_control.qpp) {
            p_param->rate_control.qpi = p_param->rate_control.qpp ? p_param->rate_control.qpp : p_param->rate_control.qpb;
        }
    }
    else {
        p_param->rate_control.mode = SM_RATECONTROL_VBR;
        p_param->rate_control.target_bitrate_kbps = sm_venc_get_default_bitrate(p_param);
        p_param->rate_control.max_bitrate_kbps = p_param->rate_control.target_bitrate_kbps * 3 / 2;
    }
    if (SM_CODE_TYPE_H264 == p_param->code_type) {
        if ((p_param->profile > SM_VCODEC_PROFILE_H264_HIGH) || (p_param->profile < SM_VCODEC_PROFILE_H264_BASELINE)) {
            p_param->profile = SM_VCODEC_PROFILE_H264_MAIN;
        }
    }
    else if (SM_CODE_TYPE_H265 == p_param->code_type){
        p_param->profile = SM_VCODEC_PROFILE_H265_MAIN;
        if (p_param->pix_fmt == SM_PIX_FMT_P010) {
            p_param->profile = SM_VCODEC_PROFILE_H265_MAIN10;
        }
    }
    
    p_param->gop_size = p_param->gop_size > 0 ? p_param->gop_size:60;
    p_param->gop_ref_size = p_param->gop_ref_size > 0 ? p_param->gop_ref_size:1;
    sm_venc_check_ext_param(p_param);
    return SM_STATUS_SUCCESS;
    //sm_venc_param_ext_t ext;
}
sm_venc_handle_t sm_venc_create(sm_key_value_t *p_encoder, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
    sm_venc_t *p_venc;
    if (sm_venc_check_param(p_param) != SM_STATUS_SUCCESS) {
        return NULL;
    }
    p_venc = (sm_venc_t *)malloc(sizeof(sm_venc_t));
    p_venc->create = sm_venc_create_intel;
    p_venc->destory = sm_venc_destory_intel;
    p_venc->encode = sm_venc_encode_intel;
    p_venc->set_property = sm_venc_set_property_intel;
    p_venc->venc_handle = p_venc->create(0, p_param, frame_callback, user_point);
    if (NULL == p_venc->venc_handle) {
        free(p_venc);
        return NULL;
    }
    return (sm_venc_handle_t)p_venc;
}
sm_status_t sm_venc_destory(sm_venc_handle_t handle)
{
    sm_venc_t *p_venc = (sm_venc_t *)handle;
    if ((NULL == p_venc) || (NULL == p_venc->venc_handle) || (NULL == p_venc->destory)) {
        return SM_STATUS_INVALID_PARAM;
    }
    return p_venc->destory(p_venc->venc_handle);
}
sm_status_t sm_venc_encode(sm_venc_handle_t handle, sm_picture_info_t *p_picture, int32_t force_idr)
{
    sm_venc_t *p_venc = (sm_venc_t *)handle;
    if ((NULL == p_venc) || (NULL == p_venc->venc_handle) || (NULL == p_venc->encode)) {
        return SM_STATUS_INVALID_PARAM;
    }
    return p_venc->encode(p_venc->venc_handle, p_picture, force_idr);
}

sm_status_t sm_venc_set_property(sm_venc_handle_t handle, sm_key_value_t *p_property)
{
    sm_venc_t *p_venc = (sm_venc_t *)handle;
    if ((NULL == p_venc) || (NULL == p_venc->venc_handle) || (NULL == p_venc->set_property)) {
        return SM_STATUS_INVALID_PARAM;
    }
    return p_venc->set_property(p_venc->venc_handle, p_property);
}