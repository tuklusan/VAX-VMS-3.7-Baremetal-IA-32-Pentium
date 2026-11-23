/* Original: RTL/SRC/MTHDSINCO.MAR pinned source revision; first entry at line 356.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_mthdsinco(const uint16_t *a,uint16_t *s,uint16_t *c){return port110_sincos(PORT_M107_D,0,a,s,c);}
int port110_mthdsincod(const uint16_t *a,uint16_t *s,uint16_t *c){return port110_sincos(PORT_M107_D,1,a,s,c);}
int port110_mthdsinco_sin(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_D,0,a,out,spare);}
int port110_mthdsinco_cos(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_D,0,a,spare,out);}
int port110_mthdsinco_sind(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_D,1,a,out,spare);}
int port110_mthdsinco_cosd(const uint16_t *a,uint16_t *out){uint16_t spare[8]={0}; return port110_sincos(PORT_M107_D,1,a,spare,out);}
