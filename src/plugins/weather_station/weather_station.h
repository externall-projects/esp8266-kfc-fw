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
#include "WebUIComponent.h"
#include "WSDraw.h"
#include "NeoPixel_esp.h"
#include "plugins.h"

#ifndef DEBUG_IOT_WEATHER_STATION
#define DEBUG_IOT_WEATHER_STATION               0
#endif

#ifndef IOT_WEATHER_STATION_HAS_TOUCHPAD
#define IOT_WEATHER_STATION_HAS_TOUCHPAD        1
#endif

// IRC pin
#ifndef IOT_WEATHER_STATION_MPR121_PIN
#define IOT_WEATHER_STATION_MPR121_PIN          12
#endif

// number of pixels
#ifndef IOT_WEATHER_STATION_WS2812_NUM
#ifndef __LED_BUILTIN_WS2812_NUM_LEDS
#define IOT_WEATHER_STATION_WS2812_NUM          0
#else
#define IOT_WEATHER_STATION_WS2812_NUM          __LED_BUILTIN_WS2812_NUM_LEDS
#endif
#endif

// WS2812 data pin
#ifndef IOT_WEATHER_STATION_WS2812_PIN
#ifndef __LED_BUILTIN_WS2812_PIN
#define IOT_WEATHER_STATION_WS2812_PIN          0
#else
#define IOT_WEATHER_STATION_WS2812_PIN          __LED_BUILTIN_WS2812_PIN
#endif
#endif

#if IOT_WEATHER_STATION_HAS_TOUCHPAD
#include "Mpr121Touchpad.h"
#endif

class WeatherStationPlugin : public PluginComponent, public WebUIInterface, public WSDraw {
// PluginComponent
public:
    WeatherStationPlugin();

    virtual PGM_P getName() const;
    virtual PluginPriorityEnum_t getSetupPriority() const override {
        return MAX_PRIORITY;
    }

    virtual void setup(PluginSetupMode_t mode) override;
    virtual void reconfigure(PGM_P source) override;
    virtual void restart() override;
    virtual bool hasReconfigureDependecy(PluginComponent *plugin) const override;

    virtual bool hasStatus() const override;
    virtual void getStatus(Print &output) override;

// WebUIInterface
public:
    virtual bool hasWebUI() const override {
        return true;
    }
    virtual WebUIInterface *getWebUIInterface() override {
        return this;
    }
    virtual void createWebUI(WebUI &webUI) override;

    virtual void getValues(JsonArray &array);
    virtual void setValue(const String &id, const String &value, bool hasValue, bool state, bool hasState);

public:
    static void loop();
    // static void serialHandler(uint8_t type, const uint8_t *buffer, size_t len);

public:
#if AT_MODE_SUPPORTED
    virtual bool hasAtMode() const override {
        return true;
    }
    virtual void atModeHelpGenerator() override;
    virtual bool atModeHandler(AtModeArgs &args) override;
#endif

private:
    void _init();
    static WeatherStationPlugin &_getInstance();
    static void _sendScreenCaptureBMP(AsyncWebServerRequest *request);
    void _installWebhooks();

private:
    typedef std::function<void (bool status)> Callback_t;

    void _drawEnvironmentalSensor(GFXCanvasCompressed& canvas, uint8_t top);

    void _httpRequest(const String &url, uint16_t timeout, JsonBaseReader *jsonReader, Callback_t finishedCallback);
    void _getWeatherInfo(Callback_t finishedCallback);
    void _getWeatherForecast(Callback_t finishedCallback);

    // void _serialHandler(const uint8_t *buffer, size_t len);
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
    EventScheduler::Timer _fadeTimer;

#if IOT_WEATHER_STATION_HAS_TOUCHPAD
    Mpr121Touchpad _touchpad;
    bool _touchpadDebug;
#endif

#if IOT_WEATHER_STATION_WS2812_NUM
    EventScheduler::Timer _pixelTimer;
    uint8_t _pixels[IOT_WEATHER_STATION_WS2812_NUM * 3];
#endif

#if DEBUG_IOT_WEATHER_STATION
public:
    bool _debugDisplayCanvasBorder;
#endif
};

#endif
