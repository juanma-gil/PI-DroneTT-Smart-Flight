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
std::queue<Coordinate> *routePoints;

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
	Coordinate point = routePoints->front();

	char buffer[50];
	sprintf(buffer, "Going to (%d, %.d, %d)", point.getX(), point.getY(), point.getZ());
	client.write(buffer);
	ttSDK->go(point.getX(), point.getY(), point.getZ(), 40, goCallback);
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
	RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
	RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();

	if (client.connected())
	{
		char msg[100];
		sprintf(msg, "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
	}

	if (res.indexOf("ok") != -1 && routePoints->size() > 1)
	{
		Coordinate origin = routePoints->front();
		routePoints->pop();
		Coordinate destination = routePoints->front();
		ttSDK->moveRealtiveTo(origin, destination, 40, goCallback);
	}
	else if (routePoints->size() == 1)
	{
		ttSDK->land(defaultCallback);
		ttRGB->SetRGB(0, 255, 0);
		client.write("Finished!");
		while (1)
			;
	}
}