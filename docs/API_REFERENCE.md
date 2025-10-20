# API Reference

Complete API reference for the HTTP Server Library.

## Table of Contents

- [Type Definitions](#type-definitions)
- [HttpServer Class](#httpserver-class)
- [HttpRequest Class](#httprequest-class)
- [HttpResponse Class](#httpresponse-class)
- [Utility Functions](#utility-functions)
- [Constants](#constants)

---

## Type Definitions

### RouteHandler

```cpp
typedef std::function<HttpResponse(HttpRequest &)> RouteHandler;
```

A function that handles HTTP requests for a specific route.

**Parameters:**
- `HttpRequest &` - The incoming request

**Returns:**
- `HttpResponse` - The response to send back

**Example:**
```cpp
RouteHandler myHandler = [](HttpRequest &req) {
    HttpResponse response;
    response.text("Hello!");
    return response;
};

server.on("/hello", myHandler);
```

---

### MiddlewareHandler

```cpp
typedef std::function<void(HttpRequest &, HttpResponse &)> MiddlewareHandler;
```

A function that processes all requests/responses (middleware).

**Parameters:**
- `HttpRequest &` - The incoming request
- `HttpResponse &` - The response (can be modified)

**Returns:**
- `void`

**Example:**
```cpp
MiddlewareHandler logger = [](HttpRequest &req, HttpResponse &response) {
    Serial.println("Request: " + req.method + " " + req.path);
};

server.use(logger);
```

---

### ErrorHandler

```cpp
typedef std::function<HttpResponse(int, const String &)> ErrorHandler;
```

A function that generates error responses.

**Parameters:**
- `int` - HTTP status code
- `const String &` - Error message

**Returns:**
- `HttpResponse` - The error response

**Example:**
```cpp
ErrorHandler customErrors = [](int code, const String &msg) {
    HttpResponse response(code);
    response.json("{\"error\":\"" + msg + "\"}");
    return response;
};

server.onError(customErrors);
```

---

## HttpServer Class

Main HTTP server class.

### Constructor

#### `HttpServer()`

Creates a new HTTP server instance.

```cpp
HttpServer server;
```

---

### Configuration Methods

#### `void begin()`

Start the HTTP server. Must be called after all configuration.

**Example:**
```cpp
server.setPort(8080);
server.on("/hello", handler);
server.begin();  // Start server
```

---

#### `void stop()`

Stop the HTTP server and close all connections.

**Example:**
```cpp
server.stop();
```

---

#### `void tick()`

Process incoming connections. **Must be called repeatedly in your main loop.**

**Example:**
```cpp
void loop() {
    server.tick();  // Handle requests
}
```

---

#### `void setPort(uint16_t port)`

Set the server port (default: 80). **Must be called before** `begin()`.

**Parameters:**
- `port` - Port number (1-65535)

**Example:**
```cpp
server.setPort(8080);
server.begin();  // Must be after setPort()
```

---

#### `void setServerName(const String &serverName)`

Set the server name for the Server HTTP header.

**Parameters:**
- `serverName` - Server name

**Default:** `"HubServer"`

**Example:**
```cpp
server.setServerName("MyRobot");
```

---

#### `void setServerVersion(const String &serverVersion)`

Set the server version for the Server HTTP header.

**Parameters:**
- `serverVersion` - Version string

**Default:** `"1.0"`

**Example:**
```cpp
server.setServerVersion("2.1.0");
```

---

#### `void setDebug(bool debug)`

Enable or disable debug logging.

**Parameters:**
- `debug` - `true` to enable, `false` to disable

**Default:** `false`

**Example:**
```cpp
server.setDebug(true);
```

---

#### `void setLogger(CachingPrinter &logger)`

Set a logger for debug output and the built-in `/log` endpoint.

**Parameters:**
- `logger` - Reference to CachingPrinter instance

**Example:**
```cpp
CachingPrinter logger(4096);
server.setLogger(logger);
server.setDebug(true);  // Enable logging
```

---

#### `void setMaxRequestSize(size_t maxSize)`

Set maximum request size in bytes.

**Parameters:**
- `maxSize` - Maximum size (2048-8192 bytes)

**Default:** `8192`

**Example:**
```cpp
server.setMaxRequestSize(4096);  // 4KB max
```

---

#### `void setClientTimeout(uint16_t timeoutMs)`

Set client connection timeout in milliseconds.

**Parameters:**
- `timeoutMs` - Timeout in milliseconds

**Default:** `5000` (5 seconds)

**Example:**
```cpp
server.setClientTimeout(3000);  // 3 seconds
```

---

### Routing Methods

#### `void on(const String &path, RouteHandler handler)`

Register a route handler for a specific path.

**Parameters:**
- `path` - URL path (e.g., "/api/status")
- `handler` - Function to handle requests

**Example:**
```cpp
server.on("/hello", [](HttpRequest &req) {
    return HttpResponse().text("Hello!");
});
```

**Note:** Paths are case-sensitive. Trailing slashes are removed.

---

#### `void onNotFound(RouteHandler handler)`

Set a custom handler for unmatched routes (404s).

**Parameters:**
- `handler` - Function to handle 404 errors

**Default:** Returns `404 Not Found`

**Example:**
```cpp
server.onNotFound([](HttpRequest &req) {
    HttpResponse response(404);
    response.text("Page not found: " + req.path);
    return response;
});
```

---

#### `void onError(ErrorHandler handler)`

Set a custom error handler for generating error responses.

**Parameters:**
- `handler` - Function to generate error responses

**Default:** Returns plain text error

**Example:**
```cpp
server.onError([](int code, const String &msg) {
    HttpResponse response(code);
    response.json("{\"error\":\"" + msg + "\"}");
    return response;
});
```

---

### Middleware

#### `void use(MiddlewareHandler middleware)`

Register middleware that runs for all requests.

**Parameters:**
- `middleware` - Function to process requests/responses

**Example:**
```cpp
// Logging middleware
server.use([](HttpRequest &req, HttpResponse &response) {
    Serial.print(req.method);
    Serial.print(" ");
    Serial.println(req.path);
});

// Add custom header to all responses
server.use([](HttpRequest &req, HttpResponse &response) {
    response.setHeader("X-Custom", "value");
});
```

**Note:** Middlewares execute in registration order.

---

### CORS Support

#### `void enableCORS(const String &origin = "*", const String &methods = "GET, POST, PUT, DELETE, OPTIONS", const String &headers = "Content-Type, Authorization")`

Enable Cross-Origin Resource Sharing.

**Parameters:**
- `origin` - Allowed origin (default: "*")
- `methods` - Allowed methods (default: "GET, POST, PUT, DELETE, OPTIONS")
- `headers` - Allowed headers (default: "Content-Type, Authorization")

**Example:**
```cpp
// Allow all origins
server.enableCORS();

// Specific origin
server.enableCORS("https://example.com");

// Custom configuration
server.enableCORS("*", "GET, POST", "Content-Type");
```

**Effect:**
- Adds CORS headers to all responses
- Automatically handles OPTIONS preflight requests

---

#### `void disableCORS()`

Disable CORS.

**Example:**
```cpp
server.disableCORS();
```

---

### Status Methods

#### `bool isRunning() const`

Check if the server is running.

**Returns:**
- `true` if running, `false` otherwise

**Example:**
```cpp
if (server.isRunning()) {
    Serial.println("Server is active");
}
```

---

#### `uint16_t getPort() const`

Get the current port number.

**Returns:**
- Port number

**Example:**
```cpp
Serial.print("Port: ");
Serial.println(server.getPort());
```

---

## HttpRequest Class

Represents an incoming HTTP request.

### Properties

#### `String method`

HTTP method (e.g., "GET", "POST", "PUT", "DELETE").

**Example:**
```cpp
if (req.method == "POST") {
    // Handle POST request
}
```

---

#### `String path`

Request path without query string (e.g., "/api/data").

**Example:**
```cpp
if (req.path == "/api/status") {
    // Handle status request
}
```

**Note:** Trailing slashes are removed (except for root "/").

---

#### `String body`

Request body content.

**Example:**
```cpp
if (req.body.length() > 0) {
    Serial.println("Body: " + req.body);
}
```

---

#### `std::map<String, String> headers`

HTTP headers (name -> value).

**Example:**
```cpp
for (auto &header : req.headers) {
    Serial.print(header.first);
    Serial.print(": ");
    Serial.println(header.second);
}
```

---

#### `std::map<String, String> query`

Query parameters (name -> value).

**Example:**
```cpp
// GET /api/data?limit=10&offset=0
String limit = req.query["limit"];    // "10"
String offset = req.query["offset"];  // "0"
```

---

### Methods

#### `bool jsonRequested() const`

Check if the client requested JSON response.

**Returns:**
- `true` if JSON requested

**Checks:**
- Accept header contains "json"
- Query parameter `?json=true`

**Example:**
```cpp
if (req.jsonRequested()) {
    response.json("{\"data\":\"value\"}");
} else {
    response.html("<html>...</html>");
}
```

---

#### `String getHeader(const String &name, const String &defaultValue = "") const`

Get a header value (case-insensitive).

**Parameters:**
- `name` - Header name
- `defaultValue` - Default if not found (default: "")

**Returns:**
- Header value or default

**Example:**
```cpp
String contentType = req.getHeader("Content-Type");
String auth = req.getHeader("Authorization", "none");
```

---

#### `bool hasHeader(const String &name) const`

Check if a header exists (case-insensitive).

**Parameters:**
- `name` - Header name

**Returns:**
- `true` if header exists

**Example:**
```cpp
if (req.hasHeader("Authorization")) {
    // Process authenticated request
}
```

---

#### `String getQueryParam(const String &name, const String &defaultValue = "") const`

Get a query parameter value.

**Parameters:**
- `name` - Parameter name
- `defaultValue` - Default if not found (default: "")

**Returns:**
- Parameter value or default

**Example:**
```cpp
int limit = req.getQueryParam("limit", "20").toInt();
String filter = req.getQueryParam("filter", "all");
```

---

#### `bool hasQueryParam(const String &name) const`

Check if a query parameter exists.

**Parameters:**
- `name` - Parameter name

**Returns:**
- `true` if parameter exists

**Example:**
```cpp
if (req.hasQueryParam("debug")) {
    // Enable debug mode
}
```

---

#### `String getContentType() const`

Get the Content-Type header.

**Returns:**
- Content-Type value or empty string

**Example:**
```cpp
String contentType = req.getContentType();
```

---

#### `bool isContentType(const String &contentType) const`

Check if content type matches (case-insensitive, substring match).

**Parameters:**
- `contentType` - Content type to check

**Returns:**
- `true` if matches

**Example:**
```cpp
if (req.isContentType("application/json")) {
    // Parse JSON body
}

if (req.isContentType("multipart/form-data")) {
    // Handle form upload
}
```

---

## HttpResponse Class

Represents an HTTP response.

### Constructors

#### `HttpResponse()`

Create a response with status 200.

```cpp
HttpResponse response;  // Status: 200
```

---

#### `HttpResponse(int statusCode)`

Create a response with a specific status code.

**Parameters:**
- `statusCode` - HTTP status code

```cpp
HttpResponse response(404);  // Not Found
```

---

#### `HttpResponse(int statusCode, const String &body)`

Create a response with status code and body.

**Parameters:**
- `statusCode` - HTTP status code
- `body` - Response body

```cpp
HttpResponse response(200, "Success");
```

---

### Properties

#### `int status`

HTTP status code.

**Example:**
```cpp
response.status = 201;  // Created
```

---

#### `String body`

Response body content.

**Example:**
```cpp
response.body = "Hello, World!";
```

---

#### `std::map<String, String> headers`

Response headers (name -> value).

**Example:**
```cpp
response.headers["Cache-Control"] = "no-cache";
```

---

### Builder Methods

All builder methods return `HttpResponse&` for method chaining.

#### `HttpResponse& setStatus(int statusCode)`

Set the HTTP status code.

**Parameters:**
- `statusCode` - HTTP status code

**Returns:**
- Reference to this response (for chaining)

**Example:**
```cpp
response.setStatus(201);
```

---

#### `HttpResponse& setBody(const String &content)`

Set the response body.

**Parameters:**
- `content` - Body content

**Returns:**
- Reference to this response (for chaining)

**Example:**
```cpp
response.setBody("Success");
```

---

#### `HttpResponse& setHeader(const String &name, const String &value)`

Set a response header.

**Parameters:**
- `name` - Header name
- `value` - Header value

**Returns:**
- Reference to this response (for chaining)

**Example:**
```cpp
response.setHeader("Cache-Control", "no-cache");
response.setHeader("X-Custom-Header", "value");
```

---

#### `HttpResponse& json(const String &jsonBody)`

Set JSON response with `Content-Type: application/json` header.

**Parameters:**
- `jsonBody` - JSON string

**Returns:**
- Reference to this response (for chaining)

**Example:**
```cpp
response.json("{\"status\":\"ok\",\"value\":42}");
```

---

#### `HttpResponse& html(const String &htmlBody)`

Set HTML response with `Content-Type: text/html; charset=utf-8` header.

**Parameters:**
- `htmlBody` - HTML string

**Returns:**
- Reference to this response (for chaining)

**Example:**
```cpp
response.html("<html><body><h1>Hello</h1></body></html>");
```

---

#### `HttpResponse& text(const String &textBody)`

Set plain text response with `Content-Type: text/plain; charset=utf-8` header.

**Parameters:**
- `textBody` - Plain text string

**Returns:**
- Reference to this response (for chaining)

**Example:**
```cpp
response.text("Plain text message");
```

---

#### `HttpResponse& cors(const String &origin = "*")`

Add CORS headers to this response.

**Parameters:**
- `origin` - Allowed origin (default: "*")

**Returns:**
- Reference to this response (for chaining)

**Headers Added:**
- `Access-Control-Allow-Origin`
- `Access-Control-Allow-Methods`
- `Access-Control-Allow-Headers`

**Example:**
```cpp
response.json("{\"data\":\"value\"}").cors();
response.json("{\"data\":\"value\"}").cors("https://example.com");
```

---

### Static Helper Methods

#### `static HttpResponse redirect(const String &location, bool permanent = false)`

Create a redirect response.

**Parameters:**
- `location` - URL to redirect to
- `permanent` - Use 301 instead of 302 (default: false)

**Returns:**
- Redirect response (302 or 301)

**Example:**
```cpp
return HttpResponse::redirect("/new-location");
return HttpResponse::redirect("/moved", true);  // 301 Permanent
```

---

#### `static HttpResponse error(int statusCode, const String &message)`

Create an error response with plain text body.

**Parameters:**
- `statusCode` - HTTP status code
- `message` - Error message

**Returns:**
- Error response

**Example:**
```cpp
return HttpResponse::error(400, "Invalid input");
return HttpResponse::error(500, "Internal error");
```

---

### Method Chaining

All builder methods support chaining.

**Example:**
```cpp
return response
    .setStatus(201)
    .setHeader("Location", "/api/resource/123")
    .setHeader("X-Request-ID", "abc123")
    .json("{\"id\":123,\"status\":\"created\"}")
    .cors();
```

---

## Utility Functions

### `int wifiScan(Print* printer = nullptr)`

Scan for available WiFi networks and print results.

**Parameters:**
- `printer` - Print destination (default: `&Serial`)

**Returns:**
- Number of networks found

**Example:**
```cpp
void setup() {
    Serial.begin(115200);
    
    int numNetworks = wifiScan(&Serial);
    Serial.print("Found ");
    Serial.print(numNetworks);
    Serial.println(" networks");
}
```

**Output:**
```
Scanning Wifi Networks...3 Networks Found
0. MyNetwork	Signal: -45 dBm
1. NeighborWiFi	Signal: -67 dBm
2. GuestNetwork	Signal: -82 dBm
```

---

## Constants

### Server Constants

```cpp
static const size_t DEFAULT_BUFFER_SIZE = 2048;   // Default buffer size
static const size_t MAX_BUFFER_SIZE = 8192;       // Maximum buffer size
static const size_t MIN_FREE_RAM = 4096;          // Minimum free RAM required
static const uint16_t CLIENT_TIMEOUT_MS = 5000;   // Client timeout (ms)
static const uint16_t WRITE_TIMEOUT_MS = 1000;    // Write timeout (ms)
static const size_t WRITE_CHUNK_SIZE = 512;       // Write chunk size
static const size_t MAX_HEADERS = 16;             // Maximum headers to parse
```

**Access:**
```cpp
Serial.println(HttpServer::DEFAULT_BUFFER_SIZE);
```

---

## HTTP Status Codes

### Success (2xx)

- `200` - OK
- `201` - Created
- `204` - No Content

### Redirection (3xx)

- `301` - Moved Permanently
- `302` - Found (Temporary Redirect)
- `304` - Not Modified

### Client Error (4xx)

- `400` - Bad Request
- `401` - Unauthorized
- `403` - Forbidden
- `404` - Not Found
- `405` - Method Not Allowed
- `413` - Payload Too Large

### Server Error (5xx)

- `500` - Internal Server Error
- `501` - Not Implemented
- `503` - Service Unavailable

---

## Complete Example

```cpp
#include <WiFi.h>
#include <http_server.h>

HttpServer server;
CachingPrinter logger(4096);

void setup() {
    Serial.begin(115200);
    
    // WiFi setup
    WiFi.begin("SSID", "password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    // Server configuration
    server.setPort(80);
    server.setServerName("MyDevice");
    server.setServerVersion("1.0.0");
    server.setLogger(logger);
    server.setDebug(true);
    server.enableCORS();
    
    // Middleware
    server.use([](HttpRequest &req, HttpResponse &response) {
        response.setHeader("X-Powered-By", "ESP32");
    });
    
    // Routes
    server.on("/", [](HttpRequest &req) {
        return HttpResponse().html("<h1>Hello!</h1>");
    });
    
    server.on("/api/status", [](HttpRequest &req) {
        String json = "{\"status\":\"ok\",\"uptime\":" + String(millis()) + "}";
        return HttpResponse().json(json);
    });
    
    // Custom 404
    server.onNotFound([](HttpRequest &req) {
        return HttpResponse::error(404, "Not found: " + req.path);
    });
    
    // Start server
    server.begin();
    Serial.println("Server started!");
}

void loop() {
    server.tick();
}
```

---

## See Also

- [Usage Guide](USAGE_GUIDE.md) - Comprehensive usage guide with examples
- [README](../README.md) - Project overview

---

**Last Updated:** October 2025
