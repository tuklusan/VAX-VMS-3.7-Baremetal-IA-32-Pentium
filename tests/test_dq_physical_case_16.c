#include <stdint.h>
#include "dq_physical_case.h"
int test_m136_case_16(void){
    return m136_physical_case(UINT32_C(0x100),UINT32_C(0x100),1u,1);
}
