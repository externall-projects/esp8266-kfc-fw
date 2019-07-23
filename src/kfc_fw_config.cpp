/**
 * Author: sascha_lammers@gmx.de
 */

#include "kfc_fw_config.h"
#include <EEPROM.h>
#include <PrintString.h>
#include <LoopFunctions.h>
#include <WiFiCallbacks.h>
#include <EventScheduler.h>
#include <session.h>
#include <misc.h>
#include "progmem_data.h"
#include "build.h"
#if NTP_CLIENT
#include "./plugins/ntp/ntp_plugin.h"
#endif

#if DEBUG_KFC_CONFIG
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

PROGMEM_STRING_DEF(safe_mode_enabled, "Device started in SAFE MODE");
PROGMEM_STRING_DEF(SPIFF_configuration_backup, "/configuration.backup");

KFCFWConfiguration config;

KFCFWConfiguration::KFCFWConfiguration() : Configuration(CONFIG_EEPROM_OFFSET, CONFIG_EEPROM_MAX_LENGTH) {
    _garbageCollectionCycleDelay = 5000;
    _wifiConnected = false;
    _offlineSince = -1UL;
    _wifiUp = -1UL;
    _setupWiFiCallbacks();
}

KFCFWConfiguration::~KFCFWConfiguration() {
    LoopFunctions::remove(KFCFWConfiguration::loop);
}

void KFCFWConfiguration::_onWiFiConnectCb(const WiFiEventStationModeConnected &event) {
    _debug_printf_P(PSTR("KFCFWConfiguration::_onWiFiConnectCb(%s, %d, %s)\n"), event.ssid.c_str(), (int)event.channel, mac2String(event.bssid).c_str());
    if (!_wifiConnected) {

        if (resetDetector.hasWakeUpDetected() && _offlineSince == -1UL) {
            Logger_notice(F("WiFi connected to %s after %lu ms"), event.ssid.c_str(), millis());
        } else{
            Logger_notice(F("WiFi connected to %s"), event.ssid.c_str());
        }

        _debug_printf_P(PSTR("Station: WiFi connected to %s, offline for %.3f, millis = %lu\n"), event.ssid.c_str(), _offlineSince == -1UL ? 0 : ((millis() - _offlineSince) / 1000.0), millis());
        _debug_printf_P(PSTR("Free heap %s\n"), formatBytes(ESP.getFreeHeap()).c_str());
        _wifiConnected = true;
        config.storeQuickConnect(event.bssid, event.channel);
    }
}

void KFCFWConfiguration::_onWiFiDisconnectCb(const WiFiEventStationModeDisconnected &event) {
    _debug_printf_P(PSTR("KFCFWConfiguration::_onWiFiDisconnectCb(%d = %s)\n"), (int)event.reason, WiFi_disconnect_reason(event.reason).c_str());
    if (_wifiConnected) {
        config_set_blink(BLINK_FAST);
        _debug_printf_P(PSTR("WiFi disconnected after %.3f seconds, millis = %lu\n"), ((_wifiUp == -1UL) ? -1.0 : ((millis() - _wifiUp) / 1000.0)), millis());

        Logger_notice(F("WiFi disconnected, SSID %s, reason %s"), event.ssid.c_str(), WiFi_disconnect_reason(event.reason).c_str());
        _offlineSince = millis();
        _wifiConnected = false;
        _wifiUp = -1UL;
        WiFiCallbacks::callEvent(WiFiCallbacks::EventEnum_t::DISCONNECTED, (void *)&event);
    }
}

void KFCFWConfiguration::_onWiFiGotIPCb(const WiFiEventStationModeGotIP &event) {

    auto ip = event.ip.toString();
    auto mask = event.mask.toString();
    auto gw = event.gw.toString();
    auto dns1 = WiFi.dnsIP().toString();
    auto dns2 = WiFi.dnsIP(1).toString();
    _debug_printf_P(PSTR("KFCFWConfiguration::_onWiFiGotIPCb(%s, %s, %s DNS %s, %s)\n"), ip.c_str(), mask.c_str(), gw.c_str(), dns1.c_str(), dns2.c_str());

    Logger_notice(F("%s: IP/Net %s/%s GW %s DNS: %s, %s"), config._H_GET(Config().flags).stationModeDHCPEnabled ? PSTR("DHCP") : PSTR("Static configuration"), ip.c_str(), mask.c_str(), gw.c_str(), dns1.c_str(), dns2.c_str());

    config_set_blink(BLINK_SOLID);
    _wifiUp = millis();
    config.storeStationConfig(event.ip, event.mask, event.gw);

    WiFiCallbacks::callEvent(WiFiCallbacks::EventEnum_t::CONNECTED, (void *)&event);
}

