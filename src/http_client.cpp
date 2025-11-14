
#include "http_client.h"
#include "picohttpparser/picohttpparser.h"

// Static task function for background HTTP requests
void HubHttpClient::httpTaskFunction(void* parameter) {
    TaskContext* context = static_cast<TaskContext*>(parameter);
    
    // Perform the HTTP request
    HubHttpClientResponse response = context->client->sendRequest(
        context->method, context->url, context->body, context->headers
    );
    
    // Call the callback with the response
    if (context->callback) {
        context->callback(response);
    }
    
    // Clean up
    delete context;
    
    // Delete this task
    vTaskDelete(NULL);
}

// HttpResponse methods implementation
std::map<String, String> HubHttpClientResponse::getJsonMap() const {
    std::map<String, String> result;
    
    if (body.length() == 0) {
        return result;
    }
    
    // Simple JSON parser for key-value pairs
    // Note: This is a basic implementation for simple JSON objects
    String json = body;
    json.trim();
    
    if (!json.startsWith("{") || !json.endsWith("}")) {
        return result;
    }
    
    json = json.substring(1, json.length() - 1); // Remove { }
    
    int start = 0;
    while (start < json.length()) {
        int colonPos = json.indexOf(':', start);
        if (colonPos == -1) break;
        
        int commaPos = json.indexOf(',', colonPos);
        if (commaPos == -1) commaPos = json.length();
        
        String key = json.substring(start, colonPos);
        String value = json.substring(colonPos + 1, commaPos);
        
        // Clean up quotes and whitespace
        key.trim();
        value.trim();
        if (key.startsWith("\"") && key.endsWith("\"")) {
            key = key.substring(1, key.length() - 1);
        }
        if (value.startsWith("\"") && value.endsWith("\"")) {
            value = value.substring(1, value.length() - 1);
        }
        
        result[key] = value;
        start = commaPos + 1;
    }
    
    return result;
}

String HubHttpClientResponse::getHeader(const String& name) const {
    for (const auto& pair : headers) {
        if (pair.first.equalsIgnoreCase(name)) {
            return pair.second;
        }
    }
    return "";
}

bool HubHttpClientResponse::hasHeader(const String& name) const {
    for (const auto& pair : headers) {
        if (pair.first.equalsIgnoreCase(name)) {
            return true;
        }
    }
    return false;
}

HubHttpClient::HubHttpClient() {
    client = new WiFiClient();
    secureClient = new WiFiClientSecure();
    userAgent = "HubRobot/1.0";
    timeout = 10000;
    useSecure = false;
}

HubHttpClient::~HubHttpClient() {
    if (client) {
        client->stop();
        delete client;
    }
    if (secureClient) {
        secureClient->stop();
        delete secureClient;
    }
}

void HubHttpClient::setTimeout(int timeoutMs) {
    timeout = timeoutMs;
}

void HubHttpClient::setUserAgent(const String& ua) {
    userAgent = ua;
}

void HubHttpClient::setSecure(bool secure) {
    useSecure = secure;
}

void HubHttpClient::setPersistentHeader(const String& name, const String& value) {
    persistentHeaders[name] = value;
}

void HubHttpClient::removePersistentHeader(const String& name) {
    persistentHeaders.erase(name);
}

void HubHttpClient::clearPersistentHeaders() {
    persistentHeaders.clear();
}

String HubHttpClient::getPersistentHeader(const String& name) const {
    auto it = persistentHeaders.find(name);
    return (it != persistentHeaders.end()) ? it->second : "";
}

String HubHttpClient::buildRequestHeaders(const std::map<String, String>& requestHeaders) {
    String headers = "";
    
    // Add User-Agent
    headers += "User-Agent: " + userAgent + "\r\n";
    
    // Add persistent headers first
    for (const auto& pair : persistentHeaders) {
        headers += pair.first + ": " + pair.second + "\r\n";
    }
    
    // Add request-specific headers (can override persistent ones)
    for (const auto& pair : requestHeaders) {
        headers += pair.first + ": " + pair.second + "\r\n";
    }
    
    // Add cookie header if we have cookies
    String cookieHeader = formatCookieHeader();
    if (cookieHeader.length() > 0) {
        headers += "Cookie: " + cookieHeader + "\r\n";
    }
    
    return headers;
}

