/* Define to prevent recursive inclusion *********************************** */
#ifndef __WIFI_HOSTED_H
#define __WIFI_HOSTED_H

/* Includes **************************************************************** */
#include "mcu.h"
#include "hal.h"

/* Module configuration **************************************************** */
#define WIFI_HOSTED_SSID                              "MMA-WiFi"
#define WIFI_HOSTED_PASSWORD                          "hantekdso2c10"

/* Exported types ********************************************************** */
enum WIFI_HOSTED_STATE
{
    WIFI_HOSTED_STATE_IDLE = 0,
    WIFI_HOSTED_STATE_WAIT_INIT_EVENT,
    WIFI_HOSTED_STATE_SEND_INIT_CONFIG,
    WIFI_HOSTED_STATE_WIFI_INIT,
    WIFI_HOSTED_STATE_SET_STA_MODE,
    WIFI_HOSTED_STATE_SCAN_START,
    WIFI_HOSTED_STATE_WAIT_SCAN_DONE,
    WIFI_HOSTED_STATE_SCAN_GET_NUM,
    WIFI_HOSTED_STATE_SCAN_GET_RECORDS,
    WIFI_HOSTED_STATE_CONNECT_TARGET,
    WIFI_HOSTED_STATE_TRANSPORT_READY,
    WIFI_HOSTED_STATE_ERROR,
};

typedef struct wifi_hosted_ctx_t
{
    uint8_t state;
    uint32_t last_tick;
    uint8_t init_event_received;
    uint8_t slave_chip_id;
} WIFI_HOSTED_CTX;

/* Exported variables ****************************************************** */
extern WIFI_HOSTED_CTX g_WIFI_HOSTED_CTX;

/* Exported functions ****************************************************** */
int8_t wifi_init(void);
void wifi_task(void);
int8_t wifi_hosted_rpc_send(uint8_t if_num, const uint8_t *payload, uint16_t len);
int16_t wifi_hosted_rpc_recv_rsp(uint8_t *buf, uint16_t buf_size, uint8_t *if_num);
int16_t wifi_hosted_rpc_recv_evt(uint8_t *buf, uint16_t buf_size, uint8_t *if_num);

#endif
