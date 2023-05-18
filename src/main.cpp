#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <RMTT_Libs.h>
#include "../include/models/Coordinate.h"

RMTT_Protocol tt_sdk;
RMTT_RGB tt_rgb;
const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";
String buffer;

void receiveDataFromClient(WiFiClient *client);

WiFiServer wifiServer(5001);

void defaultCallback(char *cmd, String res)
{
	char msg[100];
	sprintf(msg, "The command was: %s\nRes: %s", cmd, res.c_str());
	Serial.println(String(msg));
}

void setup()
{
	tt_rgb.Init();
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

	wifiServer.begin();
	tt_sdk.sdkOn(defaultCallback);
}

void loop()
{

	WiFiClient client = wifiServer.available();
	if (client)
		receiveDataFromClient(&client);
}

void receiveDataFromClient(WiFiClient *client)
{
	while (client->connected())
	{
		while (client->available() > 0)
		{
			buffer = client->readString();
			Serial.println(buffer);
			client->write("Recibido por el servidor");
		}
	}
	client->stop();
	Serial.println("Client disconnected");
}