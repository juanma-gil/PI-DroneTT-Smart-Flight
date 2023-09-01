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
#define missionDELAY (pdMS_TO_TICKS(500))
#define sensorDELAY (pdMS_TO_TICKS(500))
#define logDELAY (pdMS_TO_TICKS(2000))
#define logQueueDELAY (pdMS_TO_TICKS(50))

#define logQueueSIZE (UBaseType_t)100 * (5 * sizeof(char)) // Aprox 5 messages of 50 chars
#define logItemSIZE (UBaseType_t)100 * (sizeof(char))	   // 50 chars

/*-------------- Global Variables --------------*/

const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";

WiFiServer wifiServer(PORT);
WiFiClient client;

RMTT_TOF tt_sensor;
RMTT_Protocol *ttSDK;
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
Utils *utils;

int point_index = 0, isFirstTime = 1;
Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t sensorTaskHandle, missionTaskHandle, loggerTaskHandle;
QueueHandle_t xLogQueue = NULL;

float measure = 0;

bool sensorFlag = false;

/*-------------- Fuction Declaration --------------*/

void vSensorFunction(void *parameter);
void vMissionFunction(void *parameter);
void vLogFunction(void *parameter);
void takeOffProcess();
void defaultCallback(char *cmd, String res);
void initialCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);

/*-------------- Fuction Implementation --------------*/

void setup()
{
	ttRGB->Init();
	route = route->getInstance();
	routePoints = route->getRoute();
	utils = Utils::getInstance();
	ttSDK = RMTT_Protocol::getInstance();

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

	client = wifiServer.available();
	while (!client)
	{
		client = wifiServer.available();
		delay(10);
	};

	utils->slog("Client connected");

	/* while (routePoints->empty())
		{
			delay(10);
			route->receiveRouteFromClient(&client);
		}
		utils.slog("Route received"); */

	ttSDK->sdkOn(initialCallback);
	ttSDK->getBattery(initialCallback);
	ttRGB->SetRGB(200, 0, 255);
	utils->slog("Starting tasks");

	/*-------------- Queues --------------*/

	xLogQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
	if (xLogQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}

	/*-------------- Tasks  --------------*/

	if (xTaskCreatePinnedToCore(vSensorFunction, "Sensor", configMINIMAL_STACK_SIZE * 10, NULL, 5, &sensorTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Sensor task");
	};
	if (xTaskCreatePinnedToCore(vLogFunction, "Logger", configMINIMAL_STACK_SIZE, NULL, 2, &loggerTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Logger task");
	}
	if (xTaskCreatePinnedToCore(vMissionFunction, "Mission", configMINIMAL_STACK_SIZE * 10, NULL, 7, &missionTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Sensor task");
	};
}

void loop()
{
	vTaskDelete(NULL);
}

void vMissionFunction(void *parameter)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "Mission: running on Core %d\n", xPortGetCoreID());
	short int on = 0;
	for (;;)
	{
		vTaskDelay(missionDELAY);
		if (xQueueSend(xLogQueue, &msg, logQueueDELAY) != pdTRUE)
		{
			utils->slog("Mission: Queue error");
		}

		ttRGB->SetRGB(0, 255, 0);
		if (sensorFlag)
		{
			ttRGB->SetRGB(255, 0, 0);
			if (on)
			{
				ttSDK->motorOff(defaultCallback);
				on = 0;
			}
		}
		else
		{
			ttRGB->SetRGB(0, 255, 0);
			if (!on)
			{
				ttSDK->motorOn(defaultCallback);
				on = 1;
			}
		}

		//  if (isFirstTime)
		//  	takeOffProcess();

		// Coordinate origin = routePoints->at(point_index++);
		// if (routePoints->size() == point_index)
		// {
		// 	char msg[50];
		// 	sniprintf(msg, sizeof(msg), "Mission finished, landing ...");
		// 	ttSDK->land();
		// 	ttRGB->SetRGB(0, 255, 0);
		// 	while (1)
		// 		delay(10);
		// }
		// Coordinate destination = routePoints->at(point_index);
		// ttSDK->moveRealtiveTo(origin, destination, 10, missionCallback);
	}
}

void vSensorFunction(void *parameter)
{
	TickType_t count = xTaskGetTickCount();
	char msg[100];
	sniprintf(msg, sizeof(msg), "Sensor: running on Core %d\n", xPortGetCoreID());

	for (;;)
	{
		vTaskDelay(sensorDELAY);
		if (xQueueSend(xLogQueue, &msg, logQueueDELAY) != pdTRUE)
		{
			utils->slog("Sensor: Queue error");
		}
		measure = tt_sensor.ReadRangeSingleMillimeters(); // Performs a single-shot range measurement and returns the reading in millimeters
		if (tt_sensor.TimeoutOccurred())
		{
			xQueueSend(xLogQueue, "Sensor timeout\n", 0);
		}
		else
		{
			sensorFlag = (measure < 300) ? true : false;
		}
	}
}

/**
 * @brief task to log messages using serial and writing a log file
 * The task is created with the lowest priority and pinned to core 0,
 * the long of the message should be minor to 50 chars,
 * because the queue has a size of 50 bytes * 5 messages
 * @param parameter
 */
void vLogFunction(void *parameter)
{
	char msg[100];
	for (;;)
	{
		vTaskDelay(logDELAY);

		if (xQueueReceive(xLogQueue, &msg, 0) == pdTRUE)
		{
			utils->slog(msg);
		}
		else
		{
			utils->slog("LogFunction: Empty Queue");
		}
	}
}

void takeOffProcess()
{
	ttSDK->startUntilControl();
	ttRGB->SetRGB(0, 0, 255);
	ttSDK->takeOff(defaultCallback);
	isFirstTime = 0;
}

void initialCallback(char *cmd, String res)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "cmd: %s, res: %s\n", cmd, res.c_str());
	utils->slog(msg);
}

void defaultCallback(char *cmd, String res)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "cmd: %s, res: %s - DefaultCallback", cmd, res.c_str());
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
}

void missionCallback(char *cmd, String res)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "MissionCallback, running in core = %d", xPortGetCoreID());
	utils->slog(msg);
	snprintf(msg, sizeof(msg), "cmd: %s, res: %s\n", cmd, res.c_str());
	utils->slog(msg);
}