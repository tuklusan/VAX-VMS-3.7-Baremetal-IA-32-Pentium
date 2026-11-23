#include "vms_init_mfd.h"

/* Source: INIT/SRC/INIMFD.B32. The order and file IDs are fixed by
 * INITIAL_MFD1 and INITIAL_MFD, not by a native PC directory layout. */
struct initial_entry {
    const char *name;
    uint16_t number;
};
static const struct initial_entry mfd_level1[] = {
    {"INDEXF   SYS", 1}, {"BITMAP   SYS", 2}, {"BADBLK   SYS", 3},
    {"000000   DIR", 4}, {"CORIMG   SYS", 5}
};
static const struct initial_entry mfd_level2[] = {
    {"000000.DIR", 4}, {"BACKUP.SYS", 8}, {"BADBLK.SYS", 3},
    {"BADLOG.SYS", 9}, {"BITMAP.SYS", 2}, {"CONTIN.SYS", 7},
    {"CORIMG.SYS", 5}, {"INDEXF.SYS", 1}, {"VOLSET.SYS", 6}
};
static void word(uint8_t *b, uint16_t value) {
    b[0]=(uint8_t)value;
    b[1]=(uint8_t)(value>>8);
}
static uint16_t rad50(const char *p) {
    static const char chars[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789";
    uint16_t value=0;
    for(unsigned i=0;i<3u;i++) {
        unsigned code=0;
        for(unsigned n=1u;n<40u;n++)if(p[i]==chars[n]) {code=n;break;}
        value=(uint16_t)(value*40u+code);
    }
    return value;
}
uint32_t port_init_mfd(uint32_t mfd_lbn, unsigned structure1,
                       uint8_t b[512], port_init_write_block write_block,
                       void *context) {
    if(!b || !write_block)return PORT_INIT_MFD_BAD_ARGUMENT;
    for(unsigned i=0;i<512u;i++)b[i]=0;
    unsigned used=0;
    if(structure1){
        for(unsigned i=0;i<sizeof(mfd_level1)/sizeof(mfd_level1[0]);i++){
            const struct initial_entry *entry=&mfd_level1[i];
            word(b+used,entry->number);
            word(b+used+2u,entry->number);
            word(b+used+4u,0u);
            for(unsigned j=0;j<4u;j++)
                word(b+used+6u+j*2u,rad50(entry->name+j*3u));
            word(b+used+14u,1u);
            used+=16u;
        }
    }else{
        for(unsigned i=0;i<sizeof(mfd_level2)/sizeof(mfd_level2[0]);i++){
            const struct initial_entry *entry=&mfd_level2[i];
            word(b+used,22u);
            word(b+used+2u,1u);
            b[used+4u]=0u;
            b[used+5u]=10u;
            for(unsigned j=0;j<10u;j++)b[used+6u+j]=(uint8_t)entry->name[j];
            word(b+used+16u,1u);
            word(b+used+18u,entry->number);
            word(b+used+20u,entry->number);
            word(b+used+22u,0u);
            used+=24u;
        }
        word(b+used,0xffffu);
    }
    return write_block(context,mfd_lbn,b);
}
