#pragma once
#include <stdio.h>
#include "sm_common/sm_type.h"
#include "sm_device/sm_gpu.h"
#include "sm_common/sm_common.h"
typedef enum sm_frame_type
{
    SM_FRAME_TYPE_UNKNOWN,
    SM_FRAME_TYPE_IDR,
    SM_FRAME_TYPE_I,
    SM_FRAME_TYPE_P,
    SM_FRAME_TYPE_B,
    SM_FRAME_TYPE_COUNT
}sm_frame_type_t;

typedef enum sm_code_type
{
    SM_CODE_TYPE_UNKNOWN,

    SM_CODE_TYPE_AVC,
    SM_CODE_TYPE_H264 = SM_CODE_TYPE_AVC,
    SM_CODE_TYPE_HEVC,
    SM_CODE_TYPE_H265 = SM_CODE_TYPE_HEVC,

    SM_CODE_TYPE_AAC,
    SM_CODE_TYPE_MP3,
    SM_CODE_TYPE_G711A,
    SM_CODE_TYPE_G711U,
    SM_CODE_TYPE_COUNT
}sm_code_type_t;

typedef enum sm_rate_control_mode
{
    SM_RATECONTROL_UNKNOWN,
    SM_RATECONTROL_CBR,
    SM_RATECONTROL_VBR,
    SM_RATECONTROL_CQP,
    SM_RATECONTROL_COUNT
}sm_rate_control_mode_t;

typedef enum sm_vcodec_profile
{
    SM_VCODEC_PROFILE_UNKNOWN,
    SM_VCODEC_PROFILE_H264_BASELINE,
    SM_VCODEC_PROFILE_H264_MAIN,
    SM_VCODEC_PROFILE_H264_HIGH,
    SM_VCODEC_PROFILE_H265_MAIN,
    SM_VCODEC_PROFILE_H265_MAIN10,
    SM_VCODEC_PROFILE_COUNT
}sm_vcodec_profile_t;


typedef enum sm_pix_fmt
{
    SM_PIX_FMT_UNKNOWN,				
    SM_PIX_FMT_NV12,
    SM_PIX_FMT_NV21,
    SM_PIX_FMT_YV12,
    SM_PIX_FMT_I420,
    SM_PIX_FMT_YUY2,
    SM_PIX_FMT_P010,
    SM_PIX_FMT_BGRA,
    SM_PIX_FMT_RGBA,
    SM_PIX_FMT_ARGB,
    SM_PIX_FMT_ABGR,
    SM_PIX_FMT_COUNT
}sm_pix_fmt_t;

typedef struct sm_picture_info
{
    uint8_t *p_plane[4];
    int32_t plane_stride[4];
    uint32_t len;
    uint64_t pts;
}sm_picture_info_t;

typedef struct sm_frame_info
{
    uint8_t *p_frame;
    uint32_t frame_len;
    uint8_t **pp_nalu;
    uint32_t *nalu_len;
    sm_frame_type_t frame_type;
    uint64_t pts;
}sm_frame_info_t;

typedef void(*SM_VFRAME_CALLBACK)(void * user_ptr, sm_frame_info_t *p_frame_info);

typedef struct sm_vcodec_rate_control
{
    sm_rate_control_mode_t mode;
    union {
        struct {
            uint32_t target_bitrate_kbps;
            uint32_t max_bitrate_kbps;
        };
        struct {
            uint8_t qpi;
            uint8_t qpb;
            uint8_t qpp;
            uint8_t reserved;  
        };
    };
}sm_vcodec_rate_control_t;



typedef struct sm_vcodec_extdata {
    uint8_t *p_extdata;
    uint32_t extdata_len;		
    uint32_t len[4];
}sm_vcodec_extdata_t;

typedef enum sm_venc_preset
{
    SM_VENC_PRESET_UNKNOWN,	
    SM_VENC_PRESET_BEST_QUALITY,
    SM_VENC_PRESET_BALANCED,
    SM_VENC_PRESET_BEST_SPEED,
    SM_VENC_PRESET_COUNT
}sm_venc_preset_t;

typedef enum sm_vcodec_color_primaries {
    SM_VCODEC_COLOR_PRI_RESERVED0 = 0,
    SM_VCODEC_COLOR_PRI_BT709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
    SM_VCODEC_COLOR_PRI_UNSPECIFIED = 2,
    SM_VCODEC_COLOR_PRI_RESERVED = 3,
    SM_VCODEC_COLOR_PRI_BT470M = 4,  ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)

    SM_VCODEC_COLOR_PRI_BT470BG = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
    SM_VCODEC_COLOR_PRI_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    SM_VCODEC_COLOR_PRI_SMPTE240M = 7,  ///< functionally identical to above
    SM_VCODEC_COLOR_PRI_FILM = 8,  ///< colour filters using Illuminant C
    SM_VCODEC_COLOR_PRI_BT2020 = 9,  ///< ITU-R BT2020
    SM_VCODEC_COLOR_PRI_SMPTE428 = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
    SM_VCODEC_COLOR_PRI_SMPTEST428_1 = SM_VCODEC_COLOR_PRI_SMPTE428,
    SM_VCODEC_COLOR_PRI_SMPTE431 = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
    SM_VCODEC_COLOR_PRI_SMPTE432 = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
    SM_VCODEC_COLOR_PRI_JEDEC_P22 = 22, ///< JEDEC P22 phosphors
    SM_VCODEC_COLOR_PRI_COUNT                ///< Not part of ABI
}sm_vcodec_color_primaries_t;

