/**
 * @file eddystone_api.c
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file contains eddystone ralated functions.
 * @version 1.0
 * @date 2020-03-13
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "eddystone_api.h"

uint8_t eddy_common_received = 0;
uint8_t eddy_url_received = 0;
uint8_t eddy_tlm_received = 0;

/**
 * @brief Decode and store received UID 
    ****************** Eddystone-UID **************
    Byte offset	    Field	       Description
    0	      Frame Type	   Value = 0x00
    1	     Ranging Data	   Calibrated Tx power at 0 m
    2	       NID[0]	       10-byte Namespace
    3	       NID[1]	
    4	       NID[2]	
    5	       NID[3]	
    6	       NID[4]	
    7	       NID[5]	
    8	       NID[6]	
    9	       NID[7]	
    10	       NID[8]	
    11	       NID[9]	
    12	       BID[0]	       6-byte Instance
    13	       BID[1]	
    14	       BID[2]	
    15	       BID[3]	
    16	       BID[4]	
    17	       BID[5]	
    18	        RFU	           Reserved for future use, must be0x00
    19	        RFU	           Reserved for future use, must be0x00
    ********************************************
 * @param buf 
 * @param len 
 * @param res 
 * @return esp_err_t 
 */
static esp_err_t esp_eddystone_uid_received(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res)
{
    uint8_t pos = 0;
    //1-byte Ranging Data + 10-byte Namespace + 6-byte Instance
    if((len != EDDYSTONE_UID_DATA_LEN) && (len != (EDDYSTONE_UID_RFU_LEN+EDDYSTONE_UID_DATA_LEN))) {
        //ERROR:uid len wrong
        return -1;
    }
    res->inform.uid.ranging_data = buf[pos++];
    for(int i=0; i<EDDYSTONE_UID_NAMESPACE_LEN; i++) {
        res->inform.uid.namespace_id[i] = buf[pos++];
    }
    for(int i=0; i<EDDYSTONE_UID_INSTANCE_LEN; i++) {
        res->inform.uid.instance_id[i] = buf[pos++];
    }
    return 0;
}

/**
 * @brief Resolve received URL to url_res pointer
 * 
 * @param url_start 
 * @param url_end 
 * @return char* 
 */
static char* esp_eddystone_resolve_url_scheme(const uint8_t *url_start, const uint8_t *url_end)
{
    int pos = 0;
    static char url_buf[100] = {0};
    const uint8_t *p = url_start;

    pos += sprintf(&url_buf[pos], "%s", eddystone_url_prefix[*p++]);

    for (; p <= url_end; p++) {
        if (esp_eddystone_is_char_invalid((*p))) {
            pos += sprintf(&url_buf[pos], "%s", eddystone_url_encoding[*p]);
        } else {
            pos += sprintf(&url_buf[pos], "%c", *p);
        }
    }
    return url_buf;
}


/**
 * @brief decode and store received URL, the pointer url_res points to the resolved url
 *  ************************** Eddystone-URL *************
    Frame Specification
    Byte offset	 Field	       Description
        0	       Frame Type	    Value = 0x10
        1	        TX Power	 Calibrated Tx power at 0 m
        2	       URL Scheme	   Encoded Scheme Prefix
        3+	       Encoded URL	    Length 1-17
    *******************************************************
 * @param buf 
 * @param len 
 * @param res 
 * @return esp_err_t 
 */
static esp_err_t esp_eddystone_url_received(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res)
{
    char *url_res = NULL;
    uint8_t pos = 0;
    if(len-EDDYSTONE_URL_TX_POWER_LEN > EDDYSTONE_URL_MAX_LEN) {
        //ERROR:too long url
        return -1;
    }
    res->inform.url.tx_power = buf[pos++];   
    url_res = esp_eddystone_resolve_url_scheme(buf+pos, buf+len-1);
    memcpy(&res->inform.url.url, url_res, strlen(url_res));
    res->inform.url.url[strlen(url_res)] = '\0';
    return 0;
}


/**
 * @brief decode and store received TLM 
 *  ****************** eddystone-tlm ***************
 *  Unencrypted TLM Frame Specification
    Byte offset	       Field	     Description
        0	          Frame Type	 Value = 0x20
        1	           Version	     TLM version, value = 0x00
        2	           VBATT[0]	     Battery voltage, 1 mV/bit
        3	           VBATT[1]	
        4	           TEMP[0]	     Beacon temperature
        5	           TEMP[1]	
        6	          ADV_CNT[0]	 Advertising PDU count
        7	          ADV_CNT[1]	
        8	          ADV_CNT[2]	
        9	          ADV_CNT[3]	
        10	          SEC_CNT[0]	 Time since power-on or reboot
        11	          SEC_CNT[1]	
        12	          SEC_CNT[2]	
        13	          SEC_CNT[3]	
    ************************************************
 * @param buf 
 * @param len 
 * @param res 
 * @return esp_err_t 
 */
