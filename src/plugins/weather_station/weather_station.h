/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#if IOT_WEATHER_STATION

#include <Arduino_compat.h>
#include <SPI.h>
#include <vector>
#include <EventScheduler.h>
#include <asyncHTTPrequest.h>
#include <StreamString.h>
#include "WSDraw.h"
#include "plugins.h"

#ifndef DEBUG_IOT_WEATHER_STATION
#define DEBUG_IOT_WEATHER_STATION 0
#endif

class WeatherStationPlugin : public PluginComponent, public WSDraw {
// PluginComponent
public:
    WeatherStationPlugin();

    virtual PGM_P getName() const;
    virtual PluginPriorityEnum_t getSetupPriority() const override {
        return MIN_PRIORITY;
    }

    virtual void setup(PluginSetupMode_t mode) override;
    virtual void reconfigure(PGM_P source) override;
    virtual bool hasReconfigureDependecy(PluginComponent *plugin) const override;

    virtual bool hasStatus() const override;
    virtual const String getStatus() override;

    static void loop();
    static void serialHandler(uint8_t type, const uint8_t *buffer, size_t len);

private:
    static void _sendScreenCaptureBMP(AsyncWebServerRequest *request);
    void _installWebhooks();

private:
    typedef std::function<void (bool status)> Callback_t;

    void _httpRequest(const String &url, uint16_t timeout, JsonBaseReader *jsonReader, Callback_t finishedCallback);
    void _getWeatherInfo(Callback_t finishedCallback);
    void _getWeatherForecast(Callback_t finishedCallback);

    void _serialHandler(const uint8_t *buffer, size_t len);
    void _loop();

    void _fadeBacklight(uint16_t fromLevel, uint16_t toLevel, int8_t step = 16);
    void _broadcastCanvas(int16_t x, int16_t y, int16_t w, int16_t h);

private:
    uint32_t _updateTimer;
    uint16_t _backlightLevel;

private:
    uint32_t _pollInterval;
    time_t _pollTimer;
    asyncHTTPrequest *_httpClient;

#if DEBUG_IOT_WEATHER_STATION
public:
    bool _debugDisplayCanvasBorder;
#endif
};

#endif
