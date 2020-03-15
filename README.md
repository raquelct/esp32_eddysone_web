# ESP32 BLE Eddystone Web Server DataLogger

A ESP32 project for interfacing with a Eddystone BLE temperature sensor and sending the data over a simple HTTP Web Server.

* Using SPIFFS for storing the web page data (HTML and CSS)
* Using a custom partition table to use SPIFFS
* Need to upload the data folder separately using PlatformIo: Upload File System Image
* Change WIFI parameters on webserver.h file to match your wifi network

Using ESP-IDF 3.3 on PlatformIO.