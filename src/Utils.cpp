#include "../include/Utils.h"

Utils *Utils::instance = NULL;
SemaphoreHandle_t Utils::xSemaphore = NULL;

Utils *Utils::getInstance()
{
    if (instance == NULL)
    {
        instance = new Utils;
        xSemaphore = xSemaphoreCreateMutex();
        return instance;
    }
    else
    {
        return instance;
    }
}

void Utils::slog(const char *msg)
{
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdPASS)
    {
        Serial.println(msg);
        xSemaphoreGive(xSemaphore);
    }
}