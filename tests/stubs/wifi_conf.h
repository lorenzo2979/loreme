#ifndef GWIR_TEST_WIFI_CONF_H
#define GWIR_TEST_WIFI_CONF_H
/* Unit stub only; native CI uses actual Realtek SDK Wi-Fi declarations. */
typedef enum { RTW_MODE_STA=1, RTW_MODE_AP=2, RTW_MODE_STA_AP=3 } rtw_mode_t;
typedef enum { RTW_SECURITY_OPEN=0, RTW_SECURITY_WPA2_AES_PSK=4 } rtw_security_t;
int wifi_set_mode(rtw_mode_t mode);
int wifi_start_ap(char *ssid, rtw_security_t security, char *password,
                  int ssid_len, int password_len, int channel);
#endif
