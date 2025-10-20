
# Hub Robot HTTP Utils

**Hub Robot HTTP Utils** provides a lightweight, robust HTTP server for ESP32-based robots and embedded projects. It enables your device to serve HTTP requests, define custom API endpoints, and interact with web clients or other devices over WiFi.

## Features

- **Route-based request handling**: Easily define handlers for specific URL paths (e.g., `/api/status`).
- **Middleware support**: Add custom logic (logging, authentication, etc.) that runs for every request.
- **Built-in CORS support**: Enable cross-origin requests with a single call.
- **Custom error handling**: Define your own error and 404 responses.
- **Request/response utilities**: Access headers, query parameters, and content types with convenience methods.
- **Configurable limits and timeouts**: Control buffer sizes, request body limits, and client timeouts for reliability.
- **Memory-safe**: Designed for microcontrollers with careful bounds checking and low memory usage.
- **Request logging**: Optional debug logging for development and troubleshooting.
- **PicoHTTPParser integration**: Fast, minimal HTTP parsing using [PicoHTTPParser](https://github.com/h2o/picohttpparser).

> **Note:** HTTP client functionality is planned but not yet implemented.

## Example Usage

See the [Quick Start Guide](docs/QUICKSTART.md) for a step-by-step example.

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

## Documentation

- [Quick Start Guide](docs/QUICKSTART.md): Get up and running in minutes.
- [API Reference](docs/API_REFERENCE.md): Full class and method documentation.
- [Usage Guide](docs/USAGE_GUIDE.md): Advanced usage, tips, and best practices.

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

