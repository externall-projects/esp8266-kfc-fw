/**
 * Author: sascha_lammers@gmx.de
 */

#if IOT_CLOCK

#include "SevenSegmentPixel.h"
#include <avr/pgmspace.h>

static const char _digits2SegmentsTable[]  = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71 };  // 0-F

SevenSegmentPixel::SevenSegmentPixel(uint8_t numDigits, uint8_t numPixels, uint8_t numColons) : _pixelOrder(nullptr), _callback(nullptr)
{
    _numDigits = numDigits;
    _numPixels = numPixels;
    _numColons = numColons;
    pixel_address_t num = SevenSegmentPixel_NUM_PIXELS(numDigits, numPixels, _numColons);
    _pixelAddress = reinterpret_cast<pixel_address_t *>(malloc(numDigits * numPixels * SegmentEnum_t::NUM * sizeof(pixel_address_t)));
    _dotPixelAddress = reinterpret_cast<pixel_address_t *>(malloc(_numColons * 2 * sizeof(pixel_address_t)));
#if IOT_CLOCK_NEOPIXEL
    _pixels = new Adafruit_NeoPixel(num, IOT_CLOCK_NEOPIXEL_PIN, NEO_GRB|NEO_KHZ800);
    _pixels->begin();
#else
    _pixels = new CRGB[num];
    _controller = &FastLED.addLeds<NEOPIXEL, IOT_CLOCK_NEOPIXEL_PIN>(_pixels, num);
#endif
}

SevenSegmentPixel::~SevenSegmentPixel()
{
#if IOT_CLOCK_NEOPIXEL
    delete _pixels;
#else
    // TODO delete from FastLED
    delete _pixels;
#endif
    free(_dotPixelAddress);
    free(_pixelAddress);
    if (_pixelOrder) {
        free(_pixelOrder);
    }
}

void SevenSegmentPixel::clear() {
    setColor(0);
}

void SevenSegmentPixel::setColor(color_t color) {
#if IOT_CLOCK_NEOPIXEL
    for(pixel_address_t i = 0; i < _pixels->numPixels(); i++) {
        _pixels->setPixelColor(i, color);
    }
    show();
#else
    for(pixel_address_t i = 0; i < _controller->size(); i++) {
        _pixels[i] = color;
    }
    show();
#endif
}

void SevenSegmentPixel::setColor(pixel_address_t num, color_t color) {
#if IOT_CLOCK_NEOPIXEL
    _pixels->setPixelColor(num, color);
#else
    _pixels[num] = color;
#endif
    show();
}

SevenSegmentPixel::pixel_address_t SevenSegmentPixel::setSegments(uint8_t digit, pixel_address_t offset)
{
    if (digit < _numDigits) {
        for(uint8_t i = 0; i < _numPixels; i++) {
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::A)] = offset + i + (_numPixels * 0);
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::B)] = offset + i + (_numPixels * 1);
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::C)] = offset + i + (_numPixels * 3);
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::D)] = offset + i + (_numPixels * 4);
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::E)] = offset + i + (_numPixels * 5);
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::F)] = offset + i + (_numPixels * 6);
            _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, SegmentEnum_t::G)] = offset + i + (_numPixels * 2);
        }
    }
    return offset + (SegmentEnum_t::NUM * _numPixels);
}

SevenSegmentPixel::pixel_address_t SevenSegmentPixel::setColons(uint8_t num, pixel_address_t lowerAddress, pixel_address_t upperAddress) {
    if (num < _numColons) {
        num *= 2;
        _dotPixelAddress[num++] = lowerAddress;
        _dotPixelAddress[num++] = upperAddress;
    }
    return num;
}

void SevenSegmentPixel::setDigit(uint8_t digit, uint8_t number, color_t color)
{
    pixel_address_t addr;
    color_t segmentColor = 0;

    for(uint8_t j = SegmentEnum_t::A; j < SegmentEnum_t::NUM; j++) {
        if (number < sizeof(_digits2SegmentsTable)) {
            segmentColor = SevenSegmentPixel_COLOR(color, j, (_digits2SegmentsTable[number]));
        }
        for(uint8_t i = 0; i < _numPixels; i++) {
            addr = _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, i, j)];
#if IOT_CLOCK_NEOPIXEL
            _pixels->setPixelColor(addr, _getColor(addr, segmentColor));
#else
            _pixels[addr] = _getColor(addr, segmentColor);
#endif
        }
    }
}

