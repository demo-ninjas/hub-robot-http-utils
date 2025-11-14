#include "http_server.h"
#include <memory_utils.hpp>
#include <string_utils.hpp>
#include "picohttpparser/picohttpparser.h"
#include <esp_task_wdt.h>

// ============================================================================
// Client Connection Constants
// ============================================================================
const size_t CLIENT_INACTIVITY_TIMEOUT_MS = 300000; // 5 mins


// ============================================================================
// HttpServer Implementation
// ============================================================================

HttpServer::HttpServer() : server(80) {
    // Initialize with nullptr handlers
        notFoundHandler = nullptr;
        errorHandler = nullptr;
}

void HttpServer::begin() {
    if (running) {
        if (logger) {
            logger->println("[HTTP] Server already running");
        }
        return;
    }

    HttpServerConfig cfg; // defaults
    cfg.port = port; // preserve any previously set port
    cfg.maxRequestSize = maxRequestSize;
    cfg.clientTimeout = clientTimeout;
    cfg.connectionInactivityTimeout = connectionInactivityTimeout;
    cfg.maxConnections = maxConnections;
    cfg.keepAlive = keepAlive;
    cfg.debug = debug;
    begin(cfg);
}

void HttpServer::begin(const HttpServerConfig &config) {
    if (running) {
        if (logger) {
            logger->println("[HTTP] Server already running");
        }
        return;
    }

    port = config.port;
    maxRequestSize = config.maxRequestSize;
    clientTimeout = config.clientTimeout;
    connectionInactivityTimeout = config.connectionInactivityTimeout;
    maxConnections = config.maxConnections;
    keepAlive = config.keepAlive;
    debug = config.debug;

    server = WiFiServer(port);
    server.begin();
    running = true;

    if (debug && logger) {
        logger->print("[HTTP] Server started on port ");
        logger->println(String(port));
    }
}

void HttpServer::stop() {
    if (!running) {
        return;
    }
    
    server.stop();
    running = false;
    
    if (debug && logger) {
        logger->println("[HTTP] Server stopped");
    }
}

void HttpServer::on(const String& path, RouteHandler handler) {
    httpHandlers[path] = handler;
    
    if (debug && logger) {
        logger->print("[HTTP] Registered route: ");
        logger->println(path);
    }
}

void HttpServer::on(const String &method, const String &path, RouteHandler handler) {
    // Build pattern handler
    RoutePattern rp;
    rp.method = method;
    rp.pattern = path;
    rp.handler = handler;
    // Split pattern
    rp.segments.clear();
    String tmp = path;
    if (tmp.length() > 1 && tmp.endsWith("/")) tmp.remove(tmp.length()-1);
    int start = 0;
    while (start < tmp.length()) {
        int idx = tmp.indexOf('/', start);
        if (idx == -1) idx = tmp.length();
        String seg = tmp.substring(start, idx);
        if (seg.length() > 0) {
            rp.segments.push_back(seg);
            if (seg.startsWith(":")) rp.hasParams = true;
        }
        start = idx + 1;
    }
    patternHandlers.push_back(rp);
    if (debug && logger) {
        logger->print("[HTTP] Registered route: ");
        logger->print(method);
        logger->print(" ");
        logger->println(path);
    }
}

void HttpServer::use(MiddlewareHandler middleware) {
    // Wrap the legacy middleware to return true always
    MiddlewareHandlerBool wrapped = [middleware](HttpRequest &req, HubHttpResponse &res) {
        middleware(req, res);
        return true; // always continue
    };
    middlewares.push_back(wrapped);

    if (debug && logger) {
        logger->println("[HTTP] Registered legacy middleware (consider upgrading to short-circuit capable middleware in the future)");
    }
}

void HttpServer::use(MiddlewareHandlerBool middleware) {
    middlewares.push_back(middleware);
    if (debug && logger) {
        logger->println("[HTTP] Registered short-circuit capable middleware");
    }
}

void HttpServer::onError(ErrorHandler handler) {
    errorHandler = handler;
}

void HttpServer::onNotFound(RouteHandler handler) {
    notFoundHandler = handler;
}

void HttpServer::setDebug(bool debug) {
    this->debug = debug;
}

void HttpServer::setLogger(CachingPrinter& logger) {
    this->logger = &logger;
}

