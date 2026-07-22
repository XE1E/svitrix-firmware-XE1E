#pragma once

#include <Arduino.h>
#include "IButtonReporter.h"

class IDisplayRenderer;
class IDisplayControl;
class IDisplayNavigation;
class IDisplayNotifier;
class ISound;
class IPower;
class IUpdater;

class ServerManager_ : public IButtonReporter
{
  private:
    ServerManager_() = default;
    ServerManager_(const ServerManager_&) = delete;
    ServerManager_& operator=(const ServerManager_&) = delete;
    ServerManager_(ServerManager_&&) = delete;
    ServerManager_& operator=(ServerManager_&&) = delete;

    void (*onMqttConfigChanged_)() = nullptr;
    // Set from the async web handler, consumed on the main loop in tick().
    // Deferring keeps MQTT reconnect (mqtt.disconnect/begin + delays, and a
    // first-time setup() that allocates 63 HA entities) OFF the async task,
    // so it never races with mqtt.loop() on the loop or blocks the web server.
    volatile bool mqttReconnectPending_ = false;

  public:
    static ServerManager_& getInstance();
    void setDisplay(IDisplayRenderer *r, IDisplayControl *c, IDisplayNavigation *n, IDisplayNotifier *nt);
    bool hasDisplay() const;
    void setServices(ISound *s, IPower *p, IUpdater *u);
    bool hasServices() const;
    void setMqttReconnectCallback(void (*cb)())
    {
        onMqttConfigChanged_ = cb;
    }
    // Called from the async /save handler — only flags the reconnect; the
    // actual work runs on the main loop in tick() (see mqttReconnectPending_).
    void triggerMqttReconnect()
    {
        mqttReconnectPending_ = true;
    }
    void setup();
    void tick();
    void initConfigDefaults();
    void loadSettings();
    void sendButton(byte btn, bool state) override;
    void erase();
    void sendTCP(String message);
    void shutdown();
    bool isConnected;
    IPAddress myIP;
};

extern ServerManager_& ServerManager;
