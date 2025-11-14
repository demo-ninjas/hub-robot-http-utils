#ifndef HUB_HTTP_CLIENT_H
#define HUB_HTTP_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <map>
#include <vector>


// Forward declaration
class HubHttpClient;
class HubHttpClientResponse;

// Callback type for asynchronous requests
typedef std::function<void(const HubHttpClientResponse&)> HttpResponseCallback;

/**
 * HTTP Response structure
 */
struct HubHttpClientResponse {
    int statusCode;
    String statusMessage;
    std::map<String, String> headers;
    String body;
    std::vector<uint8_t> bodyBytes;
    bool isSuccess;
    String errorMessage;
    
    HubHttpClientResponse() : statusCode(0), isSuccess(false) {}
    
    // Parse the body as JSON into a simple key-value map
    std::map<String, String> getJsonMap() const;
    
    // Get header value (case-insensitive)
    String getHeader(const String& name) const;
    
    // Check if response has a specific header
    bool hasHeader(const String& name) const;
};

/**
 * Basic HTTP Client for the Hub Robot
 */
class HubHttpClient {
private:
    WiFiClient* client;
    WiFiClientSecure* secureClient;
    std::map<String, String> persistentHeaders;
    String userAgent;
    int timeout;
    bool useSecure;
    
    // Context object for passing to the async tasks
    struct TaskContext {
        HubHttpClient* client;
        String method;
        String url;
        String body;
        std::map<String, String> headers;
        HttpResponseCallback callback;
        
        TaskContext(HubHttpClient* c, const String& m, const String& u, 
                   const String& b, const std::map<String, String>& h, 
                   HttpResponseCallback cb) 
            : client(c), method(m), url(u), body(b), headers(h), callback(cb) {}
    };
    // Function that performs the actual HTTP request in the background task
    static void httpTaskFunction(void* parameter);
    
    String buildRequestHeaders(const std::map<String, String>& requestHeaders = {});
    HubHttpClientResponse parseResponse(WiFiClient* client);
    HubHttpClientResponse sendRequest(const String& method, const String& url, const String& body = "", const std::map<String, String>& headers = {});
    void parseUrl(const String& url, String& protocol, String& host, int& port, String& path);
    void updateCookiesFromResponse(const HubHttpClientResponse& response);
    String formatCookieHeader() const;
    
public:
    HubHttpClient();
    ~HubHttpClient();
    
    // Configuration
    void setTimeout(int timeoutMs);
    void setUserAgent(const String& ua);
    void setSecure(bool secure);
    
    // Persistent header management
    void setPersistentHeader(const String& name, const String& value);
    void removePersistentHeader(const String& name);
    void clearPersistentHeaders();
    String getPersistentHeader(const String& name) const;
    
    // HTTP Methods - Synchronous (blocking)
    HubHttpClientResponse GET(const String& url, const std::map<String, String>& headers = {});
    HubHttpClientResponse POST(const String& url, const String& body = "", const std::map<String, String>& headers = {});
    HubHttpClientResponse PUT(const String& url, const String& body = "", const std::map<String, String>& headers = {});
    HubHttpClientResponse DELETE(const String& url, const std::map<String, String>& headers = {});
    HubHttpClientResponse PATCH(const String& url, const String& body = "", const std::map<String, String>& headers = {});
    HubHttpClientResponse HEAD(const String& url, const std::map<String, String>& headers = {});
    
    // HTTP Methods - Asynchronous (non-blocking with callback)
    bool GET(const String& url, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    bool POST(const String& url, const String& body, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    bool PUT(const String& url, const String& body, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    bool DELETE(const String& url, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    bool PATCH(const String& url, const String& body, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    bool HEAD(const String& url, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    
    // Core Request method used by the synchronous helper methods above
    HubHttpClientResponse request(const String& method, const String& url, 
                        const String& body = "", 
                        const std::map<String, String>& headers = {});
    
    // Core Request method used by the asynchronous helper methods above
    bool request(const String& method, const String& url, const String& body, 
                HttpResponseCallback callback, const std::map<String, String>& headers = {});
    
    // Convenience methods
    HubHttpClientResponse postJson(const String& url, const String& jsonBody, const std::map<String, String>& headers = {});
    HubHttpClientResponse postForm(const String& url, const std::map<String, String>& formData, const std::map<String, String>& headers = {});
    bool postJson(const String& url, const String& jsonBody, HttpResponseCallback callback, const std::map<String, String>& headers = {});
    bool postForm(const String& url, const std::map<String, String>& formData, HttpResponseCallback callback, const std::map<String, String>& headers = {});
};

#endif // HUB_HTTP_CLIENT_H