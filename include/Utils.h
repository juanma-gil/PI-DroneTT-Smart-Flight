#include <Arduino.h>

class Utils
{
private:
    static Utils *instance;
    static SemaphoreHandle_t xSemaphore;
    Utils(Utils const &) {}
    Utils &operator=(Utils const &) {}
    Utils(){};

public:
    static Utils *getInstance();
    static int8_t slog(char *msg);
};