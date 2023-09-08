#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <RMTT_Libs.h>
#include <vector>
#define JSON_BUF_SIZE 1024

class Route
{
private:
    static Route *instance;
    Route(Route const &) {}
    Route &operator=(Route const &) {}
    std::vector<Coordinate> *route;
    Route()
    {
        route = new std::vector<Coordinate>();
    };

public:
    static Route *getInstance();
    std::vector<Coordinate> *getRoute()
    {
        return route;
    }
    void receiveRouteFromClient(WiFiClient *client);
    void parseJsonAsCoordinate(const char *jsonBuf);
    void insertUnrolledPoints(const char *unit, uint8_t scalar, int16_t plusX, int16_t plusY, int16_t plusZ);
    
};