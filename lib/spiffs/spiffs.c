/**
 * @file spiffs.c
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file contains SPIFFS ralated functions.
 * @version 1.0
 * @date 2020-03-14
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "spiffs.h"

char file_buff[MAX_FILE_SIZE];

esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
};

/**
 * @brief Initialize the SPI File System
 * 
 */
void esp_spiffs_init(){
    ESP_LOGI(SPIFFS_TAG, "Initializing SPIFFS");
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SPIFFS_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(SPIFFS_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(SPIFFS_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret =  esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(SPIFFS_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(SPIFFS_TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

/**
 * @brief Get the number of bytes in the file content
 * 
 * @param f - File to size
 * @return unsigned int - The number of bytes in the file
 */
unsigned int esp_spiffs_get_file_size(FILE* f){
    fseek(f, 0, SEEK_END); // seek to end of file
    unsigned int size = ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET); // seek back to beginning of file
    ESP_LOGI(SPIFFS_TAG, "File size: '%d'", size);
    return size;
}

/**
 * @brief Read a file content and returns a string pointer
 * 
 * @param file_path - Path of the file to read
 * @return char* - Pointer to the file content
 */
char* esp_spiffs_read_file(char* file_path){
    ESP_LOGI(SPIFFS_TAG, "Reading file");
    FILE* f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open file for reading");
        return;
    }

    unsigned int file_size = esp_spiffs_get_file_size(f);

    fread(file_buff, file_size, 1, f);
    fclose(f);
    file_buff[file_size]='\0';

    ESP_LOGI(SPIFFS_TAG, "Read from file: '%s'", file_buff);

    for(int i = 0; i < file_size; i++) {
        if (file_buff[i] == '\n' || file_buff[i] == '\r') {
            memmove(&file_buff[i], &file_buff[i + 1], strlen(file_buff) - i);
        }
    }

    return file_buff;
}

/**
 * @brief Unmount partition and disable SPIFFS 
 * 
 */
void esp_spiffs_unmount(){
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(SPIFFS_TAG, "SPIFFS unmounted");
}