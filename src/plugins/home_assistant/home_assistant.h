/**
  Author: sascha_lammers@gmx.de
*/

#pragma once

#include <Arduino_compat.h>
#include "KFCRestApi.h"
#include "progmem_data.h"
#include "HassJsonReader.h"
#include "plugins.h"

#ifndef DEBUG_HOME_ASSISTANT
#define DEBUG_HOME_ASSISTANT                            0
#endif

class HassPlugin : public PluginComponent, /*public WebUIInterface, */public KFCRestAPI {
// PluginComponent
public:
    using ActionEnum_t = KFCConfigurationClasses::Plugins::HomeAssistant::ActionEnum_t;
    using Action = KFCConfigurationClasses::Plugins::HomeAssistant::Action;

    typedef std::function<void(bool status)> StatusCallback_t;
    typedef std::function<void(HassJsonReader::GetState *state, KFCRestAPI::HttpRequest &request, StatusCallback_t statusCallback)> GetStateCallback_t;
    typedef std::function<void(HassJsonReader::CallService *service, KFCRestAPI::HttpRequest &request, StatusCallback_t statusCallback)> ServiceCallback_t;

    HassPlugin() {
        REGISTER_PLUGIN(this);
    }

    static HassPlugin &getInstance();

    virtual PGM_P getName() const {
        return PSTR("hass");
    }
    virtual const __FlashStringHelper *getFriendlyName() const {
        return F("Home Assistant");
    }
    virtual PluginPriorityEnum_t getSetupPriority() const {
        return PRIO_HASS;
    }
    virtual bool autoSetupAfterDeepSleep() const override {
        return true;
    }

    virtual bool hasStatus() const override {
        return true;
    }
    virtual void getStatus(Print &output) override;


    virtual PGM_P getConfigureForm() const override {
        return getName();
    }
    virtual bool canHandleForm(const String &formName) const {
        return (formName.equals(FPSTR(getName())) || formName.equals(F("hass_edit")) || formName.equals(F("hass_actions")));
    }
    virtual void createConfigureForm(AsyncWebServerRequest *request, Form &form) override;

    virtual void setup(PluginSetupMode_t mode);
    virtual void reconfigure(PGM_P source);
    virtual bool hasReconfigureDependecy(PluginComponent *plugin) const {
        return plugin->nameEquals(FSPGM(http));
    }


#if AT_MODE_SUPPORTED
    // at mode command handler
    virtual bool hasAtMode() const override {
        return true;
    }
    virtual void atModeHelpGenerator() override;
    virtual bool atModeHandler(AtModeArgs &args) override;
#endif

// // WebUIInterface
// public:
//     virtual bool hasWebUI() const override {
//         return true;
//     }
//     virtual void createWebUI(WebUI &webUI) override;
//     virtual WebUIInterface *getWebUIInterface() override {
//         return this;
//     }

//     virtual void getValues(JsonArray &array);
//     virtual void setValue(const String &id, const String &value, bool hasValue, bool state, bool hasState);

// KFCRestApi
public:
    virtual void getRestUrl(String &url) const;
    virtual void getBearerToken(String &token) const;

public:
    void executeAction(const Action &action, StatusCallback_t statusCallback);

    void getState(const String &entityId, GetStateCallback_t callback, StatusCallback_t statusCallback);
    void callService(const String &service, const JsonUnnamedObject &payload, ServiceCallback_t callback, StatusCallback_t statusCallback);

    static void _serviceCallback(HassJsonReader::CallService *service, KFCRestAPI::HttpRequest &request, StatusCallback_t statusCallback);

private:
    String _getDomain(const String &entityId);

// web server
public:
    static void removeAction(AsyncWebServerRequest *request);

private:
    void _installWebhooks();
};
