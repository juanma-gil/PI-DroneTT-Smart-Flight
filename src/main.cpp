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
#define logDELAY (pdMS_TO_TICKS(2000))
#define logQueueDELAY (pdMS_TO_TICKS(50))
#define requestQueueDELAY (pdMS_TO_TICKS(50))
#define responseQueueDELAY (pdMS_TO_TICKS(50))

#define logQueueSIZE (UBaseType_t)5 * (100 * sizeof(char))	   // Aprox 5 messages of 100 chars
#define logItemSIZE (UBaseType_t)100 * (sizeof(char))		   // 100 chars
#define requestQueueSIZE (UBaseType_t)3 * (50 * sizeof(char))  // Aprox 3 messages of 50 chars
#define requestItemSIZE (UBaseType_t)50 * (sizeof(char))	   // 50 chars
#define responseQueueSIZE (UBaseType_t)3 * (50 * sizeof(char)) // Aprox 3 messages of 50 chars
#define responseItemSIZE (UBaseType_t)50 * (sizeof(char))	   // 50 chars

#define SSID "LCD"
#define PWD "1cdunc0rd0ba"

/*-------------- Global Variables --------------*/
uint8_t dodgeTries = 0, pointIndex = 0;
int16_t estimatedX = 0, estimatedY = 0, estimatedZ = 0;

WiFiServer wifiServer(PORT);
WiFiClient client;

RMTT_TOF tt_sensor;
RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();

RMTT_RGB *ttRGB = RMTT_RGB::getInstance();

Utils *utils = Utils::getInstance();
Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

TaskHandle_t sensorTaskHandle = NULL, missionTaskHandle = NULL, loggerTaskHandle = NULL, dodgeTaskHandle = NULL, commandTaskHandle = NULL;
QueueHandle_t xLogQueue = NULL, xRequestQueue = NULL, xResponseQueue = NULL;
SemaphoreHandle_t xSensorMutex = NULL;

/*-------------- Tasks Fuction Declaration --------------*/

void vSensorFunction(void *parameter);
void vMissionFunction(void *parameter);
void vLogFunction(void *parameter);
void vCommandFunction(void *parameter);

/*-------------- Fuction Declaration --------------*/

/*-------------- Callbacks Function Declaration --------------*/

void initialCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);

/*-------------- Implementation --------------*/

