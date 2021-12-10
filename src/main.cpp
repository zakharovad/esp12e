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
/*
NodeMcu   Esp12e
d0      - 16pin
d1      - 5pin
d2      - 4pin
d3      - 0pin
d4      - 2pin
d5      - 14pin
d6      - 12pin
d7      - 13pin
d8      - 15pin
 */
//left motors
#define ENA_PIN 12 //for driver ena
#define IN1_PIN 0 //for driver in1
#define IN2_PIN 2 //for driver in1

//right motors
#define ENB_PIN 14 //for driver enb
#define IN3_PIN 15 //for driver in2
#define IN4_PIN 13 //for driver in2

//for driver  deceleration ratio
#define SPEED_СOEF 3
//for brightness
#define LED_PIN 5
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
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN3_PIN, LOW);
            digitalWrite(IN4_PIN, LOW);
            analogWrite(ENA_PIN, driveModel->speed);
            analogWrite(ENB_PIN, driveModel->speed);
            break;
        case moveForward:
            Serial.printf("moveForward direction %d speed %d\n", driveModel->direction, driveModel->speed);
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            analogWrite(ENB_PIN, driveModel->speed);
            digitalWrite(IN3_PIN, HIGH);
            digitalWrite(IN4_PIN, LOW);
            break;
        case moveBack:
            Serial.printf("moveBack direction %d speed %d\n", driveModel->direction, driveModel->speed);
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, HIGH);
            analogWrite(ENB_PIN, driveModel->speed);
            digitalWrite(IN3_PIN, LOW);
            digitalWrite(IN4_PIN, HIGH);
            break;
        case moveRight:
            Serial.printf("moveRight direction %d speed %d\n", driveModel->direction, driveModel->speed);
            analogWrite(ENA_PIN, driveModel->speed);
            analogWrite(ENB_PIN, 0);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            digitalWrite(IN3_PIN, LOW);
            digitalWrite(IN4_PIN, LOW);
            break;
        case moveLeft:
            Serial.printf("moveLeft direction %d speed %d\n", driveModel->direction, driveModel->speed);
            analogWrite(ENB_PIN, driveModel->speed);
            analogWrite(ENA_PIN, 0);
            digitalWrite(IN3_PIN, HIGH);
            digitalWrite(IN4_PIN, LOW);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, LOW);
            break;
        case (moveForward | moveLeft):
            analogWrite(ENA_PIN, driveModel->speed/SPEED_СOEF);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            analogWrite(ENB_PIN, driveModel->speed);
            digitalWrite(IN3_PIN, HIGH);
            digitalWrite(IN4_PIN, LOW);
            break;
        case (moveForward | moveRight):
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            analogWrite(ENB_PIN, driveModel->speed/SPEED_СOEF);
            digitalWrite(IN3_PIN, HIGH);
            digitalWrite(IN4_PIN, LOW);
            break;
        case (moveBack | moveLeft):
            analogWrite(ENA_PIN, driveModel->speed/SPEED_СOEF);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, HIGH);
            analogWrite(ENB_PIN, driveModel->speed);
            digitalWrite(IN3_PIN, LOW);
            digitalWrite(IN4_PIN, HIGH);
            break;
        case (moveBack | moveRight):
            analogWrite(ENA_PIN, driveModel->speed);
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, HIGH);
            analogWrite(ENB_PIN, driveModel->speed/SPEED_СOEF);
            digitalWrite(IN3_PIN, LOW);
            digitalWrite(IN4_PIN, HIGH);
            break;

    }

}

WebSocketsServer webSocket = WebSocketsServer(port);

void initPins(){
    pinMode (LED_PIN, OUTPUT);
    pinMode (ENA_PIN, OUTPUT);
    pinMode (IN1_PIN, OUTPUT);
    pinMode (IN2_PIN, OUTPUT);
    pinMode (ENB_PIN, OUTPUT);
    pinMode (IN3_PIN, OUTPUT);
    pinMode (IN4_PIN, OUTPUT);
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
void testDriver() {
    if(test){
        digitalWrite(IN1_PIN, HIGH);
        digitalWrite(IN2_PIN, LOW);
        digitalWrite(IN3_PIN, HIGH);
        digitalWrite(IN4_PIN, LOW);
    }else{
        digitalWrite(IN1_PIN, LOW);
        digitalWrite(IN2_PIN, HIGH);
        digitalWrite(IN3_PIN, LOW);
        digitalWrite(IN4_PIN, HIGH);
    }
    test = !test;
    delay(2000);
}
void loop() {

    webSocket.loop();
    //testDriver();
    lightingLoop(pLedModel);
    drivingLoop(pDriveModel);

}
