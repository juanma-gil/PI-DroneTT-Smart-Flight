#pragma once
#include <RMTT_Libs.h>
#include <ArduinoJson.h>
#include "Utils.h"
#include "esp_http_server.h"
#include "Route.h"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

httpd_handle_t startWebserver(void);
void stopWebserver(httpd_handle_t server);
esp_err_t pathHandler(httpd_req_t *req);
esp_err_t takeoffHandler(httpd_req_t *req);
esp_err_t ledHandler(httpd_req_t *req);
esp_err_t batteryHandler(httpd_req_t *req);
esp_err_t motortimeHandler(httpd_req_t *req);
esp_err_t speedHandler(httpd_req_t *req);
int8_t changeLedColor(httpd_req_t *req, char *r, char *g, char *b, size_t bufLen);
