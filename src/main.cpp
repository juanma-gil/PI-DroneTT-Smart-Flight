#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <RMTT_Libs.h>
#include <ArduinoJson.h>
#include <list>
#include "../include/models/Coordinate.h"

#define JSON_BUF_SIZE 1024
#define PORT 5001
/*-------------- Global Variables --------------*/

RMTT_Protocol tt_sdk;
RMTT_RGB tt_rgb;
const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";
WiFiServer wifiServer(PORT);
const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(3) + 50;

/*-------------- Fuction Declaration --------------*/

void defaultCallback(char *cmd, String res);
void receiveDataFromClient(WiFiClient *client);
std::list<Coordinate> parseJsonAsCoordinate(const char *jsonBuf);

/*-------------- Fuction Implementation --------------*/

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
	tt_rgb.SetRGB(0, 255, 0);
}

void loop()
{

	WiFiClient client = wifiServer.available();
	if (client)
		receiveDataFromClient(&client);
}

void defaultCallback(char *cmd, String res)
{
	char msg[100];
	sprintf(msg, "The command was: %s\nRes: %s", cmd, res.c_str());
	Serial.println(String(msg));
}

void receiveDataFromClient(WiFiClient *client)
{
	while (client->connected())
	{
		tt_rgb.SetRGB(0, 255, 0);
		while (client->available() > 0)
		{	
			String json = client->readStringUntil('\n');
			json.trim();  // Remove the trailing newline character
			client->write("Recibido por el servidor");

			Coordinate::printPoints(parseJsonAsCoordinate(json.c_str()));
		}
	}
	client->stop();
	Serial.println("Client disconnected");
}

std::list<Coordinate> parseJsonAsCoordinate(const char *jsonBuf)
{
	int i = 0;
	StaticJsonDocument<JSON_BUF_SIZE> doc;
	std::list<Coordinate> points;
	DeserializationError error = deserializeJson(doc, jsonBuf);
	if (error)
	{
		tt_rgb.SetRGB(255, 0, 0);
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.c_str());
		return points;
	}

	const char *unit = doc["unit"];
	JsonArray pointsJson = doc["points"];

	// Print the values
	Serial.print("Unit: ");
	Serial.println(unit);

	Serial.println("Points:");
	for (const JsonObject point : pointsJson)
	{
		float x = point["x"];
		float y = point["y"];
		float z = point["z"];

		Coordinate coordinate = Coordinate(unit, x, y, z);
		points.push_back(coordinate);
	}
	return points; 
}