static esp_err_t esp_eddystone_tlm_received(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res)
{
    uint8_t pos = 0;
    if(len > EDDYSTONE_TLM_DATA_LEN) {
        //ERROR:TLM too long
        return -1;
    }
    res->inform.tlm.version = buf[pos++];
    res->inform.tlm.battery_voltage = big_endian_read_16(buf, pos);
    pos += 2;
    uint16_t temp = big_endian_read_16(buf, pos);
    int8_t temp_integral = (int8_t)((temp >> 8) & 0xff);
    float temp_decimal = (temp & 0xff) / 256.0;
    res->inform.tlm.temperature = temp_integral + temp_decimal;
    pos += 2;
    res->inform.tlm.adv_count = big_endian_read_32(buf, pos);
    pos += 4;
    res->inform.tlm.time = big_endian_read_32(buf, pos);
    return 0;
}

/**
 * @brief Read the Eddystone information on the main struct
 * 
 * @param buf 
 * @param len 
 * @param res 
 * @return esp_err_t 
 */
static esp_err_t esp_eddystone_get_inform(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res)
{
    static esp_err_t ret=-1;
    switch(res->common.frame_type)
    {
        case EDDYSTONE_FRAME_TYPE_UID: {
            ret = esp_eddystone_uid_received(buf, len, res);
            break;
        }
        case EDDYSTONE_FRAME_TYPE_URL: {
            ret = esp_eddystone_url_received(buf, len, res);
            break;
        }
        case EDDYSTONE_FRAME_TYPE_TLM: {
            ret = esp_eddystone_tlm_received(buf, len, res);
            break;
        }
        default:
            break;
    }
    return ret;
}

/**
 * @brief This function is called to decode eddystone information from adv_data. 
 *        The res points to the result struct.
 * @param buf 
 * @param len 
 * @param res 
 * @return esp_err_t 
 */
static esp_err_t esp_eddystone_decode(const uint8_t* buf, uint8_t len, esp_eddystone_result_t* res)
{
    if (len == 0 || buf == NULL || res == NULL) {
        return -1;
    }
    uint8_t pos=0;
    while(res->common.srv_data_type != EDDYSTONE_SERVICE_UUID) 
    {
        pos++;
        if(pos >= len ) { 
            return -1;
        }
        uint8_t ad_type = buf[pos++];
        switch(ad_type) 
        {
            case ESP_BLE_AD_TYPE_FLAG: {
                res->common.flags = buf[pos++];
                break;
            }
            case ESP_BLE_AD_TYPE_16SRV_CMPL: {
                uint16_t uuid = little_endian_read_16(buf, pos);
                if(uuid != EDDYSTONE_SERVICE_UUID) {
                    return -1; 
                }
                res->common.srv_uuid = uuid;
                pos += 2;
                break;
            }
            case ESP_BLE_AD_TYPE_SERVICE_DATA: {
                uint16_t type = little_endian_read_16(buf, pos);
                pos += 2;
                uint8_t frame_type = buf[pos++];
                if(type != EDDYSTONE_SERVICE_UUID || !(frame_type == EDDYSTONE_FRAME_TYPE_UID || frame_type == EDDYSTONE_FRAME_TYPE_URL || 
                   frame_type == EDDYSTONE_FRAME_TYPE_TLM)) {
                    return -1;
                }
                res->common.srv_data_type = type;
                res->common.frame_type = frame_type;
                break;
            }
            default:
                break;
        }
    }
    return esp_eddystone_get_inform(buf+pos, len-pos, res);
}

/**
 * @brief Log the result stuct
 * 
 * @param res 
 */
