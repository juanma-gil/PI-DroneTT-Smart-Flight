#include "../include/main.h"

/*-------------- Fuction Implementation --------------*/

void setup()
{
	routePoints = route->getRoute();

	Serial.begin(115200);
	Serial1.begin(1000000, SERIAL_8N1, 23, 18);

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
	utils->slog("WiFi Server started");
	startWebserver();
	utils->slog("Web Server started");
	startmDNSService();
	utils->slog("mDNS service started");

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
	// if (xTaskCreatePinnedToCore(vMissionTask, "Mission", configMINIMAL_STACK_SIZE * 4, NULL, 20, &missionTaskHandle, 1) != pdPASS)
	// {
	// 	utils->slog("Failed to create Mission task");
	// };
}

void loop()
{
}

// void vMissionTask(void *parameter)
// {
// }

void vLogTask(void *parameter)
{
	char msg[500];
	for (;;)
	{
		vTaskDelay(logDELAY);
		if (xQueueReceive(xLogQueue, &msg, logDELAY) == pdTRUE)
		{
			client = client ? client : wifiServer.available();
			utils->slog(msg);
			if (client)
			{
				client.write(msg);
			}
		}
	}
}

void initialCallback(char *cmd, String res)
{
	char msg[100];
	snprintf(msg, sizeof(msg), "InitialCallback - cmd: %s, res: %s\n", cmd, res.c_str());
	utils->slog(msg);
}