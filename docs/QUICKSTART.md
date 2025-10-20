# Quick Start Guide

Get up and running with the HTTP Server Library in 5 minutes.

## Prerequisites

- ESP32 microcontroller (ESP32-S3 recommended)
- PlatformIO or Arduino IDE
- WiFi credentials

## Installation

### PlatformIO (Recommended)

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

## Basic Example

```cpp
#include <WiFi.h>
#include <http_server.h>

// WiFi credentials
const char* WIFI_SSID = "your-wifi-name";
const char* WIFI_PASSWORD = "your-wifi-password";

// Create server
HttpServer server;

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Define routes
    server.on("/", [](HttpRequest &req) {
        HttpResponse response;
        response.html("<html><body><h1>Hello from ESP32!</h1></body></html>");
        return response;
    });
    
    server.on("/api/info", [](HttpRequest &req) {
        String json = "{";
        json += "\"device\":\"ESP32\",";
        json += "\"uptime\":" + String(millis()) + ",";
        json += "\"free_heap\":" + String(ESP.getFreeHeap());
        json += "}";
        
        HttpResponse response;
        response.json(json);
        return response;
    });
    
    // Start server
    server.begin();
    Serial.println("HTTP Server started!");
    Serial.println("Try visiting: http://" + WiFi.localIP().toString());
}

void loop() {
    server.tick();  // IMPORTANT: Handle incoming requests
}
```

## Testing Your Server

1. Upload the code to your ESP32
2. Open Serial Monitor to see the IP address
3. Open a web browser and visit:
   - `http://YOUR_ESP32_IP/` - See the HTML page
   - `http://YOUR_ESP32_IP/api/info` - See JSON data

## Next Steps

### Add More Routes

```cpp
// GET endpoint
server.on("/api/status", [](HttpRequest &req) {
    return HttpResponse().text("Online");
});

// Handle query parameters
server.on("/api/echo", [](HttpRequest &req) {
    String message = req.getQueryParam("message", "Hello");
    return HttpResponse().text("You said: " + message);
});
// Visit: http://YOUR_IP/api/echo?message=test
```

### Enable CORS (for web apps)

```cpp
void setup() {
    // ... WiFi setup ...
    
    server.enableCORS();  // Allow browser access
    
    // ... routes ...
    
    server.begin();
}
```

### Add Debug Logging

```cpp
CachingPrinter logger(4096);

void setup() {
    // ... WiFi setup ...
    
    server.setLogger(logger);
    server.setDebug(true);  // Enable detailed logging
    
    // ... routes ...
    
    server.begin();
}

// Access logs at: http://YOUR_IP/log
```

### Handle POST Requests

```cpp
server.on("/api/data", [](HttpRequest &req) {
    if (req.method != "POST") {
        return HttpResponse::error(405, "Use POST method");
    }
    
    // Access request body
    Serial.println("Received: " + req.body);
    
    return HttpResponse().json("{\"status\":\"received\"}");
});
```

### Custom 404 Page

```cpp
void setup() {
    // ... setup ...
    
    server.onNotFound([](HttpRequest &req) {
        String html = "<html><body>";
        html += "<h1>404 - Page Not Found</h1>";
        html += "<p>Path: " + req.path + "</p>";
        html += "<a href='/'>Go Home</a>";
        html += "</body></html>";
        
        HttpResponse response(404);
        response.html(html);
        return response;
    });
    
    server.begin();
}
```

## Common Patterns

### JSON API

```cpp
#include <ArduinoJson.h>

server.on("/api/sensors", [](HttpRequest &req) {
    StaticJsonDocument<200> doc;
    
    doc["temperature"] = 23.5;
    doc["humidity"] = 65;
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    
    return HttpResponse().json(output);
});
```

### Authentication

```cpp
server.on("/api/secure", [](HttpRequest &req) {
    String token = req.getHeader("Authorization");
    
    if (token != "Bearer secret-token") {
        return HttpResponse::error(401, "Unauthorized");
    }
    
    return HttpResponse().json("{\"secure\":\"data\"}");
});
```