void HubHttpClient::parseUrl(const String& url, String& protocol, String& host, int& port, String& path) {
    protocol = "http";
    port = 80;
    
    String workingUrl = url;
    
    // Extract protocol
    if (workingUrl.startsWith("https://")) {
        protocol = "https";
        port = 443;
        workingUrl = workingUrl.substring(8);
    } else if (workingUrl.startsWith("http://")) {
        protocol = "http";
        port = 80;
        workingUrl = workingUrl.substring(7);
    }
    
    // Find path separator
    int pathIndex = workingUrl.indexOf('/');
    if (pathIndex == -1) {
        host = workingUrl;
        path = "/";
    } else {
        host = workingUrl.substring(0, pathIndex);
        path = workingUrl.substring(pathIndex);
    }
    
    // Extract port if specified
    int portIndex = host.indexOf(':');
    if (portIndex != -1) {
        port = host.substring(portIndex + 1).toInt();
        host = host.substring(0, portIndex);
    }
}

HubHttpClientResponse HubHttpClient::parseResponse(WiFiClient* client) {
    HubHttpClientResponse response;
    
    if (!client || !client->connected()) {
        response.errorMessage = "Client not connected";
        return response;
    }

    // Peek to see if there's data yielding to let other tasks run
    while (client->available() == 0) {
        // Yield to other tasks
        delay(1);
    }

    
    // Read status line
    String statusLine = client->readStringUntil('\n');
    statusLine.trim();
    
    if (statusLine.length() == 0) {
        response.errorMessage = "Empty response";
        return response;
    }
    
    // Parse status line (HTTP/1.1 200 OK)
    int firstSpace = statusLine.indexOf(' ');
    int secondSpace = statusLine.indexOf(' ', firstSpace + 1);
    
    if (firstSpace != -1 && secondSpace != -1) {
        response.statusCode = statusLine.substring(firstSpace + 1, secondSpace).toInt();
        response.statusMessage = statusLine.substring(secondSpace + 1);
    } else if (firstSpace != -1) {
        response.statusCode = statusLine.substring(firstSpace + 1).toInt();
    }
    
    // Read headers
    String line;
    while (client->connected() && (line = client->readStringUntil('\n')).length() > 0) {
        line.trim();
        if (line.length() == 0) break; // End of headers
        
        int colonIndex = line.indexOf(':');
        if (colonIndex != -1) {
            String headerName = line.substring(0, colonIndex);
            String headerValue = line.substring(colonIndex + 1);
            headerName.trim();
            headerValue.trim();
            response.headers[headerName] = headerValue;
        }
    }
    
    // Read body
    String contentLengthStr = response.getHeader("Content-Length");
    int contentLength = contentLengthStr.toInt();
    
    if (contentLength > 0) {
        // Read exact content length
        response.bodyBytes.reserve(contentLength);
        for (int i = 0; i < contentLength && client->connected(); i++) {
            if (client->available()) {
                uint8_t b = client->read();
                response.bodyBytes.push_back(b);
            } else {
                delay(1); // Small delay to wait for more data
            }
        }
    } else {
        // Read until connection closes or no more data
        while (client->connected() || client->available()) {
            if (client->available()) {
                uint8_t b = client->read();
                response.bodyBytes.push_back(b);
            } else {
                delay(1);
            }
        }
    }
    
    // Convert bytes to string
    response.body.reserve(response.bodyBytes.size());
    for (uint8_t b : response.bodyBytes) {
        response.body += (char)b;
    }
    
    response.isSuccess = (response.statusCode >= 200 && response.statusCode < 300);
    
    return response;
}

