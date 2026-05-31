/* Includes **************************************************************** */
#include "wifi_hosted.h"
#include "delay.h"
#include "debug.h"
#include <string.h>

/* Private macros ********************************************************** */
#define WIFI_HOSTED_TASK_PERIOD_MS                    100
#define WIFI_HOSTED_POST_RESET_DELAY_MS               3000
#define WIFI_HOSTED_SCAN_MIN_DURATION_MS              3000
#define WIFI_HOSTED_SCAN_WAIT_TIMEOUT_MS              10000
#define WIFI_SET_CONFIG_TO_CONNECT_DELAY_MS           200
#define WIFI_DHCP_POLL_PERIOD_MS                      1000
#define WIFI_DHCP_POLL_START_DELAY_MS                 5000
#define WIFI_HOSTED_SPI_FRAME_SIZE                    1600
#define WIFI_HOSTED_HEADER_SIZE                       12
#define WIFI_HOSTED_SPI_TIMEOUT_MS                    500
#define WIFI_HOSTED_RPC_MAX_PAYLOAD                   1536
#define WIFI_HOSTED_RPC_QUEUE_DEPTH                   4

#define ESP_MAX_IF                                    8
#define ESP_SERIAL_IF                                 3
#define ESP_PRIV_IF                                   5
#define ESP_PRIV_EVENT_INIT                           0x22
#define WIFI_MODE_STA                                 1

#define RPC_MSG_TYPE_REQ                              1
#define RPC_MSG_ID_REQ_SET_WIFI_MODE                  260
#define RPC_MSG_ID_REQ_WIFI_START                     280
#define RPC_MSG_ID_REQ_WIFI_STOP                      281
#define RPC_MSG_ID_REQ_WIFI_INIT                      278
#define RPC_MSG_ID_REQ_WIFI_SCAN_START                286
#define RPC_MSG_ID_REQ_WIFI_SCAN_GET_AP_NUM           288
#define RPC_MSG_ID_REQ_WIFI_SCAN_GET_AP_RECORDS       289
#define RPC_MSG_ID_REQ_WIFI_SET_CONFIG                284
#define RPC_MSG_ID_REQ_WIFI_CONNECT                   282
#define RPC_MSG_ID_REQ_WIFI_DISCONNECT                283
#define RPC_MSG_ID_REQ_WIFI_GET_CONFIG                285
#define RPC_MSG_ID_REQ_GET_DHCP_DNS_STATUS            353
#define RPC_MSG_ID_EVENT_STA_SCAN_DONE                774
#define RPC_MSG_ID_EVENT_STA_CONNECTED                775
#define RPC_MSG_ID_EVENT_STA_DISCONNECTED             776
#define RPC_MSG_ID_EVENT_DHCP_DNS_STATUS              777
#define RPC_MSG_ID_RESP_GET_DHCP_DNS_STATUS           609
#define WIFI_EVENT_ID_STA_SCAN_DONE                   43
#define PROTO_PSER_TLV_T_EPNAME                       0x01
#define PROTO_PSER_TLV_T_DATA                         0x02
#define RPC_EP_NAME_RSP                               "RPCRsp"
#define WIFI_IF_STA                                   0

#define ESP_PRIV_CAPABILITY                           0x11
#define ESP_PRIV_FIRMWARE_CHIP_ID                     0x12
#define HOST_CAPABILITIES                             0x44
#define RCVD_ESP_FIRMWARE_CHIP_ID                     0x45
#define SLV_CONFIG_TEST_RAW_TP                        0x46
#define SLV_CONFIG_THROTTLE_HIGH_THRESHOLD            0x47
#define SLV_CONFIG_THROTTLE_LOW_THRESHOLD             0x48

/* Private types *********************************************************** */
typedef struct __attribute__((packed))
{
    uint8_t if_type : 4;
    uint8_t if_num : 4;
    uint8_t flags;
    uint16_t len;
    uint16_t offset;
    uint16_t checksum;
    uint16_t seq_num;
    uint8_t throttle_cmd : 2;
    uint8_t reserved2 : 6;
    uint8_t reserved3;
} esp_payload_header_t;

typedef struct __attribute__((packed))
{
    uint8_t event_type;
    uint8_t event_len;
    uint8_t event_data[];
} esp_priv_event_t;

typedef struct
{
    uint8_t if_num;
    uint16_t len;
    uint8_t data[WIFI_HOSTED_RPC_MAX_PAYLOAD];
} wifi_rpc_slot_t;

/* Private variables ******************************************************* */
static uint16_t s_seq_num = 0;
static wifi_rpc_slot_t s_rsp_q[WIFI_HOSTED_RPC_QUEUE_DEPTH];
static wifi_rpc_slot_t s_evt_q[WIFI_HOSTED_RPC_QUEUE_DEPTH];
static uint8_t s_rsp_head = 0;
static uint8_t s_rsp_tail = 0;
static uint8_t s_rsp_count = 0;
static uint8_t s_evt_head = 0;
static uint8_t s_evt_tail = 0;
static uint8_t s_evt_count = 0;
static uint32_t s_rpc_uid = 1;
static uint16_t s_last_scan_ap_num = 0;
static uint8_t s_scan_done_event = 0;
static uint32_t s_scan_start_tick = 0;
static uint8_t s_target_ap_found = 0;
static uint8_t s_sta_connected_event = 0;
static uint8_t s_sta_got_ip = 0;
static uint32_t s_dhcp_poll_last_tick = 0;
static uint32_t s_sta_connected_tick = 0;

/* Private functions ******************************************************* */
static int8_t wifi_hosted_parse_init_event(const uint8_t *rx);
static int8_t wifi_hosted_poll_runtime(void);
static int16_t wifi_hosted_build_rpc_req(uint32_t msg_id, const uint8_t *submsg, uint16_t submsg_len, uint8_t *out, uint16_t out_max);
static int8_t wifi_hosted_poll_dhcp_dns_status_rpc(void);

static uint16_t wifi_hosted_put_varint(uint32_t value, uint8_t *out)
{
    uint16_t n = 0;
    do {
        uint8_t byte = (uint8_t)(value & 0x7FU);
        value >>= 7;
        if (value != 0U) {
            byte |= 0x80U;
        }
        out[n++] = byte;
    } while (value != 0U);
    return n;
}

static int16_t wifi_hosted_append_field_varint(uint8_t *buf, uint16_t pos, uint16_t max,
                                               uint32_t field_no, uint32_t value)
{
    uint16_t n;
    uint32_t key = (field_no << 3U) | 0U; /* varint */

    if (pos >= max) {
        return -1;
    }
    n = wifi_hosted_put_varint(key, buf + pos);
    pos = (uint16_t)(pos + n);
    if (pos >= max) {
        return -1;
    }
    n = wifi_hosted_put_varint(value, buf + pos);
    pos = (uint16_t)(pos + n);
    if (pos > max) {
        return -1;
    }
    return (int16_t)pos;
}

static int16_t wifi_hosted_get_varint(const uint8_t *buf, uint16_t len, uint16_t *pos, uint32_t *out)
{
    uint32_t value = 0;
    uint8_t shift = 0;
    while (*pos < len) {
        uint8_t b = buf[(*pos)++];
        value |= (uint32_t)(b & 0x7FU) << shift;
        if ((b & 0x80U) == 0U) {
            *out = value;
            return 0;
        }
        shift = (uint8_t)(shift + 7U);
        if (shift > 28U) {
            break;
        }
    }
    return -1;
}

static int16_t wifi_hosted_extract_rpc_from_tlv(const uint8_t *in, uint16_t in_len, const uint8_t **rpc, uint16_t *rpc_len)
{
    uint16_t p = 0;
    uint16_t l = 0;

    if (in_len < 7U) {
        return -1;
    }
    if (in[p++] != PROTO_PSER_TLV_T_EPNAME) {
        return -1;
    }
    l = (uint16_t)in[p] | ((uint16_t)in[p + 1U] << 8U);
    p = (uint16_t)(p + 2U + l);
    if ((uint16_t)(p + 3U) > in_len) {
        return -1;
    }
    if (in[p++] != PROTO_PSER_TLV_T_DATA) {
        return -1;
    }
    l = (uint16_t)in[p] | ((uint16_t)in[p + 1U] << 8U);
    p = (uint16_t)(p + 2U);
    if ((uint16_t)(p + l) > in_len) {
        return -1;
    }
    *rpc = &in[p];
    *rpc_len = l;
    return 0;
}

