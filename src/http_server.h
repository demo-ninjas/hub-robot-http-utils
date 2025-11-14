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

// Forward declarations
class HttpRequest;
class HubHttpResponse;
class HttpServer;

// Type definitions for cleaner code
typedef std::function<HubHttpResponse(HttpRequest &)> RouteHandler;
typedef std::function<void(HttpRequest &, HubHttpResponse &)> MiddlewareHandler; // legacy (always continue)
typedef std::function<bool(HttpRequest &, HubHttpResponse &)> MiddlewareHandlerBool; // return false to short-circuit
typedef std::function<HubHttpResponse(int, const String &)> ErrorHandler;

/**
 * @brief Represents an HTTP request with method, path, headers, query params and body
 */
class HttpRequest {
public:
    String method;
    String path;
    String body;
    std::map<String, String> headers;
    std::map<String, String> query;
    std::map<String, String> params; // path parameters
    std::map<String, String> headersLower; // lowercase header lookup
    
    HttpRequest();
    
    /**
     * @brief Check if the client requested JSON response (via Accept header or ?json=true query param)
     * @return true if JSON is requested
     */
    bool jsonRequested() const;
    
    /**
     * @brief Get a header value (case-insensitive)
     * @param name Header name
     * @param defaultValue Default value if header not found
     * @return Header value or default
     */
    String getHeader(const String &name, const String &defaultValue = "") const;
    
    /**
     * @brief Get a query parameter value
     * @param name Query parameter name
     * @param defaultValue Default value if parameter not found
     * @return Query parameter value or default
     */
    String getQueryParam(const String &name, const String &defaultValue = "") const;
    
    /**
     * @brief Check if request has a specific header
     * @param name Header name (case-insensitive)
     * @return true if header exists
     */
    bool hasHeader(const String &name) const;
    
    /**
     * @brief Check if request has a specific query parameter
     * @param name Query parameter name
     * @return true if parameter exists
     */
    bool hasQueryParam(const String &name) const;
    
    /**
     * @brief Get Content-Type header
     * @return Content-Type value or empty string
     */
    String getContentType() const;
    
    /**
     * @brief Check if content type matches (case-insensitive)
     * @param contentType Content type to check (e.g., "application/json")
     * @return true if matches
     */
    bool isContentType(const String &contentType) const;
};

/**
 * @brief Represents an HTTP response with status code, headers and body
 */
class HubHttpResponse {
public:
    int status;
    String body;
    std::map<String, String> headers;
    
    HubHttpResponse();
    explicit HubHttpResponse(int statusCode);
    HubHttpResponse(int statusCode, const String &body);
    
    /**
     * @brief Set response status code
     * @param statusCode HTTP status code (e.g., 200, 404, 500)
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& setStatus(int statusCode);
    
    /**
     * @brief Set response body
     * @param content Body content
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& setBody(const String &content);
    
    /**
     * @brief Set a response header
     * @param name Header name
     * @param value Header value
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& setHeader(const String &name, const String &value);
    
    /**
     * @brief Set JSON response with appropriate Content-Type header
     * @param json JSON string
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& json(const String &jsonBody);
    
    /**
     * @brief Set HTML response with appropriate Content-Type header
     * @param html HTML string
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& html(const String &htmlBody);
    
    /**
     * @brief Set plain text response with appropriate Content-Type header
     * @param text Plain text string
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& text(const String &textBody);
    
    /**
     * @brief Enable CORS for this response
     * @param origin Allowed origin (default: "*")
     * @return Reference to this response (for chaining)
     */
    HubHttpResponse& cors(const String &origin = "*");
    
    /**
     * @brief Create a redirect response
     * @param location URL to redirect to
     * @param permanent Use 301 instead of 302
     * @return Redirect response
     */
    static HubHttpResponse redirect(const String &location, bool permanent = false);
    
    /**
     * @brief Create a JSON error response
     * @param statusCode HTTP status code
     * @param message Error message
     * @return Error response
     */
    static HubHttpResponse error(int statusCode, const String &message);
};

class HttpClientConnection {
public:
    HttpClientConnection(WiFiClient client);
    ~HttpClientConnection();

    void updateActivity();
    bool isActive() const;
    bool connected();
    WiFiClient& getClient();

private:
    WiFiClient client;
    u32_t lastActivityMillis;
};

struct RoutePattern {
    String method;
    String pattern; // e.g. /api/item/:id
    std::vector<String> segments;
    RouteHandler handler;
    bool hasParams = false;
};

/**
 * @brief Lightweight HTTP server for ESP32 microcontrollers
 * 
 * Features:
 * - Route-based request handling
 * - Middleware support
 * - Memory-safe with bounds checking
 * - Configurable limits and timeouts
 * - Built-in CORS support
 * - Request logging
 * - Error handling
 */
class HttpServer {
public:
    // Configuration constants (can be modified before calling begin())
    static const size_t DEFAULT_BUFFER_SIZE = 2048;
    static const size_t MAX_BUFFER_SIZE = 8192;
    static const size_t MIN_FREE_RAM = 4096;
    static const uint16_t CLIENT_TIMEOUT_MS = 5000;
    static const uint16_t WRITE_TIMEOUT_MS = 1000;
    static const size_t WRITE_CHUNK_SIZE = 512;
    static const size_t MAX_HEADERS = 16;
    static const size_t DEFAULT_MAX_CONNECTIONS = 4;

    struct HttpServerConfig {
        uint16_t port = 80;
        size_t maxRequestSize = MAX_BUFFER_SIZE;
        uint16_t clientTimeout = CLIENT_TIMEOUT_MS;
        uint32_t connectionInactivityTimeout = 300000; // 5 min
        size_t maxConnections = DEFAULT_MAX_CONNECTIONS;
        bool keepAlive = false;
        bool debug = false;
    };
    
