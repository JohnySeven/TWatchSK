#pragma once
#include "config.h"
#include "esp_http_client.h"
#include <functional>
#include "ArduinoJson.h"
#include "SPIFFS.h"

class JsonHttpRequest
{
public:
    JsonHttpRequest(const char *requestUrl, const char *token)
    {
        requestUrl_ = requestUrl;
        token_ = token;
    }

    bool downloadFile(const char *path)
    {
        auto ret = false;
        auto file = SPIFFS.open(path, "w");
        if (file)
        {
            ESP_LOGI("HTTP", "HTTP GET intializing client...");
            esp_http_client_config_t config = {
                .url = requestUrl_,
                .event_handler = _http_event_handle};

            esp_http_client_handle_t client = esp_http_client_init(&config);
            //set method
            esp_http_client_set_method(client, esp_http_client_method_t::HTTP_METHOD_GET);
            //set auth header
            char buffer[512];
            sprintf(buffer, "Bearer %s", token_);
            esp_http_client_set_header(client, "Authorization", buffer);
            ESP_LOGI("HTTP", "HTTP GET  opening connection to %s...", requestUrl_);
            esp_err_t err = esp_http_client_open(client, 0);
            ESP_LOGI("HTTP", "HTTP GET open connection result=%d.", err);
            if (err == ESP_OK)
            {
                auto len = esp_http_client_fetch_headers(client);
                auto status = esp_http_client_get_status_code(client);
                ESP_LOGI("HTTP", "HTTP GET %s got status %d with len %d", requestUrl_, status, len);

                if (status == 200)
                {
                    int total = 0;
                    int readLen = -1;
                    while (readLen != 0)
                    {
                        readLen = esp_http_client_read(client, buffer, sizeof(buffer));
                        ESP_LOGI("HTTP", "HTTP GET read=%d", readLen);
                        total += readLen;

                        if (readLen > 0)
                        {
                            file.write((const uint8_t *)buffer, (size_t)readLen);
                        }
                    }
                    if(total == len)
                    {
                        ret = true;
                    }

                    ESP_LOGI("HTTP", "HTTP GET DONE!");
                }

                esp_http_client_cleanup(client);

                ESP_LOGI("HTTP", "Client cleanup.");
            }

            file.close();
            ESP_LOGI("HTTP", "File closed!");
        }
        else
        {
            ESP_LOGI("HTTP", "Unable to create file!");
        }

        return ret;
    }

    ~JsonHttpRequest()
    {
    }

private:
    const char *requestUrl_;
    const char *token_;

    static esp_err_t _http_event_handle(esp_http_client_event_t *evt)
    {
        switch (evt->event_id)
        {
        case HTTP_EVENT_ERROR:
            ESP_LOGI("HTTP", "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI("HTTP", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI("HTTP", "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI("HTTP", "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char *)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI("HTTP", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*if (!esp_http_client_is_chunked_response(evt->client))
            {
                printf("%.*s", evt->data_len, (char *)evt->data);
            }*/

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI("HTTP", "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI("HTTP", "HTTP_EVENT_DISCONNECTED");
            break;
        }
        return ESP_OK;
    }
};