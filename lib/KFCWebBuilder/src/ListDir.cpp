/**
 * Author: sascha_lammers@gmx.de
 */

#include "ListDir.h"
#include <misc.h>
#include "fs_mapping.h"

#include "debug_helper_disable.h"
// #include "debug_helper_enable.h"

#if ESP8266

ListDir::ListDir(const String &dirName, bool filterSubdirs) : _dirName(dirName), _listings(SPIFFS.open(FSPGM(fs_mapping_listings), FileOpenMode::read)), _listing(), _isDir(false), _filterSubdirs(filterSubdirs), _dir(SPIFFS.openDir(dirName))
{
    append_slash(_dirName);
    _debug_printf_P(PSTR("dirName=%s\n"), _dirName.c_str());
}

File ListDir::openFile(const char* mode)
{
    return _dir.openFile(mode);
}

#elif ESP32

ListDir::ListDir(const String &dirName, bool filterSubdirs) : _dirName(dirName), _listings(SPIFFS.open(FSPGM(fs_mapping_listings), FileOpenMode::read)), _listing(), _isDir(false), _filterSubdirs(filterSubdirs), _dir(SPIFFS.open(dirName))
{
    append_slash(_dirName);
    _debug_printf_P(PSTR("dirName=%s\n"), _dirName.c_str());
}

File ListDir::openFile(const char* mode)
{
    return SPIFFS.open(_file.name(), mode);
}

#endif

String ListDir::fileName()
{
    if (_listing.valid) {
        return _listing.filename;
    }
    return _filename;
}

size_t ListDir::fileSize()
{
    if (_listing.valid) {
        return _listing.header.orgSize;
    }
#if ESP8266
    return _dir.fileSize();
#elif ESP32
    return _file.size();
#endif
}

time_t ListDir::fileTime()
{
    if (_listing.valid) {
        return _listing.header.mtime;
    }
#if ESP8266
    return _dir.fileTime();
#elif ESP32
    return 0;
#endif
}

bool ListDir::_isSubdir(const String &dir) const
{
    if (!_filterSubdirs) {
        return false;
    }
    if (dir.startsWith(_dirName)) {
        auto subdir = dir.substring(_dirName.length());
        _debug_printf_P(PSTR("subdir=%s\n"), subdir.c_str());
        auto pos = subdir.indexOf('/');
        if (pos != -1) {
            auto filename = subdir.substring(pos);
            _debug_printf_P(PSTR("filename=%s pos=%u\n"), filename.c_str(), pos);
            if (filename.equals(F("/."))) {
                return false;
            }
            return true;
        }
    }
    return false;
}

bool ListDir::next()
{
    if (_listings) {
        while(true) {
            if (_listings.readBytes(reinterpret_cast<char *>(&_listing.header), sizeof(_listing.header)) != sizeof(_listing.header)) {
                _listings.close();
                _listing = {};
                _debug_println(F("read failure: header"));
                break;
            }
            else {
                _listing.filename = _listings.readStringUntil('\n');
                if (!_listing.filename.length()) {
                    _listings.close();
                    _listing = {};
                    _debug_println(F("read failure: filename"));
                    break;
                }
                else {
                    _debug_printf_P(PSTR("uuid=%x filename=%s filesize=%u/%u mtime=%u gzipped=%u dir=%u skip=%u\n"), _listing.header.uuid, _listing.filename.c_str(), _listing.header.size, _listing.header.orgSize, _listing.header.mtime, _listing.header.gzipped, _listing.header.isDir, !_listing.filename.startsWith(_dirName));
                    if (_listing.filename.startsWith(_dirName) && !_isSubdir(_listing.filename)) {
                        _listing.valid = true;
                        return true;
                    }
                }
            }
        }
    }
#if ESP8266
    while (_dir.next()) {
        _filename = _dir.fileName();
#elif ESP32
    File next;
    while((next = _dir.openNextFile())) {
        _filename = next.name();
#endif
        _debug_printf_P(PSTR("filename=%s\n"), _filename.c_str());
        if (_isSubdir(_filename)) {
            continue;
        }
        if (_filename.endsWith(F("/."))) {   // directory emulation
            _isDir = true;
            if (_filename.substring(0, _filename.length() - 1) == _dirName) {
                continue;
            }
            _filename.remove(_filename.length() - 2);
        }
        else {
            _isDir = false;
        }
#if ESP32
        if (_file.isDirectory()) {
            _isDir = true;
        }
#endif
        if (strncmp_P(_filename.c_str(), SPGM(fs_mapping_dir), strlen_P(SPGM(fs_mapping_dir)))) {
            return true;
        }
    }
    return false;
}

bool ListDir::rewind()
{
    _listings = SPIFFS.open(FSPGM(fs_mapping_listings), FileOpenMode::read);
#if ESP8266
    return _dir.rewind();
#elif ESP32
    _dir.rewindDirectory();
    return true;
#endif
}

bool ListDir::isFile() const
{
    return !isDirectory();
}

bool ListDir::isDirectory() const
{
    if (_listing.valid) {
        return _listing.header.isDir;
    }
    if (_isDir) {
        return true;
    }
#if ESP8266
    return _dir.isDirectory();
#elif ESP32
    return false;
#endif

}

bool ListDir::isMapping() const
{
    return _listing.valid;
}