void KFCFWConfiguration::_onWiFiOnDHCPTimeoutCb() {
    _debug_printf_P(PSTR("KFCFWConfiguration::_onWiFiOnDHCPTimeoutCb()\n"));
    Logger_error(F("DHCP timeout"));
    config_set_blink(BLINK_FLICKER);
}

void KFCFWConfiguration::_softAPModeStationConnectedCb(const WiFiEventSoftAPModeStationConnected &event) {
    _debug_printf_P(PSTR("KFCFWConfiguration::_softAPModeStationConnectedCb()\n"));
    Logger_notice(F("Station connected [%s]"), mac2String(event.mac).c_str());
}

void KFCFWConfiguration::_softAPModeStationDisconnectedCb(const WiFiEventSoftAPModeStationDisconnected &event) {
    _debug_printf_P(PSTR("KFCFWConfiguration::_softAPModeStationDisconnectedCb()\n"));
    Logger_notice(F("Station disconnected [%s]"), mac2String(event.mac).c_str());
}

#if defined(ESP32)

void KFCFWConfiguration::_onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch(event) {
        case WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED: {
                WiFiEventStationModeConnected dst;
                dst.ssid = reinterpret_cast<const char *>(info.connected.ssid);
                MEMNCPY_S(dst.bssid, info.connected.bssid);
                dst.channel = info.connected.channel;
                config._onWiFiConnectCb(dst);
            } break;
        case WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED: {
                WiFiEventStationModeDisconnected dst;
                dst.ssid = reinterpret_cast<const char *>(info.disconnected.ssid);
                MEMNCPY_S(dst.bssid, info.disconnected.bssid);
                dst.reason = (WiFiDisconnectReason)info.disconnected.reason;
                config._onWiFiDisconnectCb(dst);
            } break;
#if DEBUG
        case WiFiEvent_t::SYSTEM_EVENT_GOT_IP6: {
                debug_printf_P(PSTR("_nWiFiEvent(SYSTEM_EVENT_GOT_IP6, addr=%04X:%04X:%04X:%04X)\n"), info.got_ip6.ip6_info.ip.addr[0], info.got_ip6.ip6_info.ip.addr[1], info.got_ip6.ip6_info.ip.addr[2], info.got_ip6.ip6_info.ip.addr[3]);
            } break;
#endif
        case WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP: {
                WiFiEventStationModeGotIP dst;
                dst.ip = info.got_ip.ip_info.ip.addr;
                dst.gw = info.got_ip.ip_info.gw.addr;
                dst.mask = info.got_ip.ip_info.netmask.addr;
                config._onWiFiGotIPCb(dst);
            } break;
        case WiFiEvent_t::SYSTEM_EVENT_STA_LOST_IP: {
                config._onWiFiOnDHCPTimeoutCb(); // TODO esp32
            } break;
        case WiFiEvent_t::SYSTEM_EVENT_AP_STACONNECTED: {
                WiFiEventSoftAPModeStationConnected dst;
                dst.aid = info.sta_connected.aid;
                MEMNCPY_S(dst.mac, info.sta_connected.mac);
                config._softAPModeStationConnectedCb(dst);
            } break;
        case WiFiEvent_t::SYSTEM_EVENT_AP_STADISCONNECTED: {
                WiFiEventSoftAPModeStationDisconnected dst;
                dst.aid = info.sta_connected.aid;
                MEMNCPY_S(dst.mac, info.sta_connected.mac);
                config._softAPModeStationDisconnectedCb(dst);
            } break;
        default:
            break;
    }
}

#elif USE_WIFI_SET_EVENT_HANDLER_CB

