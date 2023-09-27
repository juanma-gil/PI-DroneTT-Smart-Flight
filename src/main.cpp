#include "../include/main.h"

/*-------------- Fuction Implementation --------------*/

void setup()
{
	routePoints = route->getRoute();

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
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}

	/*---------------------------------------------------------*/

	WiFi.begin(SSID, PASSWORD);
	while (WiFi.status() != WL_CONNECTED)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		utils->slog("Connecting to WiFi..");
	}

	ttRGB->Init();
	utils->slog("Connected to the WiFi network");
	Serial.println(WiFi.localIP());
	ttRGB->SetRGB(200, 255, 0);
	wifiServer.begin();

	while (!client)
	{
		client = wifiServer.available();
		vTaskDelay(pdMS_TO_TICKS(10));
	};

	utils->slog("Client connected");

	while (routePoints->empty())
	{
		delay(10);
		route->receiveRouteFromClient(&client);
	}

	Serial1.flush();
	ttSDK->sdkOn();

	/*-------------- Queues --------------*/

	xLogQueue = xQueueCreate(logQueueSIZE, logItemSIZE);
	if (xLogQueue == NULL)
	{
		utils->slog("Queue creation has FAILED");
	}

	/*-------------- Tasks  --------------*/

	if (xTaskCreatePinnedToCore(vLogTask, "Logger", configMINIMAL_STACK_SIZE * 4, NULL, 5, &loggerTaskHandle, 0) != pdPASS)
	{
		utils->slog("Failed to create Logger task");
	}
	if (xTaskCreatePinnedToCore(vMissionTask, "Mission", configMINIMAL_STACK_SIZE * 4, NULL, 20, &missionTaskHandle, 1) != pdPASS)
	{
		utils->slog("Failed to create Mission task");
	};
}

void loop()
{
	vTaskDelete(NULL);
}

void vMissionTask(void *parameter)
{
	char msg[100];

	ttRGB->SetRGB(0, 0, 255);
	ttSDK->getBattery(missionCallback);
	ttSDK->takeOff(missionCallback);

	for (;;)
	{
		vTaskDelay(missionDELAY);
		xQueueSend(xLogQueue, &msg, logQueueDELAY);

		Coordinate origin = routePoints->at(point_index++);
		if (routePoints->size() == point_index)
		{
			char msg[50];
			sniprintf(msg, sizeof(msg), "Mission finished, landing ...");
			xQueueSend(xLogQueue, &msg, logQueueDELAY);
			ttSDK->land();
			ttRGB->SetRGB(0, 255, 0);
			while (1)
				vTaskDelay(missionDELAY);
		}
		Coordinate destination = routePoints->at(point_index);
		ttSDK->moveRelativeTo(origin, destination, 20, missionCallback);
		while (tofSense(dodgeFun))
			;
		tries = 0;
	}
}

void vLogTask(void *parameter)
{
	char msg[500];
	for (;;)
	{
		vTaskDelay(logDELAY);
		if (xQueueReceive(xLogQueue, &msg, logDELAY) == pdTRUE)
		{
			utils->slog(msg);
			client.write(msg);
		}
	}
}

boolean tofSense(std::function<void()> callback)
{
	printRoutePoints();

	char msg[50];
	measure = tt_sensor.ReadRangeSingleMillimeters(); // Performs a single-shot range measurement and returns the reading in millimeters
	if (tt_sensor.TimeoutOccurred())
	{
		xQueueSend(xLogQueue, "Sensor timeout\n", 0);
		return NULL;
	}

	boolean obstacleDetected = measure < MAX_MEASURE;
	if (obstacleDetected)
		callback();

	sniprintf(msg, sizeof(msg), "MAX_MEASURE = %d | Current measure: %d\n", MAX_MEASURE, measure);
	xQueueSend(xLogQueue, &msg, logQueueDELAY);

	return obstacleDetected;
}

void dodgeFun()
{
	char msg[20];
	ttRGB->SetRGB(200, 255, 0);
	Coordinate lastPoint = routePoints->at(point_index), newPoint;
	uint16_t movement;
	switch (tries++)
	{
	case 0 ... 1:
		ttSDK->up(UP_DODGE, missionCallback);
		newPoint = Coordinate("cm", lastPoint.getX(), lastPoint.getY(), lastPoint.getZ() + UP_DODGE);
		break;
	case 2:
		movement = UP_DODGE * 2 + DOWN_DODGE;
		ttSDK->down(movement, missionCallback);
		newPoint = Coordinate("cm", lastPoint.getX(), lastPoint.getY(), lastPoint.getZ() - movement);
		break;
	case 3:
		ttSDK->up(DOWN_DODGE, missionCallback);
		ttSDK->left(LEFT_DODGE, missionCallback);
		newPoint = Coordinate("cm", lastPoint.getX(), lastPoint.getY() + LEFT_DODGE, lastPoint.getZ() + UP_DODGE);
		break;
	case 4:
		ttSDK->left(LEFT_DODGE, missionCallback);
		newPoint = Coordinate("cm", lastPoint.getX(), lastPoint.getY() + LEFT_DODGE, lastPoint.getZ());
		break;
	case 5:
		movement = LEFT_DODGE * 2 + RIGHT_DODGE;
		ttSDK->right(movement, missionCallback);
		newPoint = Coordinate("cm", lastPoint.getX(), lastPoint.getY() - movement, lastPoint.getZ());
		break;
	case 6:
		ttSDK->right(RIGHT_DODGE, missionCallback);
		newPoint = Coordinate("cm", lastPoint.getX(), lastPoint.getY() - RIGHT_DODGE, lastPoint.getZ());
		break;
	default:
		ttRGB->SetRGB(255, 0, 0);
		ttSDK->land();
		break;
	}
	routePoints->emplace(routePoints->begin() + ++point_index, newPoint);
	sniprintf(msg, sizeof(msg), "Tries: %d\n", tries);
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
	ttRGB->SetRGB(0, 0, 255);
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

	if (res.indexOf("error") != -1)
	{
		snprintf(msg, sizeof(msg), "ERROR: %s", res);
		xQueueSend(xLogQueue, &msg, logQueueDELAY);
		ttSDK->land();
	}

	snprintf(msg, sizeof(msg), "MissionCallback - cmd: %s, res: %s", cmd, res.c_str());
	xQueueSend(xLogQueue, &msg, logQueueDELAY);
}

void printRoutePoints()
{
	char msg[1000];

	for (int i = 0; i < routePoints->size(); i++)
	{
		Coordinate coord = routePoints->at(i);
		char coordStr[100];
		snprintf(coordStr, sizeof(coordStr), "x = %d, y = %d, z = %d\n", coord.getX(), coord.getY(), coord.getZ());
		strcat(msg, coordStr);
	}
	Serial.println(msg);
	client.println(msg);
}