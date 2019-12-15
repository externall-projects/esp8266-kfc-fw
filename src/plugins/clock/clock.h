/**
 * Author: sascha_lammers@gmx.de
 */

#if IOT_CLOCK

#include <Arduino_compat.h>
#include <vector>
#include <EventScheduler.h>
#include "SevenSegmentPixel.h"
#include "plugins.h"
#include "kfc_fw_config.h"
#include "./plugins/mqtt/mqtt_component.h"

#ifndef DEBUG_IOT_CLOCK
#define DEBUG_IOT_CLOCK                 0
#endif

// number of digits
#ifndef IOT_CLOCK_NUM_DIGITS
#define IOT_CLOCK_NUM_DIGITS            4
#endif

// number of colons
#ifndef IOT_CLOCK_NUM_COLONS
#if IOT_CLOCK_NUM_DIGITS == 4
#define IOT_CLOCK_NUM_COLONS            1
#else
#define IOT_CLOCK_NUM_COLONS            2
#endif
#endif

// pixels per segment
#ifndef IOT_CLOCK_NUM_PIXELS
#define IOT_CLOCK_NUM_PIXELS            2
#endif

#ifndef IOT_CLOCK_BUTTON_PIN
#define IOT_CLOCK_BUTTON_PIN            14
#endif

#if IOT_CLOCK_BUTTON_PIN
#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>
#include <Bounce2.h>
#endif

class ClockPlugin : public PluginComponent, public MQTTComponent {
// PluginComponent
public:
    typedef enum : int8_t {
        NONE = -1,
        BLINK_COLON = 0,
        FAST_BLINK_COLON,
        SOLID_COLON,
        RAINBOW,
        FLASHING,
    } AnimationEnum_t;

    typedef enum : uint8_t {
        SOLID = 0,
        NORMAL = 1,
        FAST = 2,       // twice per second
    } BlinkColonEnum_t;

    class Color {
    public:
        Color(uint8_t *values) : _red(values[0]), _green(values[1]), _blue(values[2]) {
        }
        Color(uint8_t red, uint8_t green, uint8_t blue) : _red(red), _green(green), _blue(blue) {
        }
        inline uint32_t get() {
            return ((uint32_t)_red << 16) | ((uint32_t)_green <<  8) | _blue;
        }
        inline operator int() {
            return get();
        }
        inline static uint32_t get(uint8_t red, uint8_t green, uint8_t blue) {
            return ((uint32_t)red << 16) | ((uint32_t)green <<  8) | blue;
        }
    private:
        uint8_t _red;
        uint8_t _green;
        uint8_t _blue;
    };

    ClockPlugin();

    virtual PGM_P getName() const;
    virtual PluginPriorityEnum_t getSetupPriority() const override {
        return MIN_PRIORITY;
    }

    virtual void setup(PluginSetupMode_t mode) override;
    virtual void reconfigure(PGM_P source) override;
    //virtual bool hasReconfigureDependecy(PluginComponent *plugin) const override;

    virtual bool hasStatus() const override {
        return true;
    }
    virtual const String getStatus() override;

#if AT_MODE_SUPPORTED
    virtual bool hasAtMode() const override {
        return true;
    }

    virtual void atModeHelpGenerator() override;
    virtual bool atModeHandler(Stream &serial, const String &command, int8_t argc, char **argv) override;
#endif

public:
    virtual void createAutoDiscovery(MQTTAutoDiscovery::Format_t format, MQTTAutoDiscoveryVector &vector);
    virtual uint8_t getAutoDiscoveryCount() const {
        return 1;
    }
    virtual void onConnect(MQTTClient *client);
    virtual void onMessage(MQTTClient *client, char *topic, char *payload, size_t len);

    void publishState(MQTTClient *client);

private:
    void _setColor();

private:
    uint8_t _qos;
    uint8_t _brightness;
    uint8_t _colors[3];

//
public:
#if IOT_CLOCK_BUTTON_PIN
    static void onButtonHeld(Button& btn, uint16_t duration, uint16_t repeatCount);
    static void onButtonReleased(Button& btn, uint16_t duration);
    void _onButtonReleased(uint16_t duration);
#endif
    static void loop();
    static void wifiCallback(uint8_t event, void *payload);

    void enable(bool enable);
    void setSyncing();
    void setBlinkColon(BlinkColonEnum_t value);
    void setAnimation(AnimationEnum_t animation);
    Clock updateConfig();

private:
    void _loop();
    void _setSevenSegmentDisplay(Clock &cfg);
    void _off() {
        _display->clear();
    }

private:
#if IOT_CLOCK_BUTTON_PIN
    PushButton _button;
    uint8_t _buttonCounter;
#endif
    SevenSegmentPixel *_display;
    Color _color;
    BlinkColonEnum_t _blinkColon;
    uint32_t _updateTimer;
    time_t _time;
    uint16_t _updateRate;
    uint8_t _isSyncing;
    bool _timeFormat24h;
    typedef struct {
        union {
            struct {
                uint32_t color;
            } flashing;
            struct {
                uint16_t movementSpeed;
            } rainbow;
        };
    } AnimationData_t;
    AnimationData_t _animationData;
};

#endif