/**
  Author: sascha_lammers@gmx.de
*/

#if PIN_MONITOR

// low level pin monitoring via interrupts
// NOTE: attachScheduledInterrupt() is being used to trigger the callbacks and will cause the ESP8266 to crash if interrupts are fired too quickly.
// The callback is not called inside an interrupt and does not have to be in IRAM.
//
// Use attachInterrupt for interrupts >10Hz instead of this class

#pragma once

#ifndef DEBUG_PIN_MONITOR
#define DEBUG_PIN_MONITOR               0
#endif

#include <Arduino_compat.h>
#include <vector>
#include "EventScheduler.h"

struct InterruptInfo;

class PinMonitor {
public:
    typedef void (* Callback_t)(void *arg);

    class Pin {
    public:
        Pin(uint8_t pin, uint8_t value, Callback_t callback, void *arg) : _micro(0), _callback(callback), _arg(arg), _pin(pin), _value(value) {
        }

        uint8_t getPin() const {
            return _pin;
        }
        void *getArg() const {
            return _arg;
        }

        void invoke(uint8_t value, uint32_t micro) {
            _value = value;
            _micro = micro;
            _callback(this);
        }

    private:
        friend PinMonitor;

        uint32_t _micro;
        Callback_t _callback;
        void *_arg;
        uint8_t _pin;
        uint8_t _value;
    };

private:
    PinMonitor();

public:
    ~PinMonitor();

    static PinMonitor *createInstance();
    static void deleteInstance();
    inline static PinMonitor *getInstance() {
        return _instance;
    }

    typedef std::vector<Pin> PinsVector;
    typedef PinsVector::iterator PinsVectorIterator;

    // NOTE: INPUT_PULLUP does not seem to work
    Pin *addPin(uint8_t pin, Callback_t callback, void *arg, uint8_t pinMode = INPUT);
    bool removePin(uint8_t pin, void *arg);

    void dumpPins(Stream &output);

    static void callback(InterruptInfo info);

    inline size_t size() const {
        return _pins.size();
    }

private:
    PinsVectorIterator _findPin(uint8_t pin, void *arg);
    PinsVector _pins;

    static PinMonitor *_instance;
};

#endif
