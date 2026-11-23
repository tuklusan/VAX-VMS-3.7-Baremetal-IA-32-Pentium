#include <stdio.h>
#include "vms_v37_dyn_tags.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"VMS 3.7 DYN check failed at line %d: %s\n",__LINE__,#x);return 1;}} while(0)
static const unsigned source_order[] = {
    PORT_V37_DYN_ADP, /* source ordinal 1 */
    PORT_V37_DYN_ACB, /* source ordinal 2 */
    PORT_V37_DYN_AQB, /* source ordinal 3 */
    PORT_V37_DYN_CEB, /* source ordinal 4 */
    PORT_V37_DYN_CRB, /* source ordinal 5 */
    PORT_V37_DYN_DDB, /* source ordinal 6 */
    PORT_V37_DYN_FCB, /* source ordinal 7 */
    PORT_V37_DYN_FRK, /* source ordinal 8 */
    PORT_V37_DYN_IDB, /* source ordinal 9 */
    PORT_V37_DYN_IRP, /* source ordinal 10 */
    PORT_V37_DYN_LOG, /* source ordinal 11 */
    PORT_V37_DYN_PCB, /* source ordinal 12 */
    PORT_V37_DYN_PQB, /* source ordinal 13 */
    PORT_V37_DYN_RVT, /* source ordinal 14 */
    PORT_V37_DYN_TQE, /* source ordinal 15 */
    PORT_V37_DYN_UCB, /* source ordinal 16 */
    PORT_V37_DYN_VCB, /* source ordinal 17 */
    PORT_V37_DYN_WCB, /* source ordinal 18 */
    PORT_V37_DYN_BUFIO, /* source ordinal 19 */
    PORT_V37_DYN_TYPAHD, /* source ordinal 20 */
    PORT_V37_DYN_GSD, /* source ordinal 21 */
    PORT_V37_DYN_MVL, /* source ordinal 22 */
    PORT_V37_DYN_NET, /* source ordinal 23 */
    PORT_V37_DYN_KFI, /* source ordinal 24 */
    PORT_V37_DYN_MTL, /* source ordinal 25 */
    PORT_V37_DYN_BRDCST, /* source ordinal 26 */
    PORT_V37_DYN_CXB, /* source ordinal 27 */
    PORT_V37_DYN_NDB, /* source ordinal 28 */
    PORT_V37_DYN_SSB, /* source ordinal 29 */
    PORT_V37_DYN_DPT, /* source ordinal 30 */
    PORT_V37_DYN_JPB, /* source ordinal 31 */
    PORT_V37_DYN_PBH, /* source ordinal 32 */
    PORT_V37_DYN_PDB, /* source ordinal 33 */
    PORT_V37_DYN_PIB, /* source ordinal 34 */
    PORT_V37_DYN_PFL, /* source ordinal 35 */
    PORT_V37_DYN_UNUSED, /* source ordinal 36 */
    PORT_V37_DYN_PTR, /* source ordinal 37 */
    PORT_V37_DYN_KFH, /* source ordinal 38 */
    PORT_V37_DYN_RVX, /* source ordinal 39 */
    PORT_V37_DYN_EXTGSD, /* source ordinal 40 */
    PORT_V37_DYN_SHMGSD, /* source ordinal 41 */
    PORT_V37_DYN_SHB, /* source ordinal 42 */
    PORT_V37_DYN_MBX, /* source ordinal 43 */
    PORT_V37_DYN_IRPE, /* source ordinal 44 */
    PORT_V37_DYN_SLAVCEB, /* source ordinal 45 */
    PORT_V37_DYN_SHMCEB, /* source ordinal 46 */
    PORT_V37_DYN_JIB, /* source ordinal 47 */
    PORT_V37_DYN_TWP, /* source ordinal 48 */
    PORT_V37_DYN_RBM, /* source ordinal 49 */
    PORT_V37_DYN_VCA, /* source ordinal 50 */
    PORT_V37_DYN_BCB, /* source ordinal 51 */
    PORT_V37_DYN_CDB, /* source ordinal 52 */
    PORT_V37_DYN_LPD, /* source ordinal 53 */
    PORT_V37_DYN_JMT, /* source ordinal 54 */
    PORT_V37_DYN_LKB, /* source ordinal 55 */
    PORT_V37_DYN_RSB, /* source ordinal 56 */
    PORT_V37_DYN_LKID, /* source ordinal 57 */
    PORT_V37_DYN_RSHT, /* source ordinal 58 */
    PORT_V37_DYN_CDRP, /* source ordinal 59 */
    PORT_V37_DYN_RUL, /* source ordinal 60 */
    PORT_V37_DYN_ERP, /* source ordinal 61 */
    PORT_V37_DYN_CIDG, /* source ordinal 62 */
    PORT_V37_DYN_CIMSG, /* source ordinal 63 */
};
int main(void) {
    CHECK(sizeof source_order / sizeof source_order[0]==PORT_V37_DYN_COUNT);
    for(unsigned i=0;i<PORT_V37_DYN_COUNT;i++)CHECK(source_order[i]==i+1u);
    CHECK(PORT_V37_DYN_LOG==11u);
    CHECK(PORT_V37_DYN_DPT==30u);
    CHECK(PORT_V37_DYN_DDB==6u && PORT_V37_DYN_UCB==16u);
    CHECK(PORT_V37_DYN_WCB==18u && PORT_V37_DYN_IRP==10u);
    CHECK(PORT_V37_DPT_VERSION==3u);
    puts("SHRLIB/SYSDEF.MDL original ordered 63 DYN tags and DPT version PASS");
    return 0;
}
