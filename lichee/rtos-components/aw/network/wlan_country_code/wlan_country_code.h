#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    COUNTRY_CODE_CN,  //中国
    COUNTRY_CODE_US,  //美国
    COUNTRY_CODE_JP,  //日本
    COUNTRY_CODE_EU,  //欧洲
    COUNTRY_CODE_KR,  //韩国
    COUNTRY_CODE_CA,  //加拿大
    COUNTRY_CODE_BR,  //巴西
    COUNTRY_CODE_AU,  //澳大利亚
    COUNTRY_CODE_DE,  //德国
    COUNTRY_CODE_NONE //其他
} COUNTRY_CODE_E;

int wifi_set_countrycode(COUNTRY_CODE_E countrycode);
int wifi_get_countrycode();