static int16_t wifi_hosted_extract_rpc_payload(const uint8_t *rpc, uint16_t rpc_len, uint32_t expected_msg_id,
                                               const uint8_t **payload, uint16_t *payload_len)
{
    uint16_t p = 0;
    uint32_t key;
    uint32_t wire;
    uint32_t field;
    uint32_t msg_id = 0;

    while (p < rpc_len) {
        if (wifi_hosted_get_varint(rpc, rpc_len, &p, &key) != 0) {
            return -1;
        }
        field = key >> 3U;
        wire = key & 0x07U;

        if (wire == 0U) {
            uint32_t v;
            if (wifi_hosted_get_varint(rpc, rpc_len, &p, &v) != 0) {
                return -1;
            }
            if (field == 2U) {
                msg_id = v;
            }
        } else if (wire == 2U) {
            uint32_t l;
            if (wifi_hosted_get_varint(rpc, rpc_len, &p, &l) != 0) {
                return -1;
            }
            if ((uint16_t)(p + l) > rpc_len) {
                return -1;
            }
            if ((msg_id == expected_msg_id) && (field == expected_msg_id)) {
                *payload = &rpc[p];
                *payload_len = (uint16_t)l;
                return 0;
            }
            p = (uint16_t)(p + l);
        } else {
            return -1;
        }
    }

    return -1;
}

static int16_t wifi_hosted_extract_rpc_msg(const uint8_t *rpc, uint16_t rpc_len,
                                           uint32_t *msg_id, const uint8_t **payload, uint16_t *payload_len)
{
    uint16_t p = 0;
    uint32_t key;
    uint32_t wire;
    uint32_t field;
    uint32_t l;
    uint32_t id = 0;

    while (p < rpc_len) {
        if (wifi_hosted_get_varint(rpc, rpc_len, &p, &key) != 0) {
            return -1;
        }
        field = key >> 3U;
        wire = key & 0x07U;

        if (wire == 0U) {
            uint32_t v;
            if (wifi_hosted_get_varint(rpc, rpc_len, &p, &v) != 0) {
                return -1;
            }
            if (field == 2U) {
                id = v;
            }
        } else if (wire == 2U) {
            if (wifi_hosted_get_varint(rpc, rpc_len, &p, &l) != 0) {
                return -1;
            }
            if ((uint16_t)(p + l) > rpc_len) {
                return -1;
            }
            if ((id != 0U) && (field == id)) {
                *msg_id = id;
                *payload = &rpc[p];
                *payload_len = (uint16_t)l;
                return 0;
            }
            p = (uint16_t)(p + l);
        } else {
            return -1;
        }
    }
    return -1;
}

static void wifi_hosted_log_ipv4_bytes(const char *label, const uint8_t *ip, uint16_t ip_len)
{
    if ((ip == 0) || (ip_len < 4U)) {
        return;
    }
    if ((ip_len > 4U) && (ip_len < 64U)) {
        uint8_t i;
        uint8_t has_dot = 0U;
        char tmp[64] = {0};
        uint16_t n = ip_len;
        if (n > (sizeof(tmp) - 1U)) {
            n = (uint16_t)(sizeof(tmp) - 1U);
        }
        for (i = 0; i < n; i++) {
            tmp[i] = (char)ip[i];
            if (ip[i] == '.') {
                has_dot = 1U;
            }
        }
        tmp[n] = '\0';
        if (has_dot != 0U) {
            debug_log(DNONE, "%s%s\n", label, tmp);
            return;
        }
    }
    debug_log(DNONE, "%s%d.%d.%d.%d\n", label, ip[0], ip[1], ip[2], ip[3]);
}

static void wifi_hosted_handle_rpc_event(uint32_t msg_id, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t p = 0;
    uint32_t key;
    uint32_t field;
    uint32_t wire;

    if (msg_id == RPC_MSG_ID_EVENT_STA_SCAN_DONE) {
        s_scan_done_event = 1U;
        debug_log(DNONE, "Received scan-done event\n");
        return;
    }

    if (msg_id == 773U) {
        /* Event_WifiEventNoArgs: field 2 contains event_id */
        int32_t event_id = -1;
        while (p < payload_len) {
            uint32_t v;
            if (wifi_hosted_get_varint(payload, payload_len, &p, &key) != 0) {
                break;
            }
            field = key >> 3U;
            wire = key & 0x07U;
            if (wire == 0U) {
                if (wifi_hosted_get_varint(payload, payload_len, &p, &v) != 0) {
                    break;
                }
                if (field == 2U) {
                    event_id = (int32_t)v;
                }
            } else if (wire == 2U) {
                uint32_t l;
                if (wifi_hosted_get_varint(payload, payload_len, &p, &l) != 0) {
                    break;
                }
                p = (uint16_t)(p + (uint16_t)l);
            } else {
                break;
            }
        }
        if (event_id == WIFI_EVENT_ID_STA_SCAN_DONE) {
            s_scan_done_event = 1U;
            debug_log(DNONE, "Received scan-done event\n");
        }
        return;
    }

    if (msg_id == RPC_MSG_ID_EVENT_STA_CONNECTED) {
        s_sta_connected_event = 1U;
        s_sta_got_ip = 0U;
        s_dhcp_poll_last_tick = 0U;
        s_sta_connected_tick = HAL_GetTick();
        debug_log(DNONE, "Received STA connected event from slave\n");
        return;
    }

    if (msg_id == RPC_MSG_ID_EVENT_STA_DISCONNECTED) {
        s_sta_connected_event = 0U;
        s_sta_got_ip = 0U;
        s_sta_connected_tick = 0U;
        debug_log(DWARNING, "Received STA disconnected event from slave\n");
        return;
    }

    if (msg_id != RPC_MSG_ID_EVENT_DHCP_DNS_STATUS) {
        return;
    }

    {
        int32_t iface = -1;
        int32_t net_link_up = 0;
        int32_t dhcp_up = 0;
        int32_t dns_up = 0;
        uint8_t dhcp_ip[64] = {0};
        uint8_t dhcp_nm[64] = {0};
        uint8_t dhcp_gw[64] = {0};
        uint8_t dns_ip[64] = {0};
        uint16_t dhcp_ip_len = 0;
        uint16_t dhcp_nm_len = 0;
        uint16_t dhcp_gw_len = 0;
        uint16_t dns_ip_len = 0;

        while (p < payload_len) {
            uint32_t v;
            if (wifi_hosted_get_varint(payload, payload_len, &p, &key) != 0) {
                break;
            }
            field = key >> 3U;
            wire = key & 0x07U;
            if (wire == 0U) {
                if (wifi_hosted_get_varint(payload, payload_len, &p, &v) != 0) {
                    break;
                }
                if (field == 1U) iface = (int32_t)v;
                else if (field == 2U) net_link_up = (int32_t)v;
                else if (field == 3U) dhcp_up = (int32_t)v;
                else if (field == 7U) dns_up = (int32_t)v;
            } else if (wire == 2U) {
                uint32_t l;
                uint16_t copy_len;
                if (wifi_hosted_get_varint(payload, payload_len, &p, &l) != 0) {
                    break;
                }
                if ((uint16_t)(p + l) > payload_len) {
                    break;
                }
                copy_len = (uint16_t)l;
                if (field == 4U) {
                    uint16_t i;
                    if (copy_len > sizeof(dhcp_ip)) copy_len = sizeof(dhcp_ip);
                    dhcp_ip_len = copy_len;
                    for (i = 0; i < copy_len; i++) dhcp_ip[i] = payload[p + i];
                } else if (field == 5U) {
                    uint16_t i;
                    if (copy_len > sizeof(dhcp_nm)) copy_len = sizeof(dhcp_nm);
                    dhcp_nm_len = copy_len;
                    for (i = 0; i < copy_len; i++) dhcp_nm[i] = payload[p + i];
                } else if (field == 6U) {
                    uint16_t i;
                    if (copy_len > sizeof(dhcp_gw)) copy_len = sizeof(dhcp_gw);
                    dhcp_gw_len = copy_len;
                    for (i = 0; i < copy_len; i++) dhcp_gw[i] = payload[p + i];
                } else if (field == 8U) {
                    uint16_t i;
                    if (copy_len > sizeof(dns_ip)) copy_len = sizeof(dns_ip);
                    dns_ip_len = copy_len;
                    for (i = 0; i < copy_len; i++) dns_ip[i] = payload[p + i];
                }
                p = (uint16_t)(p + (uint16_t)l);
            } else {
                break;
            }
        }

        debug_log(DNONE, "DHCP/DNS event: iface=%d link=%d dhcp=%d dns=%d\n",
                  iface, net_link_up, dhcp_up, dns_up);
        if (dhcp_up != 0) {
            s_sta_got_ip = 1U;
            wifi_hosted_log_ipv4_bytes("STA IP: ", dhcp_ip, dhcp_ip_len);
            wifi_hosted_log_ipv4_bytes("STA NM: ", dhcp_nm, dhcp_nm_len);
            wifi_hosted_log_ipv4_bytes("STA GW: ", dhcp_gw, dhcp_gw_len);
        }
        if (dns_up != 0) {
            wifi_hosted_log_ipv4_bytes("STA DNS: ", dns_ip, dns_ip_len);
        }
    }
}

