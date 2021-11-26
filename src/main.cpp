#include <Arduino.h>
#include <iostream>
#include <sstream>
#include ".././lib/ArduinoJson/src/ArduinoJson.h"
#include <ESP8266WiFi.h>
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
int port = SERVER_PORT;

struct BaseModel {
    String type = "";
    virtual String toJsonString() = 0;
};

struct LedModel :  BaseModel {
int brightness = 0;
    String type = "LedModel";
    String toJsonString(){
        StaticJsonDocument<200> doc;
        JsonObject root = doc.to<JsonObject>();
        root["type"] = type;
        root["brightness"] = brightness;
        String output;
        serializeJson(root,output);
        return output;
    };
} ledModel;

struct LedModel *pLedModel = &ledModel;

void updateLedStructure(LedModel *ledModel, JsonObject &object){
    JsonVariant brightness = object.getMember("brightness");
    ledModel->brightness = brightness.as<int>();
}
void lightingLoop(LedModel *ledModel){
    analogWrite(CONNECT_PIN, ledModel->brightness);
}

WebSocketsServer webSocket = WebSocketsServer(port);

void initPins(){
    pinMode (CONNECT_PIN, OUTPUT);
}
void onStartWifi(){}

void onConnectWS(){

}

void onDisconnectWS(){}

void onMessageWS(String json){
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    JsonObject root = doc.as<JsonObject>();
    JsonVariant type = root.getMember("type");
    if(type.as<String>() == pLedModel->type){

        updateLedStructure(pLedModel, root);
    }
}
void sendModel(BaseModel *event){
    String json = event->toJsonString();
    Serial.println("sendModel: " + json);
    webSocket.sendTXT(0,json);
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
            sendModel(&ledModel);
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
    lightingLoop(pLedModel);
}
