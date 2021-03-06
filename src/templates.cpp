/**
 * Author: sascha_lammers@gmx.de
 */

#include "../include/templates.h"
#include <PrintString.h>
#include <PrintHtmlEntitiesString.h>
#include <Timezone.h>
#include "progmem_data.h"
#include "kfc_fw_config.h"
#include "build.h"
#include "misc.h"
#include "web_server.h"
#include "status.h"
#include "reset_detector.h"
#include "plugins.h"
#include "plugins_menu.h"

#if DEBUG_TEMPLATES
#include <debug_helper_enable.h>
#else
#include <debug_helper_disable.h>
#endif

WebTemplate::WebTemplate() : _form(nullptr), _json(nullptr)
{
}

WebTemplate::~WebTemplate()
{
    if (_json) {
        delete _json;
    }
    if (_form) {
        delete _form;
    }
}

void WebTemplate::setForm(Form *form)
{
    _json = static_cast<SettingsForm *>(form)->_json;
    _form = form;
}

Form *WebTemplate::getForm()
{
    return _form;
}

JsonUnnamedObject *WebTemplate::getJson() {
    return _json;
}

PrintArgs &WebTemplate::getPrintArgs() {
    return _printArgs;
}

void WebTemplate::printSystemTime(time_t now, PrintHtmlEntitiesString &output)
{
    char buf[80];
    auto format = PSTR("%a, %d %b %Y " HTML_SA(span, HTML_A("id", "system_time")) "%H:%M:%S" HTML_E(span) " %Z");
    timezone_strftime_P(buf, sizeof(buf), format, timezone_localtime(&now));
    output.printf_P(PSTR(HTML_SA(span, HTML_A("id", "system_date") HTML_A("format", "%s")) "%s" HTML_E(span)), PrintHtmlEntitiesString(FPSTR(format)).c_str(), buf);
}

void WebTemplate::process(const String &key, PrintHtmlEntitiesString &output)
{
    if (String_equals(key, PSTR("HOSTNAME"))) {
        output.print(config.getDeviceName());
    }
    else if (String_equals(key, F("TITLE"))) {
        output.print(config._H_STR(Config().device_title));
    }
    else if (String_equals(key, PSTR("HARDWARE"))) {
#if defined(ESP8266)
        output.printf_P(PSTR("ESP8266 %s Flash, %d Mhz, Free RAM %s"), formatBytes(ESP.getFlashChipRealSize()).c_str(), system_get_cpu_freq(), formatBytes(ESP.getFreeHeap()).c_str());
#elif defined(ESP32)
        output.printf_P(PSTR("ESP32 %s Flash, %d Mhz, Free RAM %s"), formatBytes(ESP.getFlashChipSize()).c_str(), ESP.getCpuFreqMHz(), formatBytes(ESP.getFreeHeap()).c_str());
#else
#error Platform not supported
#endif
#if LOAD_STATISTICS
        output.printf_P(PSTR(HTML_S(br) "Load Average %.2f %.2f %.2f"), LOOP_COUNTER_LOAD(load_avg[0]), LOOP_COUNTER_LOAD(load_avg[1]), LOOP_COUNTER_LOAD(load_avg[2]));
#endif
    }
    else if (String_equals(key, PSTR("SOFTWARE"))) {
        output.print(F("KFC FW "));
        output.print(config.getFirmwareVersion());
        if (config._H_GET(Config().flags).isDefaultPassword) {
            output.printf_P(PSTR(HTML_S(br) HTML_S(strong) "%s" HTML_E(strong)), SPGM(default_password_warning));
        }
    }
#if RTC_SUPPORT
    else if (String_equals(key, PSTR("RTC_STATUS"))) {
        output.setRawOutput(true); // disable html entities translation
        config.printRTCStatus(output, false);
        output.setRawOutput(false);
    }
#endif
#if NTP_CLIENT || RTC_SUPPORT
    else if (String_equals(key, PSTR("TIME"))) {
        auto now = time(nullptr);
        if (!IS_TIME_VALID(now)) {
            output.print(F("No time available"));
        } else {
            printSystemTime(now, output);
        }
    }
#endif
    else if (String_equals(key, PSTR("SAFEMODE"))) {
        if (config.isSafeMode()) {
            output.print(F(" - Running in SAFE MODE"));
        }
    }
    else if (String_equals(key, PSTR("UPTIME"))) {
        output.print(formatTime((long)(millis() / 1000)));
    }
    else if (String_equals(key, PSTR("WIFI_UPTIME"))) {
        output.print(config._H_GET(Config().flags).wifiMode & WIFI_STA ? (!KFCFWConfiguration::isWiFiUp() ? F("Offline") : formatTime((millis() - KFCFWConfiguration::getWiFiUp()) / 1000)) : F("Client mode disabled"));
    }
    else if (String_equals(key, PSTR("IP_ADDRESS"))) {
        WiFi_get_address(output);
    }
    else if (String_equals(key, PSTR("FIRMWARE_UPGRADE_FAILURE"))) {
    }
    else if (String_equals(key, PSTR("FIRMWARE_UPGRADE_FAILURE_CLASS"))) {
        output.print(FSPGM(_hidden));
    }
    else if (String_equals(key, PSTR("CONFIG_DIRTY_CLASS"))) {
        if (!config.isConfigDirty()) {
            output.print(FSPGM(_hidden));
        }
    }
    else if (_form) {
        auto str = _form->process(key);
        if (str) {
            output.print(str);
        }
    }
    else if (String_endsWith(key, PSTR("_STATUS"))) {
        uint8_t cmp_length = key.length() - 7;
        for(auto plugin: plugins) {
            if (plugin->hasStatus() && strncasecmp_P(key.c_str(), plugin->getName(), cmp_length) == 0) {
                plugin->getStatus(output);
                return;
            }
        }
        output.print(FSPGM(Not_supported));
    }
    else {
        output.print(F("KEY NOT FOUND: %"));
        output.print(key);
        output.print('%');
    }
}

