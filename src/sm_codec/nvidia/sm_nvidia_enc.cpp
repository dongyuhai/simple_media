#include <stdlib.h>
#include <string.h>
#include "sm_codec/sm_venc.h"
#include "sm_common/sm_common.h"
#include "sm_common/sm_log.h"

typedef struct sm_venc_intel {

    SM_VFRAME_CALLBACK frame_callback;
    void *user_point;
    sm_vcodec_extdata_t extdata;
}sm_venc_intel_t;



HANDLE sm_venc_create_nvidia(int32_t index, sm_venc_param_t *p_param, SM_VFRAME_CALLBACK frame_callback, void *user_point)
{
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