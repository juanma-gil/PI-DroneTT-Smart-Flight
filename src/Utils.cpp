#include "../include/Utils.h"

Utils *Utils::instance = NULL;
SemaphoreHandle_t Utils::xSemaphore;

Utils *Utils::getInstance()
{
    if (instance == NULL)
    {
        instance = new Utils;
        xSemaphore = xSemaphoreCreateMutex();
        if(xSemaphoreGive(xSemaphore) == pdTRUE){
            Serial.println("Utils Semaphore released");
        }
        else{
            Serial.println("Utils Semaphore not released");
        };
        return instance;
    }
    else
    {
        return instance;
    }
}

int8_t Utils::slog(char *msg)
{
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFAIL)
        return -1;
    Serial.println(msg);
    if(xSemaphoreGive(xSemaphore) == pdFAIL) return -1;
    else return 0;
}
