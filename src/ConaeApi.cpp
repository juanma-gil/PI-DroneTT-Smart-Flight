#include "../include/ConaeApi.h"

/* Register the new URI handlers */
httpd_uri_t uri_path = {
    .uri = "/path",
    .method = HTTP_POST,
    .handler = pathHandler,
    .user_ctx = NULL};

httpd_uri_t uri_takeoff = {
    .uri = "/takeoff",
    .method = HTTP_POST,
    .handler = takeoffHandler,
    .user_ctx = NULL};

httpd_uri_t uri_led = {
    .uri = "/led",
    .method = HTTP_POST,
    .handler = ledHandler,
    .user_ctx = NULL};

httpd_uri_t uri_battery = {
    .uri = "/battery",
    .method = HTTP_GET,
    .handler = batteryHandler,
    .user_ctx = NULL};

httpd_uri_t uri_motortime = {
    .uri = "/motortime",
    .method = HTTP_GET,
    .handler = motortimeHandler,
    .user_ctx = NULL};

httpd_uri_t uri_speed = {
    .uri = "/speed",
    .method = HTTP_GET,
    .handler = speedHandler,
    .user_ctx = NULL};

/* Function for starting the webserver */
httpd_handle_t startWebserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK)
    {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_path);
        httpd_register_uri_handler(server, &uri_takeoff);
        httpd_register_uri_handler(server, &uri_battery);
        httpd_register_uri_handler(server, &uri_led);
        httpd_register_uri_handler(server, &uri_motortime);
        httpd_register_uri_handler(server, &uri_speed);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stopWebserver(httpd_handle_t server)
{
    if (server)
    {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}
/* Handler for POST /path */
esp_err_t pathHandler(httpd_req_t *req)
{
    Route *route = Route::getInstance();
    char body[500];
    size_t recvSize = MIN(req->content_len, sizeof(body));
    int ret = httpd_req_recv(req, body, recvSize);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    route->parseJsonAsCoordinate(body);

    const char res[] = "Route received!";
    httpd_resp_send(req, res, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for POST /takeoff */
esp_err_t takeoffHandler(httpd_req_t *req)
{
    RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
    String cmdRes = "";

    ttSDK->takeOff([&cmdRes](char *cmd, String res)
                      { cmdRes = res; });

    if (cmdRes.indexOf("error") != -1)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_send(req, cmdRes.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for POST /led */
esp_err_t ledHandler(httpd_req_t *req)
{
    char r[4], g[4], b[4];
    char *buf;
    size_t bufLen = httpd_req_get_url_query_len(req) + 1;

    int8_t status = changeLedColor(req, r, g, b, bufLen);

    char res[50] = "";
    snprintf(res, sizeof(res), "Color changed to: R = %s; G = %s; B = %s", r, g, b);
    httpd_resp_send(req, res, HTTPD_RESP_USE_STRLEN);
    return status;
}

/* Handler for GET /battery */
esp_err_t batteryHandler(httpd_req_t *req)
{
    RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
    String cmdRes = "";
    StaticJsonDocument<100> jsonRes;

    ttSDK->getBattery([&cmdRes](char *cmd, String res)
                      { cmdRes = res; });

    if (cmdRes.indexOf("error") != -1)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char *battery = strtok((char *)cmdRes.c_str(), " ");
    battery = strtok(nullptr, " ");

    jsonRes["battery"] = atoi(battery);
    cmdRes = "";
    serializeJsonPretty(jsonRes, cmdRes);

    httpd_resp_send(req, cmdRes.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for GET /motortime */
esp_err_t motortimeHandler(httpd_req_t *req)
{
    RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
    String cmdRes = "";
    StaticJsonDocument<100> jsonRes;

    ttSDK->getMotorTime([&cmdRes](char *cmd, String res)
                        { cmdRes = res; });

    if (cmdRes.indexOf("error") != -1)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char *motortime = strtok((char *)cmdRes.c_str(), " ");
    motortime = strtok(nullptr, " ");

    jsonRes["motortime"] = atoi(motortime);
    cmdRes = "";
    serializeJsonPretty(jsonRes, cmdRes);

    httpd_resp_send(req, cmdRes.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for GET /speed */
esp_err_t speedHandler(httpd_req_t *req)
{
    RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
    String cmdRes = "";
    StaticJsonDocument<100> jsonRes;

    ttSDK->getSpeed([&cmdRes](char *cmd, String res)
                    { cmdRes = res; });

    if (cmdRes.indexOf("error") != -1)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char *speed = strtok((char *)cmdRes.c_str(), " ");
    speed = strtok(nullptr, " ");

    jsonRes["speed"] = atoi(speed);
    cmdRes = "";
    serializeJsonPretty(jsonRes, cmdRes);

    httpd_resp_send(req, cmdRes.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

int8_t changeLedColor(httpd_req_t *req, char *r, char *g, char *b, size_t bufLen)
{
    if (bufLen > 1)
    {
        char *buf = (char *)malloc(bufLen);
        if (httpd_req_get_url_query_str(req, buf, bufLen) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "r", r, sizeof(r)) == ESP_OK &&
                httpd_query_key_value(buf, "g", g, sizeof(g)) == ESP_OK &&
                httpd_query_key_value(buf, "b", b, sizeof(b)) == ESP_OK)
            {
                RMTT_RGB *ttRGB = RMTT_RGB::getInstance();
                ttRGB->SetRGB(atoi(r), atoi(g), atoi(b));
            }
            else
            {
                return ESP_FAIL;
            }
        }
        else
        {
            return ESP_FAIL;
        }
        free(buf);
    }
    return ESP_OK;
}

void startmDNSService()
{
    // Initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    // Set hostname
    mdns_hostname_set("paulmccartney");
    // Set default instance
    mdns_instance_name_set("Robomaster TT - Paul McCartney");

    // Add HTTP service
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    // Set custom instance name for the web server
    mdns_service_instance_name_set("_http", "_tcp", "Robomaster TT - Paul McCartney");

    // Set optional TXT data for the HTTP service
		// es como metadata del servicio, no sé qué tan útil sea
    /* mdns_txt_item_t serviceTxtData[3] = {
        {"board", "{esp32}"},
        {"u", "user"},
        {"p", "password"}
    };
    mdns_service_txt_set("_http", "_tcp", serviceTxtData, 3); */
}