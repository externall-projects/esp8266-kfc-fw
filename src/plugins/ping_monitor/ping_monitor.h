/**
  Author: sascha_lammers@gmx.de
*/

#pragma once

#ifndef DEBUG_PING_MONITOR
#define DEBUG_PING_MONITOR                  0
#endif

#include <Arduino_compat.h>
#include <ESPAsyncWebServer.h>
#include <AsyncPing.h>
#include "web_socket.h"

using AsyncPingPtr = std::shared_ptr<AsyncPing>;

class WsPingClient : public WsClient {
public:
    WsPingClient(AsyncWebSocketClient *client);
    virtual ~WsPingClient();

    static WsClient *getInstance(AsyncWebSocketClient *socket);

    virtual void onText(uint8_t *data, size_t len) override;
    virtual void onDisconnect(uint8_t *data, size_t len) override;
    virtual void onError(WsErrorType type, uint8_t *data, size_t len) override;

    AsyncPingPtr &getPing();

private:
    void _cancelPing();
    AsyncPingPtr _ping;
};

class PingMonitorTask {
public:
    using PingMonitorTaskPtr = std::shared_ptr<PingMonitorTask>;

    class PingHost {
    public:
        PingHost(const String &_host = String()) : host(_host), success(0), failure(0) {
        }
        String host;
        uint32_t success;
        uint32_t failure;
    } ;

    typedef std::vector<PingHost> PingVector;

    PingMonitorTask();
    ~PingMonitorTask();

    void setInterval(uint16_t interval);
    void setCount(uint8_t count);
    void setTimeout(uint16_t timeout);

    void clearHosts();
    void addHost(String host);

    void start();
    void stop();

    void addAnswer(bool answer);
    void next();
    void begin();

    void printStats(Print &out);

    inline bool isNext() const {
        return (_nextHost != 0 && millis() > _nextHost);
    }

private:
    void _cancelPing();

    uint8_t _currentServer;
    uint8_t _count;
    uint16_t _interval;
    uint16_t _timeout;
    unsigned long _nextHost;
    PingVector _pingHosts;
    AsyncPingPtr _ping;
};
