
# Hub Robot HTTP Utils

**Hub Robot HTTP Utils** provides lightweight, robust HTTP server and client libraries for ESP32-based robots and embedded projects. It enables your device to serve HTTP requests, define custom API endpoints, make HTTP requests to external services, and interact with web clients or other devices over WiFi.

## Features

### HTTP Server
- **Route-based request handling**: Easily define handlers for specific URL paths (e.g., `/api/status`).
- **Middleware support**: Add custom logic (logging, authentication, etc.) that runs for every request.
- **Built-in CORS support**: Enable cross-origin requests with a single call.
- **Custom error handling**: Define your own error and 404 responses.
- **Request/response utilities**: Access headers, query parameters, and content types with convenience methods.
- **Configurable limits and timeouts**: Control buffer sizes, request body limits, and client timeouts for reliability.
- **Memory-safe**: Designed for microcontrollers with careful bounds checking and low memory usage.
- **Request logging**: Optional debug logging for development and troubleshooting.
- **PicoHTTPParser integration**: Fast, minimal HTTP parsing using [PicoHTTPParser](https://github.com/h2o/picohttpparser).

### HTTP Client
- **Support for standard HTTP methods**: GET, POST, PUT, DELETE, PATCH, HEAD, and custom methods
- **Synchronous & Asynchronous support**: Requests can be blocking or non-blocking (with callbacks)
- **Persistent headers**: Set headers that automatically apply to all subsequent requests (e.g., Authorization tokens)
- **Automatic cookie management**: Handles `Set-Cookie` responses, returning them in subsequent requests

## Example Usage

### HTTP Server
See the [Quick Start Guide](docs/QUICKSTART.md) for a step-by-step server example.

```cpp
#include <WiFi.h>
#include <http_server.h>

HttpServer server;

void setup() {
	// ... WiFi setup ...
	server.on("/", [](HttpRequest &req) {
		HttpResponse res;
		res.html("<h1>Hello from ESP32!</h1>");
		return res;
	});
	server.begin();
}

void loop() {
	server.tick();
}
```

### HTTP Client
```cpp
#include <WiFi.h>
#include <http_client.h>

HubHttpClient httpClient;

void setup() {
	// ... WiFi setup ...
	
	// Configure client
	httpClient.setTimeout(15000);
	httpClient.setPersistentHeader("Authorization", "Bearer your-token");
	
	// Synchronous request (blocks until complete)
	HubHttpClientResponse response = httpClient.GET("http://api.example.com/data");
	if (response.isSuccess) {
		Serial.println("Response: " + response.body);
	}
	
	// Asynchronous request (non-blocking with callback)
	bool started = httpClient.GET("http://api.example.com/status", [](const HubHttpClientResponse& response) {
		Serial.println("Async status: " + String(response.statusCode));
	});
	
	// POST JSON data synchronously
	String jsonData = "{\"sensor\":\"temperature\",\"value\":25.5}";
	response = httpClient.postJson("http://api.example.com/sensors", jsonData);
	
	// POST JSON data asynchronously
	httpClient.postJson("http://api.example.com/sensors", jsonData, [](const HubHttpClientResponse& response) {
		Serial.println("Sensor data posted: " + (response.isSuccess ? "OK" : "FAILED"));
	});
}

void loop() { }
```

## Documentation

- [Server Quick Start Guide](docs/QUICKSTART.md): Get up and running with the HTTP server in minutes.
- [HTTP Client Guide](docs/HTTP_CLIENT.md): HTTP client documentation with examples.
- [API Reference](docs/API_REFERENCE.md): Full class and method documentation for the HTTP Server.
- [Usage Guide](docs/USAGE_GUIDE.md): Advanced usage, tips, and best practices for the HTTP Server.

## Dependencies

- [Hub Robot Core](https://github.com/demo-ninjas/hub-robot-core) (required, auto-included with PlatformIO)
- [PicoHTTPParser](https://github.com/h2o/picohttpparser) (bundled snapshot)

## Development Setup

1. Install PlatformIO and VSCode.
2. Install the following VSCode extensions:
   - [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
   - [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
   - [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
3. Install the `Arduino Espressif32` framework and ESP32 toolchain (e.g., `ESP32S3`).

## PlatformIO Integration

Add this library to your `platformio.ini`:

```ini
lib_deps = \
	https://github.com/demo-ninjas/hub-robot-http-utils.git \
	https://github.com/demo-ninjas/hub-robot-core.git
```

> **Note:** This library is not yet published to the PlatformIO registry. Use the GitHub URL for now.

## License

See [LICENSE](LICENSE).
