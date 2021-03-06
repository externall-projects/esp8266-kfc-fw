/**
 * Author: sascha_lammers@gmx.de
 */

#include <Arduino_compat.h>
#include <algorithm>
#include <vector>
#include <functional>
#include <LoopFunctions.h>
#include <EventTimer.h>
#include <WiFiCallbacks.h>
#include <PrintString.h>
#include <ListDir.h>
#include "kfc_fw_config.h"
#include "blink_led_timer.h"
#include "at_mode.h"
#include "serial_handler.h"
#include "reset_detector.h"
#include "progmem_data.h"
#include "plugins.h"
#if PRINTF_WRAPPER_ENABLED
#include <printf_wrapper.h>
#endif
#if HAVE_GDBSTUB
#error uncomment the line below
// #include <GDBStub.h>
extern "C" void gdbstub_do_break();
#endif
#if 0
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

#if SPIFFS_TMP_FILES_TTL

void cleanup_tmp_dir()
{
    static MillisTimer timer(SPIFFS_TMP_CLEAUP_INTERVAL * 1000UL);
    if (timer.reached()) {
        ulong now = (millis() / 1000UL);
        String tmp_dir = sys_get_temp_dir();
        auto dir = ListDir(tmp_dir);
#if DEBUG
        int deleted = 0;
#endif
        while(dir.next()) {
            String filename = dir.fileName();
            ulong ttl = strtoul(filename.substring(tmp_dir.length()).c_str(), nullptr, HEX);
            if (ttl && now > ttl) {
                if (SPIFFS.remove(dir.fileName())) {
#if DEBUG
                    deleted++;
#endif
                }
            }
        }
        _debug_printf_P(PSTR("Cleanup %s: Removed %d file(s)\n"), tmp_dir.c_str(), deleted);

        timer.restart();
    }
}
#endif

void check_flash_size()
{
#if defined(ESP8266)
    uint32_t realSize = ESP.getFlashChipRealSize();
#endif
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

#if defined(ESP32)
    MySerial.printf_P(PSTR("Flash chip rev.: %08X\n"), ESP.getChipRevision());
#endif
#if defined(ESP8266)
    MySerial.printf_P(PSTR("Flash real id:   %08X\n"), ESP.getFlashChipId());
    MySerial.printf_P(PSTR("Flash real size: %u\n"), realSize);
#endif
    MySerial.printf_P(PSTR("Flash ide  size: %u\n"), ideSize);
    MySerial.printf_P(PSTR("Flash ide speed: %u\n"), ESP.getFlashChipSpeed());
    MySerial.printf_P(PSTR("Flash ide mode:  %s\n"), (ideMode == FM_QIO ? PSTR("QIO") : ideMode == FM_QOUT ? PSTR("QOUT") : ideMode == FM_DIO ? PSTR("DIO") : ideMode == FM_DOUT ? PSTR("DOUT") : PSTR("UNKNOWN")));

#if defined(ESP8266)
    if (ideSize != realSize) {
        MySerial.printf_P(PSTR("Flash Chip configuration wrong!\n\n"));
    } else {
        MySerial.printf_P(PSTR("Flash Chip configuration ok.\n\n"));
    }
#endif
}

void remove_crash_counter(bool initSPIFFS)
{
#if SPIFFS_SUPPORT
    if (initSPIFFS) {
        SPIFFS.begin();
    }
    char filename[strlen_P(SPGM(crash_counter_file)) + 1];
    strcpy_P(filename, SPGM(crash_counter_file));
    if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
    }
#endif
}

static void remove_crash_counter(EventScheduler::TimerPtr timer)
{
#if SPIFFS_SUPPORT
    remove_crash_counter(false);
#endif
}

