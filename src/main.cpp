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
int point_index = 0, vueltas = 3;

UBaseType_t uxHighWaterMark;

/*-------------- Fuction Declaration --------------*/

void defaultCallback(char *cmd, String res);
void goCallback(char *cmd, String res);

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

	wifiServer.begin();
}

void loop()
{
	while (routePoints->empty())
	{
		client = wifiServer.available();
		if (client)
		{
			route->receiveRouteFromClient(&client);
		}
	}

	client.write("JSON ok. Starting mission");
	ttRGB->SetRGB(200, 0, 255);

	ttSDK->startUntilControl();

	ttSDK->getBattery(defaultCallback);

	delay(1000);
	ttSDK->takeOff(defaultCallback);

	while (1)
	{
		Coordinate origin = routePoints->at(point_index++);
		char msg[100];
		if (routePoints->size() == point_index)
		{
			point_index = 0;
			vueltas--;
			if (!vueltas)
			{
				ttSDK->land(defaultCallback);
				while (1)
					;
			}
		}
		Coordinate destination = routePoints->at(point_index);
		sprintf(msg, "Free stack space remaining: %d", uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL));
		client.write(msg);
		ttSDK->moveRealtiveTo(origin, destination, 40, defaultCallback);
	}
}

void defaultCallback(char *cmd, String res)
{
	Serial.println(res);
	if (client.connected())
	{
		char msg[100];
		sprintf(msg, "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
	}
}

void goCallback(char *cmd, String res)
{
	if (client.connected())
	{
		char msg[50];
		sprintf(msg, "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
	}

	if (res.indexOf("ok") != -1) //&& routePoints->size() > point_index + 1
	{
		Coordinate origin = routePoints->at(point_index++);
		char msg[100];
		client.write(msg);
		if (routePoints->size() == point_index) point_index = 0;
		Coordinate destination = routePoints->at(point_index);
		sprintf(msg, "Stack %d", uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL));
		client.write(msg);
		ttSDK->moveRealtiveTo(origin, destination, 40, goCallback);
	}
}