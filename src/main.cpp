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
#define missionDELAY (pdMS_TO_TICKS(100))
#define sensorDELAY (pdMS_TO_TICKS(100))
#define dodgeDELAY (pdMS_TO_TICKS(10))
#define logDELAY (pdMS_TO_TICKS(2000))
#define logQueueDELAY (pdMS_TO_TICKS(50))

#define logQueueSIZE (UBaseType_t)100 * (5 * sizeof(char)) // Aprox 5 messages of 100 chars
#define logItemSIZE (UBaseType_t)100 * (sizeof(char))	   // 100 chars

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

TaskHandle_t sensorTaskHandle, missionTaskHandle, loggerTaskHandle, dodgeTaskHandle;
QueueHandle_t xLogQueue = NULL;

SemaphoreHandle_t xCmdMutex;

float measure = 0;

bool sensorFlag = false;

/*-------------- Fuction Declaration --------------*/

void vSensorFunction(void *parameter);
void vMissionFunction(void *parameter);
void vDodgeFunction(void *parameter);
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
	xCmdMutex = xSemaphoreCreateMutex();

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

	while (!client)
	{
		client = wifiServer.available();
		delay(10);
	};

	utils->slog("Client connected");

	while (routePoints->empty())
	{
		delay(10);
		route->receiveRouteFromClient(&client);
	}
	utils->slog("Route received");

	ttSDK->sdkOn(initialCallback);
	ttSDK->getBattery(initialCallback);
	ttRGB->SetRGB(200, 0, 255);

	/*-------------- Queues --------------*/

	xLogQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
	if (xLogQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}

	/*-------------- Tasks  --------------*/

	if (xTaskCreatePinnedToCore(vSensorFunction, "Sensor", configMINIMAL_STACK_SIZE * 3, NULL, 15, &sensorTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Sensor task");
	};
	if (xTaskCreatePinnedToCore(vLogFunction, "Logger", configMINIMAL_STACK_SIZE * 5, NULL, 8, &loggerTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Logger task");
	}
	if (xTaskCreatePinnedToCore(vDodgeFunction, "Dodge", configMINIMAL_STACK_SIZE * 3, NULL, 20, &dodgeTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Dodge task");
	};
	if (xTaskCreatePinnedToCore(vMissionFunction, "Mission", configMINIMAL_STACK_SIZE * 5, NULL, 12, &missionTaskHandle, 0) != pdPASS)
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
	for (;;)
	{
		vTaskDelay(missionDELAY);

		if (isFirstTime)
			takeOffProcess();

		if (xSemaphoreTake(xCmdMutex, missionDELAY * 5) == pdPASS)
		{
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
			ttSDK->moveRealtiveTo(origin, destination, 10, missionCallback);
			xSemaphoreGive(xCmdMutex);
		}
		else
		{
			snprintf(msg, sizeof(msg), "Mission failed, mutex timeout");
			xQueueSend(xLogQueue, &msg, logQueueDELAY);
		}
	}
}

void vSensorFunction(void *parameter)
{
	char msg[100];
	for (;;)
	{
		vTaskDelay(sensorDELAY);

		measure = tt_sensor.ReadRangeSingleMillimeters(); // Performs a single-shot range measurement and returns the reading in millimeters
		if (tt_sensor.TimeoutOccurred())
		{
			xQueueSend(xLogQueue, "Sensor timeout\n", 0);
			return;
		}
		sensorFlag = measure < 300;
	}
}

void vDodgeFunction(void *parameter)
{
	char msg[100];
	for (;;)
	{
		vTaskDelay(dodgeDELAY);
		if (sensorFlag && xSemaphoreTake(xCmdMutex, dodgeDELAY) == pdPASS)
		{
			// TODO: Limpiar el comando en curso, si se consigue eso, este mutex estaría de más
			// Cuando se decida esquivar tener en cuenta que habría que insertar en la ruta el
			// punto de esquive xq sino no va a poder hacer el moveRelative,luego volver a la ruta original
			ttRGB->SetRGB(255, 0, 0);
			ttSDK->land(missionCallback);
			vTaskDelete(missionTaskHandle);
			vTaskDelete(sensorTaskHandle);
			vTaskDelete(loggerTaskHandle);
			while (1)
			{
				vTaskDelay(logDELAY);
				ttRGB->SetRGB(100, 255, 50);
			}
			xSemaphoreGive(xCmdMutex);
		}
	}
}

/**
 * @brief task to log messages using serial and wifi socket
 * The task is created with the lowest priority and pinned to core 0,
 * the long of the message should be minor to 100 chars,
 * because the queue has a size of 100 bytes * 5 messages
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
			client.write(msg);
		}
		else
		{
			utils->slog("LogFunction: Empty Queue");
		}
	}
}

void takeOffProcess()
{
	ttRGB->SetRGB(0, 0, 255);
	ttSDK->getBattery(defaultCallback);
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
	snprintf(msg, sizeof(msg), "cmd: %s, res: %s - MissionCallback", cmd, res.c_str());
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
}