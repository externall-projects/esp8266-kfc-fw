/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#if defined(ESP8266)

#define SPIFFS_info(info)           SPIFFS.info(info)
#define SPIFFS_openDir(dirname)     SPIFFS.openDir(dirname)

#define WiFi_isHidden(num)          WiFi.isHidden(num)

#define String_begin(str, cstr)     char *cstr = str.begin();


typedef os_timer_func_t *           os_timer_func_t_ptr;

#define os_timer_create(timerPtr, cb, arg) \
    *timerPtr = new os_timer_t(); \
    os_timer_setfn(*timerPtr, cb, arg);

#define os_timer_delete(timer)      delete timer;

#endif