void HubHttpClient::updateCookiesFromResponse(const HubHttpClientResponse& response) {
    String setCookie = response.getHeader("Set-Cookie");
    if (setCookie.length() > 0) {
        // Simple cookie parsing - extract name=value pair
        int equalPos = setCookie.indexOf('=');
        int semicolonPos = setCookie.indexOf(';');
        
        if (equalPos != -1) {
            String cookieName = setCookie.substring(0, equalPos);
            String cookieValue;
            
            if (semicolonPos != -1 && semicolonPos > equalPos) {
                cookieValue = setCookie.substring(equalPos + 1, semicolonPos);
            } else {
                cookieValue = setCookie.substring(equalPos + 1);
            }
            
            cookieName.trim();
            cookieValue.trim();
            
            // Update or add cookie to persistent headers
            String existingCookies = getPersistentHeader("Cookie");
            String newCookieEntry = cookieName + "=" + cookieValue;
            
            if (existingCookies.length() == 0) {
                setPersistentHeader("Cookie", newCookieEntry);
            } else {
                // Check if this cookie already exists and replace it
                String cookiePrefix = cookieName + "=";
                int cookieStart = existingCookies.indexOf(cookiePrefix);
                
                if (cookieStart != -1) {
                    // Replace existing cookie
                    int cookieEnd = existingCookies.indexOf(';', cookieStart);
                    if (cookieEnd == -1) cookieEnd = existingCookies.length();
                    
                    String newCookies = existingCookies.substring(0, cookieStart) + 
                                       newCookieEntry + 
                                       existingCookies.substring(cookieEnd);
                    setPersistentHeader("Cookie", newCookies);
                } else {
                    // Add new cookie
                    setPersistentHeader("Cookie", existingCookies + "; " + newCookieEntry);
                }
            }
        }
    }
}

String HubHttpClient::formatCookieHeader() const {
    return getPersistentHeader("Cookie");   // No need to do anything, it's already formatted when parsing the Set-Cookie
}

extern const uint8_t caCertBundleStart[] asm("_binary_data_x509_crt_bundle_start");
HubHttpClientResponse HubHttpClient::sendRequest(const String& method, const String& url, 
                                      const String& body, 
                                      const std::map<String, String>& headers) {
    HubHttpClientResponse response;
    
    String protocol, host, path;
    int port;
    parseUrl(url, protocol, host, port, path);
    
    WiFiClient* activeClient;
    if (protocol == "https" || useSecure) {
        // Add the CA Certificate bundle
        secureClient->setCACertBundle(caCertBundleStart);
        // secureClient->setInsecure(); // If we want to ignore certificate validation
        activeClient = secureClient;
    } else {
        activeClient = client;
    }

    // Connect to server
    // printf("Connecting to %s:%d\n", host.c_str(), port);
    int connectResult = activeClient->connect(host.c_str(), port); 
    if (!connectResult) {
        response.errorMessage = "Connection failed to " + host + ":" + String(port) + ", with error code " + String(connectResult);
        return response;
    }
    
    // Set timeout
    activeClient->setTimeout(timeout);
    
    // Build request
    String request = method + " " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n";
    
    // Add content length for methods that have body
    if (body.length() > 0) {
        request += "Content-Length: " + String(body.length()) + "\r\n";
    }
    
    // Add headers
    request += buildRequestHeaders(headers);
    request += "\r\n";
    
    // Add body
    if (body.length() > 0) {
        request += body;
    }
    
    // Send request
    printf("Sending request:\n%s\n", request.c_str());
    if (activeClient->print(request) == 0) {
        response.errorMessage = "Failed to send request";
        activeClient->stop();
        return response;
    }
    
    // Parse response
    response = parseResponse(activeClient);
    
    // Update cookies if any
    updateCookiesFromResponse(response);
    
    // Close connection
    activeClient->stop();
    
    return response;
}

// HTTP method implementations
HubHttpClientResponse HubHttpClient::GET(const String& url, const std::map<String, String>& headers) {
    return sendRequest("GET", url, "", headers);
}

HubHttpClientResponse HubHttpClient::POST(const String& url, const String& body, const std::map<String, String>& headers) {
    return sendRequest("POST", url, body, headers);
}

HubHttpClientResponse HubHttpClient::PUT(const String& url, const String& body, const std::map<String, String>& headers) {
    return sendRequest("PUT", url, body, headers);
}

