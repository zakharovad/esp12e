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
#define LED_PIN 13 //for brightness
#define ENA_PIN 12 //for driver ena
#define IN1_PIN 16 //for driver in1
#define IN2_PIN 15 //for driver in1

#define SPEED_СOEF 5 //for driver  deceleration ratio

/* Set these to your desired credentials. */
//bit mask driver state
const unsigned int  moveForward = 1; //0001;
const unsigned int  moveBack = 2;//0010;
const unsigned int  moveLeft = 4;//0100;
const unsigned int  moveRight = 8;//1000;
const unsigned int  moveStop = 0;//0000;
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

struct DriveModel :  BaseModel {
    int speed = 0;
    int direction = 0;
    String type = "DriveModel";
    String toJsonString(){
        StaticJsonDocument<200> doc;
        JsonObject root = doc.to<JsonObject>();
        root["type"] = type;
        root["direction"] = direction;
        root["speed"] = speed;
        String output;
        serializeJson(root,output);
        return output;
    };
} driveModel;

struct LedModel *pLedModel = &ledModel;
struct DriveModel *pDriveModel = &driveModel;

void updateLedStructure(LedModel *ledModel, JsonObject &object){
    JsonVariant brightness = object.getMember("brightness");
    ledModel->brightness = brightness.as<int>();
}
void updateDriveStructure(DriveModel *driveModel, JsonObject &object){
    JsonVariant speed = object.getMember("speed");
    JsonVariant direction = object.getMember("direction");
    driveModel->speed = speed.as<int>();
    driveModel->direction = direction.as<int>();
    Serial.printf("updateDriveStructure: %d  speed: %d\n", driveModel->direction,driveModel->speed);
}

void lightingLoop(LedModel *ledModel){
    analogWrite(LED_PIN, ledModel->brightness);
}
void drivingLoop(DriveModel *driveModel){

    switch(driveModel->direction){
        case moveStop:
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, LOW);
            break;
        case moveForward:
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            break;
        case moveBack:
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, HIGH);
            break;
        case moveRight:
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            break;
        case moveLeft:
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, LOW);
            break;
        case (moveForward | moveLeft):
            analogWrite(ENA_PIN, driveModel->speed/SPEED_СOEF);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            break;
        case (moveForward | moveRight):
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            break;
        case (moveBack | moveLeft):
            analogWrite(ENA_PIN, driveModel->speed/SPEED_СOEF);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, HIGH);
            break;
        case (moveBack | moveRight):
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, HIGH);
            break;

    }

}

WebSocketsServer webSocket = WebSocketsServer(port);

void initPins(){
    pinMode (LED_PIN, OUTPUT);
    pinMode (ENA_PIN, OUTPUT);
    pinMode (IN1_PIN, OUTPUT);
    pinMode (IN2_PIN, OUTPUT);
    pinMode (11, OUTPUT);
    pinMode (10, OUTPUT);
    pinMode (9, OUTPUT);
    pinMode (8, OUTPUT);
    pinMode (7, OUTPUT);
    pinMode (6, OUTPUT);
    pinMode (5, OUTPUT);
    pinMode (4, OUTPUT);
    pinMode (2, OUTPUT);
    pinMode (3, OUTPUT);
    pinMode (1, OUTPUT);
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
    if(type.as<String>() == pDriveModel->type){

        updateDriveStructure(pDriveModel, root);
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
void testOn() {
    digitalWrite (11, HIGH);
    digitalWrite (10, HIGH);
    digitalWrite (9, HIGH);
    digitalWrite (8, HIGH);
    digitalWrite (7, HIGH);
    digitalWrite (6, HIGH);
    digitalWrite (5, HIGH);
    digitalWrite (4, HIGH);
    digitalWrite (3, HIGH);
    digitalWrite (2, HIGH);
    digitalWrite (1, HIGH);
    delay(2000);
}
void testOf() {
    digitalWrite (11, LOW);
    digitalWrite (10, LOW);
    digitalWrite (9, LOW);
    digitalWrite (8, LOW);
    digitalWrite (7, LOW);
    digitalWrite (6, LOW);
    digitalWrite (5, LOW);
    digitalWrite (4, LOW);
    digitalWrite (3, LOW);
    digitalWrite (2, LOW);
    digitalWrite (1, LOW);
    delay(2000);
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
bool test = true;
void loop() {
    if(test){
        testOn();
        test = false;
    }else{
        testOf();
        test = true;
    }
    webSocket.loop();
    lightingLoop(pLedModel);
    drivingLoop(pDriveModel);
    //digitalWrite (4, 1);
    //digitalWrite (4, 1);
    //digitalWrite (3, 1);
    //digitalWrite (1, 1);
}
