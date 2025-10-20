# HTTP Server Library - Usage Guide

A lightweight, memory-safe HTTP server library for ESP32 microcontrollers using the Arduino framework. This library is designed specifically for resource-constrained environments and provides a clean, modern API for building web services on your ESP32.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [HttpServer](#httpserver)
  - [HttpRequest](#httprequest)
  - [HttpResponse](#httpresponse)
- [Usage Examples](#usage-examples)
- [Advanced Features](#advanced-features)
- [Memory Management](#memory-management)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

## Features

✅ **Memory Safe** - Bounds checking, resource limits, automatic cleanup  
✅ **Efficient** - Optimized for microcontrollers with limited resources  
✅ **Modern API** - Clean, fluent interface with method chaining  
✅ **Route Handling** - Simple path-based routing with path parameters  
✅ **Middleware Support** - Process all requests with custom logic and short-circuit capability  
✅ **CORS Support** - Built-in Cross-Origin Resource Sharing  
✅ **Error Handling** - Custom error handlers and automatic responses  
✅ **Debug Logging** - Comprehensive logging support with built-in log endpoint  
✅ **Request Parsing** - Query parameters, headers, body, and path parameters  
✅ **Response Helpers** - JSON, HTML, text, redirect helpers  
✅ **Connection Management** - Keep-alive support, max connections, inactivity timeouts  
✅ **Default Headers** - Automatic headers for all responses  
✅ **Before-Send Hook** - Final processing before sending responses  

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
[env:esp32]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
    https://github.com/demo-ninjas/hub-robot-http-utils.git
    https://github.com/demo-ninjas/hub-robot-core.git
```

### Arduino IDE

1. Clone or download this repository
2. Copy to your Arduino `libraries` folder
3. Ensure dependencies are installed (hub-robot-core)

## Quick Start

Here's a minimal example to get you started:

```cpp
#include <WiFi.h>
#include <http_server.h>

const char* ssid = "your-ssid";
const char* password = "your-password";

HttpServer server;

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Define a simple route
    server.on("/hello", [](HttpRequest &req) {
        HttpResponse response;
        response.text("Hello, World!");
        return response;
    });
    
    // Start the server
    server.begin();
}

void loop() {
    server.tick();  // Handle incoming requests
}
```

## API Reference

### HttpServer

The main server class that handles HTTP requests.

#### Constructor

```cpp
HttpServer server;
```

#### Configuration Methods

##### `void begin()`
Start the HTTP server with current settings. Must be called after all configuration.

```cpp
server.begin();
```

#### `void begin(const HttpServerConfig &config)`
Start the HTTP server with structured configuration.

```cpp
HttpServer::HttpServerConfig config;
config.port = 8080;
config.maxRequestSize = 4096;
config.clientTimeout = 3000;
config.connectionInactivityTimeout = 60000;
config.maxConnections = 8;
config.keepAlive = true;
config.debug = true;

server.begin(config);
```

##### `void stop()`
Stop the HTTP server and close all connections.

```cpp
server.stop();
```

##### `void tick()`
Process incoming connections. **Must be called repeatedly in your main loop.**

```cpp
void loop() {
    server.tick();
}
```

##### `void setPort(uint16_t port)`
Set the server port (default: 80). Must be called **before** `begin()`.

```cpp
server.setPort(8080);
server.begin();
```

##### `void setServerName(const String &name)`
Set the server name for the Server HTTP header.

```cpp
server.setServerName("MyRobot");
```

##### `void setServerVersion(const String &version)`
Set the server version for the Server HTTP header.

```cpp
server.setServerVersion("2.0");
```

##### `void setDebug(bool debug)`
Enable or disable debug logging.

```cpp
server.setDebug(true);
```

##### `void setLogger(CachingPrinter &logger)`
Set a logger for debug output and the built-in `/log` endpoint.

```cpp
CachingPrinter logger(4096);  // 4KB buffer
server.setLogger(logger);
```

##### `void setMaxRequestSize(size_t maxSize)`
Set maximum request size in bytes (default: 8192, max: 8192).

```cpp
server.setMaxRequestSize(4096);  // 4KB max
```

##### `void setClientTimeout(uint16_t timeoutMs)`
Set client connection timeout in milliseconds (default: 5000).

```cpp
server.setClientTimeout(3000);  // 3 second timeout
```

#### Routing Methods

##### `void on(const String &path, RouteHandler handler)`
Register a route handler for a specific path (all HTTP methods).

```cpp
server.on("/api/status", [](HttpRequest &req) {
    HttpResponse response;
    response.json("{\"status\":\"ok\"}");
    return response;
});
```

##### `void on(const String &method, const String &path, RouteHandler handler)`
Register a method-specific route handler with optional path parameters.

```cpp
// Path with parameters
server.on("GET", "/api/user/:id", [](HttpRequest &req) {
    String userId = req.params["id"];
    return HttpResponse().json("{\"id\":\"" + userId + "\"}");
});

// Multiple parameters
server.on("GET", "/api/post/:postId/comment/:commentId", [](HttpRequest &req) {
    String postId = req.params["postId"];
    String commentId = req.params["commentId"];
    return HttpResponse().json("{\"post\":\"" + postId + "\",\"comment\":\"" + commentId + "\"}");
});

// Method-specific without parameters
server.on("POST", "/api/data", [](HttpRequest &req) {
    return HttpResponse().json("{\"status\":\"created\"}");
});
```

##### `void onNotFound(RouteHandler handler)`
Set a custom handler for unmatched routes (404s).

```cpp
server.onNotFound([](HttpRequest &req) {
    HttpResponse response(404);
    response.text("Page not found: " + req.path);
    return response;
});
```

##### `void onError(ErrorHandler handler)`
Set a custom error handler for generating error responses.

```cpp
server.onError([](int statusCode, const String &message) {
    HttpResponse response(statusCode);
    response.json("{\"error\":\"" + message + "\"}");
    return response;
});
```

#### Middleware

##### `void use(MiddlewareHandler middleware)`
Register legacy middleware that runs for all requests (always continues).

```cpp
server.use([](HttpRequest &req, HttpResponse &response) {
    // Log all requests
    Serial.print("Request to: ");
    Serial.println(req.path);
    
    // Add custom header to all responses
    response.setHeader("X-Powered-By", "ESP32");
});
```

##### `void use(MiddlewareHandlerBool middleware)`
Register short-circuit capable middleware that runs for all requests.

```cpp
// Authentication middleware
server.use([](HttpRequest &req, HttpResponse &response) -> bool {
    // Skip auth for public endpoints
    if (req.path.startsWith("/public/")) {
        return true;  // Continue
    }
    
    if (!req.hasHeader("Authorization")) {
        response.setStatus(401).text("Unauthorized");
        return false;  // Stop processing, send 401
    }
    
    return true;  // Continue to handler
});

// Rate limiting
server.use([](HttpRequest &req, HttpResponse &response) -> bool {
    if (isRateLimited(req)) {
        response.setStatus(429).text("Too Many Requests");
        return false;  // Stop
    }
    return true;
});
```

#### CORS Support

##### `void enableCORS(const String &origin, const String &methods, const String &headers)`
Enable Cross-Origin Resource Sharing.

```cpp
// Allow all origins
server.enableCORS();

// Specific origin
server.enableCORS("https://example.com");

// Custom configuration
server.enableCORS("*", "GET, POST", "Content-Type");
```

##### `void disableCORS()`
Disable CORS.

```cpp
server.disableCORS();
```

#### Status Methods

##### `bool isRunning() const`
Check if the server is running.

```cpp
if (server.isRunning()) {
    Serial.println("Server is active");
}
```

##### `uint16_t getPort() const`
Get the current port number.

```cpp
Serial.println(server.getPort());
```

---

### HttpRequest

Represents an incoming HTTP request.

#### Properties

```cpp
String method;                         // HTTP method (GET, POST, etc.)
String path;                          // Request path (e.g., "/api/data")
String body;                          // Request body
std::map<String, String> headers;     // HTTP headers
std::map<String, String> query;       // Query parameters
std::map<String, String> params;      // Path parameters from route patterns
```

#### Methods

##### `bool jsonRequested() const`
Check if the client requested JSON (via Accept header or ?json=true).

```cpp
if (req.jsonRequested()) {
    response.json("{\"data\":\"value\"}");
} else {
    response.html("<html>...</html>");
}
```

##### `String getHeader(const String &name, const String &defaultValue = "") const`
Get a header value (case-insensitive).

```cpp
String contentType = req.getHeader("Content-Type");
String auth = req.getHeader("Authorization", "none");
```

##### `bool hasHeader(const String &name) const`
Check if a header exists.

```cpp
if (req.hasHeader("Authorization")) {
    // Process authenticated request
}
```

##### `String getQueryParam(const String &name, const String &defaultValue = "") const`
Get a query parameter value.

```cpp
// GET /api/data?limit=10&offset=0
int limit = req.getQueryParam("limit", "20").toInt();
String filter = req.getQueryParam("filter", "all");
```

##### `bool hasQueryParam(const String &name) const`
Check if a query parameter exists.

```cpp
if (req.hasQueryParam("debug")) {
    // Enable debug mode
}
```

##### `String getContentType() const`
Get the Content-Type header.

```cpp
String contentType = req.getContentType();
```

##### `bool isContentType(const String &contentType) const`
Check if content type matches (case-insensitive).

```cpp
if (req.isContentType("application/json")) {
    // Parse JSON body
}
```

---

### HttpResponse

Represents an HTTP response to send back to the client.

#### Constructors

```cpp
HttpResponse();                                    // Status 200
HttpResponse(int statusCode);                      // Custom status
HttpResponse(int statusCode, const String &body);  // Status + body
```

#### Properties

```cpp
int status;                           // HTTP status code
String body;                         // Response body
std::map<String, String> headers;    // Response headers
```

#### Builder Methods (Chainable)

##### `HttpResponse& setStatus(int statusCode)`
Set the HTTP status code.

```cpp
response.setStatus(201);
```

##### `HttpResponse& setBody(const String &content)`
Set the response body.

```cpp
response.setBody("Success");
```

##### `HttpResponse& setHeader(const String &name, const String &value)`
Set a response header.

```cpp
response.setHeader("Cache-Control", "no-cache");
```

##### `HttpResponse& json(const String &jsonBody)`
Set JSON response with appropriate Content-Type header.

```cpp
response.json("{\"message\":\"Hello\",\"value\":42}");
```

##### `HttpResponse& html(const String &htmlBody)`
Set HTML response with appropriate Content-Type header.

```cpp
response.html("<html><body><h1>Hello</h1></body></html>");
```

##### `HttpResponse& text(const String &textBody)`
Set plain text response with appropriate Content-Type header.

```cpp
response.text("Plain text message");
```

##### `HttpResponse& cors(const String &origin = "*")`
Add CORS headers to this response.

```cpp
response.json("{\"data\":\"value\"}").cors();
```

#### Static Helper Methods

##### `static HttpResponse redirect(const String &location, bool permanent = false)`
Create a redirect response (302 or 301).

```cpp
return HttpResponse::redirect("/new-location");
return HttpResponse::redirect("/moved", true);  // 301 Permanent
```

##### `static HttpResponse error(int statusCode, const String &message)`
Create an error response.

```cpp
return HttpResponse::error(400, "Invalid input");
return HttpResponse::error(500, "Internal error");
```

#### Method Chaining Example

```cpp
return response
    .setStatus(201)
    .setHeader("Location", "/api/resource/123")
    .json("{\"id\":123,\"status\":\"created\"}")
    .cors();
```

---

## Usage Examples

### Example 1: Simple REST API

```cpp
#include <WiFi.h>
#include <http_server.h>
#include <ArduinoJson.h>

HttpServer server;

void setup() {
    Serial.begin(115200);
    
    // Connect WiFi...
    
    // GET endpoint
    server.on("/api/status", [](HttpRequest &req) {
        HttpResponse response;
        
        if (req.jsonRequested()) {
            response.json("{\"status\":\"online\",\"uptime\":" + 
                         String(millis()) + "}");
        } else {
            response.text("Status: Online");
        }
        
        return response;
    });
    
    // POST endpoint
    server.on("/api/command", [](HttpRequest &req) {
        if (req.method != "POST") {
            return HttpResponse::error(405, "Method Not Allowed");
        }
        
        if (!req.isContentType("application/json")) {
            return HttpResponse::error(400, "Expected JSON");
        }
        
        // Parse JSON body
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, req.body);
        
        if (error) {
            return HttpResponse::error(400, "Invalid JSON");
        }
        
        String command = doc["command"];
        // Execute command...
        
        HttpResponse response(200);
        response.json("{\"result\":\"success\"}");
        return response;
    });
    
    server.begin();
}

void loop() {
    server.tick();
}
```

### Example 2: Path Parameters

```cpp
// Single parameter
server.on("GET", "/api/user/:id", [](HttpRequest &req) {
    String userId = req.params["id"];
    
    String json = "{\"id\":\"" + userId + "\",\"name\":\"User " + userId + "\"}";
    return HttpResponse().json(json);
});

// Multiple parameters
server.on("GET", "/api/post/:postId/comment/:commentId", [](HttpRequest &req) {
    String postId = req.params["postId"];
    String commentId = req.params["commentId"];
    
    String json = "{";
    json += "\"postId\":\"" + postId + "\",";
    json += "\"commentId\":\"" + commentId + "\"";
    json += "}";
    
    return HttpResponse().json(json);
});

// Combine path parameters with query parameters
server.on("GET", "/api/user/:id/posts", [](HttpRequest &req) {
    String userId = req.params["id"];
    int limit = req.getQueryParam("limit", "10").toInt();
    int offset = req.getQueryParam("offset", "0").toInt();
    
    // GET /api/user/123/posts?limit=5&offset=10
    String json = "{";
    json += "\"userId\":\"" + userId + "\",";
    json += "\"limit\":" + String(limit) + ",";
    json += "\"offset\":" + String(offset);
    json += "}";
    
    return HttpResponse().json(json);
});
```

### Example 3: Query Parameters

```cpp
server.on("/api/data", [](HttpRequest &req) {
    // GET /api/data?limit=10&offset=20&sort=desc
    
    int limit = req.getQueryParam("limit", "50").toInt();
    int offset = req.getQueryParam("offset", "0").toInt();
    String sort = req.getQueryParam("sort", "asc");
    
    // Validate
    if (limit > 100) {
        return HttpResponse::error(400, "Limit too large");
    }
    
    // Build response
    String json = "{\"limit\":" + String(limit) + 
                  ",\"offset\":" + String(offset) + 
                  ",\"sort\":\"" + sort + "\"}";
    
    HttpResponse response;
    response.json(json);
    return response;
});
```

### Example 4: Headers and Authentication

```cpp
server.on("/api/secure", [](HttpRequest &req) {
    // Check authorization header
    String auth = req.getHeader("Authorization");
    
    if (auth != "Bearer secret-token") {
        HttpResponse response(401);
        response.text("Unauthorized");
        response.setHeader("WWW-Authenticate", "Bearer");
        return response;
    }
    
    // Authorized request
    HttpResponse response;
    response.json("{\"secure\":\"data\"}");
    return response;
});
```

### Example 5: File-like Responses

```cpp
server.on("/api/download", [](HttpRequest &req) {
    String data = "This is some data to download";
    
    HttpResponse response;
    response.setBody(data);
    response.setHeader("Content-Type", "application/octet-stream");
    response.setHeader("Content-Disposition", 
                      "attachment; filename=\"data.txt\"");
    return response;
});
```

### Example 6: Middleware for Logging

```cpp
void setup() {
    // Add request logging middleware
    server.use([](HttpRequest &req, HttpResponse &response) {
        Serial.print("[");
        Serial.print(millis());
        Serial.print("] ");
        Serial.print(req.method);
        Serial.print(" ");
        Serial.println(req.path);
    });
    
    // Add response timing
    unsigned long startTime = 0;
    server.use([&startTime](HttpRequest &req, HttpResponse &response) {
        startTime = millis();
    });
    
    // Routes...
    
    server.begin();
}
```

### Example 7: Short-Circuit Middleware

```cpp
void setup() {
    // Authentication middleware that can stop processing
    server.use([](HttpRequest &req, HttpResponse &response) -> bool {
        // Skip auth for public routes
        if (req.path.startsWith("/public")) {
            return true;  // Continue
        }
        
        // Check auth header
        String auth = req.getHeader("Authorization");
        if (auth.isEmpty()) {
            response.setStatus(401).text("Unauthorized");
            return false;  // Stop - send 401 immediately
        }
        
        // Validate token (simplified)
        if (auth != "Bearer secret-token") {
            response.setStatus(403).text("Forbidden");
            return false;  // Stop - send 403
        }
        
        return true;  // Continue to handler
    });
    
    // Rate limiting middleware
    unsigned long lastRequestTime = 0;
    const unsigned long rateLimit = 1000;  // 1 request per second
    
    server.use([&](HttpRequest &req, HttpResponse &response) -> bool {
        unsigned long now = millis();
        if (now - lastRequestTime < rateLimit) {
            response.setStatus(429).text("Too Many Requests");
            return false;  // Stop
        }
        lastRequestTime = now;
        return true;  // Continue
    });
    
    server.begin();
}
```

### Example 8: CORS for Web Apps

```cpp
void setup() {
    // Enable CORS for all routes
    server.enableCORS("*", "GET, POST, PUT, DELETE", "Content-Type");
    
    // Or enable per-route
    server.on("/api/public", [](HttpRequest &req) {
        HttpResponse response;
        response.json("{\"public\":\"data\"}").cors();
        return response;
    });
    
    server.begin();
}
```

### Example 9: Custom 404 Handler

```cpp
void setup() {
    server.onNotFound([](HttpRequest &req) {
        HttpResponse response(404);
        
        if (req.jsonRequested()) {
            response.json("{\"error\":\"Not Found\",\"path\":\"" + 
                         req.path + "\"}");
        } else {
            String html = "<html><body>";
            html += "<h1>404 - Not Found</h1>";
            html += "<p>Path: " + req.path + "</p>";
            html += "</body></html>";
            response.html(html);
        }
        
        return response;
    });
    
    server.begin();
}
```

### Example 10: Custom Error Handler

```cpp
void setup() {
    server.onError([](int statusCode, const String &message) {
        HttpResponse response(statusCode);
        
        // Always return JSON errors
        String json = "{\"error\":{";
        json += "\"code\":" + String(statusCode) + ",";
        json += "\"message\":\"" + message + "\"";
        json += "}}";
        
        response.json(json);
        return response;
    });
    
    server.begin();
}
```

### Example 11: Built-in Log Endpoint

```cpp
CachingPrinter logger(4096);  // 4KB log buffer

void setup() {
    Serial.begin(115200);
    
    // Set logger
    server.setLogger(logger);
    server.setDebug(true);
    
    // Now you can access logs at http://your-ip/log
    // Or with custom line count: http://your-ip/log?lines=50
    
    logger.println("Server starting...");
    server.begin();
    logger.println("Server started");
}
```

### Example 12: Redirect Responses

```cpp
server.on("/old-path", [](HttpRequest &req) {
    return HttpResponse::redirect("/new-path", true);  // 301
});

server.on("/temp-redirect", [](HttpRequest &req) {
    return HttpResponse::redirect("/other-page");  // 302
});
```

---

## Advanced Features

### Structured Configuration

Use `HttpServerConfig` for cleaner, more organized server setup:

```cpp
void setup() {
    HttpServer::HttpServerConfig config;
    
    // Network settings
    config.port = 8080;
    
    // Resource limits
    config.maxRequestSize = 4096;        // 4KB max
    config.maxConnections = 8;            // 8 concurrent connections
    
    // Timeouts
    config.clientTimeout = 3000;          // 3 second client timeout
    config.connectionInactivityTimeout = 60000;  // 1 minute inactivity
    
    // Connection behavior
    config.keepAlive = true;              // Enable persistent connections
    
    // Debugging
    config.debug = true;                  // Enable debug logging
    
    server.begin(config);
}
```

### Connection Management

The server provides fine-grained control over connections:

```cpp
// Set maximum concurrent connections
server.setMaxConnections(8);

// Enable keep-alive for persistent connections
server.setKeepAlive(true);

// Set inactivity timeout (connections idle longer than this are closed)
server.setConnectionInactivityTimeout(60000);  // 1 minute

// Get current settings
Serial.print("Max connections: ");
Serial.println(server.getMaxConnections());
Serial.print("Keep-alive: ");
Serial.println(server.getKeepAlive() ? "enabled" : "disabled");
```

### Default Headers

Add headers that will be automatically included in all responses:

```cpp
void setup() {
    // Add default headers
    server.addDefaultHeader("X-Powered-By", "ESP32");
    server.addDefaultHeader("X-API-Version", "1.0");
    server.addDefaultHeader("Cache-Control", "no-cache");
    
    // These headers will be in every response unless overridden
    server.begin();
}

// Remove a default header
server.removeDefaultHeader("X-Powered-By");

// Clear all default headers
server.clearDefaultHeaders();
```

### Before-Send Hook

Execute custom logic just before sending any response:

```cpp
void setup() {
    // Add response timing
    server.onBeforeSend([](HttpRequest &req, HttpResponse &response) {
        // Add timestamp
        response.setHeader("X-Response-Time", String(millis()));
        
        // Log response
        Serial.print("Sending ");
        Serial.print(response.status);
        Serial.print(" for ");
        Serial.println(req.path);
    });
    
    server.begin();
}
```

### Memory Management

The server implements several memory safety features:

- **Automatic memory checks** - Rejects requests when free RAM < 4KB
- **Request size limits** - Configurable max request size (default 8KB)
- **Bounded buffers** - All buffers have hard limits
- **Chunked writing** - Large responses sent in small chunks
- **Timeouts** - Client and write timeouts prevent hangs

```cpp
// Configure memory limits
server.setMaxRequestSize(4096);   // 4KB max request
server.setClientTimeout(3000);     // 3 second timeout

// Check server status
if (server.isRunning()) {
    Serial.print("Free RAM: ");
    Serial.println(ESP.getFreeHeap());
}
```

### Request Size Limits

```cpp
// Set smaller limit for constrained devices
server.setMaxRequestSize(2048);  // 2KB

// Requests larger than this will get 413 Payload Too Large
```

### Connection Timeouts

```cpp
// Client timeout (read/write operations)
server.setClientTimeout(2000);  // 2 seconds

// Longer timeout for slow networks
server.setClientTimeout(10000);  // 10 seconds

// Inactivity timeout (idle connections)
server.setConnectionInactivityTimeout(120000);  // 2 minutes
```

### Built-in Endpoints

The server provides useful built-in endpoints that can be overridden:

#### Root Endpoint (`/`)

By default, returns a simple HTML welcome page showing the server name and version:

```cpp
// Override the default root handler
server.on("/", [](HttpRequest &req) {
    return HttpResponse().html("<h1>My Custom Home Page</h1>");
});
```

#### Log Endpoint (`/log`)

Automatically available when a logger is configured. Returns recent log entries as plain text:

```cpp
// Configure logger
CachingPrinter logger(4096);
server.setLogger(logger);

// Log endpoint is now available at: http://your-ip/log
// Query parameters:
// - ?lines=N - Number of lines to return (default: 20)

// Examples:
// http://your-ip/log           -> Last 20 lines
// http://your-ip/log?lines=50  -> Last 50 lines

// Override if you want custom log behavior
server.on("/log", [](HttpRequest &req) {
    // Custom log implementation
});
```

### Debug Logging

```cpp
CachingPrinter logger(8192);  // 8KB buffer

void setup() {
    server.setLogger(logger);
    server.setDebug(true);  // Enable detailed logging
    
    // All requests and responses will be logged
    server.begin();
}

// Access logs at http://your-ip/log
```

---

## Best Practices

### 1. Always Call tick() in loop()

```cpp
void loop() {
    server.tick();  // REQUIRED - processes incoming connections
    // Other non-blocking code...
}
```

### 2. Keep Handlers Fast

Route handlers should complete quickly to avoid blocking other requests.

```cpp
// ❌ BAD - Blocks for 5 seconds
server.on("/slow", [](HttpRequest &req) {
    delay(5000);
    return HttpResponse().text("Done");
});

// ✅ GOOD - Quick response
server.on("/fast", [](HttpRequest &req) {
    return HttpResponse().text("Done");
});
```

### 3. Validate Input

Always validate query parameters, headers, and body content.

```cpp
server.on("/api/set", [](HttpRequest &req) {
    if (!req.hasQueryParam("value")) {
        return HttpResponse::error(400, "Missing 'value' parameter");
    }
    
    int value = req.getQueryParam("value").toInt();
    if (value < 0 || value > 100) {
        return HttpResponse::error(400, "Value out of range");
    }
    
    // Process valid input...
    return HttpResponse().json("{\"status\":\"ok\"}");
});
```

### 4. Set Appropriate Content-Type

Use the helper methods for automatic Content-Type headers.

```cpp
response.json("{...}");  // Sets application/json
response.html("<html>");  // Sets text/html; charset=utf-8
response.text("Hello");   // Sets text/plain; charset=utf-8
```

### 5. Use Method-Specific Routes

Prefer method-specific route registration for cleaner, more RESTful APIs:

```cpp
// ✅ GOOD - Clear, specific
server.on("GET", "/api/user/:id", getUser);
server.on("PUT", "/api/user/:id", updateUser);
server.on("DELETE", "/api/user/:id", deleteUser);

// ❌ OKAY but less clear
server.on("/api/user", [](HttpRequest &req) {
    if (req.method == "GET") return getUser(req);
    if (req.method == "POST") return createUser(req);
    return HttpResponse::error(405, "Method Not Allowed");
});
```

### 6. Validate Path Parameters

Always validate path parameters before using them:

```cpp
server.on("GET", "/api/user/:id", [](HttpRequest &req) {
    String userId = req.params["id"];
    
    // Validate
    if (userId.isEmpty()) {
        return HttpResponse::error(400, "Missing user ID");
    }
    
    int id = userId.toInt();
    if (id <= 0) {
        return HttpResponse::error(400, "Invalid user ID");
    }
    
    // Process valid ID...
});
```

### 7. Use CORS for Web Access

Enable CORS if accessing from web browsers.

```cpp
server.enableCORS();  // Allow all origins
```

### 8. Configure Before begin()

Set all configuration before starting the server.

```cpp
server.setPort(8080);
server.setServerName("MyDevice");
server.setDebug(true);
// ... all routes ...
server.begin();  // Start last
```

### 9. Use Short-Circuit Middleware for Guards

Use boolean-returning middleware for authentication, rate limiting, and other guard logic:

```cpp
// ✅ GOOD - Short-circuit on auth failure
server.use([](HttpRequest &req, HttpResponse &response) -> bool {
    if (needsAuth(req) && !isAuthenticated(req)) {
        response.setStatus(401).text("Unauthorized");
        return false;  // Stop processing
    }
    return true;
});

// ❌ BAD - Can't stop processing, handler still runs
server.use([](HttpRequest &req, HttpResponse &response) {
    if (needsAuth(req) && !isAuthenticated(req)) {
        response.setStatus(401).text("Unauthorized");
        // Handler still executes!
    }
});
```

### 10. Monitor Memory

On ESP32, monitor free heap to prevent crashes.

```cpp
server.use([](HttpRequest &req, HttpResponse &response) {
    if (ESP.getFreeHeap() < 10000) {  // Less than 10KB
        Serial.println("WARNING: Low memory!");
    }
});
```

---

## Troubleshooting

### Server Not Responding

**Problem**: Can't connect to the server

**Solutions**:
1. Check WiFi connection: `WiFi.status() == WL_CONNECTED`
2. Verify IP address: `Serial.println(WiFi.localIP())`
3. Ensure `server.begin()` was called
4. Check firewall/network settings
5. Verify port isn't blocked

```cpp
void setup() {
    // ... WiFi setup ...
    
    Serial.print("Server IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Server Port: ");
    Serial.println(server.getPort());
    
    server.begin();
    
    if (server.isRunning()) {
        Serial.println("Server started successfully");
    }
}
```

### 503 Service Unavailable

**Problem**: Server returns 503 errors

**Solution**: Not enough free RAM. The server requires at least 4KB free.

```cpp
// Check available memory
Serial.print("Free heap: ");
Serial.println(ESP.getFreeHeap());

// Reduce memory usage:
// - Decrease request size limit
// - Reduce logger buffer size
// - Optimize your code
```

### 413 Payload Too Large

**Problem**: Large requests rejected

**Solution**: Increase max request size (up to 8KB).

```cpp
server.setMaxRequestSize(8192);  // Maximum allowed
```

### Requests Timing Out

**Problem**: Slow clients disconnecting

**Solution**: Increase client timeout.

```cpp
server.setClientTimeout(10000);  // 10 seconds
```

### Missing Route

**Problem**: 404 errors for registered routes

**Solution**: Ensure paths match exactly (case-sensitive, no trailing slash).

```cpp
// These are DIFFERENT routes:
server.on("/api/data", handler1);   // Matches /api/data
server.on("/api/data/", handler2);  // Trailing slash removed -> same as above
server.on("/API/data", handler3);   // Case-sensitive - different!
```

### Memory Leaks

**Problem**: Device crashes after running for a while

**Solutions**:
1. Avoid String concatenation in loops
2. Don't store large objects in global scope
3. Use `const char*` for constants
4. Monitor heap with `ESP.getFreeHeap()`

```cpp
// ❌ BAD - Creates many String objects
String buildResponse() {
    String result = "";
    for (int i = 0; i < 100; i++) {
        result += String(i) + ",";  // Memory hungry!
    }
    return result;
}

// ✅ BETTER - Pre-allocate
String buildResponse() {
    String result;
    result.reserve(500);  // Reserve space
    for (int i = 0; i < 100; i++) {
        result += String(i) + ",";
    }
    return result;
}
```

### CORS Issues

**Problem**: Browser can't access API

**Solution**: Enable CORS.

```cpp
server.enableCORS();  // Allow all origins

// Or specific origin
server.enableCORS("https://myapp.com");
```

### Can't Change Port

**Problem**: `setPort()` has no effect

**Solution**: Must be called **before** `begin()`.

```cpp
server.setPort(8080);  // BEFORE begin()
server.begin();        // AFTER setPort()
```

---

## Performance Considerations

### ESP32-S3 Optimizations

1. **Use Core Affinity**: Run server on specific core
2. **Adjust Task Priority**: Balance with other tasks
3. **Monitor Stack**: Ensure enough stack space
4. **Optimize Handlers**: Keep processing minimal

```cpp
// Example: Dedicated server task
void serverTask(void *parameter) {
    while (true) {
        server.tick();
        vTaskDelay(1);  // Yield to other tasks
    }
}

void setup() {
    // ... setup code ...
    
    xTaskCreatePinnedToCore(
        serverTask,    // Task function
        "HttpServer",  // Task name
        8192,          // Stack size
        NULL,          // Parameters
        1,             // Priority
        NULL,          // Task handle
        0              // Core (0 or 1)
    );
}
```

### Reducing Memory Usage

```cpp
// Smaller request buffer
server.setMaxRequestSize(2048);

// Shorter timeouts
server.setClientTimeout(2000);

// Smaller log buffer
CachingPrinter logger(2048);

// Disable debug in production
server.setDebug(false);
```

---

## Additional Resources

- **ESP32 Documentation**: https://docs.espressif.com/
- **Arduino Framework**: https://www.arduino.cc/reference/
- **PlatformIO**: https://platformio.org/
- **HTTP/1.1 Spec**: https://www.rfc-editor.org/rfc/rfc2616

---

## License

See the LICENSE file in the repository root.

## Contributing

Contributions welcome! Please submit issues and pull requests on GitHub.
