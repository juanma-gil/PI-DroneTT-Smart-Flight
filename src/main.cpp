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
#define SPEED 20
#define missionDELAY (pdMS_TO_TICKS(100))
#define sensorDELAY (pdMS_TO_TICKS(100))
#define dodgeDELAY (pdMS_TO_TICKS(10))
#define logDELAY (pdMS_TO_TICKS(500))
#define logQueueDELAY (pdMS_TO_TICKS(50))

#define logQueueSIZE (UBaseType_t)100 * (5 * sizeof(char)) // Aprox 5 messages of 100 chars
#define logItemSIZE (UBaseType_t)100 * (sizeof(char))	   // 100 chars

/*-------------- Global Variables --------------*/

const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";

WiFiServer wifiServer(PORT);
WiFiClient client;

RMTT_TOF tt_sensor;
RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
Utils *utils = Utils::getInstance();

uint8_t isFirstTime = 1, dodgeTries = 0;
uint16_t pointIndex = 0, measure = 0;
int16_t estimatedX = 0;
int16_t estimatedY = 0;
int16_t estimatedZ = 0;

Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t sensorTaskHandle, missionTaskHandle, loggerTaskHandle, dodgeTaskHandle;
QueueHandle_t xLogQueue = NULL;

SemaphoreHandle_t xSensorMutex = NULL;

/*-------------- Fuction Declaration --------------*/

void vSensorFunction(void *parameter);
void vMissionFunction(void *parameter);
void vDodgeFunction(void *parameter);
void vLogFunction(void *parameter);
void takeOffProcess();
void initialCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);
void moveRelativeCallback(char *cmd, String res, MoveRelativeRes moveRelativeRes);

/*-------------- Fuction Implementation --------------*/

void setup()
{
	ttRGB->Init();
	route = route->getInstance();
	routePoints = route->getRoute();
	xSensorMutex = xSemaphoreCreateBinary(); /* The semaphore is created in the 'empty' state, meaning the semaphore must first be given using the xSemaphoreGive() API function before it can subsequently be taken (obtained) using the xSemaphoreTake() function. */

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
	utils->slog("Route received");

	ttSDK->sdkOn(initialCallback);
	delay(10);
	ttRGB->SetRGB(200, 0, 255);

	/*-------------- Queues --------------*/

	xLogQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
	if (xLogQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}

	/*-------------- Tasks  --------------*/

	if (xTaskCreatePinnedToCore(vSensorFunction, "Sensor", configMINIMAL_STACK_SIZE * 4, NULL, 15, &sensorTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Sensor task");
	};
	if (xTaskCreatePinnedToCore(vLogFunction, "Logger", configMINIMAL_STACK_SIZE * 4, NULL, 8, &loggerTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Logger task");
	}
	if (xTaskCreatePinnedToCore(vDodgeFunction, "Dodge", configMINIMAL_STACK_SIZE * 4, NULL, 20, &dodgeTaskHandle, 1) != pdPASS)
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

		takeOffProcess();

		Coordinate origin = routePoints->at(pointIndex++);
		if (routePoints->size() == pointIndex)
		{
			char msg[50];
			sniprintf(msg, sizeof(msg), "Mission finished, landing ...");
			xQueueSend(xLogQueue, &msg, logQueueDELAY);
			ttSDK->land();
			ttRGB->SetRGB(0, 255, 0);
			while (1)
				vTaskDelay(missionDELAY);
		}
		Coordinate destination = routePoints->at(pointIndex);
		ttSDK->moveRelativeTo(origin, destination, SPEED, moveRelativeCallback);
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
		if (measure < 300)
		{
			ttRGB->SetRGB(255, 0, 0);
			vTaskSuspend(missionTaskHandle);
			if (xSemaphoreGive(xSensorMutex) == pdFAIL)
			{
				xQueueSend(xLogQueue, "Sensor function couldn't release mutex\n", 0);
			}
		}
		else if (dodgeTries != 0)
		{
			dodgeTries = 0;
			vTaskResume(missionTaskHandle);
		}
	}
}

void vDodgeFunction(void *parameter)
{
	char msg[100];
	for (;;)
	{
		if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdPASS)
		{
			ttSDK->stop(missionCallback);
			vTaskSuspend(sensorTaskHandle);
			dodgeTries++;
			ttRGB->SetRGB(50, 255, 50);
			Coordinate lastPoint = routePoints->at(pointIndex - 1);
			int16_t xCoord = lastPoint.getX() + estimatedX;
			int16_t yCoord = lastPoint.getY() + estimatedY;
			int16_t zCoord = lastPoint.getZ() + estimatedZ;
			Coordinate origin = Coordinate("cm", xCoord, yCoord, zCoord);
			Coordinate destination = Coordinate("cm", xCoord, yCoord, zCoord + 40);
			snprintf(msg, sizeof(msg), "last point: %d, %d, %d\nxCoord: %d, yCoord: %d, zCoord: %d", lastPoint.getX(), lastPoint.getY(), lastPoint.getZ(), xCoord, yCoord, zCoord);
			xQueueSend(xLogQueue, &msg, logQueueDELAY);
			ttSDK->moveRelativeTo(origin, destination, SPEED, moveRelativeCallback);
			routePoints->emplace(routePoints->begin() + pointIndex, destination);
			pointIndex++;
			vTaskResume(sensorTaskHandle);
		}
		else
		{
			xQueueSend(xLogQueue, "Dodge function couldn't take mutex\n", 0);
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
	if (!isFirstTime)
		return;

	ttRGB->SetRGB(0, 0, 255);
	ttSDK->getBattery(initialCallback);
	ttSDK->takeOff(initialCallback);
	isFirstTime = 0;
}

void initialCallback(char *cmd, String res)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "cmd: %s, res: %s - InitialCallback\n", cmd, res.c_str());
	utils->slog(msg);
}

void missionCallback(char *cmd, String res)
{
	char msg[100];
	if (res.indexOf("error") != -1)
	{
		snprintf(msg, sizeof(msg), "ERROR: %s", res);
		xQueueSend(xLogQueue, &msg, logQueueDELAY);
		// ttSDK->land();
	}

	snprintf(msg, sizeof(msg), "cmd: %s, res: %s - MissionCallback", cmd, res.c_str());
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
}

void moveRelativeCallback(char *cmd, String res, MoveRelativeRes moveRelativeRes)
{
	char msg[100];
	int16_t x = moveRelativeRes.getX();
	int16_t y = moveRelativeRes.getY();
	int16_t z = moveRelativeRes.getZ();

	TickType_t execTime = pdTICKS_TO_MS(moveRelativeRes.getTime()) / 1000;

	float absDistance = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
	float pathFraction = SPEED * execTime / absDistance;

	estimatedX = pathFraction * x;
	estimatedY = pathFraction * y;
	estimatedZ = pathFraction * z;

	snprintf(msg, sizeof(msg), "Estimated X: %d cm, Y: %d cm, Z: %d cm, time: %d s, AbsDistace: %f, pathfraction: %f\n", estimatedX, estimatedY, estimatedZ, execTime, absDistance, pathFraction);
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
}