static int16_t wifi_hosted_wait_rpc_payload(uint32_t resp_msg_id, uint8_t *buf, uint16_t buf_size, uint16_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t raw[WIFI_HOSTED_RPC_MAX_PAYLOAD];
    uint8_t if_num;
    int16_t got;
    const uint8_t *rpc = 0;
    uint16_t rpc_len = 0;
    const uint8_t *payload = 0;
    uint16_t payload_len = 0;
    uint16_t i;

    while ((HAL_GetTick() - start) < timeout_ms) {
        got = wifi_hosted_rpc_recv_rsp(raw, sizeof(raw), &if_num);
        if (got > 0) {
            if (wifi_hosted_extract_rpc_from_tlv(raw, (uint16_t)got, &rpc, &rpc_len) == 0) {
                if (wifi_hosted_extract_rpc_payload(rpc, rpc_len, resp_msg_id, &payload, &payload_len) == 0) {
                    if (payload_len > buf_size) {
                        payload_len = buf_size;
                    }
                    for (i = 0; i < payload_len; i++) {
                        buf[i] = payload[i];
                    }
                    return (int16_t)payload_len;
                }
            }
        }
        (void)wifi_hosted_poll_runtime();
        delay_ms(20);
    }
    return -1;
}

static int8_t wifi_hosted_wait_rpc_ack(uint32_t resp_msg_id, uint16_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t raw[WIFI_HOSTED_RPC_MAX_PAYLOAD];
    uint8_t if_num;
    int16_t got;
    const uint8_t *rpc = 0;
    uint16_t rpc_len = 0;
    uint32_t msg_id = 0;
    const uint8_t *payload = 0;
    uint16_t payload_len = 0;

    while ((HAL_GetTick() - start) < timeout_ms) {
        got = wifi_hosted_rpc_recv_rsp(raw, sizeof(raw), &if_num);
        if (got > 0) {
            if (wifi_hosted_extract_rpc_from_tlv(raw, (uint16_t)got, &rpc, &rpc_len) == 0) {
                if (wifi_hosted_extract_rpc_msg(rpc, rpc_len, &msg_id, &payload, &payload_len) == 0) {
                    if (msg_id == resp_msg_id) {
                        debug_log(DNONE, "Received RPC response msg_id=0x%x\n", (unsigned int)msg_id);
                        (void)payload;
                        (void)payload_len;
                        return DRIVER_STATUS_OK;
                    }
                }
            }
            (void)if_num;
        }
        (void)wifi_hosted_poll_runtime();
        delay_ms(20);
    }
    return DRIVER_STATUS_TIMEOUT;
}

static int8_t wifi_hosted_send_simple_rpc_and_wait_ack(uint32_t req_msg_id, const char *name, uint16_t timeout_ms)
{
    uint8_t rpc_buf[96];
    int16_t rpc_len;
    uint16_t resp_msg_id;

    rpc_len = wifi_hosted_build_rpc_req(req_msg_id, 0, 0, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for %s\n", name);
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for %s\n", name);
        return DRIVER_STATUS_TIMEOUT;
    }
    resp_msg_id = (uint16_t)(req_msg_id + 0x100U);
    debug_log(DNONE, "Waiting for %s response msg_id=0x%x\n", name, (unsigned int)resp_msg_id);
    if (wifi_hosted_wait_rpc_ack(resp_msg_id, timeout_ms) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "No RPC response for %s\n", name);
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "%s response received\n", name);
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_poll_dhcp_dns_status_rpc(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t rpc_buf[64];
    uint8_t resp_payload[WIFI_HOSTED_RPC_MAX_PAYLOAD];
    int16_t rpc_len;
    int16_t resp_len;

    if ((s_sta_connected_event == 0U) || (s_sta_got_ip != 0U)) {
        return DRIVER_STATUS_OK;
    }
    if ((now - s_sta_connected_tick) < WIFI_DHCP_POLL_START_DELAY_MS) {
        return DRIVER_STATUS_OK;
    }
    if ((now - s_dhcp_poll_last_tick) < WIFI_DHCP_POLL_PERIOD_MS) {
        return DRIVER_STATUS_OK;
    }
    s_dhcp_poll_last_tick = now;

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_GET_DHCP_DNS_STATUS, 0, 0, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for GetDhcpDnsStatus\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DWARNING, "RPC send failed for GetDhcpDnsStatus\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Polling DHCP status (Req_GetDhcpDnsStatus)\n");

    resp_len = wifi_hosted_wait_rpc_payload(RPC_MSG_ID_RESP_GET_DHCP_DNS_STATUS,
                                            resp_payload, sizeof(resp_payload), 600);
    if (resp_len > 0) {
        debug_log(DNONE, "Received GetDhcpDnsStatus response payload (%d bytes)\n", resp_len);
        /* Response payload layout mirrors DHCP/DNS status event payload in current slave stack. */
        wifi_hosted_handle_rpc_event(RPC_MSG_ID_EVENT_DHCP_DNS_STATUS, resp_payload, (uint16_t)resp_len);
    }
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_get_config_verify_ssid(const char *ssid)
{
    uint8_t rpc_buf[96];
    uint8_t submsg[8];
    uint8_t resp_payload[WIFI_HOSTED_RPC_MAX_PAYLOAD];
    uint16_t sub_len = 0;
    uint16_t n;
    int16_t rpc_len;
    int16_t resp_len;
    uint16_t i;
    uint16_t ssid_len = (uint16_t)strlen(ssid);
    uint8_t matched = 0;

    /* Rpc_Req_WifiGetConfig: iface(field1 varint) */
    submsg[sub_len++] = 0x08;
    n = wifi_hosted_put_varint(WIFI_IF_STA, &submsg[sub_len]);
    sub_len = (uint16_t)(sub_len + n);

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_GET_CONFIG, submsg, sub_len, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiGetConfig\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiGetConfig\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Waiting for WifiGetConfig response msg_id=0x%x\n",
              (unsigned int)(RPC_MSG_ID_REQ_WIFI_GET_CONFIG + 0x100U));
    resp_len = wifi_hosted_wait_rpc_payload(RPC_MSG_ID_REQ_WIFI_GET_CONFIG + 0x100U,
                                            resp_payload, sizeof(resp_payload), 2000);
    if (resp_len <= 0) {
        debug_log(DERROR, "No/invalid response for WifiGetConfig\n");
        return DRIVER_STATUS_TIMEOUT;
    }

    for (i = 0; i + ssid_len <= (uint16_t)resp_len; i++) {
        if (memcmp(&resp_payload[i], ssid, ssid_len) == 0) {
            matched = 1U;
            break;
        }
    }
    if (matched == 0U) {
        debug_log(DWARNING, "WifiGetConfig verify failed: SSID '%s' not active yet\n", ssid);
        return DRIVER_STATUS_ERROR;
    }
    debug_log(DNONE, "WifiGetConfig verify OK: SSID '%s' is active\n", ssid);
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_consume_events(void)
{
    uint8_t evt[WIFI_HOSTED_RPC_MAX_PAYLOAD];
    uint8_t if_num;
    int16_t got;
    const uint8_t *rpc = 0;
    uint16_t rpc_len = 0;
    uint32_t msg_id = 0;
    const uint8_t *payload = 0;
    uint16_t payload_len = 0;

    while (1) {
        got = wifi_hosted_rpc_recv_evt(evt, sizeof(evt), &if_num);
        if (got <= 0) {
            break;
        }
        if (wifi_hosted_extract_rpc_from_tlv(evt, (uint16_t)got, &rpc, &rpc_len) == 0) {
            if (wifi_hosted_extract_rpc_msg(rpc, rpc_len, &msg_id, &payload, &payload_len) == 0) {
                wifi_hosted_handle_rpc_event(msg_id, payload, payload_len);
            }
        }
        (void)if_num;
    }
    return DRIVER_STATUS_OK;
}

