#include <stdlib.h>
#include <sm_codec/sm_venc.h>
void frame_callback(void * user_ptr, sm_frame_info_t *p_frame_info)
{
    printf("%d\n",p_frame_info->frame_len);
}
void test_venc()
{
    uint8_t *p_picture_buf = (uint8_t*)malloc(1920 * 1080 * 2);
    sm_venc_param_t param;
    param.width = 1920;
    param.height = 1080;
    param.code_type = SM_CODE_TYPE_H264;
    param.pix_fmt = SM_PIX_FMT_NV12;
    sm_venc_handle_t handle = sm_venc_create(NULL, &param, frame_callback, NULL);
    while (1) {
        sm_picture_info_t picture;
        picture.p_plane[0] = p_picture_buf;
        picture.p_plane[1] = p_picture_buf + param.width*param.height;
        picture.plane_stride[0] = param.width;
        picture.plane_stride[1] = param.width;
        sm_venc_encode(handle, &picture, 0);
    }
    sm_venc_destory(handle);
}