typedef enum sm_vcodec_color_transfer_characteristic {
    SM_VCODEC_COLOR_TRC_RESERVED0 = 0,
    SM_VCODEC_COLOR_TRC_BT709 = 1,  ///< also ITU-R BT1361
    SM_VCODEC_COLOR_TRC_UNSPECIFIED = 2,
    SM_VCODEC_COLOR_TRC_RESERVED = 3,
    SM_VCODEC_COLOR_TRC_GAMMA22 = 4,  ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    SM_VCODEC_COLOR_TRC_GAMMA28 = 5,  ///< also ITU-R BT470BG
    SM_VCODEC_COLOR_TRC_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
    SM_VCODEC_COLOR_TRC_SMPTE240M = 7,
    SM_VCODEC_COLOR_TRC_LINEAR = 8,  ///< "Linear transfer characteristics"
    SM_VCODEC_COLOR_TRC_LOG = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
    SM_VCODEC_COLOR_TRC_LOG_SQRT = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    SM_VCODEC_COLOR_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
    SM_VCODEC_COLOR_TRC_BT1361_ECG = 12, ///< ITU-R BT1361 Extended Colour Gamut
    SM_VCODEC_COLOR_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
    SM_VCODEC_COLOR_TRC_BT2020_10 = 14, ///< ITU-R BT2020 for 10-bit system
    SM_VCODEC_COLOR_TRC_BT2020_12 = 15, ///< ITU-R BT2020 for 12-bit system
    SM_VCODEC_COLOR_TRC_SMPTE2084 = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
    SM_VCODEC_COLOR_TRC_SMPTEST2084 = SM_VCODEC_COLOR_TRC_SMPTE2084,
    SM_VCODEC_COLOR_TRC_SMPTE428 = 17, ///< SMPTE ST 428-1
    SM_VCODEC_COLOR_TRC_SMPTEST428_1 = SM_VCODEC_COLOR_TRC_SMPTE428,
    SM_VCODEC_COLOR_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
    SM_VCODEC_COLOR_TRC_COUNT                 ///< Not part of ABI
}sm_vcodec_color_transfer_characteristic_t;

typedef enum sm_vcodec_color_space {
    SM_VCODEC_COLOR_SPACE_RGB = 0,  ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
    SM_VCODEC_COLOR_SPACE_BT709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    SM_VCODEC_COLOR_SPACE_UNSPECIFIED = 2,
    SM_VCODEC_COLOR_SPACE_RESERVED = 3,
    SM_VCODEC_COLOR_SPACE_FCC = 4,  ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
    SM_VCODEC_COLOR_SPACE_BT470BG = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    SM_VCODEC_COLOR_SPACE_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    SM_VCODEC_COLOR_SPACE_SMPTE240M = 7,  ///< functionally identical to above
    SM_VCODEC_COLOR_SPACE_YCGCO = 8,  ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    SM_VCODEC_COLOR_SPACE_YCOCG = SM_VCODEC_COLOR_SPACE_YCGCO,
    SM_VCODEC_COLOR_SPACE_BT2020_NCL = 9,  ///< ITU-R BT2020 non-constant luminance system
    SM_VCODEC_COLOR_SPACE_BT2020_CL = 10, ///< ITU-R BT2020 constant luminance system
    SM_VCODEC_COLOR_SPACE_SMPTE2085 = 11, ///< SMPTE 2085, Y'D'zD'x
    SM_VCODEC_COLOR_SPACE_CHROMA_DERIVED_NCL = 12, ///< Chromaticity-derived non-constant luminance system
    SM_VCODEC_COLOR_SPACE_CHROMA_DERIVED_CL = 13, ///< Chromaticity-derived constant luminance system
    SM_VCODEC_COLOR_SPACE_ICTCP = 14, ///< ITU-R BT.2100-0, ICtCp
    SM_VCODEC_COLOR_SPACE_COUNT                ///< Not part of ABI
}sm_vcodec_color_space_t;

typedef enum sm_vcodec_pic_struct {
    SM_VCODEC_PICSTRUCT_PROGRESSIVE = 0x00,
    SM_VCODEC_PICSTRUCT_TFF = 0x01,
    SM_VCODEC_PICSTRUCT_BFF = 0x02,
    SM_VCODEC_PICSTRUCT_COUNT
}sm_vcodec_pic_struct_t;

typedef enum sm_vcodec_frame_field_mode {
    SM_VCODEC_FRAME_FILED_MODE_PROGRESSIVE = 0x00,
    SM_VCODEC_FRAME_FILED_MODE_FIELD = 0x01,
    SM_VCODEC_FRAME_FILED_MODE_MBAFF = 0x02,
    SM_VCODEC_FRAME_FILED_MODE_COUNT
}sm_vcodec_frame_field_mode_t;

typedef struct sm_venc_signal {
    int32_t yuv_is_full_range;
    int32_t have_set_color;
    sm_vcodec_color_primaries_t color_primaries;
    sm_vcodec_color_transfer_characteristic_t color_trc;
    sm_vcodec_color_space_t color_space;
}sm_venc_signal_t;

typedef struct sm_venc_param_ext {
    uint32_t slice_num;
    //uint32_t intel_async_depth;
    sm_venc_signal_t signal;
    sm_venc_preset_t preset;
    sm_vcodec_frame_field_mode_t frame_field_mode;
    sm_vcodec_pic_struct_t pic_struct;
}sm_venc_param_ext_t;

typedef struct sm_venc_param {
    sm_code_type_t code_type;
    sm_pix_fmt_t pix_fmt;
    sm_vcodec_rate_control_t rate_control;
    int32_t width;
    int32_t height;
    sm_fraction_t fps;
    uint32_t gop_size;
    uint32_t gop_ref_size;
    sm_vcodec_profile_t profile;
    uint32_t level;
    sm_venc_param_ext_t ext;
    sm_key_value_t *p_prv_param;
}sm_venc_param_t;
/**/