static int16_t wifi_hosted_build_rpc_req(uint32_t msg_id, const uint8_t *submsg, uint16_t submsg_len, uint8_t *out, uint16_t out_max)
{
    uint16_t p = 0;
    uint16_t n = 0;
    uint32_t payload_field_tag = (msg_id << 3U) | 2U;

    if (out == 0U) {
        return -1;
    }

    out[p++] = 0x08;
    n = wifi_hosted_put_varint(RPC_MSG_TYPE_REQ, out + p);
    p = (uint16_t)(p + n);

    out[p++] = 0x10;
    n = wifi_hosted_put_varint(msg_id, out + p);
    p = (uint16_t)(p + n);

    out[p++] = 0x18;
    n = wifi_hosted_put_varint(s_rpc_uid++, out + p);
    p = (uint16_t)(p + n);

    n = wifi_hosted_put_varint(payload_field_tag, out + p);
    p = (uint16_t)(p + n);

    n = wifi_hosted_put_varint(submsg_len, out + p);
    p = (uint16_t)(p + n);

    if ((uint16_t)(p + submsg_len) > out_max) {
        return -1;
    }

    if ((submsg != 0U) && (submsg_len > 0U)) {
        uint16_t i;
        for (i = 0; i < submsg_len; i++) {
            out[p + i] = submsg[i];
        }
    }
    p = (uint16_t)(p + submsg_len);

    return (int16_t)p;
}

static uint16_t wifi_hosted_checksum(const uint8_t *buf, uint16_t len)
{
    uint16_t sum = 0;
    uint16_t i;
    for (i = 0; i < len; i++) {
        sum = (uint16_t)(sum + buf[i]);
    }
    return sum;
}

static void wifi_hosted_build_header(uint8_t *frame, uint8_t if_type, uint8_t if_num, uint16_t payload_len)
{
    esp_payload_header_t *h = (esp_payload_header_t *)frame;
    h->if_type = if_type;
    h->if_num = if_num;
    h->flags = 0;
    h->len = payload_len;
    h->offset = WIFI_HOSTED_HEADER_SIZE;
    h->checksum = 0;
    h->seq_num = s_seq_num++;
    h->throttle_cmd = 0;
    h->reserved2 = 0;
    h->reserved3 = 0;
    h->checksum = wifi_hosted_checksum(frame, (uint16_t)(WIFI_HOSTED_HEADER_SIZE + payload_len));
}

static int8_t wifi_hosted_xfer_frame(const uint8_t *tx, uint8_t *rx)
{
    uint8_t tx_local[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};

    if (tx != 0) {
        uint16_t i;
        for (i = 0; i < WIFI_HOSTED_SPI_FRAME_SIZE; i++) {
            tx_local[i] = tx[i];
        }
    }

    return g_SPI_HOSTED_HAL.transfer(tx_local, rx, WIFI_HOSTED_SPI_FRAME_SIZE, WIFI_HOSTED_SPI_TIMEOUT_MS);
}

static void wifi_hosted_q_reset(void)
{
    s_rsp_head = 0;
    s_rsp_tail = 0;
    s_rsp_count = 0;
    s_evt_head = 0;
    s_evt_tail = 0;
    s_evt_count = 0;
}

static int8_t wifi_hosted_q_push(wifi_rpc_slot_t *q, uint8_t *tail, uint8_t *count, uint8_t if_num, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    if (*count >= WIFI_HOSTED_RPC_QUEUE_DEPTH) {
        return DRIVER_STATUS_TIMEOUT;
    }
    q[*tail].if_num = if_num;
    q[*tail].len = len;
    for (i = 0; i < len; i++) {
        q[*tail].data[i] = data[i];
    }
    *tail = (uint8_t)((*tail + 1U) % WIFI_HOSTED_RPC_QUEUE_DEPTH);
    *count = (uint8_t)(*count + 1U);
    return DRIVER_STATUS_OK;
}

static int16_t wifi_hosted_q_pop(wifi_rpc_slot_t *q, uint8_t *head, uint8_t *count, uint8_t *buf, uint16_t buf_size, uint8_t *if_num)
{
    uint16_t i;
    uint16_t copy_len;
    if (*count == 0U) {
        return 0;
    }
    copy_len = q[*head].len;
    if (copy_len > buf_size) {
        copy_len = buf_size;
    }
    for (i = 0; i < copy_len; i++) {
        buf[i] = q[*head].data[i];
    }
    if (if_num != 0) {
        *if_num = q[*head].if_num;
    }
    *head = (uint8_t)((*head + 1U) % WIFI_HOSTED_RPC_QUEUE_DEPTH);
    *count = (uint8_t)(*count - 1U);
    return (int16_t)copy_len;
}

static int8_t wifi_hosted_dispatch_serial_frame(uint8_t if_num, const uint8_t *payload, uint16_t len)
{
    const char evt_ep[] = "RPCEvt";
    const char rsp_ep[] = "RPCRsp";
    uint16_t p = 0;
    uint16_t ep_len;
    uint16_t i;
    uint8_t is_evt = 0U;
    uint8_t is_rsp = 0U;

    if (len > WIFI_HOSTED_RPC_MAX_PAYLOAD) {
        return DRIVER_STATUS_ERROR;
    }

    /* TLV decode: [type=0x01][len][endpoint][type=0x02][len][data...] */
    if ((len >= 3U) && (payload[p++] == PROTO_PSER_TLV_T_EPNAME)) {
        ep_len = (uint16_t)payload[p] | ((uint16_t)payload[p + 1U] << 8U);
        p = (uint16_t)(p + 2U);
        if ((uint16_t)(p + ep_len) <= len) {
            if (ep_len == (uint16_t)(sizeof(evt_ep) - 1U)) {
                is_evt = 1U;
                for (i = 0; i < ep_len; i++) {
                    if ((char)payload[p + i] != evt_ep[i]) {
                        is_evt = 0U;
                        break;
                    }
                }
            }
            if (ep_len == (uint16_t)(sizeof(rsp_ep) - 1U)) {
                is_rsp = 1U;
                for (i = 0; i < ep_len; i++) {
                    if ((char)payload[p + i] != rsp_ep[i]) {
                        is_rsp = 0U;
                        break;
                    }
                }
            }
        }
    }

    if (is_evt != 0U) {
        return wifi_hosted_q_push(s_evt_q, &s_evt_tail, &s_evt_count, if_num, payload, len);
    }
    if (is_rsp != 0U) {
        return wifi_hosted_q_push(s_rsp_q, &s_rsp_tail, &s_rsp_count, if_num, payload, len);
    }
    /* Fallback to response queue for compatibility */
    return wifi_hosted_q_push(s_rsp_q, &s_rsp_tail, &s_rsp_count, if_num, payload, len);
}