void KFCFWConfiguration::_onWiFiEvent(System_Event_t *orgEvent) {
    //event->event_info.connected
    switch(orgEvent->event) {
        case EVENT_STAMODE_CONNECTED: {
                WiFiEventStationModeConnected dst;
                auto &src = orgEvent->event_info.connected;
                dst.ssid = reinterpret_cast<const char *>(src.ssid);
                MEMNCPY_S(dst.bssid, src.bssid);
                dst.channel = src.channel;
                config._onWiFiConnectCb(dst);
            } break;
        case EVENT_STAMODE_DISCONNECTED: {
                WiFiEventStationModeDisconnected dst;
                auto &src = orgEvent->event_info.disconnected;
                dst.ssid = reinterpret_cast<const char *>(src.ssid);
                MEMNCPY_S(dst.bssid, src.bssid);
                dst.reason = (WiFiDisconnectReason)src.reason;
                config._onWiFiDisconnectCb(dst);
            } break;
        case EVENT_STAMODE_GOT_IP: {
                WiFiEventStationModeGotIP dst;
                auto &src = orgEvent->event_info.got_ip;
                dst.gw = src.gw.addr;
                dst.ip = src.ip.addr;
                dst.mask = src.mask.addr;
                config._onWiFiGotIPCb(dst);
            } break;
        case EVENT_STAMODE_DHCP_TIMEOUT: {
                config._onWiFiOnDHCPTimeoutCb();
            } break;
        case EVENT_SOFTAPMODE_STACONNECTED: {
                WiFiEventSoftAPModeStationConnected dst;
                auto &src = orgEvent->event_info.sta_connected;
                dst.aid = src.aid;
                MEMNCPY_S(dst.mac, src.mac);
                config._softAPModeStationConnectedCb(dst);
            } break;
        case EVENT_SOFTAPMODE_STADISCONNECTED: {
                WiFiEventSoftAPModeStationDisconnected dst;
                auto &src = orgEvent->event_info.sta_disconnected;
                dst.aid = src.aid;
                MEMNCPY_S(dst.mac, src.mac);
                config._softAPModeStationDisconnectedCb(dst);
            } break;
    }
}

#else

static void __onWiFiConnectCb(const WiFiEventStationModeConnected &event) {
    config._onWiFiConnectCb(event);
}

static void __onWiFiGotIPCb(const WiFiEventStationModeGotIP &event) {
    config._onWiFiGotIPCb(event);
}

static void __onWiFiDisconnectCb(const WiFiEventStationModeDisconnected& event) {
    config._onWiFiDisconnectCb(event);
}

static void __onWiFiOnDHCPTimeoutCb() {
    config._onWiFiOnDHCPTimeoutCb();
}

static void __softAPModeStationConnectedCb(const WiFiEventSoftAPModeStationConnected &event) {
    config._softAPModeStationConnectedCb(event);
}

static void __softAPModeStationDisconnectedCb(const WiFiEventSoftAPModeStationDisconnected &event) {
    config._softAPModeStationDisconnectedCb(event);
}

#endif

void KFCFWConfiguration::_setupWiFiCallbacks() {
    _debug_println(F("KFCFWConfiguration::_setupWiFiCallbacks()"));

#if defined(ESP32)

    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);
    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
#if DEBUG
    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_GOT_IP6);
#endif
    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_STA_LOST_IP);
    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_AP_STACONNECTED);
    WiFi.onEvent(_onWiFiEvent, WiFiEvent_t::SYSTEM_EVENT_AP_STADISCONNECTED);

#elif USE_WIFI_SET_EVENT_HANDLER_CB

    wifi_set_event_handler_cb((wifi_event_handler_cb_t)&KFCFWConfiguration::_onWiFiEvent);

#else
    _onWiFiConnect = WiFi.onStationModeConnected(__onWiFiConnectCb); // using a lambda function costs 24 byte RAM per handler
    _onWiFiGotIP = WiFi.onStationModeGotIP(__onWiFiGotIPCb);
    _onWiFiDisconnect = WiFi.onStationModeDisconnected(__onWiFiDisconnectCb);
    _onWiFiOnDHCPTimeout = WiFi.onStationModeDHCPTimeout(__onWiFiOnDHCPTimeoutCb);
    _softAPModeStationConnected = WiFi.onSoftAPModeStationConnected(__softAPModeStationConnectedCb);
    _softAPModeStationDisconnected = WiFi.onSoftAPModeStationDisconnected(__softAPModeStationDisconnectedCb);
#endif
}