UpgradeTemplate::UpgradeTemplate() {
}

UpgradeTemplate::UpgradeTemplate(const String &errorMessage)
{
    _errorMessage = errorMessage;
}

void UpgradeTemplate::process(const String &key, PrintHtmlEntitiesString &output)
{
    if (String_equals(key, F("FIRMWARE_UPGRADE_FAILURE_CLASS"))) {
    }
    else if (String_equals(key, F("FIRMWARE_UPGRADE_FAILURE"))) {
        output.setRawOutput(true);
        output.print(_errorMessage);
    }
    else {
        WebTemplate::process(key, output);
    }
}

void UpgradeTemplate::setErrorMessage(const String &errorMessage)
{
    _errorMessage = errorMessage;
}

LoginTemplate::LoginTemplate() {
}

LoginTemplate::LoginTemplate(const String &errorMessage)
{
    _errorMessage = errorMessage;
}

void LoginTemplate::process(const String &key, PrintHtmlEntitiesString &output)
{
    if (String_equals(key, F("LOGIN_ERROR_MESSAGE"))) {
        output.print(_errorMessage);
    }
    else if (String_equals(key, F("LOGIN_ERROR_CLASS"))) {
        if (_errorMessage.length() == 0) {
            output.print(FSPGM(_hidden));
        }
    }
    else {
        WebTemplate::process(key, output);
    }
}

void LoginTemplate::setErrorMessage(const String &errorMessage)
{
    _errorMessage = errorMessage;
}

void ConfigTemplate::process(const String &key, PrintHtmlEntitiesString &output)
{
    if (_form) {
        const char *str = getForm()->process(key);
        if (str) {
            output.print(str);
            return;
        }
        SettingsForm *form = static_cast<SettingsForm *>(getForm());
        auto &tokens = form->getTokens();
        if (tokens.size()) {
            for(const auto &token: tokens) {
                if (token.first.equals(key)) {
                    output.print(token.second);
                    return;
                }
            }
        }
    }

    if (String_equals(key, F("NETWORK_MODE"))) {
        bool m = false;
        if (config._H_GET(Config().flags).wifiMode & WIFI_AP) {
            output.print(F("#ap_mode"));
            m = true;
        }
        if (config._H_GET(Config().flags).wifiMode & WIFI_STA) {
            if (m) {
                output.print(',');
            }
            output.print(F("#station_mode"));
        }
    }
    else if (String_startsWith(key, F("MAX_CHANNELS"))) {
        output.print(config.getMaxWiFiChannels());
    }
    else if (String_startsWith(key, F("MODE_"))) {
        if (config._H_GET(Config().flags).wifiMode == key.substring(5).toInt()) {
            output.print(FSPGM(_selected));
        }
    }
    else if (String_equals(key, F("SSL_CERT"))) {
#if SPIFFS_SUPPORT
        File file = SPIFFS.open(FSPGM(server_crt), fs::FileOpenMode::read);
        if (file) {
            output.print(file.readString());
        }
#endif
    }
    else if (String_equals(key, F("SSL_KEY"))) {
#if SPIFFS_SUPPORT
        File file = SPIFFS.open(FSPGM(server_key), fs::FileOpenMode::read);
        if (file) {
            output.print(file.readString());
        }
#endif
    }
    else {
        WebTemplate::process(key, output);
    }
}

