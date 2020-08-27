#include "sm_codec/sm_venc.h"

typedef struct sm_venc_param {
    void *venc_handle;
    void* (*create)(sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point);
    sm_status_t(*sm_venc_destory)(void* handle);
    sm_status_t(*sm_venc_encode)(void* handle, sm_frame_info_t *p_frame, uint32_t force_idr, int64_t pts);
    sm_status_t(*sm_venc_set_property)(void* handle, sm_key_value_t *p_property);
}sm_venc_param_t;

sm_venc_handle_t sm_venc_create(sm_key_value_t *p_encoder, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{

}
sm_status_t sm_venc_destory(sm_venc_handle_t handle)
{

}
sm_status_t sm_venc_encode(sm_venc_handle_t handle, sm_frame_info_t *p_frame, uint32_t force_idr, int64_t pts)
{

}

sm_status_t sm_venc_set_property(sm_venc_handle_t handle, sm_key_value_t *p_property)
{

}