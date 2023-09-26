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

uint8_t isFirstTime = 1, point_index = 0;
uint16_t measure = 0;

Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t missionTaskHandle, loggerTaskHandle;

QueueHandle_t xLogQueue = NULL;

SemaphoreHandle_t xSensorMutex = NULL;
int count;

/*-------------- Fuction Declaration --------------*/

void vMissionTask(void *parameter);
void vLogTask(void *parameter);

void initialCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);

boolean tofSense(std::function<void()> callback);
void dodgeFun();

/*-------------- Fuction Implementation --------------*/

void setup()
{
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
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}

	/*---------------------------------------------------------*/

	WiFi.begin(SSID, PASSWORD);
	while (WiFi.status() != WL_CONNECTED)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		utils->slog("Connecting to WiFi..");
	}

	ttRGB->Init();
	utils->slog("Connected to the WiFi network");
	Serial.println(WiFi.localIP());
	ttRGB->SetRGB(200, 255, 0);
	wifiServer.begin();

	while (!client)
	{
		client = wifiServer.available();
		vTaskDelay(pdMS_TO_TICKS(10));
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

	/*-------------- Tasks  --------------*/

	if (xTaskCreatePinnedToCore(vLogTask, "Logger", configMINIMAL_STACK_SIZE * 4, NULL, 5, &loggerTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Logger task");
	}
	if (xTaskCreatePinnedToCore(vMissionTask, "Mission", configMINIMAL_STACK_SIZE * 4, NULL, 20, &missionTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Mission task");
	};
}

void loop()
{
	vTaskDelete(NULL);
}

void vMissionTask(void *parameter)
{
	char msg[100];

	ttRGB->SetRGB(0, 0, 255);
	ttSDK->getBattery(missionCallback);
	ttSDK->takeOff(missionCallback);

	for (;;)
	{
		vTaskDelay(missionDELAY);
		xQueueSend(xLogQueue, &msg, logQueueDELAY);

		Coordinate origin = routePoints->at(point_index++);
		if (routePoints->size() == point_index)
		{
			char msg[50];
			sniprintf(msg, sizeof(msg), "Mission finished, landing ...");
			xQueueSend(xLogQueue, &msg, logQueueDELAY);
			ttSDK->land();
			ttRGB->SetRGB(0, 255, 0);
			while (1)
				vTaskDelay(missionDELAY);
		}
		Coordinate destination = routePoints->at(point_index);
		ttSDK->moveRelativeTo(origin, destination, 20, missionCallback);
		tofSense(dodgeFun);
	}
}

void vLogTask(void *parameter)
{
	char msg[500];
	for (;;)
	{
		vTaskDelay(logDELAY);
		if (xQueueReceive(xLogQueue, &msg, logDELAY) == pdTRUE)
		{
			utils->slog(msg);
			client.write(msg);
		}
	}
}

boolean tofSense(std::function<void()> callback)
{
	char msg[50];
	measure = tt_sensor.ReadRangeSingleMillimeters(); // Performs a single-shot range measurement and returns the reading in millimeters
	if (tt_sensor.TimeoutOccurred())
	{
		xQueueSend(xLogQueue, "Sensor timeout\n", 0);
		return NULL;
	}

	boolean obstacleDetected = measure < MAX_MEASURE;
	if (obstacleDetected)
		callback();

	sniprintf(msg, sizeof(msg), "MAX_MEASURE = %d | Current measure: %d\n", MAX_MEASURE, measure);
	xQueueSend(xLogQueue, &msg, logQueueDELAY);

	return obstacleDetected;
}

void dodgeFun()
{
	ttRGB->SetRGB(255, 0, 0);
	ttSDK->up(UP_DODGE, missionCallback);
	ttRGB->SetRGB(0, 0, 255);
}

void initialCallback(char *cmd, String res)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "InitialCallback - cmd: %s, res: %s\n", cmd, res.c_str());
	utils->slog(msg);
}

void missionCallback(char *cmd, String res)
{
	char msg[100];

	if (res.indexOf("error") != -1)
	{
		snprintf(msg, sizeof(msg), "ERROR: %s", res);
		xQueueSend(xLogQueue, &msg, logQueueDELAY);
		ttSDK->land();
	}

	snprintf(msg, sizeof(msg), "MissionCallback - cmd: %s, res: %s", cmd, res.c_str());
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
}