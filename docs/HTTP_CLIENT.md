# HTTP Client Documentation

## Features

- **Support for all Standard HTTP methods**: GET, POST, PUT, DELETE, PATCH, HEAD, and generic method names
- **Synchronous & Asynchronous Support**: Choose blocking or non-blocking requests (non-blocking requests run in a FreeRTOS task and call a callback function)
- **Persistent Headers**: Set headers that are automatically included in all requests (e.g., Authorization tokens)
- **Automatic Cookie Management**: Handles `Set-Cookie` responses and automatically sends cookies in subsequent requests
- **HTTPS Support**: Supports both HTTP and HTTPS protocols (embeds the default Mozilla root CA package + scripts to update the package as needed)

## Basic Usage

### Include and Initialize

```cpp
#include "http_client.h"

HubHttpClient httpClient;

void setup() {
    // Configure the client
    httpClient.setTimeout(15000);  // 15 second timeout
    httpClient.setUserAgent("MyApp/1.0");
}
```

### Simple GET Request

```cpp
HubHttpClientResponse response = httpClient.GET("http://api.example.com/data");

if (response.isSuccess) {
    Serial.println("Status: " + String(response.statusCode));
    Serial.println("Body: " + response.body);
} else {
    Serial.println("Error: " + response.errorMessage);
}
```

### POST with JSON

```cpp
String jsonData = "{\"temperature\":25.5,\"humidity\":60}";
HubHttpClientResponse response = httpClient.postJson("http://api.example.com/sensors", jsonData);

if (response.isSuccess) {
    Serial.println("Data posted successfully!");
}
```

### POST with Form Data

```cpp
std::map<String, String> formData;
formData["username"] = "robot";
formData["password"] = "secret123";

HttpResponse response = httpClient.postForm("http://api.example.com/login", formData);
```

## Configuration Methods

### setTimeout(int timeoutMs)
Set the request timeout in milliseconds.

```cpp
httpClient.setTimeout(10000);  // 10 seconds
```

### setUserAgent(const String& userAgent)
Set the User-Agent header for all requests.

```cpp
httpClient.setUserAgent("HubRobot-ESP32/1.0");
```

### setSecure(bool secure)
Force HTTPS for all requests.

```cpp
httpClient.setSecure(true);  // Force using the secure client for all requests
```

## Persistent Headers

Persistent headers are automatically added to all subsequent requests. This is useful for authentication tokens, API keys, or other headers that should be included with every request.

### setPersistentHeader(const String& name, const String& value)
Add or update a persistent header.

```cpp
httpClient.setPersistentHeader("Authorization", "Bearer your-token-here");
httpClient.setPersistentHeader("X-API-Key", "your-api-key");
```

### removePersistentHeader(const String& name)
Remove a specific persistent header.

```cpp
httpClient.removePersistentHeader("Authorization");
```

### clearPersistentHeaders()
Remove all persistent headers.

```cpp
httpClient.clearPersistentHeaders();
```

### getPersistentHeader(const String& name)
Get the value of a persistent header.

```cpp
String authHeader = httpClient.getPersistentHeader("Authorization");
```

## HTTP Methods

All HTTP methods are available in both synchronous and asynchronous versions.

### GET Methods

**Synchronous GET:**
```cpp
// Simple GET
HubHttpClientResponse response = httpClient.GET("http://api.example.com/users");

// GET with custom headers
std::map<String, String> headers;
headers["Accept"] = "application/json";
HubHttpClientResponse response = httpClient.GET("http://api.example.com/users", headers);
```

**Asynchronous GET:**
```cpp
// Simple async GET
bool started = httpClient.GET("http://api.example.com/users", [](const HubHttpClientResponse& response) {
    Serial.println("Users loaded: " + String(response.body.length()) + " bytes");
});

// Async GET with custom headers
std::map<String, String> headers;
headers["Accept"] = "application/json";
bool started = httpClient.GET("http://api.example.com/users", myCallback, headers);
```

### POST Methods

**Synchronous POST:**
```cpp
String jsonBody = "{\"name\":\"HubRobot\",\"type\":\"microcontroller\"}";
HubHttpClientResponse response = httpClient.POST("http://api.example.com/devices", jsonBody);
```

**Asynchronous POST:**
```cpp
String jsonBody = "{\"name\":\"HubRobot\",\"type\":\"microcontroller\"}";
bool started = httpClient.POST("http://api.example.com/devices", jsonBody, [](const HubHttpClientResponse& response) {
    Serial.println("Device created with status: " + String(response.statusCode));
});
```

### PUT, DELETE, PATCH, HEAD Methods

All other methods follow the same pattern with both synchronous and asynchronous versions:

