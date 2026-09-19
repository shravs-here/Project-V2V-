#ifndef ESPNOW_DRIVER_H
#define ESPNOW_DRIVER_H

#include <Arduino.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <vector>

class ESPNowDriver {
public:
    using ReceiveCallback =
        void (*)(const uint8_t *mac, const uint8_t *data, size_t len);

    // channel = 0 means automatically use the connected Wi-Fi channel
    ESPNowDriver(uint8_t channel = 0);

    bool begin();
    bool broadcast(const uint8_t *data, size_t len);
    bool broadcast(const char *message);

    void setReceiveCallback(ReceiveCallback callback);

    int getPeerCount();

private:
    uint8_t channel;
    ReceiveCallback receiveCallback;

    class Peer : public ESP_NOW_Peer {
    public:
        ESPNowDriver *driver;

        Peer(
            const uint8_t *mac,
            uint8_t channel,
            ESPNowDriver *driver
        );

        bool begin();

        void onReceive(
            const uint8_t *data,
            size_t len,
            bool broadcast
        ) override;

        void onSent(bool success) override;
    };

    class BroadcastPeer : public ESP_NOW_Peer {
    public:
        BroadcastPeer(uint8_t channel);

        bool begin();
        bool sendMessage(const uint8_t *data, size_t len);

        void onReceive(
            const uint8_t *data,
            size_t len,
            bool broadcast
        ) override;

        void onSent(bool success) override;
    };

    BroadcastPeer *broadcastPeer;
    std::vector<Peer *> peers;

    static void newPeerCallback(
        const esp_now_recv_info_t *info,
        const uint8_t *data,
        int len,
        void *arg
    );

    void handleNewPeer(
        const uint8_t *mac
    );

    bool peerExists(const uint8_t *mac);
};

#endif

