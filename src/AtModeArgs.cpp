/**
  Author: sascha_lammers@gmx.de
*/

#if AT_MODE_SUPPORTED

#include <Arduino_compat.h>
#include "at_mode.h"

#if DEBUG_AT_MODE
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

AtModeArgs::AtModeArgs(Stream &output) : _output(output) //, _argsArray(nullptr)
{
    clear();
}

// void AtModeArgs::_createArgs()
// {
//     uint16_t numItems = _args.size() + 1;
// #if AT_MODE_MAX_ARGUMENTS
//     if (AT_MODE_MAX_ARGUMENTS > numItems) {
//         numItems = AT_MODE_MAX_ARGUMENTS;
//     }
// #endif
//     uint8_t argc = 0;
//     _argsArray = (char **)calloc(numItems, sizeof(char *));
//     if (_argsArray) {
//         for(auto arg: _args) {
// #if 0
//             _argsArray[argc++] = strdup((const char *)arg);
// #else
//             _argsArray[argc++] = (char *)arg;
// #endif
//         }
//     }
// }

// void AtModeArgs::_freeArgs()
// {
//     if (_argsArray) {
// #if 0
//         auto ptr = _argsArray;
//         while(*ptr) {
//             free((void *)(*ptr));
//             ptr++;
//         }
// #endif
//         free(_argsArray);
//         _argsArray = nullptr;
//     }
// }

void AtModeArgs::clear()
{
    _queryMode = false;
    _args.clear();
    // _freeArgs();
}

void AtModeArgs::setQueryMode(bool mode)
{
    if (mode) {
        _queryMode = true;
        _args.clear();
    }
    else {
        _queryMode = false;
    }
}

AtModeArgs::ArgumentPtr AtModeArgs::get(uint16_t num) const
{
    if (!exists(num)) {
        return nullptr;
    }
    return _args.at(num);
}

long AtModeArgs::toInt(uint16_t num, long defaultValue) const
{
    ArgumentPtr arg;
    if (nullptr != (arg = get(num))) {
        return atol(arg);
    }
    return defaultValue;
}

long AtModeArgs::toNumber(uint16_t num, long defaultValue) const
{
    ArgumentPtr arg;
    if (nullptr != (arg = get(num))) {
        char *endPtr = nullptr;
        auto value = strtol(arg, &endPtr, 0); // auto detect base
        if (!endPtr || *endPtr) { // null or does not point to the end of the string = parse error
            return defaultValue;
        }
        return value;
    }
    return defaultValue;
}

long long AtModeArgs::toLongLong(uint16_t num, long long defaultValue) const
{
    ArgumentPtr arg;
    if (nullptr != (arg = get(num))) {
        return atoll(arg);
    }
    return defaultValue;
}

double AtModeArgs::toDouble(uint16_t num, double defaultValue) const
{
    ArgumentPtr arg;
    if (nullptr != (arg = get(num))) {
        return atof(arg);
    }
    return defaultValue;
}


uint32_t AtModeArgs::toMillis(uint16_t num, uint32_t minTime, uint32_t maxTime, uint32_t defaultValue) const
{
    auto arg = get(num);
    if (!arg) {
        _debug_printf_P(PSTR("toMillis(): arg=%u does not exist\n"), num);
        return defaultValue;
    }
    char *endPtr = nullptr;
    auto value = strtod(arg, &endPtr);
    String suffix(endPtr);
    suffix.trim();
    suffix.toLowerCase();

    uint32_t result;
/*
    if (String_equals(suffix, F("ms")) || String_startsWith(suffix, F("milli"))) {
        result = (uint32_t)value;
    }
    else
*/
    if (String_startsWith(suffix, F("min"))) {
        result =  (uint32_t)(value * 1000.0 * 60);
    }
    else if (String_equals(suffix, F("h")) || String_startsWith(suffix, F("hour"))) {
        result =  (uint32_t)(value * 1000.0 * 60 * 60);
    }
    else if (String_equals(suffix, F("d")) || String_startsWith(suffix, F("day"))) {
        result =  (uint32_t)(value * 1000.0 * 60 * 60 * 24);
    }
    else if (String_equals(suffix, F("s")) || String_startsWith(suffix, F("sec"))) {
        result = (uint32_t)(value * 1000.0);
    }
    else {
        result = (uint32_t)value;
    }
    result = std::min(maxTime, result);
    if (result < minTime) {
        _debug_printf_P(PSTR("toMillis(): arg=%s < minTime=%u\n"), arg, minTime);
        return defaultValue;
    }
    _debug_printf_P(PSTR("toMillis(): arg=%s converted to %u\n"), arg, result);
    return result;
}

#endif