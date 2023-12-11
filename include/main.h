#include "ConaeApi.h"
#include "Utils.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <RMTT_Libs.h>
#include "Route.h"

#define PORT 5001

#define missionDELAY (pdMS_TO_TICKS(100))
#define logDELAY (pdMS_TO_TICKS(500))

#define logQueueDELAY (pdMS_TO_TICKS(500))

#define logQueueSIZE 5
#define logItemSIZE (UBaseType_t)100 * (sizeof(char)) // 100 chars

#define SSID "Fibertel WiFi142 2.4GHz" // "LCD" // "Fibertel WiFi576 2.4GHz"
#define PASSWORD "0141745658"           // "1cdunc0rd0ba" // "00436133012"

/*-------------- Global Variables --------------*/
WiFiServer wifiServer(PORT);
WiFiClient client;
RMTT_TOF tt_sensor;
RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
Utils *utils = Utils::getInstance();

Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t missionTaskHandle, loggerTaskHandle;

QueueHandle_t xLogQueue = NULL;

/*-------------- Fuction Declaration --------------*/

void vLogTask(void *parameter);
void initialCallback(char *cmd, String res);