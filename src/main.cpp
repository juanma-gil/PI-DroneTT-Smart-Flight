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
#define sendCmdDELAY (pdMS_TO_TICKS(100))
#define recvCmdDELAY (pdMS_TO_TICKS(100))
#define dodgeDELAY (pdMS_TO_TICKS(10))
#define logDELAY (pdMS_TO_TICKS(100))
#define logQueueDELAY (pdMS_TO_TICKS(50))

#define logQueueSIZE 5
#define logItemSIZE (UBaseType_t)100 * (sizeof(char)) // 100 chars

/*-------------- Global Variables --------------*/

const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";

WiFiServer wifiServer(PORT);
WiFiClient client;

RMTT_TOF tt_sensor;
RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
Utils *utils = Utils::getInstance();

uint8_t isFirstTime = 1;
uint16_t point_index = 0;
uint16_t measure = 0;

Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t sensorTaskHandle, sendCmdTaskHandle, recvCmdTaskHandle, loggerTaskHandle;

QueueHandle_t xLogQueue = NULL;
QueueHandle_t xSendCmdQueue = NULL;
QueueHandle_t xRecvCmdQueue = NULL;

SemaphoreHandle_t xSensorMutex = NULL;
int count;

/*-------------- Fuction Declaration --------------*/

void vSendCmdTask(void *parameter);
void vRecvCmdTask(void *parameter);
void vLogFunction(void *parameter);
void missionCallback(char *cmd, String res);

/*-------------- Fuction Implementation --------------*/

void setup()
{
    ttRGB->Init();
    route = route->getInstance();
    routePoints = route->getRoute();
    xSensorMutex = xSemaphoreCreateBinary();
    count = 0;

    Serial.begin(115200);
    Serial1.begin(1000000, SERIAL_8N1, 23, 18);

    /* Initialize sensor */
    Wire.begin(27, 26);
    Wire.setClock(100000);
    tt_sensor.SetTimeout(500);
    if (!tt_sensor.Init())
    {
        utils->slog("Failed to detect and initialize sensor!");
        while (1)
        {
            delay(10);
        }
    }

    /*---------------------------------------------------------*/

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        utils->slog("Connecting to WiFi..");
    }

    utils->slog("Connected to the WiFi network");
    Serial.println(WiFi.localIP());
    ttRGB->SetRGB(200, 255, 0);
    wifiServer.begin();

    while (!client)
    {
        client = wifiServer.available();
        delay(10);
    };

    utils->slog("Client connected");

    if (xSensorMutex == NULL)
    {
        client.write("Mutex creation has FAILED");
    }

    while (routePoints->empty())
    {
        delay(10);
        route->receiveRouteFromClient(&client);
    }

    Serial1.flush();
    ttSDK->sdkOn();
    ttRGB->SetRGB(200, 0, 255);

    /*-------------- Queues --------------*/

    xLogQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
    if (xLogQueue == NULL)
    {
        utils->slog("Queue creation has FAILED");
    }

    xSendCmdQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
    if (xSendCmdQueue == NULL)
    {
        utils->slog("Queue creation has FAILED");
    }

    xRecvCmdQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
    if (xRecvCmdQueue == NULL)
    {
        utils->slog("Queue creation has FAILED");
    }

    /*-------------- Tasks  --------------*/

    if (xTaskCreatePinnedToCore(vLogFunction, "Logger", configMINIMAL_STACK_SIZE * 4, NULL, 20, &loggerTaskHandle, 0) != pdPASS)
    {
        utils->slog("Failed to create Logger task");
    }
    if (xTaskCreatePinnedToCore(vRecvCmdTask, "ReceiveCmd", configMINIMAL_STACK_SIZE * 4, NULL, 20, &recvCmdTaskHandle, 1) != pdPASS)
    {
        utils->slog("Failed to create ReceiveCmd task");
    };
    if (xTaskCreatePinnedToCore(vSendCmdTask, "SendCmd", configMINIMAL_STACK_SIZE * 5, NULL, 20, &sendCmdTaskHandle, 0) != pdPASS)
    {
        utils->slog("Failed to create SendCmd task");
    };
}

void loop()
{
    char msg[100];
    delay(3000);
    sprintf(msg, "[TELLO] Re%02x%02x %s", ++count, 1, "takeoff");
    if (xQueueSend(xSendCmdQueue, msg, 10) == pdFAIL)
    {
        utils->slog("Loop fail to send motoron");
    }
    delay(10000);
    sprintf(msg, "[TELLO] Re%02x%02x %s",  ++count, 1, "go 300 0 0 20");
    if (xQueueSend(xSendCmdQueue, msg, 10) == pdFAIL)
    {
        utils->slog("Loop fail to send go");
    }
    delay(8000);
    sprintf(msg, "[TELLO] Re%02x%02x %s",  ++count, 1, "stop");
    if (xQueueSend(xSendCmdQueue, msg, 10) == pdFAIL)
    {
        utils->slog("Loop fail to send up 40");
    }
    delay(2000);
    sprintf(msg, "[TELLO] Re%02x%02x %s",  ++count, 1, "up 40");
    if (xQueueSend(xSendCmdQueue, msg, 10) == pdFAIL)
    {
        utils->slog("Loop fail to send up 40");
    }
    delay(4000);
    sprintf(msg, "[TELLO] Re%02x%02x %s",  ++count, 1, "land");
    if (xQueueSend(xSendCmdQueue, msg, 10) == pdFAIL)
    {
        utils->slog("Loop fail to send land");
    }
}

void vSendCmdTask(void *parameter)
{
    char msg[100];
    for (;;)
    {
        vTaskDelay(sendCmdDELAY);
        if (xQueueReceive(xSendCmdQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            Serial1.printf(msg);
            sprintf(msg, "%s", msg);
            xQueueSend(xLogQueue, &msg, logQueueDELAY);
        }
    }
}

void vRecvCmdTask(void *parameter)
{
    String rsp = "";
    char msg[100];
    
    for (;;)
    {
        vTaskDelay(recvCmdDELAY);
        while (Serial1.available())
        {
            // Serial.printf("Waiting for response\n");
            rsp += String(char(Serial1.read()));
        }
        if(rsp.length() == 0) continue;
        if (xQueueSend(xLogQueue, rsp.c_str(), logQueueDELAY) == pdFAIL)
        {
            utils->slog("RecvCmdTask: Fail to send msg to queue");
        }
        else
        {
            sprintf(msg, "Drone response: %s\n", rsp.c_str());
            utils->slog(msg);
        }
        rsp = "";
    }
}

void vLogFunction(void *parameter)
{
    char msg[500];
    for (;;)
    {
        vTaskDelay(logDELAY);
        if (xQueueReceive(xLogQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            utils->slog(msg);
            client.write(msg);
        }
        else
        {
            utils->slog("LogFunction: Empty Queue");
        }
    }
}

void missionCallback(char *cmd, String res)
{
    char msg[500];

    if (res.indexOf("error") != -1)
    {
        snprintf(msg, sizeof(msg), "ERROR: %s", res);
        xQueueSend(xLogQueue, &msg, logQueueDELAY);
        ttSDK->land();
    }

    snprintf(msg, sizeof(msg), "cmd: %s, res: %s - MissionCallback", cmd, res.c_str());
    xQueueSend(xLogQueue, &msg, logQueueDELAY);
}