void StatusTemplate::process(const String &key, PrintHtmlEntitiesString &output)
{
    if (String_equals(key, F("GATEWAY"))) {
        WiFi.gatewayIP().printTo(output);
    }
    else if (String_equals(key, F("DNS"))) {
        WiFi.dnsIP().printTo(output);
        if (WiFi.dnsIP(1)) {
            output.print(FSPGM(comma_));
            WiFi.dnsIP(1).printTo(output);
        }
    }
    else if (String_equals(key, F("SSL_STATUS"))) {
        #if ASYNC_TCP_SSL_ENABLED
            #if WEBSERVER_TLS_SUPPORT
                output.printf_P(PSTR("TLS enabled, HTTPS %s"), _Config.getOptions().isHttpServerTLS() ? F("enabled") : FSPGM(Disabled));
            #else
                output.printf_P(PSTR("TLS enabled, HTTPS not supported"));
            #endif
        #else
            output.print(FSPGM(Not_supported));
        #endif
    }
    else if (String_equals(key, F("WIFI_MODE"))) {
        switch (WiFi.getMode()) {
            case WIFI_STA:
                output.print(F("Station mode"));
                break;
            case WIFI_AP:
                output.print(F("Access Point"));
                break;
            case WIFI_AP_STA:
                output.print(F("Access Point and Station Mode"));
                break;
            case WIFI_OFF:
                output.print(F("Off"));
                break;
            default:
                output.printf_P(PSTR("Invalid mode %d"), WiFi.getMode());
                break;
        }
    }
    else if (String_equals(key, F("WIFI_SSID"))) {
        if (WiFi.getMode() == WIFI_AP_STA) {
            output.print(F("Station connected to " HTML_S(strong)));
            WiFi_Station_SSID(output);
            output.print(F(HTML_E(strong) HTML_S(br) "Access Point " HTML_S(strong)));
            WiFi_SoftAP_SSID(output);
            output.print(F(HTML_E(strong)));
        }
        else if (WiFi.getMode() & WIFI_STA) {
            WiFi_Station_SSID(output);
        }
        else if (WiFi.getMode() & WIFI_AP) {
            WiFi_SoftAP_SSID(output);
        }
    }
    else if (String_equals(key, F("WIFI_STATUS"))) {
        WiFi_get_status(output);
    }
    else {
        WebTemplate::process(key, output);
    }
}

PasswordTemplate::PasswordTemplate(const String &errorMessage)
{
    _errorMessage = errorMessage;
}

void PasswordTemplate::process(const String &key, PrintHtmlEntitiesString &output)
{
    if (String_equals(key, F("PASSWORD_ERROR_MESSAGE"))) {
        output.print(_errorMessage);
    }
    else if (String_equals(key, F("PASSWORD_ERROR_CLASS"))) {
        if (_errorMessage.length() == 0) {
            output.print(FSPGM(_hidden));
        }
    }
    else {
        WebTemplate::process(key, output);
    }
}

WifiSettingsForm::WifiSettingsForm(AsyncWebServerRequest *request) : SettingsForm(request)
{
    using KFCConfigurationClasses::MainConfig;

    add<uint8_t>(F("mode"), _H_FLAGS_VALUE(Config().flags, wifiMode));
    addValidator(new FormRangeValidator(F("Invalid mode"), WIFI_OFF, WIFI_AP_STA));

    add(F("wifi_ssid"), _H_STR_VALUE(MainConfig().network.WiFiConfig._ssid));
    addValidator(new FormLengthValidator(1, sizeof(MainConfig().network.WiFiConfig._ssid) - 1));

    add(F("wifi_password"), _H_STR_VALUE(MainConfig().network.WiFiConfig._password));
    addValidator(new FormLengthValidator(8, sizeof(MainConfig().network.WiFiConfig._password) - 1));

    add(F("ap_wifi_ssid"), _H_STR_VALUE(MainConfig().network.WiFiConfig._softApSsid));
    addValidator(new FormLengthValidator(1, sizeof(MainConfig().network.WiFiConfig._softApSsid) - 1));

    add(F("ap_wifi_password"), _H_STR_VALUE(MainConfig().network.WiFiConfig._softApPassword));
    addValidator(new FormLengthValidator(8, sizeof(MainConfig().network.WiFiConfig._softApPassword) - 1));

    add<uint8_t>(F("channel"), _H_STRUCT_VALUE(MainConfig().network.softAp, _channel));
    addValidator(new FormRangeValidator(1, config.getMaxWiFiChannels()));

    add<uint8_t>(F("encryption"), _H_STRUCT_VALUE(MainConfig().network.softAp, _encryption));

    addValidator(new FormEnumValidator<uint8_t, WiFiEncryptionTypeArray().size()>(F("Invalid encryption"), WIFI_ENCRYPTION_ARRAY));

    add<bool>(F("ap_hidden"), _H_FLAGS_BOOL_VALUE(Config().flags, hiddenSSID), FormField::InputFieldType::CHECK);

    finalize();
}

