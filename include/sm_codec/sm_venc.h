#include <stdio.h>
#include "sm_codec_common.h"
#include "sm_common/sm_status.h"
typedef struct sm_venc_handle { int32_t i; } *sm_venc_handle_t;
//vaapi  path; x264/x265; plaform[x]
sm_venc_handle_t sm_venc_create(sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point);
sm_status_t sm_venc_destory(sm_venc_handle_t handle);
sm_status_t sm_venc_encode(sm_venc_handle_t handle, sm_frame_info_t *p_frame, uint32_t force_idr);

sm_status_t sm_venc_set_property(sm_venc_handle_t handle, sm_key_value_t *p_property);
