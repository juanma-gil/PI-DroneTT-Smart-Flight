#include "MeasureUnit.h"
/*
Esta clase toma las coordenadas x,y,z que recibe en un json y las devuelve en cm, ya que es la unidad que se utiliza en las coordenadas del drone.
*/
class Coordinate {
    private:
    MeasureUnit unit;
    float x;
    float y;
    float z;

    public:
    Coordinate(MeasureUnit, float, float, float);
    float getX();
    float getY();
    float getZ();
    float Coordinate::getScalar();
};