static int8_t wifi_hosted_process_rx_frame(const uint8_t *rx)
{
    const esp_payload_header_t *h = (const esp_payload_header_t *)rx;
    uint16_t len = h->len;
    uint16_t offset = h->offset;
    uint16_t rx_crc = h->checksum;
    uint16_t calc_crc;
    uint8_t tmp[WIFI_HOSTED_SPI_FRAME_SIZE];
    uint16_t i;

    if ((h->if_type == ESP_MAX_IF) || (len == 0U)) {
        return DRIVER_STATUS_TIMEOUT;
    }

    if ((offset != WIFI_HOSTED_HEADER_SIZE) || (len > (WIFI_HOSTED_SPI_FRAME_SIZE - WIFI_HOSTED_HEADER_SIZE))) {
        return DRIVER_STATUS_ERROR;
    }

    for (i = 0; i < WIFI_HOSTED_SPI_FRAME_SIZE; i++) {
        tmp[i] = rx[i];
    }
    ((esp_payload_header_t *)tmp)->checksum = 0;
    calc_crc = wifi_hosted_checksum(tmp, (uint16_t)(offset + len));
    if (calc_crc != rx_crc) {
        return DRIVER_STATUS_ERROR;
    }

    if (h->if_type == ESP_SERIAL_IF) {
        return wifi_hosted_dispatch_serial_frame(h->if_num, rx + offset, len);
    }

    if (h->if_type == ESP_PRIV_IF) {
        return wifi_hosted_parse_init_event(rx);
    }

    return DRIVER_STATUS_TIMEOUT;
}

static int8_t wifi_hosted_parse_init_event(const uint8_t *rx)
{
    const esp_payload_header_t *h = (const esp_payload_header_t *)rx;
    uint16_t len = h->len;
    uint16_t offset = h->offset;
    uint16_t rx_crc = h->checksum;
    uint16_t calc_crc;
    uint8_t tmp[WIFI_HOSTED_SPI_FRAME_SIZE];

    if ((h->if_type == ESP_MAX_IF) || (len == 0U)) {
        return DRIVER_STATUS_TIMEOUT;
    }

    if ((offset != WIFI_HOSTED_HEADER_SIZE) || (len > (WIFI_HOSTED_SPI_FRAME_SIZE - WIFI_HOSTED_HEADER_SIZE))) {
        debug_log(DWARNING, "Hosted RX invalid header: if=%d len=%d off=%d\n", h->if_type, len, offset);
        return DRIVER_STATUS_ERROR;
    }

    {
        uint16_t i;
        for (i = 0; i < WIFI_HOSTED_SPI_FRAME_SIZE; i++) {
            tmp[i] = rx[i];
        }
    }
    ((esp_payload_header_t *)tmp)->checksum = 0;
    calc_crc = wifi_hosted_checksum(tmp, (uint16_t)(offset + len));
    if (calc_crc != rx_crc) {
        debug_log(DWARNING, "Hosted RX checksum mismatch\n");
        return DRIVER_STATUS_ERROR;
    }

    if (h->if_type != ESP_PRIV_IF) {
        debug_log(DAPPEND, "Hosted RX IF=%d (not init event)\n", h->if_type);
        return DRIVER_STATUS_TIMEOUT;
    }

    {
        const esp_priv_event_t *evt = (const esp_priv_event_t *)(rx + offset);
        uint8_t pos = 0;

        if ((len < 2U) || (evt->event_type != ESP_PRIV_EVENT_INIT)) {
            return DRIVER_STATUS_TIMEOUT;
        }

        while ((pos + 1U) < evt->event_len) {
            uint8_t tag = evt->event_data[pos++];
            uint8_t tag_len = evt->event_data[pos++];
            if ((uint8_t)(pos + tag_len) > evt->event_len) {
                break;
            }
            if ((tag == ESP_PRIV_FIRMWARE_CHIP_ID) && (tag_len >= 1U)) {
                g_WIFI_HOSTED_CTX.slave_chip_id = evt->event_data[pos];
            }
            if ((tag == ESP_PRIV_CAPABILITY) && (tag_len >= 1U)) {
                debug_log(DNONE, "ESP hosted cap: 0x%x\n", evt->event_data[pos]);
            }
            pos = (uint8_t)(pos + tag_len);
        }
    }

    g_WIFI_HOSTED_CTX.init_event_received = 1;
    debug_log(DNONE, "Received ESP_PRIV_EVENT_INIT (chip_id=0x%x)\n", g_WIFI_HOSTED_CTX.slave_chip_id);
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_poll_for_init_event(void)
{
    uint8_t tx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    uint8_t rx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};

    wifi_hosted_build_header(tx, ESP_MAX_IF, 0, 0);
    if (wifi_hosted_xfer_frame(tx, rx) != DRIVER_STATUS_OK) {
        return DRIVER_STATUS_TIMEOUT;
    }

    (void)wifi_hosted_process_rx_frame(rx);
    (void)wifi_hosted_consume_events();
    if (g_WIFI_HOSTED_CTX.init_event_received != 0U) {
        return DRIVER_STATUS_OK;
    }
    return DRIVER_STATUS_TIMEOUT;
}

