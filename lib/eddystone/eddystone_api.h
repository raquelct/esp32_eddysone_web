/**
 * @file eddystone_api.h
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file contains eddystone ralated functions.
 * @version 1.0
 * @date 2020-03-13
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __EDDYSTONE_API_H__
#define __EDDYSTONE_API_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "eddystone_protocol.h"

#include "esp_log.h"

#define MAX_STRING_SIZE 50

typedef struct {
    struct {
        uint8_t   flags;          /*<! AD flags data */
        uint16_t  srv_uuid;       /*<! complete list of 16-bit service uuid*/
        uint16_t  srv_data_type;  /*<! service data type */
        uint8_t   frame_type;     /*<! Eddystone UID, URL or TLM */
    } common;
    union {
        struct {
            /*<! Eddystone-UID */
            int8_t  ranging_data;     /*<! calibrated Tx power at 0m */
            uint8_t namespace_id[10];
            uint8_t instance_id[6];
        } uid;
        struct {
            /*<! Eddystone-URL */
            int8_t  tx_power;                    /*<! calibrated Tx power at 0m */
            char    url[EDDYSTONE_URL_MAX_LEN];  /*<! the decoded URL */
        } url;
        struct {
            /*<! Eddystone-TLM */
            uint8_t   version;           /*<! TLM version,0x00 for now */
            uint16_t  battery_voltage;   /*<! battery voltage in mV */
            float     temperature;       /*<! beacon temperature in degrees Celsius */
            uint32_t  adv_count;         /*<! adv pdu count since power-up */
            uint32_t  time;              /*<! time since power-up, a 0.1 second resolution counter */
        } tlm;
    } inform;
} esp_eddystone_result_t;

/* Static variables */ 
static const char* EDDY_TAG = "EDDYSTONE";

/* BLE Scan params */
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x40,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

/* Eddystone-URL scheme prefixes */
static const char* eddystone_url_prefix[4] = {
    "http://www.",
    "https://www.",
    "http://",
    "https://"
};

/* Eddystone-URL HTTP URL encoding */
static const char* eddystone_url_encoding[14] = {
    ".com/",
    ".org/",
    ".edu/",
    ".net/",
    ".info/",
    ".biz/",
    ".gov/",
    ".com",
    ".org",
    ".edu",
    ".net",
    ".info",
    ".biz",
    ".gov"
 };

/* Public Global Variables */
uint8_t eddy_common_received;       /* flag to common eddystone frame received */
uint8_t eddy_url_received;          /* flag to url eddystone frame received */
uint8_t eddy_tlm_received;          /* flag to tlm eddystone frame received */
char eddy_namespace_id[MAX_STRING_SIZE];         /* device uuid name */
char eddy_instance_id[MAX_STRING_SIZE];          /* device uuid mac */
char eddy_tx_power[MAX_STRING_SIZE];             /* calibrated Tx power at 0m */
char eddy_url[MAX_STRING_SIZE];                  /* the decoded URL */
char eddy_tlm_version[MAX_STRING_SIZE];          /* TLM version,0x00 for now */
char eddy_tlm_battery_voltage[MAX_STRING_SIZE];  /* battery voltage in mV */
char eddy_tlm_temperature[MAX_STRING_SIZE];      /* beacon temperature in degrees Celsius */
char eddy_tlm_adv_count[MAX_STRING_SIZE];        /* adv pdu count since power-up */
char eddy_tlm_time[MAX_STRING_SIZE];             /* time since power-up, a 0.1 second resolution counter */

/* Utils */
static inline uint16_t little_endian_read_16(const uint8_t *buffer, uint8_t pos)
{
    return ((uint16_t)buffer[pos]) | (((uint16_t)buffer[(pos)+1]) << 8);
}

static inline uint16_t big_endian_read_16(const uint8_t *buffer, uint8_t pos)
{
    return (((uint16_t)buffer[pos]) << 8) | ((uint16_t)buffer[(pos)+1]);
}

static inline uint32_t big_endian_read_32(const uint8_t *buffer, uint8_t pos)
{
    return (((uint32_t)buffer[pos]) << 24) | (((uint32_t)buffer[(pos)+1]) << 16) | (((uint32_t)buffer[(pos)+2]) << 8) | ((uint32_t)buffer[(pos)+3]);
}

/* Static functions */
static esp_err_t esp_eddystone_decode(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res);
static esp_err_t esp_eddystone_uid_received(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res);
static esp_err_t esp_eddystone_url_received(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res);
static char* esp_eddystone_resolve_url_scheme(const uint8_t* url_start, const uint8_t* url_end);
static esp_err_t esp_eddystone_tlm_received(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res);
static esp_err_t esp_eddystone_get_inform(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res);
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
static void esp_eddystone_show_inform(const esp_eddystone_result_t* res);

/* Public funtions */ 
void esp_eddystone_init(void);

#endif /* __EDDYSTONE_API_H__ */