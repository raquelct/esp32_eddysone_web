/**
 * @file webserver.c
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file contains webserver ralated functions.
 * @version 1.0
 * @date 2020-03-13
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "webserver.h"

uint8_t wifi_got_ip = 0;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

/**
 * @brief Handles the wifi events
 * 
 * @param ctx 
 * @param event
 * @return esp_err_t - Error code
 */
static esp_err_t esp_webserver_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        wifi_got_ip = 1;
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        wifi_got_ip = 0;
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/**
 * @brief Initialize the wifi connection
 * 
 */
void esp_webserver_wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(esp_webserver_event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

/**
 * @brief Format an html string replacing an existing placeholder for a giving value
 * 
 * @param buffer - The html string 
 * @param placeholder - The existing placeholder on the htm string
 * @param value - The value that will replace the placeholder
 * @return char* - The new html string
 */
char* esp_webserver_format_html(char* buffer, char* placeholder, char* value) {
    char *result; 
    int i, cnt = 0; 
    int value_len = strlen(value); 
    int placeholder_len = strlen(placeholder); 

    ESP_LOGI(WEB_TAG, "Replacing: %s For: %s", placeholder, value);
  
    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; buffer[i] != '\0'; i++) 
    { 
        if (strstr(&buffer[i], placeholder) == &buffer[i]) 
        { 
            cnt++; 
  
            // Jumping to index after the old word. 
            i += placeholder_len - 1; 
        } 
    } 
  
    // Making new string of enough length 
    result = (char *)malloc(i + cnt * (value_len - placeholder_len) + 1); 
  
    i = 0; 
    while (*buffer) 
    { 
        // compare the substring with the result 
        if (strstr(buffer, placeholder) == buffer) 
        { 
            strcpy(&result[i], value); 
            i += value_len; 
            buffer += placeholder_len; 
        } 
        else
            result[i++] = *buffer++; 
    } 
  
    result[i] = '\0'; 

    return result;
}

/**
 * @brief Handles the HTTP requests
 * 
 * @param conn - netconn struct
 */
static void esp_webserver_netconn_serve(struct netconn *conn)
{
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    err_t err;
    char* html_file;
    char* css_file;

    /* Read the data from the port, blocking if nothing yet there.
    We assume the request (the part we care about) is in one netbuf */
    err = netconn_recv(conn, &inbuf);

    if (err == ERR_OK) {
      netbuf_data(inbuf, (void**)&buf, &buflen);
      ESP_LOGI(WEB_TAG, "%s", buf);
      esp_spiffs_init();

      /* Is this an HTTP GET command? (only check the first 6 chars)*/
      if (!strncmp(buf, "GET / ", 6)) {
        /* Send the HTML header
              * subtract 1 from the size, since we dont send the \0 in the string
              * NETCONN_NOCOPY: our data is const static, so no need to copy it
        */
        netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);

        /* Send our HTML file */
        html_file = esp_spiffs_read_file("/spiffs/index.html");

        if (eddy_common_received) {
            html_file = esp_webserver_format_html(html_file, NAME_PLACEHOLDER, &eddy_namespace_id);
            html_file = esp_webserver_format_html(html_file, MAC_PLACEHOLDER, &eddy_instance_id);
        } else {
            html_file = esp_webserver_format_html(html_file, NAME_PLACEHOLDER, "NOT FOUND");
            html_file = esp_webserver_format_html(html_file, MAC_PLACEHOLDER, "NOT FOUND");
        }
        
        if (eddy_url_received) {
            html_file = esp_webserver_format_html(html_file, RSSI_PLACEHOLDER, &eddy_tx_power);
            html_file = esp_webserver_format_html(html_file, URL_PLACEHOLDER, &eddy_url);
        } else {
            html_file = esp_webserver_format_html(html_file, RSSI_PLACEHOLDER, "NOT FOUND");
            html_file = esp_webserver_format_html(html_file, URL_PLACEHOLDER, "NOT FOUND");
        }

        if (eddy_tlm_received) {
            html_file = esp_webserver_format_html(html_file, VER_PLACEHOLDER, &eddy_tlm_version);
            html_file = esp_webserver_format_html(html_file, BAT_PLACEHOLDER, &eddy_tlm_battery_voltage);
            html_file = esp_webserver_format_html(html_file, TEMP_PLACEHOLDER, &eddy_tlm_temperature);
            html_file = esp_webserver_format_html(html_file, ADV_PLACEHOLDER, &eddy_tlm_adv_count);
            html_file = esp_webserver_format_html(html_file, TIME_PLACEHOLDER, &eddy_tlm_time);
        } else {
            html_file = esp_webserver_format_html(html_file, VER_PLACEHOLDER, "NOT FOUND");
            html_file = esp_webserver_format_html(html_file, BAT_PLACEHOLDER, "NOT FOUND");
            html_file = esp_webserver_format_html(html_file, TEMP_PLACEHOLDER, "NOT FOUND");
            html_file = esp_webserver_format_html(html_file, ADV_PLACEHOLDER, "NOT FOUND");
            html_file = esp_webserver_format_html(html_file, TIME_PLACEHOLDER, "NOT FOUND");
        }

        netconn_write(conn, html_file, strlen(html_file), NETCONN_NOCOPY);
      } 
      else if(!strncmp(buf, "GET /style.css", 14)) {
        /* Send our CSS file */
        css_file = esp_spiffs_read_file("/spiffs/style.css");
        netconn_write(conn, css_file, strlen(css_file), NETCONN_NOCOPY);
      }
      esp_spiffs_unmount();
    }
    /* Close the connection (server closes in HTTP) */
    netconn_close(conn);

    /* Delete the buffer (netconn_recv gives us ownership,
    so we have to make sure to deallocate the buffer) */
    netbuf_delete(inbuf);
}

/**
 * @brief Start the HTTP server
 * 
 * @param pvParameters 
 */
static void esp_webserver_http_server(void *pvParameters)
{
    struct netconn *conn, *newconn;
    err_t err;
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, 80);
    netconn_listen(conn);
    do {
      err = netconn_accept(conn, &newconn);
      if (err == ERR_OK) {
        esp_webserver_netconn_serve(newconn);
        netconn_delete(newconn);
      }
    } while(err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
}

void esp_webserver_create_task(void)
{
    xTaskCreate(&esp_webserver_http_server, "esp_webserver_http_server", 8192, NULL, 5, NULL);
}