static int8_t wifi_hosted_send_init_config(void)
{
    uint8_t tx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    uint8_t rx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    uint8_t *payload = tx + WIFI_HOSTED_HEADER_SIZE;
    esp_priv_event_t *evt = (esp_priv_event_t *)payload;
    uint8_t *pos = evt->event_data;
    uint8_t data_len = 0;

    evt->event_type = ESP_PRIV_EVENT_INIT;

    *pos++ = HOST_CAPABILITIES;                 *pos++ = 1; *pos++ = 0x00; data_len += 3;
    *pos++ = RCVD_ESP_FIRMWARE_CHIP_ID;         *pos++ = 1; *pos++ = g_WIFI_HOSTED_CTX.slave_chip_id; data_len += 3;
    *pos++ = SLV_CONFIG_TEST_RAW_TP;            *pos++ = 1; *pos++ = 0x00; data_len += 3;
    *pos++ = SLV_CONFIG_THROTTLE_HIGH_THRESHOLD;*pos++ = 1; *pos++ = 80;   data_len += 3;
    *pos++ = SLV_CONFIG_THROTTLE_LOW_THRESHOLD; *pos++ = 1; *pos++ = 60;   data_len += 3;

    evt->event_len = data_len;
    wifi_hosted_build_header(tx, ESP_PRIV_IF, 0, (uint16_t)(2 + data_len));

    if (wifi_hosted_xfer_frame(tx, rx) != DRIVER_STATUS_OK) {
        return DRIVER_STATUS_TIMEOUT;
    }

    debug_log(DNONE, "Sent ESP hosted init config TLVs\n");
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_poll_runtime(void)
{
    uint8_t tx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    uint8_t rx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    GPIO_PinState drdy;

    drdy = HAL_GPIO_ReadPin(g_SPI_HOSTED_HAL.driver->drdy_port, g_SPI_HOSTED_HAL.driver->drdy_pin);
    if (drdy != GPIO_PIN_SET) {
        return DRIVER_STATUS_OK;
    }

    wifi_hosted_build_header(tx, ESP_MAX_IF, 0, 0);
    if (wifi_hosted_xfer_frame(tx, rx) != DRIVER_STATUS_OK) {
        return DRIVER_STATUS_TIMEOUT;
    }
    (void)wifi_hosted_process_rx_frame(rx);
    (void)wifi_hosted_consume_events();
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_set_station_mode(void)
{
    uint8_t rpc_buf[64];
    uint8_t submsg_set_mode[2] = {0x08, WIFI_MODE_STA};
    int16_t rpc_len;

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_SET_WIFI_MODE, submsg_set_mode,
                                        sizeof(submsg_set_mode), rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for SetWifiMode\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for SetWifiMode\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Sent RPC: SetWifiMode(STA)\n");

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_START, 0, 0, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiStart\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiStart\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Sent RPC: WifiStart\n");

    debug_log(DNONE, "Requested slave STA mode + wifi_start via RPC\n");
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_send_wifi_init_rpc(void)
{
    uint8_t cfg[192];
    uint8_t req_submsg[220];
    uint8_t rpc_buf[320];
    uint16_t cfg_len = 0;
    uint16_t req_len = 0;
    int16_t ret;
    uint16_t n;

    /* Build wifi_init_config message with practical defaults. */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 1, 10);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* static_rx_buf_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 2, 32);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* dynamic_rx_buf_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 3, 1);   if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* tx_buf_type: dynamic */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 4, 0);   if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* static_tx_buf_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 5, 32);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* dynamic_tx_buf_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 6, 32);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* cache_tx_buf_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 7, 0);   if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* csi_enable */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 8, 1);   if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* ampdu_rx_enable */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 9, 1);   if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* ampdu_tx_enable */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 10, 1);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* amsdu_tx_enable */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 11, 0);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* nvs_enable */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 12, 0);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* nano_enable */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 13, 6);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* rx_ba_win */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 14, 0);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* wifi_task_core_id */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 15, 752);if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* beacon_max_len */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 16, 32); if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* mgmt_sbuf_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 19, 7);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* espnow_max_encrypt_num */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 20, 0x1F2F3F4FU); if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* magic */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 21, 1);  if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* rx_mgmt_buf_type */
    ret = wifi_hosted_append_field_varint(cfg, cfg_len, sizeof(cfg), 22, 32); if (ret < 0) return DRIVER_STATUS_ERROR; cfg_len = (uint16_t)ret; /* rx_mgmt_buf_num */

    /* Build Rpc_Req_WifiInit submessage: field #1 -> cfg bytes */
    req_submsg[req_len++] = 0x0A; /* field 1, wire type len-delimited */
    n = wifi_hosted_put_varint(cfg_len, req_submsg + req_len);
    req_len = (uint16_t)(req_len + n);
    {
        uint16_t i;
        for (i = 0; i < cfg_len; i++) {
            req_submsg[req_len + i] = cfg[i];
        }
    }
    req_len = (uint16_t)(req_len + cfg_len);

    ret = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_INIT, req_submsg, req_len, rpc_buf, sizeof(rpc_buf));
    if (ret <= 0) {
        debug_log(DERROR, "RPC build failed for WifiInit\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)ret) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiInit\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Sent RPC: WifiInit\n");
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_start_scan_rpc(void)
{
    uint8_t rpc_buf[128];
    int16_t rpc_len;

    /* Empty Rpc_Req_WifiScanStart submessage:
     * config == NULL, config_set == false, block == false -> slave uses default scan config. */
    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_SCAN_START, 0, 0, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiScanStart\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiScanStart\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Sent RPC: WifiScanStart\n");
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_scan_get_ap_num_rpc(void)
{
    uint8_t rpc_buf[128];
    uint8_t resp_payload[64];
    int16_t rpc_len;
    int16_t resp_len;
    uint16_t p = 0;
    uint32_t key;
    uint32_t field;
    uint32_t wire;
    uint32_t v;

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_SCAN_GET_AP_NUM, 0, 0, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiScanGetApNum\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiScanGetApNum\n");
        return DRIVER_STATUS_TIMEOUT;
    }

    resp_len = wifi_hosted_wait_rpc_payload(0x220U, resp_payload, sizeof(resp_payload), 3000);
    if (resp_len <= 0) {
        debug_log(DWARNING, "No/invalid response for WifiScanGetApNum\n");
        return DRIVER_STATUS_TIMEOUT;
    }

    s_last_scan_ap_num = 0;
    while (p < (uint16_t)resp_len) {
        if (wifi_hosted_get_varint(resp_payload, (uint16_t)resp_len, &p, &key) != 0) {
            break;
        }
        field = key >> 3U;
        wire = key & 0x07U;
        if (wire != 0U) {
            break;
        }
        if (wifi_hosted_get_varint(resp_payload, (uint16_t)resp_len, &p, &v) != 0) {
            break;
        }
        if (field == 2U) {
            s_last_scan_ap_num = (uint16_t)v;
        }
    }

    debug_log(DNONE, "Scan AP count: %d\n", s_last_scan_ap_num);
    return DRIVER_STATUS_OK;
}

static void wifi_hosted_log_scan_records(const uint8_t *payload, uint16_t payload_len)
{
    uint16_t p = 0;
    uint32_t key;
    uint32_t field;
    uint32_t wire;
    uint32_t l;
    uint16_t ap_idx = 0;

    while (p < payload_len) {
        if (wifi_hosted_get_varint(payload, payload_len, &p, &key) != 0) {
            break;
        }
        field = key >> 3U;
        wire = key & 0x07U;
        if ((field == 3U) && (wire == 2U)) {
            uint16_t ap_end;
            uint16_t ap_p;
            char ssid[34] = {0};
            int32_t rssi = 0;
            uint32_t channel = 0;
            uint8_t ssid_len = 0;

            if (wifi_hosted_get_varint(payload, payload_len, &p, &l) != 0) {
                break;
            }
            ap_end = (uint16_t)(p + (uint16_t)l);
            ap_p = p;
            while (ap_p < ap_end) {
                uint32_t k2, f2, w2, v2;
                if (wifi_hosted_get_varint(payload, ap_end, &ap_p, &k2) != 0) {
                    break;
                }
                f2 = k2 >> 3U;
                w2 = k2 & 0x07U;
                if ((f2 == 2U) && (w2 == 2U)) {
                    if (wifi_hosted_get_varint(payload, ap_end, &ap_p, &v2) != 0) break;
                    if ((uint16_t)(ap_p + (uint16_t)v2) > ap_end) break;
                    while ((ssid_len < 33U) && (ssid_len < v2)) {
                        ssid[ssid_len] = (char)payload[ap_p + ssid_len];
                        ssid_len++;
                    }
                    ssid[ssid_len] = '\0';
                    ap_p = (uint16_t)(ap_p + (uint16_t)v2);
                } else if (((f2 == 3U) || (f2 == 5U)) && (w2 == 0U)) {
                    if (wifi_hosted_get_varint(payload, ap_end, &ap_p, &v2) != 0) break;
                    if (f2 == 3U) channel = v2;
                    if (f2 == 5U) {
                        if (v2 > 0x7FFFFFFFU) {
                            rssi = (int32_t)(v2 - 0x100000000ULL);
                        } else {
                            rssi = (int32_t)v2;
                        }
                    }
                } else if (w2 == 0U) {
                    if (wifi_hosted_get_varint(payload, ap_end, &ap_p, &v2) != 0) break;
                } else if (w2 == 2U) {
                    if (wifi_hosted_get_varint(payload, ap_end, &ap_p, &v2) != 0) break;
                    ap_p = (uint16_t)(ap_p + (uint16_t)v2);
                } else {
                    break;
                }
            }
            debug_log(DNONE, "AP[%d] SSID='%s' RSSI=%d CH=%d\n", ap_idx, ssid, rssi, channel);
            if (strcmp(ssid, WIFI_HOSTED_SSID) == 0) {
                s_target_ap_found = 1U;
            }
            ap_idx++;
            p = ap_end;
        } else if (wire == 0U) {
            uint32_t tmp;
            if (wifi_hosted_get_varint(payload, payload_len, &p, &tmp) != 0) break;
        } else if (wire == 2U) {
            if (wifi_hosted_get_varint(payload, payload_len, &p, &l) != 0) break;
            p = (uint16_t)(p + (uint16_t)l);
        } else {
            break;
        }
    }
}

static int8_t wifi_hosted_scan_get_ap_records_rpc(void)
{
    uint8_t rpc_buf[160];
    uint8_t req_submsg[8];
    uint8_t resp_payload[WIFI_HOSTED_RPC_MAX_PAYLOAD];
    int16_t rpc_len;
    int16_t resp_len;
    uint16_t req_len = 0;

    req_submsg[req_len++] = 0x08; /* field 1: number */
    req_len = (uint16_t)(req_len + wifi_hosted_put_varint(s_last_scan_ap_num, &req_submsg[req_len]));

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_SCAN_GET_AP_RECORDS,
                                        req_submsg, req_len, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiScanGetApRecords\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiScanGetApRecords\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    resp_len = wifi_hosted_wait_rpc_payload(0x221U, resp_payload, sizeof(resp_payload), 3000);
    if (resp_len <= 0) {
        debug_log(DWARNING, "No/invalid response for WifiScanGetApRecords\n");
        return DRIVER_STATUS_TIMEOUT;
    }

    wifi_hosted_log_scan_records(resp_payload, (uint16_t)resp_len);
    return DRIVER_STATUS_OK;
}

static int8_t wifi_hosted_connect_target_ap_rpc(void)
{
    uint8_t rpc_buf[256];
    uint8_t submsg[200];
    uint8_t cfg[180];
    uint8_t sta[150];
    uint16_t sub_len = 0;
    uint16_t cfg_len = 0;
    uint16_t sta_len = 0;
    uint16_t n;
    int16_t rpc_len;
    const char *ssid = WIFI_HOSTED_SSID;
    const char *pass = WIFI_HOSTED_PASSWORD;
    uint16_t ssid_len = (uint16_t)strlen(ssid);
    uint16_t pass_len = (uint16_t)strlen(pass);
    uint16_t i;

    if (wifi_hosted_send_simple_rpc_and_wait_ack(RPC_MSG_ID_REQ_WIFI_DISCONNECT, "WifiDisconnect", 1500) != DRIVER_STATUS_OK) {
        debug_log(DWARNING, "WifiDisconnect did not ack, continuing\n");
    }

    /* wifi_sta_config: ssid(bytes field 1), password(bytes field 2) */
    sta[sta_len++] = 0x0A;
    n = wifi_hosted_put_varint(ssid_len, &sta[sta_len]);
    sta_len = (uint16_t)(sta_len + n);
    for (i = 0; i < ssid_len; i++) sta[sta_len++] = (uint8_t)ssid[i];

    sta[sta_len++] = 0x12;
    n = wifi_hosted_put_varint(pass_len, &sta[sta_len]);
    sta_len = (uint16_t)(sta_len + n);
    for (i = 0; i < pass_len; i++) sta[sta_len++] = (uint8_t)pass[i];

    /* wifi_config oneof: sta is field 2 */
    cfg[cfg_len++] = 0x12;
    n = wifi_hosted_put_varint(sta_len, &cfg[cfg_len]);
    cfg_len = (uint16_t)(cfg_len + n);
    for (i = 0; i < sta_len; i++) cfg[cfg_len++] = sta[i];

    /* Rpc_Req_WifiSetConfig: iface(field1 varint), cfg(field2 bytes) */
    submsg[sub_len++] = 0x08;
    n = wifi_hosted_put_varint(WIFI_IF_STA, &submsg[sub_len]);
    sub_len = (uint16_t)(sub_len + n);
    submsg[sub_len++] = 0x12;
    n = wifi_hosted_put_varint(cfg_len, &submsg[sub_len]);
    sub_len = (uint16_t)(sub_len + n);
    for (i = 0; i < cfg_len; i++) submsg[sub_len++] = cfg[i];

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_SET_CONFIG, submsg, sub_len, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiSetConfig\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiSetConfig\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Sent RPC: WifiSetConfig SSID='%s'\n", WIFI_HOSTED_SSID);
    debug_log(DNONE, "Waiting for WifiSetConfig response msg_id=0x%x\n",
              (unsigned int)(RPC_MSG_ID_REQ_WIFI_SET_CONFIG + 0x100U));
    if (wifi_hosted_wait_rpc_ack(RPC_MSG_ID_REQ_WIFI_SET_CONFIG + 0x100U, 2000) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "No RPC response for WifiSetConfig\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "WifiSetConfig response received\n");

    if (wifi_hosted_get_config_verify_ssid(WIFI_HOSTED_SSID) != DRIVER_STATUS_OK) {
        debug_log(DWARNING, "Retry path: WifiDisconnect -> WifiSetConfig -> WifiGetConfig verify\n");
        if (wifi_hosted_send_simple_rpc_and_wait_ack(RPC_MSG_ID_REQ_WIFI_DISCONNECT, "WifiDisconnect", 1500) != DRIVER_STATUS_OK) {
            return DRIVER_STATUS_TIMEOUT;
        }
        if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
            debug_log(DERROR, "RPC resend failed for WifiSetConfig\n");
            return DRIVER_STATUS_TIMEOUT;
        }
        if (wifi_hosted_wait_rpc_ack(RPC_MSG_ID_REQ_WIFI_SET_CONFIG + 0x100U, 2000) != DRIVER_STATUS_OK) {
            debug_log(DERROR, "No RPC response for WifiSetConfig retry\n");
            return DRIVER_STATUS_TIMEOUT;
        }
        if (wifi_hosted_get_config_verify_ssid(WIFI_HOSTED_SSID) != DRIVER_STATUS_OK) {
            debug_log(DWARNING, "Fallback path: WifiStop -> WifiStart -> WifiSetConfig -> verify\n");
            if (wifi_hosted_send_simple_rpc_and_wait_ack(RPC_MSG_ID_REQ_WIFI_STOP, "WifiStop", 1500) != DRIVER_STATUS_OK) {
                return DRIVER_STATUS_TIMEOUT;
            }
            if (wifi_hosted_send_simple_rpc_and_wait_ack(RPC_MSG_ID_REQ_WIFI_START, "WifiStart", 1500) != DRIVER_STATUS_OK) {
                return DRIVER_STATUS_TIMEOUT;
            }
            if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
                debug_log(DERROR, "RPC resend failed for WifiSetConfig after stop/start\n");
                return DRIVER_STATUS_TIMEOUT;
            }
            if (wifi_hosted_wait_rpc_ack(RPC_MSG_ID_REQ_WIFI_SET_CONFIG + 0x100U, 2000) != DRIVER_STATUS_OK) {
                debug_log(DERROR, "No RPC response for WifiSetConfig after stop/start\n");
                return DRIVER_STATUS_TIMEOUT;
            }
            if (wifi_hosted_get_config_verify_ssid(WIFI_HOSTED_SSID) != DRIVER_STATUS_OK) {
                debug_log(DERROR, "WifiGetConfig verify still failed after stop/start fallback\n");
                return DRIVER_STATUS_ERROR;
            }
        }
    }
    delay_ms(WIFI_SET_CONFIG_TO_CONNECT_DELAY_MS);

    rpc_len = wifi_hosted_build_rpc_req(RPC_MSG_ID_REQ_WIFI_CONNECT, 0, 0, rpc_buf, sizeof(rpc_buf));
    if (rpc_len <= 0) {
        debug_log(DERROR, "RPC build failed for WifiConnect\n");
        return DRIVER_STATUS_ERROR;
    }
    if (wifi_hosted_rpc_send(0, rpc_buf, (uint16_t)rpc_len) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "RPC send failed for WifiConnect\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "Sent RPC: WifiConnect\n");
    debug_log(DNONE, "Waiting for WifiConnect response msg_id=0x%x\n",
              (unsigned int)(RPC_MSG_ID_REQ_WIFI_CONNECT + 0x100U));
    if (wifi_hosted_wait_rpc_ack(RPC_MSG_ID_REQ_WIFI_CONNECT + 0x100U, 3000) != DRIVER_STATUS_OK) {
        debug_log(DERROR, "No RPC response for WifiConnect\n");
        return DRIVER_STATUS_TIMEOUT;
    }
    debug_log(DNONE, "WifiConnect response received\n");
    return DRIVER_STATUS_OK;
}


/* Exported variables ****************************************************** */
WIFI_HOSTED_CTX g_WIFI_HOSTED_CTX =
{
    .state = WIFI_HOSTED_STATE_IDLE,
    .last_tick = 0,
    .init_event_received = 0,
    .slave_chip_id = 0xFF,
};

/* Exported functions ****************************************************** */
int8_t wifi_init(void)
{
    if (g_SPI_HOSTED_HAL.init() != DRIVER_STATUS_OK) {
        g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
        return DRIVER_STATUS_ERROR;
    }

    g_SPI_HOSTED_HAL.reset_slave();
    debug_log(DNONE, "Waiting %d ms after ESP reset...\n", WIFI_HOSTED_POST_RESET_DELAY_MS);
    delay_ms(WIFI_HOSTED_POST_RESET_DELAY_MS);

    g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_WAIT_INIT_EVENT;
    g_WIFI_HOSTED_CTX.last_tick = HAL_GetTick();
    g_WIFI_HOSTED_CTX.init_event_received = 0;
    g_WIFI_HOSTED_CTX.slave_chip_id = 0xFF;
    wifi_hosted_q_reset();
    return DRIVER_STATUS_OK;
}

void wifi_task(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - g_WIFI_HOSTED_CTX.last_tick) < WIFI_HOSTED_TASK_PERIOD_MS) {
        return;
    }
    g_WIFI_HOSTED_CTX.last_tick = now;
    (void)wifi_hosted_consume_events();

    switch (g_WIFI_HOSTED_CTX.state) {
        case WIFI_HOSTED_STATE_WAIT_INIT_EVENT:
            if (wifi_hosted_poll_for_init_event() == DRIVER_STATUS_OK) {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_SEND_INIT_CONFIG;
            }
            break;

        case WIFI_HOSTED_STATE_SEND_INIT_CONFIG:
            if (wifi_hosted_send_init_config() == DRIVER_STATUS_OK) {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_WIFI_INIT;
                debug_log(DNONE, "ESP-hosted transport init complete\n");
            } else {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
            }
            break;

        case WIFI_HOSTED_STATE_WIFI_INIT:
            if (s_sta_connected_event != 0U) {
                debug_log(DNONE, "STA already connected on slave, skipping WifiInit/scan/connect sequence\n");
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
                break;
            }
            if (wifi_hosted_send_wifi_init_rpc() == DRIVER_STATUS_OK) {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_SET_STA_MODE;
            } else {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
            }
            break;

        case WIFI_HOSTED_STATE_SET_STA_MODE:
            if (s_sta_connected_event != 0U) {
                debug_log(DNONE, "STA already connected on slave, skipping SetWifiMode/WifiStart\n");
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
                break;
            }
            if (wifi_hosted_set_station_mode() == DRIVER_STATUS_OK) {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_SCAN_START;
            } else {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
            }
            break;

        case WIFI_HOSTED_STATE_SCAN_START:
            if (s_sta_connected_event != 0U) {
                debug_log(DNONE, "STA already connected on slave, skipping scan start\n");
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
                break;
            }
            if (wifi_hosted_start_scan_rpc() == DRIVER_STATUS_OK) {
                s_scan_done_event = 0U;
                s_target_ap_found = 0U;
                s_scan_start_tick = HAL_GetTick();
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_WAIT_SCAN_DONE;
            } else {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
            }
            break;

        case WIFI_HOSTED_STATE_WAIT_SCAN_DONE:
            (void)wifi_hosted_poll_runtime();
            if (s_sta_connected_event != 0U) {
                debug_log(DNONE, "STA already connected, skipping scan fetch and moving to transport-ready\n");
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
                break;
            }
            if ((s_scan_done_event != 0U) &&
                ((HAL_GetTick() - s_scan_start_tick) >= WIFI_HOSTED_SCAN_MIN_DURATION_MS)) {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_SCAN_GET_NUM;
            } else if ((HAL_GetTick() - s_scan_start_tick) >= WIFI_HOSTED_SCAN_WAIT_TIMEOUT_MS) {
                debug_log(DWARNING, "Scan wait timeout, continuing to fetch results\n");
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_SCAN_GET_NUM;
            }
            break;

        case WIFI_HOSTED_STATE_SCAN_GET_NUM:
            if (s_sta_connected_event != 0U) {
                debug_log(DNONE, "STA connected while waiting scan count, moving to transport-ready\n");
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
                break;
            }
            if (wifi_hosted_scan_get_ap_num_rpc() == DRIVER_STATUS_OK) {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_SCAN_GET_RECORDS;
            } else {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
            }
            break;

        case WIFI_HOSTED_STATE_SCAN_GET_RECORDS:
            if (s_last_scan_ap_num > 0U) {
                if (wifi_hosted_scan_get_ap_records_rpc() != DRIVER_STATUS_OK) {
                    g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
                    break;
                }
            } else {
                debug_log(DNONE, "No APs found in scan\n");
            }
            if (s_target_ap_found != 0U) {
                debug_log(DNONE, "Target SSID '%s' found, connecting...\n", WIFI_HOSTED_SSID);
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_CONNECT_TARGET;
            } else {
                debug_log(DNONE, "Target SSID '%s' not found\n", WIFI_HOSTED_SSID);
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
            }
            break;

        case WIFI_HOSTED_STATE_CONNECT_TARGET:
            if (wifi_hosted_connect_target_ap_rpc() == DRIVER_STATUS_OK) {
                s_sta_connected_event = 0U;
                s_sta_got_ip = 0U;
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_TRANSPORT_READY;
            } else {
                g_WIFI_HOSTED_CTX.state = WIFI_HOSTED_STATE_ERROR;
            }
            break;

        case WIFI_HOSTED_STATE_TRANSPORT_READY:
            (void)wifi_hosted_poll_runtime();
            (void)wifi_hosted_poll_dhcp_dns_status_rpc();
        case WIFI_HOSTED_STATE_IDLE:
        case WIFI_HOSTED_STATE_ERROR:
        default:
            break;
    }
}

