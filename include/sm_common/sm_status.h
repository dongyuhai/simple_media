#pragma once
#include <stdio.h>

typedef enum sm_status
{
    SM_STATUS_SUCCESS,								///<Unknown
    SM_STATUS_FAIL,
    SM_STATUS_NOT_SUPOORT,
    SM_STATUS_MALLOC,
    SM_STATUS_INVALID_PARAM,
}sm_status_t;