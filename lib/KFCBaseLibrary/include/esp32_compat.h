/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#if defined(ESP32)

#include <FS.h>

#include <esp_timer.h>

typedef esp_timer_cb_t os_timer_func_t_ptr;
typedef struct esp_timer os_timer_t;

#ifndef OS_TIMER_DEBUG
#define OS_TIMER_DEBUG                      0
#endif

#if OS_TIMER_DEBUG

#define os_timer_create(timer, cb, arg) \
    { \
        const esp_timer_create_args_t args = { cb, arg, ESP_TIMER_TASK, _sharedEmptyString.c_str() }; \
        esp_err_t result; \
        if ((result = esp_timer_create(&args, &timer)) != ESP_OK) { \
            debug_printf_P(PSTR("esp_timer_create() failed: %d\n"), result); \
        } \
    }

#define os_timer_arm(timer, period, repeat) \
    { \
        esp_err_t result; \
        if ((repeat)) { \
            if ((result = esp_timer_start_periodic(timer, period * 1000ULL)) != ESP_OK) { \
                debug_printf_P(PSTR("esp_timer_start_periodic() failed: %d\n"), result); \
            } \
        } else { \
            if ((result = esp_timer_start_once(timer, period * 1000ULL)) != ESP_OK) { \
                debug_printf_P(PSTR("esp_timer_start_once() failed: %d\n"), result); \
            } \
        } \
    }

#else

#define os_timer_create(timer, cb, arg) \
    { \
        const esp_timer_create_args_t args = { cb, arg, ESP_TIMER_TASK, _sharedEmptyString.c_str() }; \
        esp_timer_create(&args, &timer); \
    }

#define os_timer_arm(timer, period, repeat) \
    if ((repeat)) { \
        esp_timer_start_periodic(timer, period * 1000ULL); \
    } else { \
        esp_timer_start_once(timer, period * 1000ULL); \
    }

#endif

#define os_timer_disarm(timer)              esp_timer_stop(timer)
#define os_timer_delete(timer)              esp_timer_delete(timer)


#include <esp_wifi.h>
#define wifi_get_country(country)           (esp_wifi_get_country(country) == ESP_OK)

#include <esp_wifi_types.h>
typedef wifi_err_reason_t WiFiDisconnectReason;

enum RFMode {
    RF_DEFAULT = 0
};

#ifndef WL_MAC_ADDR_LENGTH
#define WL_MAC_ADDR_LENGTH                  6
#endif

typedef struct {
    String ssid;
    uint8_t bssid[WL_MAC_ADDR_LENGTH];
    uint8_t channel;
} WiFiEventStationModeConnected;

typedef struct {
    String ssid;
    uint8_t bssid[WL_MAC_ADDR_LENGTH];
    WiFiDisconnectReason reason;
} WiFiEventStationModeDisconnected;

typedef struct {
    IPAddress ip;
    IPAddress mask;
    IPAddress gw;
} WiFiEventStationModeGotIP;

typedef struct {
    uint8_t aid;
    uint8_t mac[WL_MAC_ADDR_LENGTH];
} WiFiEventSoftAPModeStationConnected;

typedef struct {
    uint8_t aid;
    uint8_t mac[WL_MAC_ADDR_LENGTH];
} WiFiEventSoftAPModeStationDisconnected;

#define WiFi_isHidden(num)                  (WiFi.SSID(num).length() == 0)

enum dhcp_status {
    DHCP_STOPPED,
    DHCP_STARTED
};

enum dhcp_status wifi_station_dhcpc_status(void);

#define SPIFFS_openDir(dirname)             Dir(SPIFFS.open(dirname))

class Dir {
public:
    Dir() {
    }
    Dir(const fs::File &root) {
        _root = root;
    }

    bool next() {
        _file = _root.openNextFile();
        return (bool)_file;
    }

    String fileName() const {
        return _file.name();
    }

    size_t fileSize() const {
        return _file.size();
    }

    fs::File openFile(const char *mode) {
        return _file;
    }

private:
    fs::File _root;
    fs::File _file;
};

typedef struct {
    size_t totalBytes;
    size_t usedBytes;
    int blockSize;
    int maxOpenFiles;
    int maxPathLength;
    int pageSize;
} FSInfo;


#define SPIFFS_info(info) \
    memset(&info, 0, sizeof(info)); \
    info.totalBytes = SPIFFS.totalBytes(); \
    info.usedBytes = SPIFFS.usedBytes();


void analogWrite(uint8_t pin, uint16_t value);

inline void panic() {
    for(;;) {
        *((int *) 0) = 0;
    }
}

#endif