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
int point_index = 0;

UBaseType_t uxHighWaterMark;

/*-------------- Fuction Declaration --------------*/

void defaultCallback(char *cmd, String res);
void missionCallback(char *cmd, String res);

/*-------------- Fuction Implementation --------------*/

void setup()
{
	Serial.begin(115200);
	Serial1.begin(1000000, SERIAL_8N1, 23, 18);
	delay(100);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.println("Connecting to WiFi..");
	}

	wifiServer.begin();

	while (!client)
	{
		client = wifiServer.available();
		vTaskDelay(pdMS_TO_TICKS(10));
	};

	while (!client.connected())
		;

	ttSDK->sdkOn();

	ttSDK->takeOff(missionCallback);

	delay(10000);

	ttSDK->land(missionCallback);
}

void loop()
{
}

void missionCallback(char *cmd, String res)
{
	char msg[100];

	if (client.connected())
	{
		snprintf(msg, sizeof(msg), "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
	}
}