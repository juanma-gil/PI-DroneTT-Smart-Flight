#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <Arduino.h>
#include <WiFi.h>
#include <RMTT_Libs.h>
#include <ArduinoJson.h>

#define JSON_BUF_SIZE 1024
#define PORT 5001
/*-------------- Global Variables --------------*/

RMTT_Protocol tt_sdk;
RMTT_RGB tt_rgb;
const char *ssid = "LCD";
const char *password = "1cdunc0rd0ba";
WiFiServer wifiServer(PORT);
WiFiClient client;
std::queue<Coordinate> route = std::queue<Coordinate>();

/*-------------- Fuction Declaration --------------*/

void defaultCallback(char *cmd, String res);
void goCallback(char *cmd, String res);
void receiveDataFromClient();
void parseJsonAsCoordinate(const char *jsonBuf);
void insertUnrolledPoints(const char *unit, uint8_t scalar, int16_t plusX, int16_t plusY, int16_t plusZ);

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
	tt_rgb.SetRGB(200, 255, 0);

	wifiServer.begin();
}

void loop()
{

	while (route.empty())
	{
		client = wifiServer.available();
		if (client)
		{
			receiveDataFromClient();
		}
	}

	client.write("JSON ok. Starting mission");
	tt_rgb.SetRGB(200, 0, 255);

	tt_sdk.startUntilControl();

	tt_sdk.getBattery(defaultCallback);

	delay(1000);
	tt_sdk.takeOff(defaultCallback);
	Coordinate point = route.front();

	char buffer[50];
	sprintf(buffer, "Going to (%d, %.d, %d)", point.getX(), point.getY(), point.getZ());
	client.write(buffer);
	tt_sdk.go(point.getX(), point.getY(), point.getZ(), 40, goCallback);
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

void receiveDataFromClient()
{
	while (client.connected())
	{
		tt_rgb.SetRGB(0, 255, 0);
		while (client.available() > 0)
		{
			String json = client.readStringUntil('\n');
			json.trim(); // Remove the trailing newline character
			client.write("Received JSON");
			parseJsonAsCoordinate(json.c_str());
			return;
		}
	}
}

void parseJsonAsCoordinate(const char *jsonBuf)
{
	int i = 0;
	StaticJsonDocument<JSON_BUF_SIZE> doc;
	DeserializationError error = deserializeJson(doc, jsonBuf);
	if (error)
	{
		tt_rgb.SetRGB(255, 0, 0);
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.c_str());
	}

	const char *unit = doc["unit"];
	JsonArray pointsJson = doc["points"];

	for (const JsonObject point : pointsJson)
	{
		int16_t x = point["x"];
		int16_t y = point["y"];
		int16_t z = point["z"];

		Coordinate p1 = route.empty() ? Coordinate((char *)unit, 0.0, 0.0, 0.0) : route.back();
		Coordinate p2 = Coordinate((char *)unit, x, y, z);

		int16_t xDistance = abs(p1.getX() - p2.getX());
		int16_t yDistance = abs(p1.getY() - p2.getY());
		int16_t zDistance = abs(p1.getZ() - p2.getZ());

		if (xDistance >= MAX_DISTANCE || yDistance >= MAX_DISTANCE || zDistance >= MAX_DISTANCE ||
			xDistance <= MIN_DISTANCE || yDistance <= MIN_DISTANCE || zDistance <= MIN_DISTANCE)
		{
			uint8_t scalar = Coordinate::getPointScalar(xDistance, yDistance, zDistance);

			int16_t plusX = x / scalar;
			int16_t plusY = y / scalar;
			int16_t plusZ = z / scalar;

			insertUnrolledPoints(unit, scalar, plusX, plusY, plusZ);
		}

		Coordinate coordinate = Coordinate((char *)unit, x, y, z);
		route.push(coordinate);
	}
	Coordinate::printPoints(route);
}

void insertUnrolledPoints(const char *unit, uint8_t scalar, int16_t plusX, int16_t plusY, int16_t plusZ)
{
	Coordinate lastPoint = route.back();

	int16_t x = lastPoint.getX();
	int16_t y = lastPoint.getY();
	int16_t z = lastPoint.getZ();

	for (int16_t i = 1; i <= abs(scalar); i++)
		route.push(Coordinate((char *)unit, x + i * plusX, y + i * plusY, z + i * plusZ)); //
}

void goCallback(char *cmd, String res)
{
	if (client.connected())
	{
		char msg[100];
		sprintf(msg, "cmd: %s, res: %s\n", cmd, res.c_str());
		client.write(msg);
	}

	if (res.indexOf("ok") != -1 && route.size() > 1)
	{
		Coordinate origin = route.front();
		route.pop();
		Coordinate destination = route.front();
		tt_sdk.moveRealtiveTo(origin, destination, 40, goCallback);
	}
	else if (route.size() == 1)
	{
		tt_sdk.land(defaultCallback);
		tt_rgb.SetRGB(0, 255, 0);
		client.write("Finished!");
		while (1)
			;
	}
}