**Synchronous:**
```cpp
HubHttpClientResponse response = httpClient.PUT("http://api.example.com/device/123", jsonBody);
HubHttpClientResponse response = httpClient.DELETE("http://api.example.com/device/123");
HubHttpClientResponse response = httpClient.PATCH("http://api.example.com/device/123", patchData);
HubHttpClientResponse response = httpClient.HEAD("http://api.example.com/device/123");
```

**Asynchronous:**
```cpp
bool started = httpClient.PUT("http://api.example.com/device/123", jsonBody, myCallback);
bool started = httpClient.DELETE("http://api.example.com/device/123", myCallback);
bool started = httpClient.PATCH("http://api.example.com/device/123", patchData, myCallback);
bool started = httpClient.HEAD("http://api.example.com/device/123", myCallback);
```

### Generic Request Method

For any other HTTP method, use the `request` function:

**Synchronous:**
```cpp
HubHttpClientResponse response = httpClient.request("OPTIONS", "http://api.example.com/cors-check");
```

**Asynchronous:**
```cpp
bool started = httpClient.request("OPTIONS", "http://api.example.com/cors-check", "", myCallback);
```

## Convenience Methods

### JSON POST Methods

**Synchronous postJson:**
This method will automatically set the `Content-Type` header to `application/json`:

```cpp
String json = "{\"sensor\":\"temperature\",\"value\":25.5}";
HubHttpClientResponse response = httpClient.postJson("http://api.example.com/data", json);
```

**Asynchronous postJson:**
```cpp
String json = "{\"sensor\":\"temperature\",\"value\":25.5}";
bool started = httpClient.postJson("http://api.example.com/data", json, [](const HubHttpClientResponse& response) {
    Serial.println("JSON posted successfully: " + String(response.statusCode));
});
```

### Form POST Methods

**Synchronous postForm:**
Automatically formats a map into form data and sets the `Content-Type` header to `application/x-www-form-urlencoded`:

```cpp
std::map<String, String> form;
form["email"] = "user@example.com";
form["action"] = "subscribe";

HubHttpClientResponse response = httpClient.postForm("http://api.example.com/newsletter", form);
```

**Asynchronous postForm:**
```cpp
std::map<String, String> form;
form["email"] = "user@example.com";
form["action"] = "subscribe";

bool started = httpClient.postForm("http://api.example.com/newsletter", form, [](const HubHttpClientResponse& response) {
    if (response.isSuccess) {
        Serial.println("Newsletter subscription successful!");
    }
});
```

## Cookie Management

The HTTP client will automatically accepts + stores cookies from the server (via `Set-Cookie`) - and will automatically attach them to subsequent requests (assuming you're using the same client object)

### Example Cookie Workflow

```cpp
// First request - server sets a session cookie
HttpResponse response1 = httpClient.GET("http://example.com/login");
// If server responds with: Set-Cookie: session_id=abc123; Path=/
// The cookie is automatically stored

// Second request - cookie is automatically sent
HttpResponse response2 = httpClient.GET("http://example.com/profile");
// Request automatically includes: Cookie: session_id=abc123
```

## Error Handling

Always check the response for errors:

```cpp
HttpResponse response = httpClient.GET("http://api.example.com/data");

if (!response.isSuccess) {
    Serial.println("Request failed!");
    Serial.println("Status: " + String(response.statusCode));
    Serial.println("Error: " + response.errorMessage);
    return;
}

// Process successful response
Serial.println("Success! Body: " + response.body);
```

## HTTPS/SSL Support

The client supports HTTPS connections using an embedded ROOT CA bundle from Mozilla.

Check the [README.md](./data/README.md) for the steps to take to update the CA Bundle.


## Example Applications

### IoT Sensor Data Upload

```cpp
void uploadSensorData() {
    // Set API key once
    httpClient.setPersistentHeader("X-API-Key", "your-sensor-api-key");
    
    // Prepare sensor data
    String sensorData = "{";
    sensorData += "\"temperature\":" + String(temperature) + ",";
    sensorData += "\"humidity\":" + String(humidity) + ",";
    sensorData += "\"timestamp\":" + String(millis());
    sensorData += "}";
    
    // Upload data
    HttpResponse response = httpClient.postJson("https://example.com/api/sensors", sensorData);
    
    if (response.isSuccess) {
        Serial.println("Sensor data uploaded successfully!");
    } else {
        Serial.println("Upload failed: " + response.errorMessage);
    }
}
```

## Troubleshooting

### Common Issues

1. **Connection Timeouts**: Increase timeout value or check network connectivity
2. **Memory Issues**: Monitor heap usage, especially with large responses
3. **SSL/HTTPS Issues**: Do you need to update the Root CA bundle??
4. **Header Issues**: Check header names and values for proper formatting
