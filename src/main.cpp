
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include ".././lib/WebSockets/src/WebSocketsServer.h"
#define __AVR__ //for  WebSocketServerEvent
#ifndef APSSID
#define APSSID "adz"
#define APPSK  "Admin1234%"
#endif
/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t size) {
    switch (type) {
        case WStype_DISCONNECTED:            // if the websocket is disconnected
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {              // if a new websocket connection is established
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        }
            break;
        case WStype_TEXT:                     // if new text data is received
            Serial.printf("[%u] get Text: %s\n", num, payload);
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, size);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
        break;
        default:
            break;
    }
}



void startWebSocket() {
    webSocket.begin();
    webSocket.onEvent( webSocketEvent);
    Serial.println("WebSocket server started.");
}
void setup() {
    delay(1000);
    Serial.begin(115200);
    delay(10);
    Serial.println("\r\n");
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    startWebSocket();
}
void loop() {
    webSocket.loop();
}