void HttpServer::setPort(uint16_t port) {
    if (running) {
        if (logger) {
            logger->println("[HTTP] Cannot change port while server is running");
        }
        return;
    }
    
    this->port = port;
    server = WiFiServer(port);
}

void HttpServer::setServerName(const String& serverName) {
    this->serverName = serverName;
}

void HttpServer::setServerVersion(const String& serverVersion) {
    this->serverVersion = serverVersion;
}

void HttpServer::enableCORS(const String &origin, const String &methods, const String &headers) {
    corsEnabled = true;
    corsOrigin = origin;
    corsMethods = methods;
    corsHeaders = headers;
    
    if (debug && logger) {
        logger->println("[HTTP] CORS enabled");
    }
}

void HttpServer::disableCORS() {
    corsEnabled = false;
    
    if (debug && logger) {
        logger->println("[HTTP] CORS disabled");
    }
}

void HttpServer::setMaxRequestSize(size_t maxSize) {
    if (maxSize > MAX_BUFFER_SIZE) {
        maxRequestSize = MAX_BUFFER_SIZE;
    } else if (maxSize < DEFAULT_BUFFER_SIZE) {
        maxRequestSize = DEFAULT_BUFFER_SIZE;
    } else {
        maxRequestSize = maxSize;
    }
}

void HttpServer::setClientTimeout(uint16_t timeoutMs) {
    clientTimeout = timeoutMs;
}

void HttpServer::setConnectionInactivityTimeout(uint32_t timeoutMs) {
    connectionInactivityTimeout = timeoutMs;
}

void HttpServer::setMaxConnections(size_t maxConn) {
    maxConnections = maxConn == 0 ? 1 : maxConn;
}

void HttpServer::setKeepAlive(bool enabled) {
    keepAlive = enabled;
}

void HttpServer::addDefaultHeader(const String &name, const String &value) {
    defaultHeaders[name] = value;
}

void HttpServer::removeDefaultHeader(const String &name) {
    defaultHeaders.erase(name);
}

void HttpServer::clearDefaultHeaders() {
    defaultHeaders.clear();
}

void HttpServer::onBeforeSend(std::function<void(HttpRequest &, HubHttpResponse &)> finalizer) {
    beforeSendHook = finalizer;
}

void HttpServer::tick() {
    if (!running) {
        return;
    }

    WiFiClient newClient = server.accept();
    esp_task_wdt_reset();   // Feed the watchdog

    if (newClient) {
        if (debug && logger) {
            logger->println("[HTTP] New client connected");
        }
        connections.push_back(std::unique_ptr<HttpClientConnection>(new HttpClientConnection(std::move(newClient))));
    }

    u64_t now = millis();
    for (size_t i = 0; i < connections.size(); ) {
        HttpClientConnection* conn = connections[i].get();
        if (conn->connected()) {
            if (!conn->isActive()) {
                if (debug && logger) {
                    logger->println("[HTTP] Closing inactive client connection");
                }
                connections.erase(connections.begin() + i);
                continue;
            }
            
            esp_task_wdt_reset();   // Feed the watchdog
            if (!handleConnection(conn)) {
                if (debug && logger) {
                    logger->println("[HTTP] Closing client connection after handling");
                }
                connections.erase(connections.begin() + i);
            } else {
                i++;
            }
        } else {
            if (debug && logger) {
                logger->println("[HTTP] Client disconnected");
            }
            connections.erase(connections.begin() + i);
        }

        if (millis() - now > 256) {
            break;  // Avoid blocking too long (we'll check remaining clients next tick)
        }
    }
}

bool HttpServer::hasEnoughMemory() const {
    return freeRam() >= MIN_FREE_RAM;
}

String HttpServer::toLowerCase(const String &str) const {
    String result = str;
    result.toLowerCase();
    return result;
}

void HttpServer::applyCORS(HubHttpResponse &response) {
    if (!corsEnabled) {
        return;
    }
    
    response.setHeader("Access-Control-Allow-Origin", corsOrigin);
    response.setHeader("Access-Control-Allow-Methods", corsMethods);
    response.setHeader("Access-Control-Allow-Headers", corsHeaders);
    response.setHeader("Access-Control-Max-Age", "86400");
}

void HttpServer::applyMiddlewares(HttpRequest &req, HubHttpResponse &response) {
    for (size_t i = 0; i < middlewares.size(); i++) {
        if (!middlewares[i](req, response)) {
            break; // short-circuit
        }
    }
}