void KFCFWConfiguration::restoreFactorySettings() {
    _debug_println(F("KFCFWConfiguration::restoreFactorySettings()"));
    PrintString str;

    clear();
    _H_SET(Config().version, FIRMWARE_VERSION);

    ConfigFlags flags;
    memset(&flags, 0, sizeof(flags));
    flags.wifiMode = WIFI_AP;
    flags.isFactorySettings = true;
    flags.isDefaultPassword = true;
    flags.atModeEnabled = true;
    flags.softAPDHCPDEnabled = true;
    flags.stationModeDHCPEnabled = true;
    flags.ntpClientEnabled = true;
    flags.webServerMode = HTTP_MODE_UNSECURE;
    flags.webServerPerformanceModeEnabled = true;
    flags.mqttMode = MQTT_MODE_SECURE;
    flags.mqttAutoDiscoveryEnabled = true;
    flags.ledMode = MODE_SINGLE_LED;
    flags.hueEnabled = true;
    flags.useStaticIPDuringWakeUp = true;
    _H_SET(Config().flags, flags);


    uint8_t mac[6];
    WiFi.macAddress(mac);
    str.printf_P(PSTR("KFC%02X%02X%02X"), mac[3], mac[4], mac[5]);
    _H_SET_STR(Config().device_name, str);
    const char *defaultPassword = PSTR("12345678");
    _H_SET_STR(Config().device_pass, defaultPassword);

    _H_SET_STR(Config().soft_ap.wifi_ssid, str);
    _H_SET_STR(Config().soft_ap.wifi_pass, defaultPassword);
    _H_SET_STR(Config().wifi_ssid, str);

    _H_SET_IP(Config().dns1, IPAddress(8, 8, 8, 8));
    _H_SET_IP(Config().dns2, IPAddress(8, 8, 4, 4));
    _H_SET_IP(Config().local_ip, IPAddress(192, 168, 4, mac[5] <= 1 || mac[5] >= 253 ? (mac[4] <= 1 || mac[4] >= 253 ? (mac[3] <= 1 || mac[3] >= 253 ? mac[3] : rand() % 98 + 1) : mac[4]) : mac[5]));
    _H_SET_IP(Config().gateway, IPAddress(192, 168, 4, 1));
    _H_SET_IP(Config().soft_ap.address, IPAddress(192, 168, 4, 1));
    _H_SET_IP(Config().soft_ap.subnet, IPAddress(255, 255, 255, 0));
    _H_SET_IP(Config().soft_ap.gateway, IPAddress(192, 168, 4, 1));
    _H_SET_IP(Config().soft_ap.dhcp_start, IPAddress(192, 168, 4, 2));
    _H_SET_IP(Config().soft_ap.dhcp_end, IPAddress(192, 168, 4, 100));
#if defined(ESP32)
    _H_SET(Config().soft_ap.encryption, WIFI_AUTH_WPA2_PSK);
#elif defined(ESP8266)
    _H_SET(Config().soft_ap.encryption, ENC_TYPE_CCMP);
#endif
    _H_SET(Config().soft_ap.channel, 7);

#if WEBSERVER_TLS_SUPPORT
    _H_SET(Config().http_port, flags.webServerMode == HTTP_MODE_SECURE ? 443 : 80);
#elif WEBSERVER_SUPPORT
    _H_SET(Config().http_port, 80);
#endif
#if NTP_CLIENT
    _H_SET_STR(Config().ntp.timezone, F("UTC"));
    _H_SET_STR(Config().ntp.servers[0], F("pool.ntp.org"));
    _H_SET_STR(Config().ntp.servers[1], F("time.nist.gov"));
    _H_SET_STR(Config().ntp.servers[2], F("time.windows.com"));
#if USE_REMOTE_TIMEZONE
    // https://timezonedb.com/register
    _H_SET_STR(Config().ntp.remote_tz_dst_ofs_url, F("http://api.timezonedb.com/v2.1/get-time-zone?key=_YOUR_API_KEY_&by=zone&format=json&zone=${timezone}"));
#endif
#endif
    _H_SET(Config().led_pin, LED_BUILTIN);
#if MQTT_SUPPORT
    _H_SET_STR(Config().mqtt_topic, F("home/${device_name}"));
//     auto &mqttOptions = _H_W_GET(Config().mqtt_options);
//     BNZERO_S(&mqttOptions);
// #  if ASYNC_TCP_SSL_ENABLED
//     mqttOptions.port = flags.mqttMode == MQTT_MODE_SECURE ? 8883 : 1883;
// #  else
//     mqttOptions.port = 1883;
//     mqttOptions.keepalive = 15;
// #  endif
#  if MQTT_AUTO_DISCOVERY
    _H_SET_STR(Config().mqtt_discovery_prefix, F("homeassistant"));
#  endif
#endif
#if SYSLOG
    _H_SET(Config().syslog_port, 514);
#endif
#if HOME_ASSISTANT_INTEGRATION
    _H_SET_STR(Config().homeassistant.api_endpoint, F("http://<CHANGE_ME>:8123/api/"));
#endif
#if HUE_EMULATION
    _H_SET(Config().hue.tcp_port, HUE_BASE_PORT);
    _H_SET_STR(Config().hue.devices, F("lamp 1\nlamp 2"));
#endif
#if PING_MONITOR
    _H_SET_STR(Config().ping.host1, F("${gateway}"));
    _H_SET_STR(Config().ping.host2, F("8.8.8.8"));
    _H_SET_STR(Config().ping.host3, F("www.google.com"));
    _H_SET(Config().ping.count, 4);
    _H_SET(Config().ping.interval, 60);
    _H_SET(Config().ping.timeout, 5000);
#endif
#if SERIAL2TCP
    _H_SET(Config().serial2tcp.port, 2323);
    _H_SET(Config().serial2tcp.auth_mode, true);
    _H_SET(Config().serial2tcp.auto_connect, false);
    _H_SET(Config().serial2tcp.auto_reconnect, 15);
    _H_SET(Config().serial2tcp.port, 2323);
    _H_SET(Config().serial2tcp.serial_port, SERIAL2TCP_HARDWARE_SERIAL);
    _H_SET(Config().serial2tcp.rx_pin, D7);
    _H_SET(Config().serial2tcp.tx_pin, D8);
    _H_SET(Config().serial2tcp.baud_rate, 115200);
    _H_SET(Config().serial2tcp.idle_timeout, 300);
    _H_SET(Config().serial2tcp.keep_alive, 60);
#endif


#if CUSTOM_CONFIG_PRESET
    #include "retracted/custom_config.h"
#endif

}

