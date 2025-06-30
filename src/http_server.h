#ifndef HUB_HTTP_SERVER_H
#define HUB_HTTP_SERVER_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#else
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#endif

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <string>
#include <stdexcept>
#include <definitions.h>

class HttpRequest {
public:
    String method;
    String path;
    String body;
    std::map<String, String> headers;
    std::map<String, String> query;
    HttpRequest();

    bool jsonRequested();
};

class HttpResponse {
public:
    int status;
    String body;
    std::map<String, String> headers;
    HttpResponse();
};

class HttpServer {
public:
    HttpServer();
    void begin();
    void tick();
    void on(const String &path, std::function<HttpResponse(WiFiClient *, HttpRequest &)> handler);
    void setDebug(bool debug);
    void setLogger(CachingPrinter &logger);
    void setPort(uint16_t port);
    void setServerName(const String &serverName);
    void setServerVersion(const String &serverVersion);

private:
    bool debug = false;
    bool running = false;
    CachingPrinter *logger = nullptr;
    uint16_t port = 80;
    String serverName = "HubServer";
    String serverVersion = "1.0";
    WiFiServer server;
    std::map<String, std::function<HttpResponse(WiFiClient *, HttpRequest &)>> httpHandlers;
    void handleClient(WiFiClient newClient);
    void respondToClient(WiFiClient &newClient, HttpResponse &response);
    void handleRequest(HttpRequest req, WiFiClient *client);
};


int wifiScan(Print* printer = nullptr);


#endif // HUB_HTTP_SERVER_H