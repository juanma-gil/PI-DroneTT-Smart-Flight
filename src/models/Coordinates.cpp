#include "../../include/models/Coordinate.h"

Coordinate::Coordinate(const char *unit, float x, float y, float z)
{
    this->unit = unit;
    this->x = x;
    this->y = y;
    this->z = z;
}

float Coordinate::getX()
{
    return x * getScalar();
}

float Coordinate::getY()
{
    return y * getScalar();
}

float Coordinate::getZ()
{
    return z * getScalar();
}

float Coordinate::getScalar()
{
    if (unit == "mm")
    {
        return 1 / 10;
    }
    else if (unit == "cm")
    {
        return 1;
    }
    else if (unit == "M")
    {
        return 1 * 100;
    }
    else if (unit == "Km")
    {
        return 1 * 100 * 1000;
    }
    else
    {
        return 1;
    }
}
void Coordinate::toString(char *buffer)
{
    sprintf(buffer, "(x = %.2f, y = %.2f, z = %.2f, unit = %s)", x, y, z, unit);
}

// void Coordinate::printPoints(std::queue<Coordinate> points)
// {
// 	for (Coordinate point : points)
// 	{
// 		char buffer[50];
// 		point.toString(buffer);
// 		Serial.println(buffer);
// 	}
// }