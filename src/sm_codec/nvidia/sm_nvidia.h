#pragma once
#include "sm_codec/sm_codec_common.h"
HANDLE sm_venc_create_nvidia(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point);

sm_status_t sm_venc_encode_nvidia(HANDLE handle, sm_picture_info_t *p_picture, int32_t force_idr);

sm_status_t sm_venc_destory_nvidia(HANDLE handle);

sm_status_t sm_venc_set_property_nvidia(HANDLE handle, sm_key_value_t *p_property);