HubHttpClientResponse HubHttpClient::DELETE(const String& url, const std::map<String, String>& headers) {
    return sendRequest("DELETE", url, "", headers);
}

HubHttpClientResponse HubHttpClient::PATCH(const String& url, const String& body, const std::map<String, String>& headers) {
    return sendRequest("PATCH", url, body, headers);
}

HubHttpClientResponse HubHttpClient::HEAD(const String& url, const std::map<String, String>& headers) {
    return sendRequest("HEAD", url, "", headers);
}

HubHttpClientResponse HubHttpClient::request(const String& method, const String& url, 
                                  const String& body, 
                                  const std::map<String, String>& headers) {
    return sendRequest(method, url, body, headers);
}

// Asynchronous HTTP Methods
bool HubHttpClient::GET(const String& url, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false; // No callback provided
    }
    
    TaskContext* context = new TaskContext(this, "GET", url, "", headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192, // Stack size in bytes
        context,
        1, // Priority
        NULL // Task handle
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

bool HubHttpClient::POST(const String& url, const String& body, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    TaskContext* context = new TaskContext(this, "POST", url, body, headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192,
        context,
        1,
        NULL
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

bool HubHttpClient::PUT(const String& url, const String& body, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    TaskContext* context = new TaskContext(this, "PUT", url, body, headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192,
        context,
        1,
        NULL
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

bool HubHttpClient::DELETE(const String& url, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    TaskContext* context = new TaskContext(this, "DELETE", url, "", headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192,
        context,
        1,
        NULL
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

bool HubHttpClient::PATCH(const String& url, const String& body, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    TaskContext* context = new TaskContext(this, "PATCH", url, body, headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192,
        context,
        1,
        NULL
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

bool HubHttpClient::HEAD(const String& url, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    TaskContext* context = new TaskContext(this, "HEAD", url, "", headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192,
        context,
        1,
        NULL
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

bool HubHttpClient::request(const String& method, const String& url, const String& body, 
                           HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    TaskContext* context = new TaskContext(this, method, url, body, headers, callback);
    
    BaseType_t result = xTaskCreate(
        httpTaskFunction,
        "http_task",
        8192,
        context,
        1,
        NULL
    );
    
    if (result != pdPASS) {
        delete context;
        return false;
    }
    
    return true;
}

// Convenience methods
HubHttpClientResponse HubHttpClient::postJson(const String& url, const String& jsonBody, const std::map<String, String>& headers) {
    std::map<String, String> jsonHeaders = headers;
    jsonHeaders["Content-Type"] = "application/json";
    return POST(url, jsonBody, jsonHeaders);
}

HubHttpClientResponse HubHttpClient::postForm(const String& url, const std::map<String, String>& formData, const std::map<String, String>& headers) {
    String body = "";
    bool first = true;
    
    for (const auto& pair : formData) {
        if (!first) body += "&";
        body += pair.first + "=" + pair.second; // Note: Should URL encode in production
        first = false;
    }
    
    std::map<String, String> formHeaders = headers;
    formHeaders["Content-Type"] = "application/x-www-form-urlencoded";
    return POST(url, body, formHeaders);
}

// Asynchronous convenience methods
bool HubHttpClient::postJson(const String& url, const String& jsonBody, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    std::map<String, String> jsonHeaders = headers;
    jsonHeaders["Content-Type"] = "application/json";
    return POST(url, jsonBody, callback, jsonHeaders);
}

bool HubHttpClient::postForm(const String& url, const std::map<String, String>& formData, HttpResponseCallback callback, const std::map<String, String>& headers) {
    if (!callback) {
        return false;
    }
    
    String body = "";
    bool first = true;
    
    for (const auto& pair : formData) {
        if (!first) body += "&";
        body += pair.first + "=" + pair.second; // Note: Should URL encode in production
        first = false;
    }
    
    std::map<String, String> formHeaders = headers;
    formHeaders["Content-Type"] = "application/x-www-form-urlencoded";
    return POST(url, body, callback, formHeaders);
}

