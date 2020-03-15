/**
 * @file webserver.h
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file contains webserver ralated functions.
 * @version 1.0
 * @date 2020-03-13
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "spiffs.h"
#include "eddystone_api.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

/* WIFI parametrs (Change to match you network) */
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASS"

/* HTML placeholders defines */
#define NAME_PLACEHOLDER "%NAME%"
#define MAC_PLACEHOLDER "%MAC%"
#define RSSI_PLACEHOLDER "%RSSI%"
#define VER_PLACEHOLDER "%VER%"
#define URL_PLACEHOLDER "%URL%"
#define BAT_PLACEHOLDER "%BAT%"
#define TEMP_PLACEHOLDER "%TEMP%"
#define ADV_PLACEHOLDER "%ADV%"
#define TIME_PLACEHOLDER "%TIME%"

/* Static variables */
static const char *WEB_TAG = "WEB SERVER";
static const char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

/* Public Global Variables */
uint8_t wifi_got_ip;

/* Public functions */
void esp_webserver_wifi_init(void);
void esp_webserver_create_task(void);

#endif /* __WEBSERVER_H__ */