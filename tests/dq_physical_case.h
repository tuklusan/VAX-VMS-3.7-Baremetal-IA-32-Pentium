#ifndef VMS37_M136_PHYS_TEST_H
#define VMS37_M136_PHYS_TEST_H
#include <stdint.h>
int m136_physical_case(uint32_t lbn,uint32_t requested,uint32_t expected,
                       int should_read);
#endif