void KFCFWConfiguration::setLastError(const String &error) {
    _lastError = error;
}

const char *KFCFWConfiguration::getLastError() const {
    return _lastError.c_str();
}

void KFCFWConfiguration::garbageCollector() {
    if (_readAccess && millis() > _readAccess + _garbageCollectionCycleDelay) {
        _debug_println(F("KFCFWConfiguration::garbageCollector(): releasing memory"));
        release();
    }
}

void KFCFWConfiguration::setConfigDirty(bool dirty) {
    _dirty = dirty;
}

bool KFCFWConfiguration::isConfigDirty() const {
    return _dirty;
}

const String KFCFWConfiguration::getFirmwareVersion() {
#if DEBUG
#define __DEBUG_CFS_APPEND " DEBUG MODE ENABLED"
#else
#define __DEBUG_CFS_APPEND ""
#endif
    return getShortFirmwareVersion() + F(" " __DATE__ __DEBUG_CFS_APPEND);
}

const String KFCFWConfiguration::getShortFirmwareVersion() {
    return F(FIRMWARE_VERSION_STR " Build " __BUILD_NUMBER);
}

void KFCFWConfiguration::storeQuickConnect(const uint8_t *bssid, int8_t channel) {
    _debug_printf_P(PSTR("KFCFWConfiguration::storeQuickConnect(%s, %d)\n"), mac2String(bssid).c_str(), channel);

    WiFiQuickConnect_t quickConnect;
    RTCMemoryManager::read(CONFIG_RTC_MEM_ID, &quickConnect, sizeof(quickConnect));
    quickConnect.channel = channel;
    memcpy(quickConnect.bssid, bssid, WL_MAC_ADDR_LENGTH);
    RTCMemoryManager::write(CONFIG_RTC_MEM_ID, &quickConnect, sizeof(quickConnect));
}

void KFCFWConfiguration::storeStationConfig(uint32_t ip, uint32_t netmask, uint32_t gateway) {
    _debug_printf_P(PSTR("KFCFWConfiguration::storeStationConfig(%s, %s, %s): dhcp client started = %d\n"),
        IPAddress(ip).toString().c_str(),
        IPAddress(netmask).toString().c_str(),
        IPAddress(gateway).toString().c_str(),
        (wifi_station_dhcpc_status() == DHCP_STARTED)
    );


    WiFiQuickConnect_t quickConnect;
    if (RTCMemoryManager::read(CONFIG_RTC_MEM_ID, &quickConnect, sizeof(quickConnect))) {
        quickConnect.local_ip = ip;
        quickConnect.subnet = netmask;
        quickConnect.gateway = gateway;
        quickConnect.dns1 = (uint32_t)WiFi.dnsIP();
        quickConnect.dns2 = (uint32_t)WiFi.dnsIP(1);
        auto &flags = config._H_GET(Config().flags);
        quickConnect.use_static_ip = flags.useStaticIPDuringWakeUp || !flags.stationModeDHCPEnabled;
        RTCMemoryManager::write(CONFIG_RTC_MEM_ID, &quickConnect, sizeof(quickConnect));
    } else {
        debug_println(F("KFCFWConfiguration::storeStationConfig(): reading RTC memory failed"));
    }
}

