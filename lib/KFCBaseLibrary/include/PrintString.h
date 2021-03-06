/**
 * Author: sascha_lammers@gmx.de
 */

/**
 * String class with "Print" interface
 **/

#pragma once

#include <Arduino_compat.h>

#ifndef WSTRING_HAVE_SETLEN
#if ESP8266
#define WSTRING_HAVE_SETLEN                 1
#elif defined(_MSC_VER)
#define WSTRING_HAVE_SETLEN                 0
#else
#define WSTRING_HAVE_SETLEN                 1
#endif
#endif

class PrintString : public String, public Print {
public:
    using Print::print;

    PrintString() {
    }
    PrintString(const String &str) : String(str) {
    }

    PrintString(const char *format, ...);
    PrintString(const char *format, va_list arg);
    PrintString(const __FlashStringHelper *format, ...);
    PrintString(const __FlashStringHelper *format, va_list arg);
    PrintString(double n, int digits, bool trimTrailingZeros);
    PrintString(const uint8_t* buffer, size_t len);
    PrintString(const __FlashBufferHelper *buffer, size_t len);

    size_t print(double n, int digits, bool trimTrailingZeros);
    size_t vprintf(const char *format, va_list arg);
    size_t vprintf_P(PGM_P format, va_list arg);
    size_t write_P(PGM_P buf, size_t size);

    size_t ltrim() {
        return String_ltrim(*this);
    }
    size_t rtrim() {
        return String_rtrim(*this);
    }
    size_t trim() {
        String::trim();
        return length();
    }

    bool endsWith(char ch) {
        return String_endsWith(*this, ch);
    }

    virtual size_t write(uint8_t data) override;
    virtual size_t write(const uint8_t* buf, size_t size) override;
    size_t write(const char *buf, size_t size) {
        return write(reinterpret_cast<const uint8_t *>(buf), size);
    }

private:
    size_t _setLength(char *buf, size_t size, size_t len);
};
