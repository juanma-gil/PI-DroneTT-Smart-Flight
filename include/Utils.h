#include <Arduino.h>
#include <stdio.h>
#include <errno.h>

class Utils
{
private:
    static Utils *instance;
    static FILE *logfile;
    static SemaphoreHandle_t xSemaphore;
    Utils(Utils const &) {}
    Utils &operator=(Utils const &) {}
    Utils(){};

public:
    static Utils *getInstance();
    static int8_t slog(char *msg);
  //  static uint8_t fileLog(char  *msg);
};