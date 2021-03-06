/**
* Author: sascha_lammers@gmx.de
*/

#include "DumpBinary.h"

DumpBinary::DumpBinary(Print &output) : _output(output)
{
    _perLine = 16;
    _groupBytes = 2;
}

DumpBinary::DumpBinary(const String &title, Print &output) : DumpBinary(output)
{
    output.print(title);
    output.println(':');
}

DumpBinary &DumpBinary::setPerLine(uint8_t perLine)
{
    _perLine = perLine;
    return *this;
}

DumpBinary &DumpBinary::setGroupBytes(uint8_t groupBytes)
{
    _groupBytes = groupBytes;
    return *this;
}

DumpBinary &DumpBinary::print(const String &str)
{
    _output.print(str);
    return *this;
}

DumpBinary &DumpBinary::dump(const uint8_t *data, size_t length, ptrdiff_t offset)
{
    ptrdiff_t end = length + offset;
    if (end - offset < 1) {
        return *this;
    }
    uint8_t perLine = _perLine ? _perLine : (end - offset);
    ptrdiff_t pos = offset;
    while (pos < end) {
        _output.printf_P(PSTR("[%04X] "), pos);
        uint8_t j = 0;
        for (ptrdiff_t i = pos; i < end && j < perLine; i++, j++) {
            if (isprint(data[i])) {
                _output.print((char)data[i]);
            } else {
                _output.print('.');
            }
        }
        if (perLine == 0xff) {
            _output.print(' ');
        }
        else {
            while (j++ <= perLine) {
                _output.print(' ');
            }
        }
        j = 0;
        for (; pos < end && j < perLine; pos++, j++) {
            _output.printf_P(PSTR("%02X"), (int)data[pos]);
            if (j % _groupBytes == 1) {
                _output.print(' ');
            }
        }
        if (_perLine != 0xff) {
            _output.println();
        }
#if ESP8266
        delay(1);
#endif
    }
    return *this;
}
