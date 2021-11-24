
#include <Arduino.h>
#include ".././lib/ArduinoJson/src/ArduinoJson.h"
#include <ESP8266WiFi.h>
#include <bits/basic_string.h>
#include ".././lib/WebSockets/src/WebSocketsServer.h"
#define __AVR__ //for  WebSocketServerEvent
#ifndef APSSID
#define APSSID "adz"
#define APPSK  "Admin1234%"
#define SERVER_PORT 81;
#endif
#define CONNECT_PIN 13
/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;
int brightness[4] = {10, 80, 150, 255};
int port = SERVER_PORT;
struct BaseModel{
    String type;

}baseModel;
struct LedModel : BaseModel{
   int brightness;

} ledModel;

WebSocketsServer webSocket = WebSocketsServer(port);

void initPins(){
    pinMode (CONNECT_PIN, OUTPUT);
}
void onStartWifi(){
    analogWrite(CONNECT_PIN, brightness[0]);
}
void onConnectWS(){
    analogWrite(CONNECT_PIN, brightness[1]);
}
void onDisconnectWS(){
    analogWrite(CONNECT_PIN, brightness[0]);
}

void onMessageWS(String json){
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);

    // Test if parsing succeeds.
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    JsonObject root = doc.to<JsonObject>();
    //Serial.println(String((char )root["type"]));
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t size) {
    switch (type) {
        case WStype_DISCONNECTED: {
            // if the websocket is disconnected
            Serial.printf("[%u] Disconnected!\n", num);
            onDisconnectWS();
        }
        break;

        case WStype_CONNECTED: {              // if a new websocket connection is established
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            onConnectWS();
        }
        break;

        case WStype_TEXT: {
            Serial.printf("[%u] get Text: %s\n", num, payload);
            String str = String((char *)payload);
            onMessageWS(str);
        }                  // if new text data is received
        break;

        case WStype_BIN:{
            Serial.printf("[%u] get binary length: %u\n", num, size);
            // send message to client
            // webSocket.sendBIN(num, payload, length);
        }
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
    initPins();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    onStartWifi();
    startWebSocket();
}
void loop() {
    webSocket.loop();
}