int8_t wifi_hosted_rpc_send(uint8_t if_num, const uint8_t *payload, uint16_t len)
{
    uint8_t tx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    uint8_t rx[WIFI_HOSTED_SPI_FRAME_SIZE] = {0};
    uint16_t i;
    uint16_t p = 0;
    uint16_t ep_len = (uint16_t)(sizeof(RPC_EP_NAME_RSP) - 1U);
    uint16_t tlv_len;

    if ((g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_WIFI_INIT) &&
        (g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_TRANSPORT_READY) &&
        (g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_SET_STA_MODE) &&
        (g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_SCAN_START) &&
        (g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_SCAN_GET_NUM) &&
        (g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_SCAN_GET_RECORDS) &&
        (g_WIFI_HOSTED_CTX.state != WIFI_HOSTED_STATE_CONNECT_TARGET)) {
        return DRIVER_STATUS_ERROR;
    }
    if ((payload == 0) || (len == 0U)) {
        return DRIVER_STATUS_ERROR;
    }

    /* protocomm_pserial expects TLV format on ESP_SERIAL_IF:
     * [type=0x01][len][endpoint][type=0x02][len][data] */
    tlv_len = (uint16_t)(1U + 2U + ep_len + 1U + 2U + len);
    if (tlv_len > (WIFI_HOSTED_SPI_FRAME_SIZE - WIFI_HOSTED_HEADER_SIZE)) {
        return DRIVER_STATUS_ERROR;
    }

    tx[WIFI_HOSTED_HEADER_SIZE + p++] = PROTO_PSER_TLV_T_EPNAME;
    tx[WIFI_HOSTED_HEADER_SIZE + p++] = (uint8_t)(ep_len & 0xFFU);
    tx[WIFI_HOSTED_HEADER_SIZE + p++] = (uint8_t)((ep_len >> 8) & 0xFFU);
    for (i = 0; i < ep_len; i++) {
        tx[WIFI_HOSTED_HEADER_SIZE + p++] = (uint8_t)RPC_EP_NAME_RSP[i];
    }
    tx[WIFI_HOSTED_HEADER_SIZE + p++] = PROTO_PSER_TLV_T_DATA;
    tx[WIFI_HOSTED_HEADER_SIZE + p++] = (uint8_t)(len & 0xFFU);
    tx[WIFI_HOSTED_HEADER_SIZE + p++] = (uint8_t)((len >> 8) & 0xFFU);
    for (i = 0; i < len; i++) {
        tx[WIFI_HOSTED_HEADER_SIZE + p++] = payload[i];
    }

    wifi_hosted_build_header(tx, ESP_SERIAL_IF, (uint8_t)(if_num & 0x0F), tlv_len);

    if (wifi_hosted_xfer_frame(tx, rx) != DRIVER_STATUS_OK) {
        return DRIVER_STATUS_TIMEOUT;
    }

    (void)wifi_hosted_process_rx_frame(rx);
    return DRIVER_STATUS_OK;
}

int16_t wifi_hosted_rpc_recv_rsp(uint8_t *buf, uint16_t buf_size, uint8_t *if_num)
{
    if ((buf == 0) || (buf_size == 0U)) {
        return -1;
    }
    return wifi_hosted_q_pop(s_rsp_q, &s_rsp_head, &s_rsp_count, buf, buf_size, if_num);
}

int16_t wifi_hosted_rpc_recv_evt(uint8_t *buf, uint16_t buf_size, uint8_t *if_num)
{
    if ((buf == 0) || (buf_size == 0U)) {
        return -1;
    }
    return wifi_hosted_q_pop(s_evt_q, &s_evt_head, &s_evt_count, buf, buf_size, if_num);
}