void KFCFWConfiguration::setup() {
    _debug_println(F("KFCFWConfiguration::setup()"));

    String version = KFCFWConfiguration::getFirmwareVersion();
    if (!resetDetector.hasWakeUpDetected()) {
        Logger_notice(F("Starting KFCFW %s"), version.c_str());
        config_set_blink(BLINK_FLICKER);
    }

    // ~5ms
    rng.begin(version.c_str());
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    rng.stir(WiFi.macAddress(mac), WL_MAC_ADDR_LENGTH);
    auto channel = WiFi.channel();
    rng.stir((const uint8_t *)&channel, sizeof(channel));
    rng.stir((const uint8_t *)&config, sizeof(config));

    LoopFunctions::add(KFCFWConfiguration::loop);
}

void KFCFWConfiguration::read() {
    _debug_println(F("KFCFWConfiguration::read()"));

    if (!Configuration::read()) {
        config.restoreFactorySettings();
        Configuration::write();
    } else {
        auto version = config._H_GET(Config().version);
        if (FIRMWARE_VERSION > version) {
            PrintString message;
            message.printf_P(PSTR("Upgrading EEPROM settings from %d.%d.%d to " FIRMWARE_VERSION_STR), (version >> 16), (version >> 8) & 0xff, (version & 0xff));
            Logger_warning(message);
            _debug_println(message);
            config._H_SET(Config().version, FIRMWARE_VERSION);
            Configuration::write();
        }
    }
}

void KFCFWConfiguration::write() {
    _debug_println(F("KFCFWConfiguration::write()"));

    auto &flags = config._H_W_GET(Config().flags);
    flags.isFactorySettings = false;
    if (!Configuration::write()) {
        Logger_error(F("Failure to write settings to EEPROM"));
        _debug_println(F("Failure to write settings to EEPROM"));
    }
}

// TODO restore system time and timezone
void KFCFWConfiguration::wakeUpFromDeepSleep() {
    _debug_println(F("KFCFWConfiguration::wakeUpFromDeepSleep()"));

#if defined(ESP32)
    wifi_config_t _config;
    wifi_sta_config_t &config = _config.sta;
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &_config) == ESP_OK) {
#elif defined(ESP8266)
    struct station_config config;
    if (wifi_station_get_config_default(&config)) {
#endif
        int32_t channel;
        uint8_t *bssidPtr;
        WiFiQuickConnect_t quickConnect;

        if (RTCMemoryManager::read(CONFIG_RTC_MEM_ID, &quickConnect, sizeof(quickConnect))) {
            channel = quickConnect.channel;
            bssidPtr = quickConnect.bssid;
        } else {
            channel = 0;
            // bssidPtr = nullptr;
        }

        if (channel <= 0 || !bssidPtr) {

            _debug_printf_P(PSTR("Cannot read quick connect from RTC memory, running WiFi.begin(%s, ***) only\n"), config.ssid);
            if (WiFi.begin((const char*)config.ssid, (const char*)config.password, channel, bssidPtr, true) == WL_CONNECT_FAILED) {
                Logger_error(F("Failed to start WiFi"));
            }

        } else {

            if (quickConnect.use_static_ip && quickConnect.local_ip) {
                _debug_printf_P(PSTR("WiFi.config(): configuring static ip\n"));
                WiFi.config(quickConnect.local_ip, quickConnect.gateway, quickConnect.subnet, quickConnect.dns1, quickConnect.dns2);
            }

            wl_status_t result;
            if ((result = WiFi.begin((const char*)config.ssid, (const char*)config.password, channel, bssidPtr, true)) == WL_CONNECT_FAILED) {
                Logger_error(F("Failed to start WiFi"));
            }

            _debug_printf_P(PSTR("WiFi.begin() = %d, channel %d, bssid %s, config: static ip %d, %s/%s gateway %s, dns %s, %s\n"),
                result,
                channel,
                mac2String(bssidPtr).c_str(),
                quickConnect.use_static_ip ? 1 : 0,
                inet_ntoString(quickConnect.local_ip).c_str(),
                inet_ntoString(quickConnect.gateway).c_str(),
                inet_ntoString(quickConnect.subnet).c_str(),
                inet_ntoString(quickConnect.dns1).c_str(),
                inet_ntoString(quickConnect.dns2).c_str()
            );

        }

    } else {
        Logger_error(F("Failed to load WiFi configuration"));
    }

}

