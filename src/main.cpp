#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <RMTT_Libs.h>
#include "../include/Route.h"
#include "../include/Utils.h"

#define PORT 5001
#define missionDELAY (pdMS_TO_TICKS(2000))
#define sensorDELAY (pdMS_TO_TICKS(3000))

/*-------------- Global Variables --------------*/

const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";

WiFiServer wifiServer(PORT);
WiFiClient client;

RMTT_Protocol *ttSDK;
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
Utils *utils;

int point_index = 0, isFirstTime = 1;
Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;

UBaseType_t uxHighWaterMark;

TaskHandle_t sensorTaskHandle, missionTaskHandle;

/*-------------- Fuction Declaration --------------*/

void vSensorFunction(void *parameter);
void vMissionFunction(void *parameter);
void takeOffProcess();
void defaultCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);

/*-------------- Fuction Implementation --------------*/

void setup()
{
	ttRGB->Init();
	route = route->getInstance();
	routePoints = route->getRoute();

	Serial.begin(115200);
	Serial1.begin(1000000, SERIAL_8N1, 23, 18);

	delay(1000);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.println("Connecting to WiFi..");
	}

	Serial.println("Connected to the WiFi network");
	Serial.println(WiFi.localIP());
	ttRGB->SetRGB(200, 255, 0);
	ttSDK = RMTT_Protocol::getInstance();
	utils = Utils::getInstance();
	wifiServer.begin();

	client = wifiServer.available();
	while (!client)
	{
		client = wifiServer.available();
		delay(10);
	};

	Serial.println("Client connected");
	/* while (routePoints->empty())
	{
		delay(10);
		route->receiveRouteFromClient(&client);
	}
	Serial.println("Route received"); */

	// ttSDK->startUntilControl();
	ttSDK->sdkOn(defaultCallback);
	ttRGB->SetRGB(200, 0, 255);
	ttSDK->getBattery(defaultCallback);
	char msg[100];
	Serial.println("Starting tasks");
	// client.write("JSON ok. Starting mission");

	/*-------------- Tasks  --------------*/
	if (xTaskCreatePinnedToCore(vSensorFunction, "Sensor", 8000, NULL, 5, &sensorTaskHandle, 0) != pdPASS)
	{
		Serial.println("Failed to create Sensor task");
	};
	if (xTaskCreatePinnedToCore(vMissionFunction, "Mission", 8000, NULL, 7, &missionTaskHandle, 1) != pdPASS)
	{
		Serial.println("Failed to create Sensor task");
	};
	vTaskStartScheduler();
}

void loop()
{
}

void vMissionFunction(void *parameter)
{
	TickType_t count;
	char msg[100];
	if (sprintf(msg, "Mission: running on Core %d\n", xPortGetCoreID()) != -1)
		if (utils->slog(msg) == -1)
		{
			while (1)
				delay(10);
		};

	for (;;)
	{
		vTaskDelay(missionDELAY);
		count = xTaskGetTickCount();
		ttSDK->motorOn(defaultCallback);
		count = xTaskGetTickCount() - count;
		if (sprintf(msg, "Mission waited for: %d ms\n", pdTICKS_TO_MS(count)) != -1)
			if (utils->slog(msg) == -1)
			{
				while (1)
					delay(10);
			};
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
	TickType_t count;
	char msg[100];
	if (sprintf(msg, "Sensor: running on Core %d\n", xPortGetCoreID()) != -1)
		if (utils->slog(msg) == -1)
		{
			while (1)
				delay(10);
		};
	for (;;)
	{
		vTaskDelay(sensorDELAY);
		count = xTaskGetTickCount();
		ttSDK->motorOff(defaultCallback);
		count = xTaskGetTickCount() - count;
		if (sprintf(msg, "Sensor waited for: %d ms\n", pdTICKS_TO_MS(count)) != -1)
			if (utils->slog(msg) == -1)
			{
				while (1)
					delay(10);
			};
	}
}

void takeOffProcess()
{
	ttSDK->startUntilControl();
	ttRGB->SetRGB(0, 0, 255);
	ttSDK->takeOff(defaultCallback);
	isFirstTime = 0;
}

void defaultCallback(char *cmd, String res)
{
	if (client.connected())
	{
		char msg[50];
		snprintf(msg, sizeof(msg), "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
		if (utils->slog(msg) == -1)
		{
			while (1)
				delay(10);
		};
	} /*
	 char msg[50];
	 snprintf(msg, sizeof(msg), "cmd: %s, res: %s\n", cmd, res.c_str());
	 utils->slog(msg);
	  */
}

void missionCallback(char *cmd, String res)
{
	/* char msg[100];

	if (client.connected())
	{
		snprintf(msg, sizeof(msg), "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
	}

	if (res.indexOf("ok") == -1)
	{
		snprintf(msg, sizeof(msg), "ERROR: landing ...\n");
		client.write(msg);
		ttSDK->land();
		ttRGB->SetRGB(255, 0, 0);
	} */
}