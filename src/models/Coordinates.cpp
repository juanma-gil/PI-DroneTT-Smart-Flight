#include "../../include/models/Coordinate.h"

Coordinate::Coordinate(MeasureUnit unit, float x, float y, float z)
{
    this->unit = unit;
    this->x = x;
    this->y = y;
    this->z = z;
}

float Coordinate::getX()
{
    return this->x * this->getScalar();
}

float Coordinate::getY()
{
    return this->y * this->getScalar();
}

float Coordinate::getZ()
{
    return this->z * this->getScalar();
}

float Coordinate::getScalar()
{
    switch (this->unit)
    {
    case MeasureUnit::MM:
        return 1 / 10;
        break;
    case MeasureUnit::CM:
        return 1;
        break;
    case MeasureUnit::M:
        return 1 * 100;
        break;
    case MeasureUnit::KM:
        return 1 * 100 * 1000;
        break;
    default:
        break;
    }
}