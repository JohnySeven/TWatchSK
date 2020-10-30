#include "ArduinoJson.h"
#include "esp_websocket_client.h"
static const char *WS_TAG = "web_socket";

class SignalKSocket
{
    public:
        bool connect();
        void setup(char*server, int port);
        bool disconnect();
    private:
        esp_websocket_client_handle_t websocket;
};