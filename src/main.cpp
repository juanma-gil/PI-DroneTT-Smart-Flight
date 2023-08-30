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
#define sensorDELAY (pdMS_TO_TICKS(150))

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

RMTT_TOF tt_sensor;
float measure = 0;

bool sensorFlag = false;

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

	/* Initialize sensor */
	Wire.begin(27, 26);
	Wire.setClock(100000);
	tt_sensor.SetTimeout(500);
	if (!tt_sensor.Init())
	{
		Serial.println("Failed to detect and initialize sensor!");
		while (1)
		{
		}
	}

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
	TickType_t count = xTaskGetTickCount();
	char msg[100];
	Serial.printf("Mission: running on Core %d\n", xPortGetCoreID());

	for (;;)
	{
		count = xTaskGetTickCount() - count;
		vTaskDelay(missionDELAY);
		/* sprintf(msg, "Mission waited for %d ms\n", pdTICKS_TO_MS(count));
		Serial.println(msg);
		if (utils->slog(msg) == -1)
		{
			while (1)
				delay(10);
		}; */
		// ttSDK->motorOn(defaultCallback);
		if (sensorFlag)
		{
			ttRGB->SetRGB(255, 0, 0);
			ttSDK->motorOff(defaultCallback);
		}
		else
		{
			ttRGB->SetRGB(0, 255, 0);
			ttSDK->motorOn(defaultCallback);
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
	Serial.printf("Sensor: running on Core %d\n", xPortGetCoreID());
	for (;;)
	{
		vTaskDelay(sensorDELAY);
		count = xTaskGetTickCount() - count;
		/* sprintf(msg, "Sensor waited for %d ms\n", pdTICKS_TO_MS(count));
		Serial.println(msg);
		if (utils->slog(msg) == -1)
		{
			while (1)
				delay(10);
		};  */
		measure = tt_sensor.ReadRangeSingleMillimeters(); // Performs a single-shot range measurement and returns the reading in millimeters
		// measure = measure / 1000;
		// Serial.println(measure);
		if (tt_sensor.TimeoutOccurred())
		{
			Serial.println("TIMEOUT");
		}
		else
		{
			sensorFlag = (measure < 300) ? true : false;
			// ttSDK->motorOff(defaultCallback);
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