void HttpServer::applyDefaultHeaders(HubHttpResponse &response) {
    for (std::map<String,String>::const_iterator it = defaultHeaders.begin(); it != defaultHeaders.end(); ++it) {
        if (!response.headers.count(it->first)) {
            response.setHeader(it->first, it->second);
        }
    }
}

HubHttpResponse HttpServer::generateErrorResponse(int statusCode, const String &message) {
    if (errorHandler) {
        return errorHandler(statusCode, message);
    }
    
    HubHttpResponse response(statusCode);
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}

String HttpServer::getStatusText(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

void HttpServer::logRequest(const HttpRequest &req) {
    if (!debug || !logger) {
        return;
    }
    
    logger->println("[HTTP] Request:");
    logger->print("  Method: ");
    logger->println(req.method);
    logger->print("  Path: ");
    logger->println(req.path);
    
    if (req.query.size() > 0) {
        logger->println("  Query:");
        for (std::map<String, String>::const_iterator it = req.query.begin(); it != req.query.end(); ++it) {
            logger->print("    ");
            logger->print(it->first);
            logger->print(": ");
            logger->println(it->second);
        }
    }
    
    if (req.headers.size() > 0) {
        logger->println("  Headers:");
        for (std::map<String, String>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it) {
            logger->print("    ");
            logger->print(it->first);
            logger->print(": ");
            logger->println(it->second);
        }
    }
    
    if (req.body.length() > 0) {
        logger->print("  Body: ");
        if (req.body.length() > 100) {
            logger->print(req.body.substring(0, 100));
            logger->println("...");
        } else {
            logger->println(req.body);
        }
    }
}

void HttpServer::logResponse(const HubHttpResponse &response) {
    if (!debug || !logger) {
        return;
    }
    
    logger->print("[HTTP] Response: ");
    logger->print(String(response.status));
    logger->print(" ");
    logger->println(getStatusText(response.status));
}


bool HttpServer::handleConnection(HttpClientConnection* connection) {
    // Memory check
    if (!hasEnoughMemory()) {
        if (logger) {
            logger->println("[HTTP] Insufficient memory for new client");
        }
        HubHttpResponse response = generateErrorResponse(503, "Service Unavailable");
        respondToClient(connection->getClient(), response);
        return false;   // Done with this client connection - it can be closed
    }

    WiFiClient &client = connection->getClient();

    // Wait for client to be ready for upto 4ms - then give up and try later
    u64_t startWait = millis();
    while (millis() - startWait < 4 && !client.available()) {
        delay(1);
    }

    if (!client.available()) {
        return true;
    }

    if (client.available() > 0) {
        const char* method;
        const char* path;
        std::vector<byte> buf(DEFAULT_BUFFER_SIZE);
        size_t parse_result = 0;
        int minor_version;
        struct phr_header headers[MAX_HEADERS];
        size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
        
        while (parse_result <= 0) {
            // Check if we've exceeded max request size
            if (prevbuflen >= maxRequestSize) {
                if (logger) {
                    logger->println("[HTTP] Request too large");
                }
                HubHttpResponse response = generateErrorResponse(413, "Payload Too Large");
                respondToClient(client, response);
                return false;
            }
            
            byte tmpBuf[DEFAULT_BUFFER_SIZE];
            int read_result = client.read(tmpBuf, DEFAULT_BUFFER_SIZE);
            
            if (read_result == -1) {
                if (logger) {
                    logger->println("[HTTP] Read error");
                }
                return false;
            } else if (read_result == 0) {
                return false;
            }

            buflen = read_result;
            
            // Bounds check before resizing
            if (buflen + prevbuflen > maxRequestSize) {
                if (logger) {
                    logger->println("[HTTP] Request exceeds max size");
                }
                HubHttpResponse response = generateErrorResponse(413, "Payload Too Large");
                respondToClient(client, response);
                return false;
            }
            
            // Extend buffer if needed
            if (buflen + prevbuflen > buf.size()) {
                size_t newSize = prevbuflen + buflen;
                if (newSize > maxRequestSize) {
                    newSize = maxRequestSize;
                }
                buf.resize(newSize);
            }
            
            // Append to buffer
            memcpy(buf.data() + prevbuflen, tmpBuf, buflen);
            buflen = buflen + prevbuflen;

            // Parse the request
            num_headers = MAX_HEADERS;
            const char* buf_char = reinterpret_cast<const char*>(buf.data());
            parse_result = phr_parse_request(buf_char, buflen, &method, &method_len, 
                                            &path, &path_len, &minor_version, 
                                            headers, &num_headers, prevbuflen);
            
            if (parse_result == -1) {
                if (logger) {
                    logger->println("[HTTP] Parse error");
                }
                HubHttpResponse response = generateErrorResponse(400, "Bad Request");
                respondToClient(client, response);
                return false;
            } else if (parse_result == -2) {
                // Incomplete request, continue reading
                prevbuflen = buflen;
                continue;
            } else {
                break;
            }
        }

        if (parse_result <= 0) {
            return false;
        }

        // Extract request body
        String body = "";
        if (static_cast<size_t>(parse_result) < buflen) {
            size_t bodyLen = buflen - parse_result;
            if (bodyLen > 0) {
                body = String(reinterpret_cast<const char*>(buf.data()) + parse_result);
            }
        }

        // Build HttpRequest object
        HttpRequest req;
        req.method = String(method, method_len);
        req.body = body;
        
        // Parse path and query string
        std::string full_path = std::string(path, path_len);
        size_t queryPos = full_path.find('?');
        
        if (queryPos != std::string::npos) {
            req.path = String(full_path.substr(0, queryPos).c_str());
            
            // Parse query parameters
            std::string query_string = full_path.substr(queryPos + 1);
            std::vector<std::string> query_pairs = split(query_string, '&');
            for (size_t i = 0; i < query_pairs.size(); i++) {
                std::vector<std::string> key_value = split(query_pairs[i], '=');
                if (key_value.size() == 2) {
                    req.query[String(key_value[0].c_str())] = String(key_value[1].c_str());
                } else if (key_value.size() == 1) {
                    req.query[String(key_value[0].c_str())] = "";
                }
            }
        } else {
            req.path = String(path, path_len);
        }
        
        // Remove trailing slash (except for root)
        if (req.path.length() > 1 && req.path.endsWith("/")) {
            req.path.remove(req.path.length() - 1);
        }
        
        // Parse headers
        for (size_t h = 0; h < num_headers; h++) {
            String headerName = String(headers[h].name, headers[h].name_len);
            String headerValue = String(headers[h].value, headers[h].value_len);
            req.headers[headerName] = headerValue;
            String lower = headerName; lower.toLowerCase();
            req.headersLower[lower] = headerValue;
        }

        logRequest(req);

        // Create response
        HubHttpResponse response;
        
        // Handle OPTIONS for CORS preflight
        if (corsEnabled && req.method == "OPTIONS") {
            response.setStatus(204);
            applyCORS(response);
            respondToClient(client, response);
            return true;
        }
        
        // Apply middlewares
        applyMiddlewares(req, response);
        
        // Attempt param/method route matching first
        bool routed = false;
        for (size_t r = 0; r < patternHandlers.size(); r++) {
            if (matchPattern(patternHandlers[r], req.method, req.path, req)) {
                try {
                    response = patternHandlers[r].handler(req);
                } catch (...) {
                    if (logger) logger->println("[HTTP] Handler threw exception");
                    response = generateErrorResponse(500, "Internal Server Error");
                }
                routed = true;
                break;
            }
        }

        if (!routed && httpHandlers.find(req.path) != httpHandlers.end()) {
            try {
                response = httpHandlers[req.path](req);
            } catch (...) {
                if (logger) logger->println("[HTTP] Handler threw exception");
                response = generateErrorResponse(500, "Internal Server Error");
            }
            routed = true;
        }

        if (!routed && req.path == "/") {
            // Default root handler
            String html = "<html><head><title>" + serverName + "</title></head>";
            html += "<body><h1>Hello!</h1><h3>You're connected to " + serverName + "!</h3>";
            html += "<p>Version: " + serverVersion + "</p></body></html>";
            response.html(html);
            routed = true;
        } else if (!routed && req.path == "/log") {
            // Built-in log endpoint
            if (logger == nullptr) {
                response = generateErrorResponse(404, "Logging not enabled");
            } else {
                size_t num_lines = 20;
                if (req.hasQueryParam("lines")) {
                    num_lines = req.getQueryParam("lines").toInt();
                    if (num_lines == 0) num_lines = 20;
                }
                String log_tail = logger->tail(num_lines).c_str();
                response.text(log_tail);
            }
            routed = true;
        }

        if (!routed) {
            // Not found
            if (notFoundHandler) {
                response = notFoundHandler(req);
            } else {
                response = generateErrorResponse(404, "Not Found");
            }
        }
        
        // Apply CORS headers
        if (corsEnabled) {
            applyCORS(response);
        }
        
        // Add server header
        if (!response.headers.count("Server")) {
            response.setHeader("Server", serverName + "/" + serverVersion);
        }
        // Apply default headers
        applyDefaultHeaders(response);
        // Keep-Alive / Connection header
        if (keepAlive) {
            response.setHeader("Connection", "keep-alive");
        } else {
            response.setHeader("Connection", "close");
        }
        // Final hook
        if (beforeSendHook) {
            beforeSendHook(req, response);
        }
        
        // Send response
        logResponse(response);
        respondToClient(client, response);
        
        connection->updateActivity();
    }

    return true;
}

bool HttpServer::respondToClient(WiFiClient& client, HubHttpResponse& response) {
    // Build status line
    String statusLine = "HTTP/1.1 " + String(response.status) + " " + getStatusText(response.status);
    client.println(statusLine);
    
    // Write headers
    for (std::map<String, String>::const_iterator it = response.headers.begin(); 
         it != response.headers.end(); ++it) {
        client.print(it->first);
        client.print(": ");
        client.println(it->second);
    }

    // Calculate and send Content-Length
    size_t total_bytes = utf8ByteLength(response.body);
    client.print("Content-Length: ");
    client.println(String(total_bytes));
    client.println();
    
    // Send body in chunks
    if (total_bytes > 0) {
        size_t bytes_written = 0;
        unsigned long start = millis();
        
        while (bytes_written < total_bytes) {
            // Check timeout
            if (millis() - start > WRITE_TIMEOUT_MS) {
                if (logger) {
                    logger->println("[HTTP] Write timeout");
                }
                return false;
            }
            
            // Calculate chunk size
            size_t remaining = total_bytes - bytes_written;
            size_t chunk_size = (remaining < WRITE_CHUNK_SIZE) ? remaining : WRITE_CHUNK_SIZE;
            
            // Write chunk
            size_t written = client.write(
                reinterpret_cast<const uint8_t*>(response.body.c_str()) + bytes_written, 
                chunk_size
            );
            
            if (written == 0) {
                if (logger) {
                    logger->println("[HTTP] Write error");
                }
                return false;
            }
            
            bytes_written += written;
            
            // Small yield to prevent watchdog issues
            if (bytes_written < total_bytes) {
                delay(1);
            }
        }
    }
    
    return true;
}

// ============================================================================
// HttpRequest Implementation
// ============================================================================

HttpRequest::HttpRequest() {
    method = "GET";
    path = "/";
    body = "";
}

bool HttpRequest::jsonRequested() const {
    // Check Accept header
    String accept = getHeader("Accept");
    if (accept.length() > 0) {
        accept.toLowerCase();
        if (accept.indexOf("application/json") != -1 || accept.indexOf("json") != -1) {
            return true;
        }
    }
    
    // Check query parameter
    String jsonParam = getQueryParam("json");
    if (jsonParam.length() > 0) {
        jsonParam.toLowerCase();
        if (jsonParam == "true" || jsonParam == "1" || jsonParam == "yes") {
            return true;
        }
    }
    
    return false;
}

String HttpRequest::getHeader(const String &name, const String &defaultValue) const {
    String lowerName = name; lowerName.toLowerCase();
    std::map<String,String>::const_iterator it = headersLower.find(lowerName);
    if (it != headersLower.end()) return it->second;
    return defaultValue;
}

String HttpRequest::getQueryParam(const String &name, const String &defaultValue) const {
    std::map<String, String>::const_iterator it = query.find(name);
    if (it != query.end()) {
        return it->second;
    }
    return defaultValue;
}

bool HttpRequest::hasHeader(const String &name) const {
    String lowerName = name; lowerName.toLowerCase();
    return headersLower.find(lowerName) != headersLower.end();
}

bool HttpServer::matchPattern(const RoutePattern &rp, const String &method, const String &path, HttpRequest &req) {
    if (!rp.method.equalsIgnoreCase(method)) return false;
    String normPath = path;
    if (normPath.length() > 1 && normPath.endsWith("/")) normPath.remove(normPath.length()-1);
    // Split incoming path
    std::vector<String> pathSegs;
    int start = 0;
    while (start < normPath.length()) {
        int idx = normPath.indexOf('/', start);
        if (idx == -1) idx = normPath.length();
        String seg = normPath.substring(start, idx);
        if (seg.length() > 0) pathSegs.push_back(seg);
        start = idx + 1;
    }
    if (pathSegs.size() != rp.segments.size()) return false;
    for (size_t i = 0; i < rp.segments.size(); i++) {
        const String &patSeg = rp.segments[i];
        const String &actSeg = pathSegs[i];
        if (patSeg.startsWith(":")) {
            String key = patSeg.substring(1);
            req.params[key] = actSeg;
        } else if (patSeg != actSeg) {
            return false;
        }
    }
    return true;
}

bool HttpRequest::hasQueryParam(const String &name) const {
    return query.find(name) != query.end();
}

String HttpRequest::getContentType() const {
    return getHeader("Content-Type");
}

bool HttpRequest::isContentType(const String &contentType) const {
    String ct = getContentType();
    ct.toLowerCase();
    String checkType = contentType;
    checkType.toLowerCase();
    return ct.indexOf(checkType) != -1;
}

// ============================================================================
// HttpResponse Implementation
// ============================================================================

HubHttpResponse::HubHttpResponse() : status(200) {}

HubHttpResponse::HubHttpResponse(int statusCode) : status(statusCode) {}

HubHttpResponse::HubHttpResponse(int statusCode, const String &responseBody) 
    : status(statusCode), body(responseBody) {}

HubHttpResponse& HubHttpResponse::setStatus(int statusCode) {
    status = statusCode;
    return *this;
}

HubHttpResponse& HubHttpResponse::setBody(const String &content) {
    body = content;
    return *this;
}

HubHttpResponse& HubHttpResponse::setHeader(const String &name, const String &value) {
    headers[name] = value;
    return *this;
}

HubHttpResponse& HubHttpResponse::json(const String &jsonBody) {
    body = jsonBody;
    headers["Content-Type"] = "application/json";
    return *this;
}

HubHttpResponse& HubHttpResponse::html(const String &htmlBody) {
    body = htmlBody;
    headers["Content-Type"] = "text/html; charset=utf-8";
    return *this;
}

HubHttpResponse& HubHttpResponse::text(const String &textBody) {
    body = textBody;
    headers["Content-Type"] = "text/plain; charset=utf-8";
    return *this;
}

HubHttpResponse& HubHttpResponse::cors(const String &origin) {
    headers["Access-Control-Allow-Origin"] = origin;
    headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    return *this;
}

HubHttpResponse HubHttpResponse::redirect(const String &location, bool permanent) {
    HubHttpResponse response(permanent ? 301 : 302);
    response.setHeader("Location", location);
    return response;
}

HubHttpResponse HubHttpResponse::error(int statusCode, const String &message) {
    HubHttpResponse response(statusCode);
    response.text(message);
    return response;
}



HttpClientConnection::HttpClientConnection(WiFiClient client) : client(client) {
    lastActivityMillis = millis();
}

HttpClientConnection::~HttpClientConnection() {
    if (client) {
        client.stop();
    }
}

void HttpClientConnection::updateActivity() {
    lastActivityMillis = millis();
}

bool HttpClientConnection::isActive() const {
    return (millis() - lastActivityMillis) < CLIENT_INACTIVITY_TIMEOUT_MS;
}

bool HttpClientConnection::connected() {
    return client.connected();
}

WiFiClient& HttpClientConnection::getClient() {
    return client;
}



// ============================================================================
// WiFi Utility Functions
// ============================================================================

int wifiScan(Print* printer) {
    if (printer == nullptr) {
        printer = &Serial;
    }

    printer->print("Scanning Wifi Networks...");
    int numSsid = WiFi.scanNetworks();
    if (numSsid == 0) {
        printer->println("None Found");
        return 0;
    } else if (numSsid == -1) {
        printer->println("Failed");
        return 0;
    }

    printer->println(String(numSsid) + " Networks Found");
    for (int i = 0; i < numSsid; i++) {
        printer->print(i);
        printer->print(". ");
        printer->print(WiFi.SSID(i));
        printer->print("\tSignal: ");
        printer->print(WiFi.RSSI(i));
        printer->println(" dBm");
    }

    return numSsid;
}