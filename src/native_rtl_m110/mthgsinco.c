/* Original: RTL/SRC/MTHGSINCO.MAR pinned source revision; first entry at line 335.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_mthgsinco(const uint16_t *a,uint16_t *s,uint16_t *c){return port110_sincos(PORT_M107_G,0,a,s,c);}
int port110_mthgsincod(const uint16_t *a,uint16_t *s,uint16_t *c){return port110_sincos(PORT_M107_G,1,a,s,c);}
int port110_mthgsinco_sin(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_G,0,a,out,spare);}
int port110_mthgsinco_cos(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_G,0,a,spare,out);}
int port110_mthgsinco_sind(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_G,1,a,out,spare);}
int port110_mthgsinco_cosd(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_G,1,a,spare,out);}