NetworkSettingsForm::NetworkSettingsForm(AsyncWebServerRequest *request) : SettingsForm(request)
{
    using KFCConfigurationClasses::MainConfig;

    add(F("hostname"), _H_STR_VALUE(Config().device_name));
    addValidator(new FormLengthValidator(4, sizeof(Config().device_name) - 1));
    add(F("title"), _H_STR_VALUE(Config().device_title));
    addValidator(new FormLengthValidator(1, sizeof(Config().device_title) - 1));

    add<bool>(F("dhcp_client"), _H_FLAGS_BOOL_VALUE(Config().flags, stationModeDHCPEnabled));

    add(F("ip_address"), _H_STRUCT_IP_VALUE(MainConfig().network.settings, _localIp));
    add(F("subnet"), _H_STRUCT_IP_VALUE(MainConfig().network.settings, _subnet));
    add(F("gateway"), _H_STRUCT_IP_VALUE(MainConfig().network.settings, _gateway));
    add(F("dns1"), _H_STRUCT_IP_VALUE(MainConfig().network.settings, _dns1));
    add(F("dns2"), _H_STRUCT_IP_VALUE(MainConfig().network.settings, _dns2));

    add<bool>(F("softap_dhcpd"), _H_FLAGS_BOOL_VALUE(Config().flags, softAPDHCPDEnabled));

    add(F("dhcp_start"), _H_STRUCT_IP_VALUE(MainConfig().network.softAp, _dhcpStart));
    add(F("dhcp_end"), _H_STRUCT_IP_VALUE(MainConfig().network.softAp, _dhcpEnd));
    add(F("ap_ip_address"), _H_STRUCT_IP_VALUE(MainConfig().network.softAp, _address));
    add(F("ap_subnet"), _H_STRUCT_IP_VALUE(MainConfig().network.softAp, _subnet));

    finalize();
}

PasswordSettingsForm::PasswordSettingsForm(AsyncWebServerRequest *request) : SettingsForm(request)
{
    add(new FormField(F("password"), config._H_STR(Config().device_pass)));
    addValidator(new FormMatchValidator(F("The entered password is not correct"), [](FormField &field) {
        return field.getValue().equals(config._H_STR(Config().device_pass));
    }));

    add(F("password2"), _H_STR_VALUE(Config().device_pass))
        ->setValue(emptyString);

    addValidator(new FormRangeValidator(F("The password has to be at least %min% characters long"), 6, 0));
    addValidator(new FormRangeValidator(6, sizeof(Config().device_pass) - 1))
        ->setValidateIfValid(false);

    add(F("password3"), emptyString, FormField::InputFieldType::TEXT);
    addValidator(new FormMatchValidator(F("The password confirmation does not match"), [](FormField &field) {
            return field.equals(field.getForm().getField(F("password2")));
        }))
        ->setValidateIfValid(false);

    finalize();
}

SettingsForm::SettingsForm(AsyncWebServerRequest *request) : Form(&_data), _json(nullptr)
{
    _data.setCallbacks(
        [request](const String &name) {
            if (!request) {
                return String();
            }
            return request->arg(name);
        },
        [request](const String &name) {
            if (!request) {
                return false;
            }
            return request->hasArg(name.c_str());
        }
    );
}


File2String::File2String(const String &filename)
{
    _filename = filename;
}

String File2String::toString()
{
    return SPIFFS.open(_filename, fs::FileOpenMode::read).readString();
}

void File2String::fromString(const String &value)
{
    SPIFFS.open(_filename, fs::FileOpenMode::write).print(value);
    //write((const uint8_t *)value.c_str(), value.length());
}

// void EmptyTemplate::process(const String &key, PrintHtmlEntitiesString &output) {
// }

#include <TemplateDataProvider.h>

