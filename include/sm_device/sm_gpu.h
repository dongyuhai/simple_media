#include <stdio.h>
#include "sm_common/sm_type.h"
#include "sm_common/sm_status.h"
typedef enum sm_gpu_platform
{
    SM_GPU_PLATFORM_UNKNOWN = 0,						///<Unknown hardware type, the binary value is 0000 0000
    SM_GPU_PLATFORM_AMD = 1,						///<AMD graphics, the binary value is 0000 0001
    SM_GPU_PLATFORM_INTEL = 2,						///<Intel graphics, the binary value is 0000 0010
    SM_GPU_PLATFORM_NVIDIA = 4,						///<Nvidia graphics, the binary value is 0000 0100
    SM_GPU_PLATFORM_COUNT								///<The maximum input value 
}sm_gpu_platform_t;

typedef struct sm_gpu_info {
    char gpu_name[128];									///gpu name
    sm_gpu_platform_t platform;						///gpu platform

}sm_gpu_info_t;

int32_t sm_get_gpu_num();
sm_status_t sm_get_gpu_info_by_index(int32_t index, sm_gpu_info_t *info);