#define VERSION_MAJOR            1
#define VERSION_MINOR            0
#define VERSION_REVISION         0
#define VERSION_BUILDNUM         26
#define VERSION_GCID             0xbd0864f2
#define VERSION_GCIDH            0xf15ab0eb
#define VERSION_PGCID            0xbeed8596
#define VERSION_PGCIDH           0x8a1e4f10
#define LIB_NAME                 "bt-lib-for-a"
#define CUSTOMER_NAME            lib
#define CN_1                     'l'
#define CN_2                     'i'
#define CN_3                     'b'
#define CN_4                     '#'
#define CN_5                     '#'
#define CN_6                     '#'
#define CN_7                     '#'
#define CN_8                     '#'
#define BUILDING_TIME            "Thu Feb 28 11:08:06 2019"
#define NAME2STR(a)              #a
#define CUSTOMER_NAME_S          #NAME2STR(CUSTOMER_NAME)
#define NUM4STR(a,b,c,d)         #a "." #b "." #c "." #d
#define VERSIONBUILDSTR(a,b,c,d) NUM4STR(a,b,c,d)
#define VERSION_BUILD_STR        VERSIONBUILDSTR(VERSION_MAJOR,VERSION_MINOR,VERSION_REVISION,VERSION_BUILDNUM)
