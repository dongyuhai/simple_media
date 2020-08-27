#include <stdio.h>
#include "sm_common/sm_type.h"
#include "sm_device/sm_gpu.h"
#include "sm_common/sm_common.h"
typedef enum sm_frame_type
{
    SM_FRAME_TYPE_UNKNOWN,							///<Unknown frame
    SM_FRAME_TYPE_IDR,								///<IDR frame
    SM_FRAME_TYPE_I,								///<I-frame
    SM_FRAME_TYPE_P,								///<P-frame
    SM_FRAME_TYPE_B,								///<B-frame
    SM_FRAME_TYPE_COUNT							///<Number of frame types enumerated
}sm_frame_type_t;

typedef enum sm_code_type
{
    SM_CODE_TYPE_UNKNOWN,							///<Unknown

    SM_CODE_TYPE_AVC,								///<AVC/H264
    SM_CODE_TYPE_H264 = SM_CODE_TYPE_AVC,		///<H264
    SM_CODE_TYPE_HEVC,								///<HEVC/H265
    SM_CODE_TYPE_H265 = SM_CODE_TYPE_HEVC,	///<H265

    SM_CODE_TYPE_AAC,
    SM_CODE_TYPE_MP3,
    SM_CODE_TYPE_G711A,
    SM_CODE_TYPE_G711U,
    SM_CODE_TYPE_COUNT								///<The maximum input value 
}sm_code_type_t;

typedef enum sm_rate_control_mode
{
    SM_RATECONTROL_UNKNOWN,						///<Unknown
    SM_RATECONTROL_CBR,							///<Constant Bit Rate
    SM_RATECONTROL_VBR,							///<Variable Bit Rate
    SM_RATECONTROL_CQP,							///<Constant Quantization Parameter
    SM_RATECONTROL_COUNT							///<The maximum input value 
}sm_rate_control_mode_t;

typedef enum sm_venc_profile
{
    SM_VENC_PROFILE_UNKNOWN,							///<Unknown
    SM_VENC_PROFILE_H264_BASELINE,						///<H264 baseline
    SM_VENC_PROFILE_H264_MAIN,							///<H264 main
    SM_VENC_PROFILE_H264_HIGH,							///<H264 high
    SM_VENC_PROFILE_H265_MAIN,							///<H265 main
    SM_VENC_PROFILE_H265_MAIN10,							///<H265 main
    SM_VENC_PROFILE_COUNT								///<The maximum input value 
}sm_venc_profile_t;


typedef enum sm_video_pix_fmt
{
    SM_VIDEO_PIX_FMT_UNKNOWN,				
    SM_VIDEO_PIX_FMT_NV12,
    SM_VIDEO_PIX_FMT_NV21,
    SM_VIDEO_PIX_FMT_YV12,
    SM_VIDEO_PIX_FMT_I420,
    SM_VIDEO_PIX_FMT_YUY2,
    SM_VIDEO_PIX_FMT_P010,
    SM_VIDEO_PIX_FMT_BGRA,
    SM_VIDEO_PIX_FMT_RGBA,
    SM_VIDEO_PIX_FMT_ARGB,
    SM_VIDEO_PIX_FMT_ABGR,
    SM_VIDEO_PIX_FMT_COUNT
}sm_video_pix_fmt_t;

typedef struct sm_frame_info
{
    uint8_t *p_plane[4];
    uint32_t len;
    int64_t pts;
}sm_frame_info_t;

typedef struct sm_packet_info
{
    uint8_t *p_frame;
    uint32_t frame_len;
    uint8_t **pp_nalu;
    uint32_t *nalu_len;
    sm_frame_type_t frame_type;
    int64_t pts;
}sm_packet_info_t;

typedef void(*SM_VFRAME_CALLBACK)(void * user_ptr, sm_packet_info_t *p_frame_info);

typedef struct sm_venc_rate_control
{
    sm_rate_control_mode_t mode;					///<Bitrate controlling methods
    union {
        struct {
            uint32_t target_bitrate;					///<Target bitrate: only valid when the bitrate is variable or constant.
            uint32_t max_bitrate;						///<The maximun bitrate: only valid when the bitrate is variable.
        };
        struct {
            uint8_t qpi;								///<I-Frame QP
            uint8_t qpb;								///<B-Frame QP
            uint8_t qpp;								///<P-Frame QP
            uint8_t reserved;							///<Reserved      
        };
    };
}sm_venc_rate_control_t;



typedef struct sm_venc_extdata {
    uint8_t *p_extdata;									///<Extended data pointer. The data includes vps(Video Parameter Set),sps(Sequence Parameter Set) and pps(Picture Parameter Set).
    uint32_t extdata_len;								///<The total length of entended data
    uint32_t len[4];									///<len[0] vps_len;len[1] sps_len;len[2] pps_len
}sm_venc_extdata_t;

typedef enum sm_venc_color_primaries {
    SM_VENC_COLOR_PRI_RESERVED0 = 0,
    SM_VENC_COLOR_PRI_BT709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
    SM_VENC_COLOR_PRI_UNSPECIFIED = 2,
    SM_VENC_COLOR_PRI_RESERVED = 3,
    SM_VENC_COLOR_PRI_BT470M = 4,  ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)

    SM_VENC_COLOR_PRI_BT470BG = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
    SM_VENC_COLOR_PRI_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    SM_VENC_COLOR_PRI_SMPTE240M = 7,  ///< functionally identical to above
    SM_VENC_COLOR_PRI_FILM = 8,  ///< colour filters using Illuminant C
    SM_VENC_COLOR_PRI_BT2020 = 9,  ///< ITU-R BT2020
    SM_VENC_COLOR_PRI_SMPTE428 = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
    SM_VENC_COLOR_PRI_SMPTEST428_1 = SM_VENC_COLOR_PRI_SMPTE428,
    SM_VENC_COLOR_PRI_SMPTE431 = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
    SM_VENC_COLOR_PRI_SMPTE432 = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
    SM_VENC_COLOR_PRI_JEDEC_P22 = 22, ///< JEDEC P22 phosphors
    SM_VENC_COLOR_PRI_COUNT                ///< Not part of ABI
}sm_venc_color_primaries_t;

