// #include <iostream>
// #include <cstdlib>
// #include <cstring>
// #include <unistd.h>
// #include <Arduino.h>
// #include <WiFi.h>
// #include <RMTT_Libs.h>

// #define PORT 5001

// /*-------------- Global Variables --------------*/

// RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
// RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
// const char *ssid = "LCD";
// const char *password = "1cdunc0rd0ba";
// WiFiServer wifiServer(PORT);
// WiFiClient client;
// int point_index = 0, vueltas = 0;

// /*-------------- Fuction Declaration --------------*/

// void defaultCallback(char *cmd, String res);

// /*-------------- Fuction Implementation --------------*/

// void setup()
// {
//     ttRGB->Init();
//     Serial.begin(115200);
//     Serial1.begin(1000000, SERIAL_8N1, 23, 18);

//     delay(1000);
//     WiFi.begin(ssid, password);

//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(1000);
//         Serial.println("Connecting to WiFi..");
//     }

//     Serial.println("Connected to the WiFi network");
//     Serial.println(WiFi.localIP());
//     ttRGB->SetRGB(200, 255, 0);

//     wifiServer.begin();
// }

// void loop()
// {
//     ttRGB->SetRGB(0, 255, 0);
//     client = wifiServer.available();
//     while (!client)
//     {
//         // No client connected, continue checking
//         delay(100); // Adjust the delay as needed to reduce CPU load
//         client = wifiServer.available();
//     }

//     // Client connected, process the request
//     Serial.println("Client connected.");

//     // Send the response to the client
//     client.write("Hello from ESP32 Server!\n");

//     ttSDK->startUntilControl();
//     ttSDK->getBattery(defaultCallback);
//     ttSDK->takeOff(defaultCallback);
//     // ttSDK->motorOn(defaultCallback);
//     while (vueltas <= 5)
//     {
//         delay(500);
//         ttSDK->curve(0, 0, 60, 60, 60, 60, 40, defaultCallback);
//         ttSDK->curve(0, 0, -60, -60, -60, -60, 40, defaultCallback);
//         vueltas++;
//     }
//     ttSDK->land(defaultCallback);

//     client.stop();
//     while (1)
//     {
//     };
// }

// void defaultCallback(char *cmd, String res)
// {
//     Serial.println(res);
//     if (client.connected())
//     {
//         char msg[100];
//         sprintf(msg, "cmd: %s, res: %s\n", cmd, res.c_str());
//         client.write(msg);
//     }
// }