#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <RMTT_Libs.h>
#include "../include/Route.h"
#include "../include/Utils.h"

#define PORT 5001

/* Measures in cm */
#define MAX_MEASURE 1000
#define UP_DODGE 50
#define DOWN_DODGE 50
#define LEFT_DODGE 50
#define RIGHT_DODGE 50

#define missionDELAY (pdMS_TO_TICKS(100))
#define logDELAY (pdMS_TO_TICKS(500))

#define logQueueDELAY (pdMS_TO_TICKS(500))

#define logQueueSIZE 5
#define logItemSIZE (UBaseType_t)100 * (sizeof(char)) // 100 chars

#define SSID "LCD"
#define PASSWORD "1cdunc0rd0ba"

/*-------------- Global Variables --------------*/

WiFiServer wifiServer(PORT);
WiFiClient client;

RMTT_TOF tt_sensor;
RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
Utils *utils = Utils::getInstance();

uint8_t isFirstTime = 1, point_index = 0, tries = 0;
uint16_t measure = 0;

Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t missionTaskHandle, loggerTaskHandle;

QueueHandle_t xLogQueue = NULL;

/*-------------- Fuction Declaration --------------*/

void vMissionTask(void *parameter);
void vLogTask(void *parameter);

void initialCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);
boolean tofSense(std::function<void()> callback);
void dodgeFun();