typedef enum sm_venc_color_transfer_characteristic {
    SM_VENC_COLOR_TRC_RESERVED0 = 0,
    SM_VENC_COLOR_TRC_BT709 = 1,  ///< also ITU-R BT1361
    SM_VENC_COLOR_TRC_UNSPECIFIED = 2,
    SM_VENC_COLOR_TRC_RESERVED = 3,
    SM_VENC_COLOR_TRC_GAMMA22 = 4,  ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    SM_VENC_COLOR_TRC_GAMMA28 = 5,  ///< also ITU-R BT470BG
    SM_VENC_COLOR_TRC_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
    SM_VENC_COLOR_TRC_SMPTE240M = 7,
    SM_VENC_COLOR_TRC_LINEAR = 8,  ///< "Linear transfer characteristics"
    SM_VENC_COLOR_TRC_LOG = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
    SM_VENC_COLOR_TRC_LOG_SQRT = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    SM_VENC_COLOR_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
    SM_VENC_COLOR_TRC_BT1361_ECG = 12, ///< ITU-R BT1361 Extended Colour Gamut
    SM_VENC_COLOR_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
    SM_VENC_COLOR_TRC_BT2020_10 = 14, ///< ITU-R BT2020 for 10-bit system
    SM_VENC_COLOR_TRC_BT2020_12 = 15, ///< ITU-R BT2020 for 12-bit system
    SM_VENC_COLOR_TRC_SMPTE2084 = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
    SM_VENC_COLOR_TRC_SMPTEST2084 = SM_VENC_COLOR_TRC_SMPTE2084,
    SM_VENC_COLOR_TRC_SMPTE428 = 17, ///< SMPTE ST 428-1
    SM_VENC_COLOR_TRC_SMPTEST428_1 = SM_VENC_COLOR_TRC_SMPTE428,
    SM_VENC_COLOR_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
    SM_VENC_COLOR_TRC_COUNT                 ///< Not part of ABI
}sm_venc_color_transfer_characteristic_t;

/**
* @ingroup group_hwe_variables_enum
* @brief mw_venc_color_space_t
* YUV colorspace type.
* These values match the ones defined by ISO/IEC 23001-8_2013 7.3.
*/
typedef enum sm_venc_color_space {
    SM_VENC_COLOR_SPACE_RGB = 0,  ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
    SM_VENC_COLOR_SPACE_BT709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    SM_VENC_COLOR_SPACE_UNSPECIFIED = 2,
    SM_VENC_COLOR_SPACE_RESERVED = 3,
    SM_VENC_COLOR_SPACE_FCC = 4,  ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
    SM_VENC_COLOR_SPACE_BT470BG = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    SM_VENC_COLOR_SPACE_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    SM_VENC_COLOR_SPACE_SMPTE240M = 7,  ///< functionally identical to above
    SM_VENC_COLOR_SPACE_YCGCO = 8,  ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    SM_VENC_COLOR_SPACE_YCOCG = SM_VENC_COLOR_SPACE_YCGCO,
    SM_VENC_COLOR_SPACE_BT2020_NCL = 9,  ///< ITU-R BT2020 non-constant luminance system
    SM_VENC_COLOR_SPACE_BT2020_CL = 10, ///< ITU-R BT2020 constant luminance system
    SM_VENC_COLOR_SPACE_SMPTE2085 = 11, ///< SMPTE 2085, Y'D'zD'x
    SM_VENC_COLOR_SPACE_CHROMA_DERIVED_NCL = 12, ///< Chromaticity-derived non-constant luminance system
    SM_VENC_COLOR_SPACE_CHROMA_DERIVED_CL = 13, ///< Chromaticity-derived constant luminance system
    SM_VENC_COLOR_SPACE_ICTCP = 14, ///< ITU-R BT.2100-0, ICtCp
    SM_VENC_COLOR_SPACE_COUNT                ///< Not part of ABI
}sm_venc_color_space_t;

typedef enum sm_venc_pic_struct {
    SM_VENC_PICSTRUCT_PROGRESSIVE = 0x00,
    SM_VENC_PICSTRUCT_FIELD_TFF = 0x01,
    SM_VENC_PICSTRUCT_FIELD_BFF = 0x02,
    SM_VENC_PICSTRUCT_COUNT
}sm_venc_pic_struct_t;


typedef struct sm_venc_param {
    sm_code_type_t code_type;						///<Code type, H264 or H265 
    sm_video_pix_fmt_t fourcc;							///<Color format of input data 
    sm_venc_rate_control_t rate_control;				///<Frame control 
    int32_t width;										///<width of input video
    int32_t height;										///<Height of input video
    sm_fraction_t fps;									///<Frame rate 
    int32_t slice_num;									///<Slice number 
    int32_t gop_pic_size;								///<GOP size
    int32_t gop_ref_size;								///<Referenced GOP size 
    sm_venc_profile_t profile;							///<Profile
    int32_t level;								///<Level
    int32_t intel_async_depth;
    int32_t yuv_is_full_range;
    sm_venc_color_primaries_t color_primaries;
    sm_venc_color_transfer_characteristic_t color_trc;
    sm_venc_color_space_t color_space;
    sm_venc_pic_struct_t pic_struct;
    sm_key_value_t *p_prv_param;
}sm_venc_param_t;