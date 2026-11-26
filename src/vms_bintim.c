#include "vms_bintim.h"

#define TICKS_PER_SECOND UINT64_C(10000000)
#define TICKS_PER_DAY (UINT64_C(86400)*TICKS_PER_SECOND)
#define VMS_UNIX_DAY_OFFSET UINT64_C(40587)

struct civil_time {
    unsigned year,month,day,hour,minute,second,hundredth;
};

static uint64_t divide_u64(uint64_t value,uint64_t divisor,uint64_t *remainder){
    uint64_t quotient=0u,rem=0u;
    for(int bit=63;bit>=0;--bit){
        rem=(rem<<1)|((value>>(unsigned)bit)&1u);
        if(rem>=divisor){rem-=divisor;quotient|=UINT64_C(1)<<(unsigned)bit;}
    }
    if(remainder)*remainder=rem;
    return quotient;
}

static unsigned month_days(unsigned year,unsigned month){
    static const uint8_t days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(month<1u||month>12u)return 0u;
    if(month==2u&&year%4u==0u&&(year%100u!=0u||year%400u==0u))return 29u;
    return days[month-1u];
}

static int calendar_to_ticks(unsigned year,unsigned month,unsigned day,
                             unsigned hour,unsigned minute,unsigned second,
                             uint64_t *ticks){
    if(!ticks||year<1858u||year>9999u||!month||month>12u||!day||
       day>month_days(year,month)||hour>23u||minute>59u||second>59u)return 0;
    int y=(int)year;y-=month<=2u;
    int era=y/400;
    unsigned yoe=(unsigned)(y-era*400);
    unsigned m=(unsigned)((int)month+(month>2u?-3:9));
    unsigned doy=(153u*m+2u)/5u+day-1u;
    unsigned doe=yoe*365u+yoe/4u-yoe/100u+doy;
    int64_t delta=(int64_t)(era*146097+(int)doe-719468+40587);
    if(delta<0)return 0;
    uint64_t seconds=(uint64_t)delta*86400u+(uint64_t)hour*3600u+
                     (uint64_t)minute*60u+second;
    *ticks=seconds*TICKS_PER_SECOND;
    return 1;
}

static int current_fields(uint64_t ticks,struct civil_time *out){
    if(!out)return 0;
    uint64_t day_ticks=0u;
    uint64_t days=divide_u64(ticks,TICKS_PER_DAY,&day_ticks);
    if(days<VMS_UNIX_DAY_OFFSET)return 0;
    if(days-VMS_UNIX_DAY_OFFSET>3000000u)return 0;
    uint32_t z=(uint32_t)(days-VMS_UNIX_DAY_OFFSET)+719468u;
    uint32_t era=z/146097u;
    unsigned doe=z-era*146097u;
    unsigned yoe=(doe-doe/1460u+doe/36524u-doe/146096u)/365u;
    unsigned year=yoe+era*400u;
    unsigned doy=doe-(365u*yoe+yoe/4u-yoe/100u);
    unsigned mp=(5u*doy+2u)/153u;
    unsigned day=doy-(153u*mp+2u)/5u+1u;
    unsigned month=mp+(mp<10u?3u:(unsigned)-9);
    year+=month<=2u;
    if(year<1858u||year>9999u)return 0;
    uint64_t second_fraction=0u;
    uint64_t seconds=divide_u64(day_ticks,TICKS_PER_SECOND,&second_fraction);
    out->year=year;out->month=month;out->day=day;
    out->hour=(unsigned)divide_u64(seconds,3600u,NULL);
    uint64_t second_remainder=0u,minute_remainder=0u;
    uint64_t minutes=divide_u64(seconds,60u,&second_remainder);
    (void)divide_u64(minutes,60u,&minute_remainder);
    out->minute=(unsigned)minute_remainder;
    out->second=(unsigned)second_remainder;
    out->hundredth=(unsigned)divide_u64(second_fraction,100000u,NULL);
    return 1;
}

static int is_space(char c){return c==' '||c=='\t';}
static void skip_space(const char *s,size_t n,size_t *at){
    while(*at<n&&is_space(s[*at]))++*at;
}
static int has_hyphen(const char *s,size_t n){
    for(size_t i=0;i<n;i++)if(s[i]=='-')return 1;
    return 0;
}

/* EXE$BINTIM CONVERT ignores blanks inside a numeric field and stops at its
 * expected punctuation. A missing field retains its NUMTIM/zero default. */
static int number_field(const char *s,size_t n,size_t *at,char terminator,
                        unsigned maximum,unsigned *value,int *present){
    unsigned candidate=0;int digits=0;
    while(*at<n){
        char c=s[*at];
        if(c==terminator){
            ++*at;
            if(digits)*value=candidate;
            *present=digits;
            return 1;
        }
        if(is_space(c)){++*at;continue;}
        if(c<'0'||c>'9')return 0;
        unsigned digit=(unsigned)(c-'0');
        if(digit>maximum||candidate>(maximum-digit)/10u)return 0;
        candidate=candidate*10u+digit;digits=1;++*at;
    }
    if(digits)*value=candidate;
    *present=digits;
    return 1;
}

