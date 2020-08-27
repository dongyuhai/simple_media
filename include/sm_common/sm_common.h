#include "sm_status.h"
#include "sm_type.h"

#define SM_ALIGN32(X) (((uint32_t)((X)+31)) & (~ (uint32_t)31))
#define SM_ALIGN16(X) (((uint32_t)((X)+15)) & (~ (uint32_t)15))

typedef struct sm_key_value {
    char *p_key;
    char *p_value;
}sm_key_value_t;