    HttpServer();
    
    /**
     * @brief Start the HTTP server
     */
    void begin();
    void begin(const HttpServerConfig &config); // config-based begin
    
    /**
     * @brief Stop the HTTP server
     */
    void stop();
    
    /**
     * @brief Process incoming connections (call this in your main loop)
     */
    void tick();
    
    /**
     * @brief Register a route handler
     * @param path URL path (e.g., "/api/status")
     * @param handler Function to handle requests to this path
     */
    void on(const String &path, RouteHandler handler);
    void on(const String &method, const String &path, RouteHandler handler); // method-specific + params
    
    /**
     * @brief Register a middleware handler (executes for all requests)
     * @param middleware Function to process request/response
     */
    void use(MiddlewareHandler middleware);
    void use(MiddlewareHandlerBool middleware); // short-circuit capable
    
    /**
     * @brief Set custom error handler
     * @param handler Function to generate error responses
     */
    void onError(ErrorHandler handler);
    
    /**
     * @brief Set custom 404 Not Found handler
     * @param handler Function to handle unmatched routes
     */
    void onNotFound(RouteHandler handler);
    
    /**
     * @brief Enable or disable debug logging
     * @param debug true to enable debug output
     */
    void setDebug(bool debug);
    
    /**
     * @brief Set logger for debug output
     * @param logger Reference to CachingPrinter logger
     */
    void setLogger(CachingPrinter &logger);
    
    /**
     * @brief Set server port (must be called before begin())
     * @param port Port number (default: 80)
     */
    void setPort(uint16_t port);
    
    /**
     * @brief Set server name for Server header
     * @param serverName Server name
     */
    void setServerName(const String &serverName);
    
    /**
     * @brief Set server version for Server header
     * @param serverVersion Version string
     */
    void setServerVersion(const String &serverVersion);
    
    /**
     * @brief Enable CORS for all routes
     * @param origin Allowed origin (default: "*")
     * @param methods Allowed methods (default: "GET, POST, PUT, DELETE, OPTIONS")
     * @param headers Allowed headers (default: "Content-Type, Authorization")
     */
    void enableCORS(const String &origin = "*", 
                    const String &methods = "GET, POST, PUT, DELETE, OPTIONS",
                    const String &headers = "Content-Type, Authorization");
    
    /**
     * @brief Disable CORS
     */
    void disableCORS();
    
    /**
     * @brief Set maximum request body size
     * @param maxSize Maximum size in bytes (default: 8192)
     */
    void setMaxRequestSize(size_t maxSize);
    
    /**
     * @brief Set client timeout
     * @param timeoutMs Timeout in milliseconds (default: 5000)
     */
    void setClientTimeout(uint16_t timeoutMs);
    void setConnectionInactivityTimeout(uint32_t timeoutMs);
    void setMaxConnections(size_t maxConn);
    void setKeepAlive(bool enabled);
    
    /**
     * @brief Get server running state
     * @return true if server is running
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Get current port
     * @return Port number
     */
    uint16_t getPort() const { return port; }
    bool getKeepAlive() const { return keepAlive; }
    size_t getMaxConnections() const { return maxConnections; }

    void addDefaultHeader(const String &name, const String &value);
    void removeDefaultHeader(const String &name);
    void clearDefaultHeaders();
    void onBeforeSend(std::function<void(HttpRequest &, HubHttpResponse &)> finalizer);

private:
    // Configuration
    bool debug = false;
    bool running = false;
    bool corsEnabled = false;
    CachingPrinter *logger = nullptr;
    uint16_t port = 80;
    uint16_t clientTimeout = CLIENT_TIMEOUT_MS;
    size_t maxRequestSize = MAX_BUFFER_SIZE;
    uint32_t connectionInactivityTimeout = 300000;
    size_t maxConnections = DEFAULT_MAX_CONNECTIONS;
    bool keepAlive = false;
    String serverName = "Hub-Server";
    String serverVersion = "1.0";
    String corsOrigin = "*";
    String corsMethods = "GET, POST, PUT, DELETE, OPTIONS";
    String corsHeaders = "Content-Type, Authorization";
    
    // Server components
    WiFiServer server;
    std::map<String, RouteHandler> httpHandlers; // legacy any-method exact path
    std::vector<RoutePattern> patternHandlers;
    std::vector<MiddlewareHandlerBool> middlewares; // unified middleware list
    RouteHandler notFoundHandler;
    ErrorHandler errorHandler;
    std::vector<std::unique_ptr<HttpClientConnection>> connections;
    std::map<String, String> defaultHeaders;
    std::function<void(HttpRequest &, HubHttpResponse &)> beforeSendHook;
    
    // Internal methods
    bool handleConnection(HttpClientConnection* connection);
    bool respondToClient(WiFiClient &client, HubHttpResponse &response);
    HubHttpResponse generateErrorResponse(int statusCode, const String &message);
    void applyCORS(HubHttpResponse &response);
    void applyMiddlewares(HttpRequest &req, HubHttpResponse &response);
    void applyDefaultHeaders(HubHttpResponse &response);
    bool parseHeaders(HttpRequest &req, const String &method, const String &path);
    String getStatusText(int statusCode);
    void logRequest(const HttpRequest &req);
    void logResponse(const HubHttpResponse &response);
    String toLowerCase(const String &str) const;
    bool hasEnoughMemory() const;
    bool matchPattern(const RoutePattern &rp, const String &method, const String &path, HttpRequest &req);
};

/**
 * @brief Scan for available WiFi networks
 * @param printer Optional printer for output (default: Serial)
 * @return Number of networks found
 */
int wifiScan(Print* printer = nullptr);

#endif // HUB_HTTP_SERVER_H