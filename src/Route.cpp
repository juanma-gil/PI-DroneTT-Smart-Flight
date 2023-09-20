#include "../include/Route.h"

Route *Route::instance = NULL;

Route *Route::getInstance()
{
    if (instance == NULL)
    {
        instance = new Route;
        return instance;
    }
    else
    {
        return instance;
    }
}

void Route::receiveRouteFromClient(WiFiClient *client)
{
    RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
    ttRGB->SetRGB(0, 255, 0);
    String json;
    if (client->available() > 0)
    {
        json = client->readStringUntil('\n');
        json.trim(); // Remove the trailing newline character
        client->write("Received JSON");
        return parseJsonAsCoordinate(json.c_str());
    }
}

void Route::parseJsonAsCoordinate(const char *jsonBuf)
{
    std::vector<Coordinate> *route = Route::getInstance()->getRoute();
    int i = -1;
    StaticJsonDocument<JSON_BUF_SIZE> doc;
    DeserializationError error = deserializeJson(doc, jsonBuf);
    if (error)
    {
        RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
        ttRGB->SetRGB(255, 0, 0);
        Serial.printf("deserializeJson() failed: %s", error.c_str());
    }

    const char *unit = doc["unit"];
    JsonArray pointsJson = doc["points"];

    for (const JsonObject point : pointsJson)
    {
        int16_t x = point["x"];
        int16_t y = point["y"];
        int16_t z = point["z"];

        Coordinate p1 = route->empty() ? Coordinate((char *)unit, 0, 0, 0) : route->at(++i);
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

        route->push_back(coordinate);
    }
    // Coordinate::printPoints(*route);
}

void Route::insertUnrolledPoints(const char *unit, uint8_t scalar, int16_t plusX, int16_t plusY, int16_t plusZ)
{
    Route *routeInstance = Route::getInstance();
    Coordinate lastPoint = route->back();

    int16_t x = lastPoint.getX();
    int16_t y = lastPoint.getY();
    int16_t z = lastPoint.getZ();

    for (int16_t i = 1; i <= abs(scalar); i++)
        Route::getRoute()->push_back(Coordinate((char *)unit, x + i * plusX, y + i * plusY, z + i * plusZ));
}
