#include "ESPNowDriver.h"
#include <esp_mac.h>


ESPNowDriver::ESPNowDriver(uint8_t channel)
    : channel(channel),
      receiveCallback(nullptr),
      broadcastPeer(nullptr) {
}


// ===============================
// PEER
// ===============================

ESPNowDriver::Peer::Peer(
    const uint8_t *mac,
    uint8_t channel,
    ESPNowDriver *driver
)
    : ESP_NOW_Peer(
        mac,
        channel,
        WIFI_IF_STA,
        nullptr
      ),
      driver(driver) {
}


bool ESPNowDriver::Peer::begin() {
    return add();
}


void ESPNowDriver::Peer::onReceive(
    const uint8_t *data,
    size_t len,
    bool broadcast
) {
    if (driver->receiveCallback) {
        driver->receiveCallback(
            addr(),
            data,
            len
        );
    }
}


void ESPNowDriver::Peer::onSent(bool success) {
    Serial.print("Unicast: ");
    Serial.println(
        success ? "SUCCESS" : "FAILED"
    );
}


// ===============================
// BROADCAST PEER
// ===============================

ESPNowDriver::BroadcastPeer::BroadcastPeer(
    uint8_t channel
)
    : ESP_NOW_Peer(
        ESP_NOW.BROADCAST_ADDR,
        channel,
        WIFI_IF_STA,
        nullptr
      ) {
}


bool ESPNowDriver::BroadcastPeer::begin() {
    return add();
}


bool ESPNowDriver::BroadcastPeer::sendMessage(
    const uint8_t *data,
    size_t len
) {
    return send(data, len) > 0;
}


void ESPNowDriver::BroadcastPeer::onReceive(
    const uint8_t *data,
    size_t len,
    bool broadcast
) {
}


void ESPNowDriver::BroadcastPeer::onSent(
    bool success
) {
    Serial.print("Broadcast: ");
    Serial.println(
        success ? "SUCCESS" : "FAILED"
    );
}


// ===============================
// BEGIN
// ===============================

bool ESPNowDriver::begin() {

    WiFi.mode(WIFI_STA);

    // Wait until Wi-Fi STA interface is started
    while (!WiFi.STA.started()) {
        delay(10);
    }


    // =================================================
    // AUTOMATIC CHANNEL SELECTION
    // =================================================

    if (channel == 0) {

        channel = WiFi.channel();

        Serial.println();
        Serial.println(
            "ESP-NOW channel automatically selected."
        );
    }


    Serial.println();
    Serial.println("=== ESP-NOW DRIVER ===");

    Serial.print("MAC: ");
    Serial.println(
        WiFi.macAddress()
    );

    Serial.print("Wi-Fi channel: ");
    Serial.println(
        WiFi.channel()
    );

    Serial.print("ESP-NOW channel: ");
    Serial.println(
        channel
    );


    // =================================================
    // ESP-NOW INITIALIZATION
    // =================================================

    if (!ESP_NOW.begin()) {

        Serial.println(
            "ESP-NOW INIT FAILED"
        );

        return false;
    }

    Serial.println(
        "ESP-NOW initialized"
    );


    // =================================================
    // BROADCAST PEER
    // =================================================

    broadcastPeer =
        new BroadcastPeer(channel);


    if (!broadcastPeer->begin()) {

        Serial.println(
            "Broadcast peer FAILED"
        );

        return false;
    }


    // =================================================
    // LISTEN FOR UNKNOWN DEVICES
    // =================================================

    ESP_NOW.onNewPeer(
        ESPNowDriver::newPeerCallback,
        this
    );


    Serial.println(
        "Broadcast peer ready"
    );

    Serial.println(
        "Waiting for vehicles..."
    );


    return true;
}


// ===============================
// BROADCAST
// ===============================

bool ESPNowDriver::broadcast(
    const uint8_t *data,
    size_t len
) {
    if (!broadcastPeer) {
        return false;
    }

    return broadcastPeer->sendMessage(
        data,
        len
    );
}


bool ESPNowDriver::broadcast(
    const char *message
) {
    return broadcast(
        (const uint8_t *)message,
        strlen(message) + 1
    );
}


// ===============================
// NEW PEER
// ===============================

void ESPNowDriver::newPeerCallback(
    const esp_now_recv_info_t *info,
    const uint8_t *data,
    int len,
    void *arg
) {

    ESPNowDriver *driver =
        static_cast<ESPNowDriver *>(arg);


    driver->handleNewPeer(
        info->src_addr
    );


    // Deliver the packet too
    if (driver->receiveCallback) {

        driver->receiveCallback(
            info->src_addr,
            data,
            len
        );
    }
}


// ===============================
// HANDLE NEW PEER
// ===============================

void ESPNowDriver::handleNewPeer(
    const uint8_t *mac
) {

    if (peerExists(mac)) {
        return;
    }


    Serial.print(
        "New vehicle discovered: "
    );


    for (int i = 0; i < 6; i++) {

        if (i > 0) {
            Serial.print(":");
        }

        Serial.printf(
            "%02X",
            mac[i]
        );
    }

    Serial.println();


    Peer *peer =
        new Peer(
            mac,
            channel,
            this
        );


    if (!peer->begin()) {

        Serial.println(
            "Failed to register vehicle"
        );

        delete peer;

        return;
    }


    peers.push_back(peer);


    Serial.print(
        "Vehicle registered. Total: "
    );

    Serial.println(
        peers.size()
    );
}


// ===============================
// PEER CHECK
// ===============================

bool ESPNowDriver::peerExists(
    const uint8_t *mac
) {

    for (auto peer : peers) {

        if (
            memcmp(
                peer->addr(),
                mac,
                6
            ) == 0
        ) {

            return true;
        }
    }

    return false;
}


// ===============================
// PEER COUNT
// ===============================

int ESPNowDriver::getPeerCount() {
    return peers.size();
}


// ===============================
// CALLBACK
// ===============================

void ESPNowDriver::setReceiveCallback(
    ReceiveCallback callback
) {
    receiveCallback = callback;
}