class PluginStatusStream {
public:
    PluginStatusStream() : _iterator(plugins.begin()) {
        _fillBuffer();
    }

    size_t readBytes(uint8_t *data, size_t size) {
        if (_buffer.length() == 0) {
            if (_fillBuffer() == 0) {
                return 0;
            }
        }
        if (size > _buffer.length()) {
            size = _buffer.length();
        }
        memcpy(data, _buffer.c_str(), size);
        _buffer.remove(0, size);
        return size;
    }

private:
    size_t _fillBuffer() {
        while (_iterator != plugins.end()) {
            auto &plugin = **_iterator;
            if (plugin.hasStatus()) {
                _buffer += F("<div class=\"row\"><div class=\"col-lg-3 col-lg-auto\">");
                _buffer += FPSTR(plugin.getFriendlyName());
                _buffer += "</div><div class=\"col\">";
                plugin.getStatus(_buffer);
                _buffer += "</div></div>";
                ++_iterator;
                break;
            }
            ++_iterator;
        }
        return _buffer.length();
    }

private:
    PrintHtmlEntitiesString _buffer;
    PluginsVector::iterator _iterator;
};


bool TemplateDataProvider::callback(const String& name, DataProviderInterface& provider, WebTemplate &webTemplate) {

    enum class FillBufferMethod {
        NONE = 0,
        PRINT_ARGS,
        BUFFER_STREAM
    };

    auto &printArgs = webTemplate.getPrintArgs();
    PrintHtmlEntitiesString value;
    FillBufferMethod fbMethod = FillBufferMethod::NONE;

    // menus
    if (String_equals(name, PSTR("MENU_HTML_MAIN"))) {
        bootstrapMenu.html(printArgs);
        fbMethod = FillBufferMethod::PRINT_ARGS;
    }
    else if (String_startsWith(name, PSTR("MENU_HTML_MAIN_"))) {
        bootstrapMenu.html(printArgs, bootstrapMenu.findMenuByLabel(name.substring(15)));
        fbMethod = FillBufferMethod::PRINT_ARGS;
    }
    else if (String_startsWith(name, PSTR("MENU_HTML_SUBMENU_"))) {
        bootstrapMenu.htmlSubMenu(printArgs, bootstrapMenu.findMenuByLabel(name.substring(18)), 0);
        fbMethod = FillBufferMethod::PRINT_ARGS;
    }
    // forms
    else if (String_equals(name, PSTR("FORM_HTML"))) {
        auto form = webTemplate.getForm();
        if (form) {
            form->createHtml(printArgs);
            fbMethod = FillBufferMethod::PRINT_ARGS;
        }
    }
    else if (String_equals(name, PSTR("FORM_VALIDATOR"))) {
        auto form = webTemplate.getForm();
        if (form) {
            form->createJavascript(printArgs);
            fbMethod = FillBufferMethod::PRINT_ARGS;
        }
    }
    else if (String_equals(name, PSTR("FORM_JSON"))) {
        auto json = webTemplate.getJson();
        if (json) {
            auto stream = std::shared_ptr<JsonBuffer>(new JsonBuffer(*json));
            provider.setFillBuffer([stream](uint8_t *buffer, size_t size) {
                return stream->fillBuffer(buffer, size);
            });
            return true;
        }
    }
    // plugin status
    else if (String_equals(name, PSTR("PLUGIN_STATUS"))) {
        auto stream = std::shared_ptr<PluginStatusStream>(new PluginStatusStream());
        provider.setFillBuffer([stream](uint8_t *buffer, size_t size) {
            return stream->readBytes(buffer, size);
        });
        return true;
    }
    // other variables
    else {
        webTemplate.process(name, value);
        // debug_printf_P(PSTR("%s=%s\n"), name.c_str(), value.c_str());
        fbMethod = FillBufferMethod::BUFFER_STREAM;
    }

    switch(fbMethod)  {
        case FillBufferMethod::PRINT_ARGS: {
                provider.setFillBuffer([&printArgs](uint8_t *buffer, size_t size) {
                    return printArgs.fillBuffer(buffer, size);
                });
                return true;
            }
        case FillBufferMethod::BUFFER_STREAM: {
                auto stream = std::shared_ptr<BufferStream>(new BufferStream(std::move(value)));
                provider.setFillBuffer([stream](uint8_t *buffer, size_t size) {
                    return stream->readBytes(buffer, size);
                });
                return true;
            }
        default:
        case FillBufferMethod::NONE:
            break;
    }
    return false;
}
