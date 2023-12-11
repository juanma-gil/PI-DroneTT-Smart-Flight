#include<RMTT_Libs.h>
#include <ArduinoJson.h>

#ifndef CONAEAPI_H
#define CONAEAPI_H
#include "Utils.h"
#include "Route.h"
#endif

#include "esp_http_server.h"
#include <mdns.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void startWebserver();
esp_err_t webserverStopHandler(httpd_req_t *req);
esp_err_t pathHandler(httpd_req_t *req);
esp_err_t sdkHandler(httpd_req_t *req);
esp_err_t takeoffHandler(httpd_req_t *req);
esp_err_t landHandler(httpd_req_t *req);
esp_err_t motorHandler(httpd_req_t *req);
esp_err_t ledHandler(httpd_req_t *req);
esp_err_t batteryHandler(httpd_req_t *req);
esp_err_t motortimeHandler(httpd_req_t *req);
esp_err_t speedHandler(httpd_req_t *req);
int8_t changeLedColor(httpd_req_t *req, char *r, char *g, char *b, size_t bufLen);
void startmDNSService();