void setup()
{
    Serial.begin(KFC_SERIAL_RATE);
    DEBUG_HELPER_INIT();

#if DEBUG_RESET_DETECTOR
    resetDetector._init();
#endif
    BlinkLEDTimer::setBlink(__LED_BUILTIN, BlinkLEDTimer::OFF);

#if HAVE_GDBSTUB
    gdbstub_do_break();
    disable_at_mode(Serial);
#endif

    if (resetDetector.getResetCounter() >= 20) {
        delay(5000);    // delay boot if too many resets are detected
        resetDetector.armTimer();
    }
    Serial.println(F("Booting KFC firmware..."));

    if (resetDetector.hasWakeUpDetected()) {
        config.wakeUpFromDeepSleep();
        resetDetector.clearCounter();
    }
    Serial.printf_P(PSTR("SAFE MODE %d, reset counter %d, wake up %d\n"), resetDetector.getSafeMode(), resetDetector.getResetCounter(), resetDetector.hasWakeUpDetected());
    config.setSafeMode(resetDetector.getSafeMode());

    if (resetDetector.hasResetDetected()) {
        if (resetDetector.getResetCounter() >= 4) {
            KFC_SAFE_MODE_SERIAL_PORT.println(F("4x reset detected. Restoring factory defaults in a 5 seconds..."));
            for(uint8_t i = 0; i < (RESET_DETECTOR_TIMEOUT + 500) / (100 + 250); i++) {
                BlinkLEDTimer::setBlink(__LED_BUILTIN, BlinkLEDTimer::SOLID);
                delay(100);
                BlinkLEDTimer::setBlink(__LED_BUILTIN, BlinkLEDTimer::OFF);
                delay(250);
            }
            config.restoreFactorySettings();
            config.write();
            resetDetector.setSafeMode(false);
        }
    }

    bool safe_mode = false;
    bool incr_crash_counter = false;
    if (resetDetector.getSafeMode()) {

        safe_mode = true;

        KFC_SAFE_MODE_SERIAL_PORT.println(F("Starting in safe mode..."));
        delay(2000);
        resetDetector.clearCounter();
        resetDetector.setSafeMode(false);
        config.setSafeMode(true);

    } else {

        if (resetDetector.getResetCounter() > 1) {

            KFC_SAFE_MODE_SERIAL_PORT.println(F("Multiple resets detected. Reboot continues in 20 seconds..."));
            KFC_SAFE_MODE_SERIAL_PORT.println(F("Press reset again to start in safe mode."));
            KFC_SAFE_MODE_SERIAL_PORT.println(F("\nTo restore factory defaults, press reset once a second until the LED starts to flash. After 5 seconds the normal boot process continues. To put the device to deep sleep until next reset, continue to press reset till the LED starts to flicker"));

            KFC_SAFE_MODE_SERIAL_PORT.printf_P(PSTR("\nCrash detected: %d\nReset counter: %d\n\n"), resetDetector.hasCrashDetected(), resetDetector.getResetCounter());
#if DEBUG
            KFC_SAFE_MODE_SERIAL_PORT.println(F("\nAvailable keys:\n"));
            KFC_SAFE_MODE_SERIAL_PORT.println(F(
                "    l: Enter wait loop\n"
                "    s: reboot in safe mode\n"
                "    r: reboot in normal mode\n"
                "    f: restore factory settings\n"
                "    c: clear RTC memory\n"
                //"    o: reboot in normal mode, start heap timer and block WiFi\n"
            ));
#endif

            BlinkLEDTimer::setBlink(__LED_BUILTIN, BlinkLEDTimer::SOS);
            resetDetector.setSafeMode(1);

            if (
#if DEBUG
            __while(10000, [&safe_mode]() {
                if (Serial.available()) {
                    switch(Serial.read()) {
                        case 'c':
                            RTCMemoryManager::clear();
                            remove_crash_counter(nullptr);
                            KFC_SAFE_MODE_SERIAL_PORT.println(F("RTC memory cleared"));
                            break;
                        case 'l':
                            __while(-1UL, []() {
                                return (Serial.read() != 'x');
                            }, 10e3, []() {
                                KFC_SAFE_MODE_SERIAL_PORT.println(F("Press 'x' to restart the device..."));
                                return true;
                            });
                            config.restartDevice();
                        case 'f':
                            config.restoreFactorySettings();
                            // continue switch
                        case 'r':
                            resetDetector.setSafeMode(false);
                            resetDetector.clearCounter();
                            remove_crash_counter(nullptr);
                            safe_mode = false;
                            config.setSafeMode(false);
                            return false;
                        case 's':
                            resetDetector.setSafeMode(1);
                            resetDetector.clearCounter();
                            safe_mode = true;
                            return false;
                    }
                }
                return true;
            }, 1000, []() {
                KFC_SAFE_MODE_SERIAL_PORT.print('.');
                return true;
            })
#else
            __while(5000, nullptr, 1000, []() {
                KFC_SAFE_MODE_SERIAL_PORT.print('.');
                return true;
            })
#endif
            ) {
                // timeout occured, disable safe mode
                resetDetector.setSafeMode(false);
                incr_crash_counter = true;
            }
            KFC_SAFE_MODE_SERIAL_PORT.println();
        }

    }

#if SPIFFS_SUPPORT
    SPIFFS.begin();
    if (resetDetector.hasCrashDetected() || incr_crash_counter) {
        File file = SPIFFS.open(FSPGM(crash_counter_file), fs::FileOpenMode::read);
        char counter = 0;
        if (file) {
            counter = file.read() + 1;
            file.close();
        }
        file = SPIFFS.open(FSPGM(crash_counter_file), fs::FileOpenMode::write);
        file.write(counter);
        file.close();
        if (counter >= 3) {  // boot in safe mode if there were 3 crashes within the first minute
            resetDetector.setSafeMode(1);
        }
    }
#endif

    Scheduler.begin();

    config.read();
    if (safe_mode) {

        config.setSafeMode(true);
        MySerialWrapper.replace(&KFC_SAFE_MODE_SERIAL_PORT, true);
        DebugSerial = MySerialWrapper;

        #if AT_MODE_SUPPORTED
            at_mode_setup();
        #endif

        prepare_plugins();
        setup_plugins(PluginComponent::PLUGIN_SETUP_SAFE_MODE);

        // check if wifi is up
        Scheduler.addTimer(1000, true, [](EventScheduler::TimerPtr timer) {
            timer->rearm(60000);
            if (!WiFi.isConnected()) {
                _debug_println(F("WiFi not connected, restarting"));
                config.reconfigureWiFi();
            }
        });

    } else {

        #if AT_MODE_SUPPORTED
            at_mode_setup();
        #endif

        if (resetDetector.hasCrashDetected()) {
            Logger_error(F("System crash detected: %s\n"), resetDetector.getResetInfo().c_str());
        }

#if DEBUG
        if (!resetDetector.hasWakeUpDetected()) {
            check_flash_size();
            _debug_printf_P(PSTR("Free Sketch Space %u\n"), ESP.getFreeSketchSpace());
#if defined(ESP8266)
            _debug_printf_P(PSTR("CPU frequency %d\n"), system_get_cpu_freq());
#endif
        }
#endif

#if SPIFFS_CLEANUP_TMP_DURING_BOOT
        if (!resetDetector.hasWakeUpDetected()) {
            _debug_println(F("Cleaning up /tmp directory"));
            auto dir = ListDir(sys_get_temp_dir());
            while(dir.next()) {
                _IF_DEBUG(bool status =) SPIFFS.remove(dir.fileName());
                _debug_printf_P(PSTR("remove=%s result=%d\n"), dir.fileName().c_str(), status);
            }
        }
#endif
#if SPIFFS_TMP_FILES_TTL
        LoopFunctions::add(cleanup_tmp_dir);
#endif

        prepare_plugins();

        setup_plugins(resetDetector.hasWakeUpDetected() ? PluginComponent::PLUGIN_SETUP_AUTO_WAKE_UP : PluginComponent::PLUGIN_SETUP_DEFAULT);

#if SPIFFS_SUPPORT
        _debug_printf_P(PSTR("remove crash counter=180s\n"));
        Scheduler.addTimer(180 * 1000UL, false, [](EventScheduler::TimerPtr timer) {
            remove_crash_counter(timer);
            resetDetector.clearCounter();
        }); // remove file and reset counters after 3min.
#endif
    }

#if LOAD_STATISTICS
    load_avg_timer = millis() + 30000;
#endif

    _debug_println(F("end"));
}