static void esp_eddystone_show_inform(const esp_eddystone_result_t* res)
{
    switch(res->common.frame_type)
    {
        case EDDYSTONE_FRAME_TYPE_UID: {
            eddy_common_received = 1;
            sprintf(eddy_namespace_id, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", 
            res->inform.uid.namespace_id[0], res->inform.uid.namespace_id[1], res->inform.uid.namespace_id[2],
            res->inform.uid.namespace_id[3], res->inform.uid.namespace_id[4], res->inform.uid.namespace_id[5],
            res->inform.uid.namespace_id[6], res->inform.uid.namespace_id[7], res->inform.uid.namespace_id[8], 
            res->inform.uid.namespace_id[9]);
            sprintf(eddy_instance_id, "%02X:%02X:%02X:%02X:%02X:%02X", 
            res->inform.uid.instance_id[0], res->inform.uid.instance_id[1], res->inform.uid.instance_id[2],
            res->inform.uid.instance_id[3], res->inform.uid.instance_id[4],res->inform.uid.instance_id[5]);

            ESP_LOGI(EDDY_TAG, "Eddystone UID inform:");
            ESP_LOGI(EDDY_TAG, "Measured power(RSSI at 0m distance):%d dbm", res->inform.uid.ranging_data);
            ESP_LOGI(EDDY_TAG, "Namespace ID: %s", eddy_namespace_id);
            ESP_LOGI(EDDY_TAG, "Instance ID: %s", eddy_instance_id);
            break;
        }
        case EDDYSTONE_FRAME_TYPE_URL: {
            eddy_url_received = 1;
            sprintf(eddy_tx_power, "%d dbm", res->inform.url.tx_power);
            sprintf(eddy_url, "%s", res->inform.url.url);

            ESP_LOGI(EDDY_TAG, "Eddystone URL inform:");
            ESP_LOGI(EDDY_TAG, "Measured power(RSSI at 0m distance):%s", eddy_tx_power);
            ESP_LOGI(EDDY_TAG, "URL: %s", eddy_url);
            break;
        }
        case EDDYSTONE_FRAME_TYPE_TLM: {
            eddy_tlm_received = 1;
            sprintf(eddy_tlm_version, "%d", res->inform.tlm.version);
            sprintf(eddy_tlm_battery_voltage, "%d mV", res->inform.tlm.battery_voltage);
            sprintf(eddy_tlm_temperature, "%3.2f C", res->inform.tlm.temperature);
            sprintf(eddy_tlm_adv_count, "%d", res->inform.tlm.adv_count);
            sprintf(eddy_tlm_time, "%d s", (res->inform.tlm.time)/10);

            ESP_LOGI(EDDY_TAG, "Eddystone TLM inform:");
            ESP_LOGI(EDDY_TAG, "version: %s", eddy_tlm_version);
            ESP_LOGI(EDDY_TAG, "battery voltage: %s", eddy_tlm_battery_voltage);
            ESP_LOGI(EDDY_TAG, "beacon temperature in degrees Celsius: %s", eddy_tlm_temperature);
            ESP_LOGI(EDDY_TAG, "adv pdu count since power-up: %s", eddy_tlm_adv_count);
            ESP_LOGI(EDDY_TAG, "time since power-up: %s", eddy_tlm_time);
            break;
        }
        default:
            break;
    }
}

/**
 * @brief Handles BLE Scan events
 * 
 * @param event 
 * @param param 
 */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param)
{
    esp_err_t err;

    switch(event)
    {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            if((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(EDDY_TAG,"Scan start failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(EDDY_TAG,"Start scanning...");
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;
            switch(scan_result->scan_rst.search_evt)
            {
                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                    esp_eddystone_result_t eddystone_res;
                    memset(&eddystone_res, 0, sizeof(eddystone_res));
                    esp_err_t ret = esp_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &eddystone_res);
                    if (ret) {
                        // error:The received data is not an eddystone frame packet or a correct eddystone frame packet.
                        // just return
                        return;
                    } else {   
                        // The received adv data is a correct eddystone frame packet.
                        // Here, we get the eddystone infomation in eddystone_res, we can use the data in res to do other things.
                        // For example, just print them:
                        ESP_LOGI(EDDY_TAG, "--------Eddystone Found----------");
                        esp_log_buffer_hex("Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                        ESP_LOGI(EDDY_TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);
                        esp_eddystone_show_inform(&eddystone_res);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:{
            if((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(EDDY_TAG,"Scan stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(EDDY_TAG,"Stop scan successfully");
            }
            break;
        }
        default:
            break;
    }
}

/**
 * @brief Register the BLE callback function
 * 
 */
void esp_eddystone_appRegister(void)
{
    esp_err_t status;
    
    ESP_LOGI(EDDY_TAG,"Register callback");

    /*<! register the scan callback function to the gap module */
    if((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(EDDY_TAG,"gap register error: %s", esp_err_to_name(status));
        return;
    }
}

/**
 * @brief Initialize BLE and start sacanning
 * 
 */
void esp_eddystone_init(void)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    esp_bluedroid_init();
    esp_bluedroid_enable();
    esp_eddystone_appRegister();

    /* set scan parameters */
    esp_ble_gap_set_scan_params(&ble_scan_params);
}
