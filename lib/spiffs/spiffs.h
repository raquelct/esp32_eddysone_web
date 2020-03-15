/**
 * @file spiffs.h
 * @author Raquel Teixeira (raquelteixeira@trixlog.com)
 * @brief This file contains SPIFFS ralated functions.
 * @version 1.0
 * @date 2020-03-14
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __SPIFFS_H__
#define __SPIFFS_H__

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#define MAX_FILE_SIZE 1000

/* Static variables */ 
static const char *SPIFFS_TAG = "SPIFFS";

/* Public funtions */ 
void esp_spiffs_init();
char* esp_spiffs_read_file(char* file_path);
void esp_spiffs_unmount();

#endif /* __SPIFFS_H__ */