void KFCFWConfiguration::enterDeepSleep(uint32_t time_in_ms, RFMode mode, uint16_t delayAfterPrepare) {
    _debug_printf_P(PSTR("KFCFWConfiguration::enterDeepSleep(%d, %d, %d)\n"), time_in_ms, mode, delayAfterPrepare);

    _debug_printf_P(PSTR("Prepearing device for deep sleep...\n"));
    // WiFiCallbacks::getVector().clear(); // disable WiFi callbacks to speed up shutdown
    // Scheduler.terminate(); // halt scheduler

    delay(1);

    for(auto &plugin: plugins) {
        plugin.callPrepareDeepSleep(time_in_ms, mode);
    }
    if (delayAfterPrepare) {
        delay(delayAfterPrepare);
    }
    _debug_printf_P(PSTR("Entering deep sleep for %d milliseconds, RF mode %d\n"), time_in_ms, mode);

#if NTP_RESTORE_SYSTEM_TIME_AFTER_WAKEUP
    // save current time to restore when waking up
    ntp_client_prepare_deep_sleep(time_in_ms);
#endif
#if defined(ESP8266)
    ESP.deepSleep(time_in_ms * 1000ULL, mode);
#else
    ESP.deepSleep(time_in_ms * 1000UL);
#endif
}

void KFCFWConfiguration::restartDevice() {
    _debug_println(F("KFCFWConfiguration::restartDevice()"));

    Logger_notice(F("Device is being restarted"));
    config_set_blink(BLINK_FLICKER);
    ESP.restart();
}

void KFCFWConfiguration::loop() {

    rng.loop();
    config.garbageCollector();
}

uint8_t KFCFWConfiguration::getMaxWiFiChannels() {
    wifi_country_t country;
    if (!wifi_get_country(&country)) {
        country.nchan = 255;
    }
    return country.nchan;
}

String KFCFWConfiguration::getWiFiEncryptionType(uint8_t type) {
#if defined(ESP32)
    switch(type) {
        case WIFI_AUTH_OPEN:
            return F("open");
        case WIFI_AUTH_WEP:
            return F("WEP");
        case WIFI_AUTH_WPA_PSK:
            return F("WPA/PSK");
        case WIFI_AUTH_WPA2_PSK:
            return F("WPA2/PSK");
        case WIFI_AUTH_WPA_WPA2_PSK:
            return F("WPA/WPA2/PSK");
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return F("WPA2/ENTERPRISE");
    }
#else
    switch(type) {
        case ENC_TYPE_NONE:
            return F("open");
        case ENC_TYPE_WEP:
            return F("WEP");
        case ENC_TYPE_TKIP:
            return F("WPA/PSK");
        case ENC_TYPE_CCMP:
            return F("WPA2/PSK");
        case ENC_TYPE_AUTO:
            return F("auto");
    }
#endif
    return F("N/A");
}

bool KFCFWConfiguration::reconfigureWiFi() {
    _debug_println(F("KFCFWConfiguration::reconfigureWiFi()"));

    WiFi.persistent(false); // disable during disconnects since it saves the configuration
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.persistent(true);

    return connectWiFi();
}

#if DEBUG
bool __debug_block_wifi_connect = false;
#endif

