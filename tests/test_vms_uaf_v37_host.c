#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_uaf_v37.h"
#include "vms_validate.h"

#define TEST(test) do {if(!(test)){fprintf(stderr,"VMS37 UAF line %u\n",(unsigned)__LINE__);return 1;}}while(0)
int main(void) {
    uint8_t data[VMS37_UAF_FIXED]={0};
    struct port_uaf_record rec;
    memcpy(data+VMS37_UAF_USERNAME,"SYSTEM      ",12);
    memcpy(data+VMS37_UAF_ACCOUNT,"SYSTEM  ",8);
    memcpy(data+VMS37_UAF_HASH,"\x5c\x24\x9e\x3a\0\0\0\0",8);
    data[VMS37_UAF_DIRECTORY]=8;
    memcpy(data+VMS37_UAF_DIRECTORY+1,"[SYSMGR]",8);
    data[VMS37_UAF_DEVICE]=12;
    memcpy(data+VMS37_UAF_DEVICE+1,"SYS$SYSROOT:",12);
    memcpy(data+VMS37_UAF_CLI,"DCL      ",9);
    data[VMS37_UAF_ENCRYPT]=0;
    data[VMS37_UAF_FLAGS]=VMS37_UAF_FLAGS_CAPTIVE|
              VMS37_UAF_FLAGS_DISWELCOM|VMS37_UAF_FLAGS_DISMAIL|
              VMS37_UAF_FLAGS_DISACNT;
    data[VMS37_UAF_SALT]=0x34;
    data[VMS37_UAF_SALT+1]=0x12;
    TEST(vms37_uaf_decode(&rec,data,sizeof data));
    TEST(!memcmp(rec.username,"SYSTEM      ",12));
    TEST(!memcmp(rec.hash,"\x5c\x24\x9e\x3a\0\0\0\0",8));
    TEST(rec.algorithm==0 && rec.salt_lo==0x34 && rec.salt_hi==0x12);
    TEST(rec.reserved[0]==15);
    TEST(!memcmp(rec.reserved+8,data,sizeof data));
    TEST(!vms37_uaf_decode(&rec,data,sizeof data-1));
    TEST(!vms37_uaf_decode(0,data,sizeof data));
    TEST(!vms37_uaf_decode(&rec,0,sizeof data));
    puts("VMS 3.7 original UAF fields, flags, retained data and bounds PASS");
    return 0;
}
