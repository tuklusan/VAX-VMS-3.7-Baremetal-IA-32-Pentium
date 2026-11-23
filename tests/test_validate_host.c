#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_validate.h"
static int failures;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"VALIDATE test at %d failed\n",__LINE__); failures++; } } while (0)

static int read_sector(void *ctx,uint32_t lba,uint16_t out[256]) {
    FILE *f=ctx;
    uint8_t bytes[512];
    if (fseek(f,(long)lba*512,SEEK_SET) || fread(bytes,1,512,f)!=512) return -1;
    for (unsigned i=0;i<256;++i)
        out[i]=(uint16_t)(bytes[2*i]|(uint16_t)bytes[2*i+1]<<8);
    return 0;
}

struct fake {
    struct port_uaf_service *service;
    struct port_uaf_record sample;
    int opens,connects,gets,disconnects,closes,delays,delay_total;
    int fail_shared,locked,forced_error;
};
static int fake_open(void *ctx,int shared,int system_only) {
    struct fake *f=ctx;
    CHECK(system_only==1);
    CHECK(f->service->privileges==0x77fu);
    ++f->opens;
    if (shared && f->fail_shared) return f->fail_shared;
    return PORT_UAF_NORMAL;
}
static int fake_connect(void *ctx) {
    struct fake *f=ctx;
    ++f->connects;
    return PORT_UAF_NORMAL;
}
static int fake_get(void *ctx,const uint8_t key[12],void *out,uint16_t size) {
    struct fake *f=ctx;
    (void)key;
    CHECK(size>=512);
    ++f->gets;
    if (f->forced_error) return f->forced_error;
    if (f->locked-- > 0) return PORT_UAF_RLK;
    memcpy(out,&f->sample,sizeof f->sample);
    return PORT_UAF_NORMAL;
}
static void fake_disconnect(void *ctx) { ++((struct fake*)ctx)->disconnects; }
static void fake_close(void *ctx) { ++((struct fake*)ctx)->closes; }
static void fake_delay(void *ctx,unsigned ms) {
    struct fake *f=ctx;
    ++f->delays;
    f->delay_total+=(int)ms;
}
static void fake_setup(struct fake *f,struct port_uaf_service *s) {
    memset(f,0,sizeof *f);
    memset(s,0,sizeof *s);
    f->service=s;
    memcpy(f->sample.username,"LABUSER     ",12);
    memcpy(f->sample.hash,"\x66\x25\x0e\x0c\0\0\0\0",8);
    s->context=f;
    s->privileges=0x37u;
    s->allowed_privileges=0x748u;
    s->ops.open=fake_open;
    s->ops.connect=fake_connect;
    s->ops.get=fake_get;
    s->ops.disconnect=fake_disconnect;
    s->ops.close=fake_close;
    s->ops.delay_ms=fake_delay;
}

int main(void) {
    FILE *image=fopen("portdisk.img","rb");
    CHECK(image!=NULL);
    if (!image) return 1;
    CHECK(port_uaf_guest_selftest(read_sector,image)==0);
    struct port_uaf_disk disk;
    port_uaf_disk_init(&disk,read_sector,image,3);
    struct port_uaf_service svc=port_uaf_disk_service(&disk);
    struct port_uaf_record rec;
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"labuser",7,&rec,sizeof rec)==PORT_UAF_NORMAL);
    CHECK(!memcmp(rec.username,"LABUSER     ",12));
    CHECK(port_uaf_validate(&svc,(const uint8_t*)"LABUSER",7,(const uint8_t*)"pASSword",8,&rec,sizeof rec)==PORT_UAF_NORMAL);
    CHECK(port_uaf_validate(&svc,(const uint8_t*)"LABUSER",7,(const uint8_t*)"wrong",5,&rec,sizeof rec)==PORT_UAF_INVPWD);
    CHECK(port_uaf_validate(&svc,(const uint8_t*)"PURDYUSR",8,(const uint8_t*)"password",8,&rec,sizeof rec)==PORT_UAF_NORMAL);
    CHECK(port_uaf_validate(&svc,(const uint8_t*)"DEFAULT",7,(const uint8_t*)"PASSWORD",8,&rec,sizeof rec)==PORT_UAF_INVUSR);
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"absent",6,&rec,sizeof rec)==PORT_UAF_INVUSR);
    CHECK(!memcmp(rec.username,"DEFAULT     ",12));
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"LABUSER",7,&rec,32)==PORT_UAF_BAD_ARG);
    port_uaf_disk_init(&disk,read_sector,image,1000);
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"LABUSER",7,&rec,sizeof rec)==PORT_UAF_IO);
    fclose(image);

    struct fake f;
    fake_setup(&f,&svc);
    f.fail_shared=PORT_UAF_SNE;
    f.locked=2;
    CHECK(port_uaf_validate(&svc,(const uint8_t*)"LABUSER",7,(const uint8_t*)"PASSWORD",8,&rec,sizeof rec)==PORT_UAF_NORMAL);
    CHECK(f.opens==2 && f.connects==1 && f.gets==3);
    CHECK(f.delays==2 && f.delay_total==150);
    CHECK(f.disconnects==1 && f.closes==1 && svc.privileges==0x37u);
    fake_setup(&f,&svc);
    f.locked=5;
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"LABUSER",7,&rec,sizeof rec)==PORT_UAF_RLK);
    CHECK(f.gets==3 && f.delays==2 && f.disconnects==1 && f.closes==1);
    fake_setup(&f,&svc);
    f.forced_error=PORT_UAF_IO;
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"LABUSER",7,&rec,sizeof rec)==PORT_UAF_IO);
    CHECK(f.disconnects==1 && f.closes==1 && svc.privileges==0x37u);
    fake_setup(&f,&svc);
    f.fail_shared=PORT_UAF_DME;
    CHECK(port_uaf_searchuser(&svc,(const uint8_t*)"LABUSER",7,&rec,sizeof rec)==PORT_UAF_NORMAL);
    CHECK(f.opens==2);
    if (failures) return 1;
    puts("PASS: LOGIN VALIDATE indexed account records, password, fallback, retry, privileges, close");
    return 0;
}
