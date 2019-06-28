/**
* Author: sascha_lammers@gmx.de
*/

#include "FormData.h"

FormData::FormData() {
}

FormData::~FormData() {
}

void FormData::clear() {
    _data.clear();
}

const String FormData::arg(const String &name) const {
    auto it = _data.find(name);
    if (it != _data.end()) {
        return it->second;
    }
    return String();
}

bool FormData::hasArg(const String &name) const {
    return _data.find(name) != _data.end();
}

void FormData::set(const String &name, const String &value) {
    _data[name] = value;
}