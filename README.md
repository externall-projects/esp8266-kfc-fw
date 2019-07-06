# esp8266-kfc-fw
Firmware for ESP8266 and IoT devices

The firmware offers a plugin interface to easily extend its functionality. The plugins can be configured via web interface and have access to the network, file system, logging, serial console, I2C bus, GPIO, EEPROM etc... via API functions.

## Libraries

### KFCSyslog

Send messages to a syslog server via UDP (or TCP not implemented yet)

### KFCTimezone

Platform independent timezone implementation with remote API support (Self hosted PHP script or https://timezonedb.com/register)

### KFCWebBuilder

Framework to build Web UIs with bootstrap and store them mostly compressed in a virtual file system

### KFCVirtualFileSystem

Read only file system with long filename support, optimized for low memory environments

### KFCJson

Library to read streamed JSON documents

### KFCEventScheduler

Platform indepentend Timer, Scheduler, WiFi Callbacks and loop function callbacks

## Plugins

### MQTT Client

MQTT Client with API to easily add components that work with Home Assistant, as well as auto discovery

### Http2Serial

Serial console access over the web UI using web sockets

### NTP Client

NTP Client with timezone support via REST API

## Required third party libraries

### ESPAsyncWebServer

https://github.com/me-no-dev/ESPAsyncWebServer

### asyncHTTPrequest

https://github.com/boblemaire/asyncHTTPrequest

### AsyncMqttClient (MQTT plugin)

https://github.com/marvinroger/async-mqtt-client

### AsyncWebSocket (Http2Serial plugin)

https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/src/AsyncWebSocket.h
