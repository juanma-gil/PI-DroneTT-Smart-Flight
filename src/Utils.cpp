#include "../include/Utils.h"

Utils *Utils::instance = NULL;
SemaphoreHandle_t Utils::xSemaphore;

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

void Utils::slog(char *msg)
{
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFAIL)
        return;
    Serial.println(msg);
    xSemaphoreGive(xSemaphore) == pdFAIL;
}
