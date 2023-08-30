#include "../include/Utils.h"

Utils *Utils::instance = NULL;
SemaphoreHandle_t Utils::xSemaphore = NULL;
FILE *Utils::logfile = NULL;

Utils *Utils::getInstance()
{
    if (instance == NULL)
    {
        instance = new Utils;
        xSemaphore = xSemaphoreCreateMutex();
        if (xSemaphoreGive(xSemaphore) == pdTRUE)
        {
            Serial.println("Utils Semaphore released");
        }
        else
        {
            Serial.println("Utils Semaphore not released");
        };
        // Open the file for writing, overwriting any existing content
        logfile = fopen("log.txt", "w+");
        if(logfile == NULL) Serial.printf("Unable to open the log file. Error: %d\n", errno);

        return instance;
    }
    else
    {
        return instance;
    }
}

int8_t Utils::slog(char *msg)
{
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdPASS)
    {
        if (logfile != NULL)
        {
            fprintf(logfile, "%s\n", msg);
        }
        else
        {
            Serial.println("Unable to open the log file.");
        }
        xSemaphoreGive(xSemaphore);
        return 0;
    }
    else
    {
        return -1;
    }
}