bool KFCFWConfiguration::connectWiFi() {
#if DEBUG
    if (__debug_block_wifi_connect) {
        _debug_println(F("KFCFWConfiguration::connectWiFi() blocked"));
        config_set_blink(BLINK_SOLID);
        return true;
    }
#endif
    _debug_println(F("KFCFWConfiguration::connectWiFi()"));
    setLastError(String());

    bool station_mode_success = false;
    bool ap_mode_success = false;

    WiFi.setHostname(config._H_STR(Config().device_name));
    // WiFi.hostname(config._H_STR(Config().device_name));

    auto flags = config._H_GET(Config().flags);
    if (flags.wifiMode & WIFI_STA) {
        _debug_println(F("KFCFWConfiguration::connectWiFi(): station mode"));
        WiFi.setAutoConnect(false); // WiFi callbacks have to be installed first
        WiFi.setAutoReconnect(true);

        bool result;
        if (flags.stationModeDHCPEnabled) {
            result = WiFi.config((uint32_t)0, (uint32_t)0, (uint32_t)0, (uint32_t)0, (uint32_t)0);
        } else {
            result = WiFi.config(config._H_GET_IP(Config().local_ip), config._H_GET_IP(Config().gateway), config._H_GET_IP(Config().subnet), config._H_GET_IP(Config().dns1), config._H_GET_IP(Config().dns2));
        }
        if (!result) {
            PrintString message;
            message.printf_P(PSTR("Failed to configure Station Mode with %s"), flags.stationModeDHCPEnabled ? PSTR("DHCP") : PSTR("static address"));
            setLastError(message);
            Logger_error(message);
        } else {
            if (WiFi.begin(config._H_STR(Config().wifi_ssid), config._H_STR(Config().wifi_pass)) == WL_CONNECT_FAILED) {
                String message = F("Failed to start Station Mode");
                setLastError(message);
                Logger_error(message);
            } else {
                _debug_printf_P(PSTR("Station Mode SSID %s\n"), WiFi.SSID().c_str());
                station_mode_success = true;
            }
        }
    }
    else {
        _debug_println(F("KFCFWConfiguration::connectWiFi(): disabling station mode"));
        WiFi.disconnect(true);
        station_mode_success = true;
    }

    if (flags.wifiMode & WIFI_AP) {
        _debug_println(F("KFCFWConfiguration::connectWiFi(): ap mode"));

        // config._H_GET(Config().soft_ap.encryption not used

        if (!WiFi.softAPConfig(config._H_GET_IP(Config().soft_ap.address), config._H_GET_IP(Config().soft_ap.gateway), config._H_GET_IP(Config().soft_ap.subnet))) {
            String message = F("Cannot configure AP mode");
            setLastError(message);
            Logger_error(message);
        } else {

#if defined(ESP8266)

            // setup after WiFi.softAPConfig()
            struct dhcps_lease dhcp_lease;
            wifi_softap_dhcps_stop();
            dhcp_lease.start_ip.addr = config._H_GET_IP(Config().soft_ap.dhcp_start);
            dhcp_lease.end_ip.addr = config._H_GET_IP(Config().soft_ap.dhcp_end);
            dhcp_lease.enable = flags.stationModeDHCPEnabled;
            if (!wifi_softap_set_dhcps_lease(&dhcp_lease)) {
                String message = F("Failed to configure DHCP server");
                setLastError(message);
                Logger_error(message);
            } else {
                if (flags.stationModeDHCPEnabled) {
                    if (!wifi_softap_dhcps_start()) {
                        String message = F("Failed to start DHCP server");
                        setLastError(message);
                        Logger_error(message);
                    } else {
                        ap_mode_success = true;
                        Logger_notice(F("AP Mode sucessfully initialized"));
                    }
                } else {
                    Logger_notice(F("DHCP server disabled"));
                }
            }

#endif

            if (!WiFi.softAP(config._H_STR(Config().soft_ap.wifi_ssid), config._H_STR(Config().soft_ap.wifi_pass), config._H_GET(Config().soft_ap.channel), flags.hiddenSSID)) {
                String message = F("Cannot start AP mode");
                setLastError(message);
                Logger_error(message);
            }
        }
    }
    else {
        _debug_println(F("KFCFWConfiguration::connectWiFi(): disabling AP mode"));
        WiFi.softAPdisconnect(true);
        ap_mode_success = true;
    }

    if (!station_mode_success || !ap_mode_success) {
        config_set_blink(BLINK_FAST);
    }

    return (station_mode_success && ap_mode_success);

}

void KFCFWConfiguration::printVersion(Print &output) {
    output.printf_P(PSTR("KFC Firmware %s\nFlash size %s\n"), KFCFWConfiguration::getFirmwareVersion().c_str(), formatBytes(ESP.getFlashChipSize()).c_str());
    if (resetDetector.getSafeMode()) {
        Logger_notice(FSPGM(safe_mode_enabled));
        output.println(FSPGM(safe_mode_enabled));
    }
}

void KFCFWConfiguration::printInfo(Print &output) {

    printVersion(output);

    auto flags = config._H_GET(Config().flags);
    if (flags.wifiMode & WIFI_AP) {
        output.printf_P(PSTR("AP Mode SSID %s\n"), config._H_STR(Config().soft_ap.wifi_ssid));
    }
    if (flags.wifiMode & WIFI_STA) {
        output.printf_P(PSTR("Station Mode SSID %s\n"), config._H_STR(Config().wifi_ssid));
    }
    if (flags.isFactorySettings) {
        output.println(F("Running on factory settings"));
    }
    if (flags.isDefaultPassword) {
        String str = FSPGM(default_password_warning);
        Logger_security(str);
        output.println(str);
    }
    output.printf_P(PSTR("Device %s ready!\n"), config._H_STR(Config().device_name));

#if AT_MODE_SUPPORTED
    if (flags.atModeEnabled) {
        output.println(F("Modified AT instruction set available.\n\nType AT? for help"));
    }
#endif
}

bool KFCFWConfiguration::isWiFiUp() {
    return config._wifiUp != -1UL;
}

unsigned long KFCFWConfiguration::getWiFiUp() {
    return config._wifiUp;
}