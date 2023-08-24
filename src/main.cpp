#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <RMTT_Libs.h>
#include "../include/Route.h"

#define PORT 5001

/*-------------- Global Variables --------------*/

RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";
WiFiServer wifiServer(PORT);
WiFiClient client;
Route *route = Route::getInstance();
std::vector<Coordinate> *routePoints;
int point_index = 0, isFirstTime = 1;
UBaseType_t uxHighWaterMark;

/*-------------- Fuction Declaration --------------*/
void vSensorFunction(void *parameter);
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
		delay(500);
		Serial.println("Connecting to WiFi..");
	}

	Serial.println("Connected to the WiFi network");
	Serial.println(WiFi.localIP());
	ttRGB->SetRGB(200, 255, 0);

	wifiServer.begin();

	client = wifiServer.available();
	while (!client)
	{
		client = wifiServer.available();
		delay(500);
	};

	Serial.println("Client connected");
	while (routePoints->empty())
	{
		delay(500);
		route->receiveRouteFromClient(&client);
	}
	Serial.println("Route received");

	ttRGB->SetRGB(200, 0, 255);

	client.write("JSON ok. Starting mission");
	/*-------------- Tasks  --------------*/
	if (xTaskCreatePinnedToCore(vSensorFunction, "Sensor", 8000, NULL, 1, NULL, 0) != pdPASS)
	{
		Serial.println("Failed to create Sensor task");
	};
	Serial.println("Created Tasks");
	vTaskStartScheduler();
}

void loop()
{
	delay(100);
	if (isFirstTime)
		takeOffProcess();

	Coordinate origin = routePoints->at(point_index++);
	if (routePoints->size() == point_index)
	{
		char msg[50];
		sniprintf(msg, sizeof(msg), "Mission finished, landing ...");
		ttSDK->land();
		ttRGB->SetRGB(0, 255, 0);
		while (1)
			delay(500);
	}
	Coordinate destination = routePoints->at(point_index);
	ttSDK->moveRealtiveTo(origin, destination, 10, missionCallback);
}

void vSensorFunction(void *parameter)
{
	delay(20000);
	ttRGB->SetRGB(255, 0, 0);
	char msg[50];
	ttSDK->up(150, missionCallback);
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
	}
}

void missionCallback(char *cmd, String res)
{
	char msg[100];

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
	}
}