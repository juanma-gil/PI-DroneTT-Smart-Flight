#pragma once

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
  static void slog(const char *msg);
};