#if LOAD_STATISTICS
unsigned long load_avg_timer = 0;
uint32_t load_avg_counter = 0;
float load_avg[3] = {0, 0, 0};
#endif

void loop()
{
    auto &loopFunctions = LoopFunctions::getVector();
    for(uint8_t i = 0; i < loopFunctions.size(); i++) { // do not use iterators since the vector can be modifed inside the callback
        if (loopFunctions[i].deleteCallback) {
            loopFunctions.erase(loopFunctions.begin() + i);
            i--;
        } else if (loopFunctions[i].callback) {
            loopFunctions[i].callback();
        } else {
            loopFunctions[i].callbackPtr();
        }
    }
#if LOAD_STATISTICS
    load_avg_counter++;
    if (millis() >= load_avg_timer) {
        load_avg_timer = millis() + 1000;
        if (load_avg[0] == 0) {
            load_avg[0] = load_avg_counter / 30;
            load_avg[1] = load_avg_counter / 30;
            load_avg[2] = load_avg_counter / 30;

        } else {
            load_avg[0] = ((load_avg[0] * 60) + load_avg_counter) / (60 + 1);
            load_avg[1] = ((load_avg[1] * 300) + load_avg_counter) / (300 + 1);
            load_avg[2] = ((load_avg[2] * 900) + load_avg_counter) / (900 + 1);

        }
        load_avg_counter = 0;
    }
#endif
#if ESP32
    run_scheduled_functions();
#endif
}