void setup()
{
	ttRGB->Init();

	routePoints = route->getRoute();

	/* The semaphore is created in the 'empty' state, meaning the semaphore must first be given using the xSemaphoreGive() API function before it can subsequently be taken (obtained) using the xSemaphoreTake() function. */
	xSensorMutex = xSemaphoreCreateBinary();

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

	/* Initialize WiFi connection */

	WiFi.begin(SSID, PWD);
	while (WiFi.status() != WL_CONNECTED)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		utils->slog("Connecting to WiFi..");
	}

	utils->slog("Connected to the WiFi network");
	Serial.println(WiFi.localIP());
	ttRGB->SetRGB(200, 255, 0);
	wifiServer.begin();

	/* Wait for a client to connect */
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

	/* Receive Route */
	while (routePoints->empty())
	{
		delay(10);
		route->receiveRouteFromClient(&client);
	}
	utils->slog("Route received");

	/*-------------- SDK --------------*/

	ttSDK->sdkOn(initialCallback);
	ttRGB->SetRGB(200, 0, 255);

	/*-------------- Queues --------------*/

	xLogQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
	if (xLogQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}
	xRequestQueue = xQueueCreate(requestQueueSIZE, requestItemSIZE);
	if (xRequestQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}
	xResponseQueue = xQueueCreate(responseQueueSIZE, responseItemSIZE);
	if (xResponseQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}

	/*-------------- Tasks  --------------*/
	ttRGB->SetRGB(0, 0, 255);

	if (xTaskCreatePinnedToCore(vSensorFunction, "Sensor", configMINIMAL_STACK_SIZE * 4, NULL, 15, &sensorTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Sensor task");
	};
	if (xTaskCreatePinnedToCore(vLogFunction, "Logger", configMINIMAL_STACK_SIZE * 4, NULL, 8, &loggerTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Logger task");
	}
	if (xTaskCreatePinnedToCore(vMissionFunction, "Mission", configMINIMAL_STACK_SIZE * 5, NULL, 12, &missionTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Mission task");
	};
	if (xTaskCreatePinnedToCore(vCommandFunction, "Command", configMINIMAL_STACK_SIZE * 5, NULL, 20, &commandTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Command task");
	};
}

void loop()
{
	vTaskDelete(NULL);
}

void vMissionFunction(void *parameter)
{
	char cmd[50];
	snprintf(cmd, sizeof(cmd), "takeoff");
	if (xQueueSend(xRequestQueue, (char *)&cmd, requestQueueDELAY) == pdFAIL)
	{
		xQueueSend(xLogQueue, "Failed to send command takeoff to queue", logQueueDELAY);
	}

	for (;;)
	{
		snprintf(cmd, sizeof(cmd), "go %d %d %d %d", 300, 0, 0, SPEED);
		if (xQueueSend(xRequestQueue, (char *)&cmd, requestQueueDELAY) == pdFAIL)
		{
			xQueueSend(xLogQueue, "Failed to send command go to queue", logQueueDELAY);
		}
		ttRGB->SetRGB(0, 255, 0);
		vTaskDelay(pdMS_TO_TICKS(300000));
	}
}

void vSensorFunction(void *parameter)
{
	char cmd[50];
	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(9000));
		ttRGB->SetRGB(255, 0, 0);
		sniprintf(cmd, sizeof(cmd), "stop");
		if (xQueueSend(xRequestQueue, (char *)&cmd, requestQueueDELAY) == pdFAIL)
		{
			xQueueSend(xLogQueue, "Failed to send command stop to queue", logQueueDELAY);
		}
		snprintf(cmd, sizeof(cmd), "up %d", 40);
		if (xQueueSend(xRequestQueue, (char *)&cmd, requestQueueDELAY) == pdFAIL)
		{
			utils->slog("Failed to send command up 40 to queue");
		}
		ttRGB->SetRGB(150, 0, 150);
		vTaskDelay(pdMS_TO_TICKS(200000));
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
			// utils->slog("LogFunction: Empty Queue");
		}
	}
}

void vCommandFunction(void *parameter)
{
	char req[50], res[50], msg[50];
	for (;;)
	{
		if (xQueueReceive(xRequestQueue, (char *)&req, requestQueueDELAY) == pdFALSE)
			continue;

		sniprintf(msg, sizeof(msg), "Drone req = %s", req);

		if (xQueueSend(xLogQueue, msg, logQueueDELAY) == pdFAIL)
		{
			xQueueSend(xLogQueue, "CommandFunction: Request was not sent to log function", logQueueDELAY);
		}

		snprintf(res, sizeof(res), "Drone res = %s", (const char *)ttSDK->sendCmd((char *)req).c_str());

		if (xQueueSend(xResponseQueue, (char *)&res, responseQueueDELAY) == pdFAIL)
		{
			xQueueSend(xLogQueue, "CommandFunction: Response no sent", logQueueDELAY);
		}
		if (xQueueSend(xLogQueue, (char *)&res, logQueueDELAY))
		{
			xQueueSend(xLogQueue, "CommandFunction: Response was not sent to log function", logQueueDELAY);
		}
	}
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
	// if (res.indexOf("error") != -1)
	// {
	// 	snprintf(msg, sizeof(msg), "ERROR: %s", res);
	// 	xQueueSend(xLogQueue, &msg, logQueueDELAY);
	// 	ttSDK->land();
	// }

	snprintf(msg, sizeof(msg), "MissionCallback - cmd: %s, res: %s", cmd, res.c_str());
	client.write(msg);
	// xQueueSend(xLogQueue, &msg, logQueueDELAY);
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