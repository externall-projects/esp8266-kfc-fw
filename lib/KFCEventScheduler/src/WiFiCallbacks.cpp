/**
Author: sascha_lammers@gmx.de
*/

#include "WiFiCallbacks.h"

#if DEBUG_WIFICALLBACKS
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

WiFiCallbacks::CallbackVector WiFiCallbacks::_callbacks;
bool WiFiCallbacks::_locked = false;

uint8_t WiFiCallbacks::add(uint8_t events, WiFiCallbacks::Callback_t callback, WiFiCallbacks::CallbackPtr_t callbackPtr)
{
    _debug_printf_P(PSTR("events=%u, callbackPtr=%p callback=%p\n"), events, resolve_lambda((void *)callbackPtr), resolve_lambda(lambda_target(callback)));

    events &= EventEnum_t::ANY;

    for (auto &callback : _callbacks) {
        if (callbackPtr == callback.callbackPtr) {
            _debug_printf_P(PSTR("callbackPtr=%p, changed events from %u to %u\n"), callbackPtr, callback.events, events);
            callback.events |= events;
            return callback.events;
        }
    }
    _debug_printf_P(PSTR("callbackPtr=%p new entry, events %d\n"), callbackPtr,  events);
    _callbacks.push_back(CallbackEntry_t({events, callback, callbackPtr}));
    return events;
}

int8_t WiFiCallbacks::remove(uint8_t events, WiFiCallbacks::CallbackPtr_t callbackPtr)
{
    _debug_printf_P(PSTR("events=%u, callbackPtr=%p\n"), events, resolve_lambda((void *)callbackPtr));
    for (auto iterator = _callbacks.begin(); iterator != _callbacks.end(); ++iterator) {
        if (callbackPtr == iterator->callbackPtr) {
            _debug_printf_P(PSTR("callbackPtr=%p changed events from %u to %u\n"), callbackPtr, iterator->events, iterator->events & ~events);
            iterator->events &= ~events;
            if (iterator->events == 0) {
                if (!_locked) {
                    _callbacks.erase(iterator);
                }
                return 0;
            }
            return iterator->events;
        }
    }
    return -1;
}

void WiFiCallbacks::callEvent(WiFiCallbacks::EventEnum_t event, void *payload)
{
    _debug_printf_P(PSTR("event=%u payload=%p\n"), event, payload);
    _locked = true;
    for (const auto &entry : _callbacks) {
        if (entry.events & event) {
            if (entry.callback) {
                _debug_printf_P(PSTR("callback=%p\n"), resolve_lambda(lambda_target(entry.callback)));
                entry.callback(event, payload);
            } else {
                _debug_printf_P(PSTR("callbackPtr=%p\n"), resolve_lambda((void *)entry.callbackPtr));
                entry.callbackPtr(event, payload);
            }
        }
    }
    _callbacks.erase(std::remove_if(_callbacks.begin(), _callbacks.end(), [](const CallbackEntry_t &wc) {
        return wc.events == 0;
    }), _callbacks.end());
    _locked = false;
}

WiFiCallbacks::CallbackVector &WiFiCallbacks::getVector()
{
    return _callbacks;
}