static int month_number(const char *s,size_t n,size_t *at,unsigned *month){
    static const char names[12][3]={
        {'J','A','N'},{'F','E','B'},{'M','A','R'},{'A','P','R'},
        {'M','A','Y'},{'J','U','N'},{'J','U','L'},{'A','U','G'},
        {'S','E','P'},{'O','C','T'},{'N','O','V'},{'D','E','C'}};
    if(*at>=n||s[*at]=='-'){if(*at<n)++*at;return 1;}
    if(n-*at<4u)return 0;
    char first=s[*at],second=s[*at+1u],third=s[*at+2u];
    if(first>='a'&&first<='z')first=(char)(first-'a'+'A');
    if(second>='a'&&second<='z')second=(char)(second-'a'+'A');
    if(third>='a'&&third<='z')third=(char)(third-'a'+'A');
    unsigned found=0u;
    for(unsigned i=0;i<12u;i++)
        if(names[i][0]==first&&names[i][1]==second&&names[i][2]==third){
            found=i+1u;break;
        }
    if(!found||s[*at+3u]!='-')return 0;
    *month=found;*at+=4u;return 1;
}

static int parse_clock(const char *s,size_t n,size_t *at,
                       struct civil_time *t,int absolute){
    skip_space(s,n,at);
    if(*at==n)return 1;
    unsigned *fields[3]={&t->hour,&t->minute,&t->second};
    const char separators[3]={':',':','.'};
    const unsigned limits[3]={23u,59u,59u};
    for(unsigned i=0;i<3u;i++){
        int present=0;
        if(!number_field(s,n,at,separators[i],limits[i],fields[i],&present))
            return 0;
        if(!present&&!absolute)*fields[i]=0u;
        if(*at==n)return 1;
        if(s[*at-1u]!=separators[i])return 0;
        if(i<2u&&*at==n)return 1;
    }

    /* The source keeps two decimal places and rounds on the third. */
    unsigned hundredth=0u,digits=0u,rounding=0u;
    while(*at<n&&!is_space(s[*at])){
        char c=s[*at];
        if(c<'0'||c>'9')return 0;
        if(digits<2u)hundredth=hundredth*10u+(unsigned)(c-'0');
        else if(digits==2u)rounding=(unsigned)(c-'0');
        ++digits;++*at;
    }
    if(digits==1u)hundredth*=10u;
    else if(digits>=2u&&rounding>=5u)++hundredth;
    if(t->hour>23u)return 0;
    t->hundredth=hundredth;
    skip_space(s,n,at);
    return *at==n;
}

static int parse_absolute(const char *s,size_t n,uint64_t now,
                          uint64_t *candidate){
    struct civil_time t;
    if(!current_fields(now,&t))return 0;
    size_t at=0;unsigned day=t.day,year=t.year;
    int present=0;
    if(!number_field(s,n,&at,'-',9999u,&day,&present)||!present)return 0;
    if(!month_number(s,n,&at,&t.month))return 0;
    if(!number_field(s,n,&at,' ',9999u,&year,&present)||!present)return 0;
    if(year<1601u)return 0;
    t.day=day;t.year=year;
    /* A missing time is an empty remainder, retaining all NUMTIM fields. */
    if(!parse_clock(s,n,&at,&t,1))return 0;
    uint64_t result=0;
    if(!calendar_to_ticks(t.year,t.month,t.day,t.hour,t.minute,t.second,
                          &result))return 0;
    if(result>UINT64_MAX-(uint64_t)t.hundredth*100000u)return 0;
    *candidate=result+(uint64_t)t.hundredth*100000u;
    return *candidate!=0u;
}

static int parse_delta(const char *s,size_t n,uint64_t *candidate){
    size_t at=0;unsigned days=0u;int present=0;
    if(!number_field(s,n,&at,' ',9999u,&days,&present)||!present)return 0;
    struct civil_time t={0};
    if(at<n&&!parse_clock(s,n,&at,&t,0))return 0;
    if(days>9999u||t.hour>23u||t.minute>59u||t.second>59u||
       t.hundredth>100u)return 0;
    uint64_t magnitude=(uint64_t)days*TICKS_PER_DAY+
        (uint64_t)t.hour*3600u*TICKS_PER_SECOND+
        (uint64_t)t.minute*60u*TICKS_PER_SECOND+
        (uint64_t)t.second*TICKS_PER_SECOND+
        (uint64_t)t.hundredth*100000u;
    if(magnitude>INT64_MAX)return 0;
    *candidate=(uint64_t)(-(int64_t)magnitude);
    return 1;
}

uint32_t port_vms_bintim(const char *text,size_t bytes,uint64_t current_time,
                         uint64_t *binary_time){
    if(!text||!binary_time||!bytes||bytes>255u)return PORT_BINTIM_IVTIME;
    size_t begin=0,end=bytes;
    skip_space(text,end,&begin);
    while(end>begin&&is_space(text[end-1u]))--end;
    if(begin==end)return PORT_BINTIM_IVTIME;
    const char *s=text+begin;const size_t n=end-begin;
    uint64_t candidate=0u;
    const int ok=has_hyphen(s,n)?
        parse_absolute(s,n,current_time,&candidate):
        parse_delta(s,n,&candidate);
    if(!ok)return PORT_BINTIM_IVTIME;
    *binary_time=candidate;
    return PORT_BINTIM_NORMAL;
}