### Redirects

```cpp
server.on("/old", [](HttpRequest &req) {
    return HttpResponse::redirect("/new");
});
```

## Troubleshooting

### Can't connect to server?

1. Check Serial Monitor for the IP address
2. Make sure you're on the same WiFi network
3. Try pinging the ESP32: `ping YOUR_ESP32_IP`

### Server stops responding?

Make sure `server.tick()` is in your `loop()`:

```cpp
void loop() {
    server.tick();  // Don't forget this!
}
```

### Memory issues?

Reduce buffer sizes:

```cpp
server.setMaxRequestSize(2048);  // 2KB instead of 8KB
```

### Want more details?

- See the [Usage Guide](USAGE_GUIDE.md) for comprehensive documentation
- Check the [API Reference](API_REFERENCE.md) for all methods
- View examples in the repository

## Full Example with Everything

```cpp
#include <WiFi.h>
#include <http_server.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASSWORD = "your-password";

HttpServer server;
CachingPrinter logger(4096);

void setup() {
    Serial.begin(115200);
    
    // Connect WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Configure server
    server.setServerName("MyESP32");
    server.setServerVersion("1.0.0");
    server.setLogger(logger);
    server.setDebug(true);
    server.enableCORS();
    
    // Middleware - log all requests
    server.use([](HttpRequest &req, HttpResponse &response) {
        Serial.print("[");
        Serial.print(millis());
        Serial.print("] ");
        Serial.print(req.method);
        Serial.print(" ");
        Serial.println(req.path);
    });
    
    // Home page
    server.on("/", [](HttpRequest &req) {
        String html = "<html><head><title>ESP32 Server</title></head><body>";
        html += "<h1>Welcome to ESP32!</h1>";
        html += "<h3>Available Endpoints:</h3>";
        html += "<ul>";
        html += "<li><a href='/api/info'>/api/info</a> - Device info (JSON)</li>";
        html += "<li><a href='/api/status'>/api/status</a> - Status</li>";
        html += "<li><a href='/log'>/log</a> - View logs</li>";
        html += "</ul>";
        html += "</body></html>";
        
        return HttpResponse().html(html);
    });
    
    // JSON API
    server.on("/api/info", [](HttpRequest &req) {
        StaticJsonDocument<300> doc;
        
        doc["device"] = "ESP32";
        doc["version"] = "1.0.0";
        doc["uptime_ms"] = millis();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["wifi_rssi"] = WiFi.RSSI();
        
        String output;
        serializeJson(doc, output);
        
        return HttpResponse().json(output);
    });
    
    // Simple text endpoint
    server.on("/api/status", [](HttpRequest &req) {
        return HttpResponse().text("OK");
    });
    
    // POST example
    server.on("/api/echo", [](HttpRequest &req) {
        if (req.method != "POST") {
            return HttpResponse::error(405, "Use POST method");
        }
        
        StaticJsonDocument<200> doc;
        doc["received"] = req.body;
        doc["length"] = req.body.length();
        doc["timestamp"] = millis();
        
        String output;
        serializeJson(doc, output);
        
        return HttpResponse().json(output);
    });
    
    // Custom 404
    server.onNotFound([](HttpRequest &req) {
        String html = "<html><body>";
        html += "<h1>404 - Not Found</h1>";
        html += "<p>Path: " + req.path + "</p>";
        html += "<a href='/'>Go Home</a>";
        html += "</body></html>";
        
        HttpResponse response(404);
        response.html(html);
        return response;
    });
    
    // Start server
    server.begin();
    Serial.println("Server started!");
    Serial.println("Visit: http://" + WiFi.localIP().toString());
}

void loop() {
    server.tick();
}
```

## What's Next?

✅ Read the [Usage Guide](USAGE_GUIDE.md) for advanced features  
✅ Check the [API Reference](API_REFERENCE.md) for complete documentation  
✅ Build your own API!  

---

**Happy Coding! 🚀**
