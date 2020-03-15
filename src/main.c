/**
 * @file main.c
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file is the main application for a ESP32 BLE eddystone receiver and WebServer DataLogger
 * @version 1.0
 * @date 2020-03-13
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "nvs_flash.h"
#include "esp_log.h"

#include "eddystone_api.h"
#include "webserver.h"


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    system_init();

    esp_webserver_wifi_init();
    esp_webserver_create_task(); 

    while(!wifi_got_ip){ // wait for esp to connect to wifi and get ip
        sys_delay_ms(100);
    } 

    esp_eddystone_init();
}