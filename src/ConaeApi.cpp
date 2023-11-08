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
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Process the content and handle the POST request as needed

    /* Send a response if necessary */
    const char resp[] = "POST /path Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for POST /takeoff */
esp_err_t takeoffHandler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Process the content and handle the POST request as needed

    /* Send a response if necessary */
    const char resp[] = "POST /takeoff Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for POST /led */
esp_err_t ledHandler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Process the content and handle the POST request as needed

    /* Send a response if necessary */
    const char resp[] = "POST /led Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for GET /battery */
esp_err_t batteryHandler(httpd_req_t *req)
{
    RMTT_Protocol *ttSDK = RMTT_Protocol::getInstance();
    String cmdRes = "";
    StaticJsonDocument<100> jsonRes;

    ttSDK->getBattery([&cmdRes](char *cmd, String res) { cmdRes = res; });

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
    // Implement logic to retrieve motor time information

    /* Send a response with motor time information */
    const char motor_time_info[] = "Motor Time Information";
    httpd_resp_send(req, motor_time_info, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for GET /speed */
esp_err_t speedHandler(httpd_req_t *req)
{
    // Implement logic to retrieve speed information

    /* Send a response with speed information */
    const char speed_info[] = "Speed Information";
    httpd_resp_send(req, speed_info, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}