void SevenSegmentPixel::setColon(uint8_t num, ColonEnum_t type, color_t color)
{
    pixel_address_t addr;
    num *= 2;
    if (type & ColonEnum_t::LOWER) {
        addr = _dotPixelAddress[num];
#if IOT_CLOCK_NEOPIXEL
        _pixels->setPixelColor(addr, _getColor(addr, color));
#else
        _pixels[addr] = _getColor(addr, color);
#endif
    }
    if (type & ColonEnum_t::UPPER) {
        addr = _dotPixelAddress[num + 1];
#if IOT_CLOCK_NEOPIXEL
        _pixels->setPixelColor(addr, _getColor(addr, color));
#else
        _pixels[addr] = _getColor(addr, color);
#endif
    }
}

void SevenSegmentPixel::rotate(uint8_t digit, uint8_t position, color_t color)
{
    clearDigit(digit);
    auto addr = _pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(digit, position % 2, position / 2)];
#if IOT_CLOCK_NEOPIXEL
    _pixels->setPixelColor(addr, _getColor(addr, color));
#else
    _pixels[addr] = _getColor(addr, color);
#endif
}

void SevenSegmentPixel::setSegment(uint8_t digit, int segment, color_t color)
{
    segment = ((uint8_t)segment) % SegmentEnum_t::NUM;
    for(uint8_t i = 0; i < _numPixels; i++) {
        auto addr = SevenSegmentPixel_PIXEL_ADDRESS(digit, i, segment);
#if IOT_CLOCK_NEOPIXEL
        _pixels->setPixelColor(addr, _getColor(addr, color));
#else
        _pixels[addr] = _getColor(addr, color);
#endif
    }
}

void SevenSegmentPixel::print(const char *text, color_t color)
{
    if (!text || !*text){
        clear();
    }
    uint8_t digit = 0;
    uint8_t colon = 0;
    while(*text) {
        if (isdigit(*text) || *text == '#') {
            if (digit >= _numDigits) {
                break;
            }
            if (*text == '#') {
                clearDigit(digit);
            }
            else {
                setDigit(digit, *text - '0', color);
            }
            digit++;
        }
        else if (*text == ' ' || *text == '.' || *text == ':') {
            if (colon < _numColons) {
                if (*text == ' ') {
                    clearColon(colon);
                }
                else if (*text == ':') {
                    setColon(colon, BOTH, color);
                }
                else if (*text == '.') {
                    setColon(colon, LOWER, color);
                    setColon(colon, UPPER, 0);
                }
                colon++;
            }

        }
        text++;
    }
}

SevenSegmentPixel::color_t SevenSegmentPixel::_getColor(pixel_address_t addr, color_t color)
{
    if (color && _callback) {
        if (_pixelOrder) {
            return _callback(_pixelOrder[addr], color);
        }
        return _callback(addr, color);
    }
    return color;
}

void SevenSegmentPixel::dump(Print &output)
{
    for(uint8_t d = 0; d < _numDigits; d++) {
        for(uint8_t s = 0; s < SegmentEnum_t::NUM; s++) {
            output.printf_P(PSTR("segment: digit=%u, segment=%c, addresses="), d, getSegmentChar(s));
            for(uint8_t p = 0; p < _numPixels; p++) {
                output.print(_pixelAddress[SevenSegmentPixel_PIXEL_ADDRESS(d, p, s)]);
                if (p != _numPixels - 1) {
                    output.print(F(", "));
                }
            }
            output.println();
        }
    }
    for(uint8_t c = 0; c < _numColons; c++) {
        output.printf_P(PSTR("colon %u: lower address=%u upper=%u\n"), c, _dotPixelAddress[c * 2], _dotPixelAddress[c * 2 + 1]);
    }
}

#endif