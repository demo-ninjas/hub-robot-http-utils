#include "http_server.h"
#include <memory_utils.hpp>
#include <string_utils.hpp>
#include "picohttpparser/picohttpparser.h"

void HttpServer::begin() {
    server.begin();
    this->running = true;
    if (this->debug && this->logger) {
        this->logger->println("HTTP Server started on port " + String(this->port));
    }
}

void HttpServer::on(const String& path, std::function<HttpResponse(WiFiClient*, HttpRequest&)> handler) {
    this->httpHandlers[path] = handler;
}

void HttpServer::setDebug(bool debug) {
    this->debug = debug;
}

void HttpServer::setLogger(CachingPrinter& logger) {
    this->logger = &logger;
}

void HttpServer::setPort(uint16_t port) {
    if (this->running) {
        if (this->logger) {
            this->logger->println("Cannot change port while server is running");
        }
        return;
    }
    
    this->port = port;
    this->server = WiFiServer(port);
}

void HttpServer::setServerName(const String& serverName) {
    this->serverName = serverName;
}

void HttpServer::setServerVersion(const String& serverVersion) {
    this->serverVersion = serverVersion;
}

HttpServer::HttpServer() : server(80) {}

void HttpServer::tick() {
    if (!this->running) {
        return;
    }

    WiFiClient newClient = server.accept();
    if (newClient) {
        this->handleClient(newClient);
    }
}


void HttpServer::handleClient(WiFiClient newClient) {
    // Check if we have enough resources for the new client
    if (freeRam() < 4096) { // Require at least 4KB of memory to handle the new client
        newClient.println("HTTP/1.1 503 Server Out of Resources");
        newClient.println("Connection: close");
        newClient.println("Content-Length: 0");
        newClient.println();
        newClient.stop();
        return;
    }


    if (newClient.available() > 0) {
        const char* method, * path;
        std::vector<byte> buf(2048);
        size_t parse_result = 0;
        int minor_version;
        struct phr_header headers[12];
        size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
        while (parse_result <= 0) {
            byte tmpBuf[2048];
            int read_result = newClient.read(tmpBuf, 2048);
            if (read_result == -1) {
                // IOError - so print to serial and stop the client
                if (this->logger) {
                    this->logger->println("Error reading from HTTP client - will stop client");
                }
                newClient.stop();
            } else if (read_result == 0) {
                return;
            }

            buflen = read_result;
            // Check if we need to extend the buffer
            if (buflen + prevbuflen > buf.size()) {
                // Create a new buffer and copy the old buffer into it
                buf.resize(prevbuflen + buflen);
                memcpy(buf.data() + prevbuflen, tmpBuf, buflen);
            } else {
                // Append tmpBuf to buf
                memcpy(buf.data() + prevbuflen, tmpBuf, buflen);
            }
            buflen = buflen + prevbuflen;

            /* parse the request */
            num_headers = sizeof(headers) / sizeof(headers[0]);
            const char* buf_char = reinterpret_cast<const char*>(buf.data());
            parse_result = phr_parse_request(buf_char, buflen, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, prevbuflen);
            if (parse_result == -1) {
                // Failed to parse the request, respond with a 400 Bad Request
                if (this->logger) {
                    this->logger->println("Failed to parse the request from HTTP client - will respond with a 400 Bad Request");
                }
                newClient.println("HTTP/1.1 400 Bad Request");
                newClient.println("Content-Type: text/plain");
                newClient.println("Connection: close");
                newClient.println("Content-Length: 0");
                newClient.println();
                newClient.stop();
                break;
            } else if (parse_result == -2) {
                // Request is incomplete, continue the loop
                prevbuflen = buflen;
                continue;
            } else {
                break;
            }
        }

        if (parse_result <= 0) {
            // Failed to parse the request, so move onto the next client
            return;
        }

        if (buflen == sizeof(buf)) {
            // Request is too long, respond with a 413 Payload Too Large
            if (this->logger) {
                this->logger->println("Request from HTTP client is too large - will respond with a 413 Payload Too Large");
            }
            newClient.println("HTTP/1.1 413 Payload Too Large");
            newClient.println("Content-Type: text/plain");
            newClient.println("Connection: close");
            newClient.println("Content-Length: 0");
            newClient.println();
            newClient.stop();
            return;
        }

        // Grab the Request Details
        String body = "";
        // Set body to the remainder of the buffer after the headers
        if (parse_result < buflen) {
            body = String(reinterpret_cast<const char*>(buf.data()) + parse_result);
        }

        HttpRequest req = HttpRequest();
        req.method = String(method, method_len);
        std::string full_path = std::string(path, path_len);
        if (full_path.find('?') != std::string::npos) {
            req.path = String(full_path.substr(0, full_path.find('?')).c_str());
            if (req.path.endsWith("/")) {
                req.path.remove(req.path.length() - 1);
            }
            std::string query_string = full_path.substr(full_path.find('?') + 1);
            std::vector<std::string> query_pairs = split(query_string, '&');
            for (std::string pair : query_pairs) {
                std::vector<std::string> key_value = split(pair, '=');
                if (key_value.size() == 2) {
                    req.query[String(key_value[0].c_str())] = String(key_value[1].c_str());
                }
            }
        } else {
            req.path = String(path, path_len);
        }
        req.body = body;
        for (size_t h = 0; h != num_headers; ++h) {
            req.headers[String(headers[h].name, headers[h].name_len)] = String(headers[h].value, (int)headers[h].value_len);
        }


        // Write some debugging info to the serial port
        if (this->debug && this->logger) {
            this->logger->println("HTTP Request:");
            this->logger->println("  Method: " + req.method);
            this->logger->println("  Path: " + req.path);
            this->logger->println("  Query Params: ");
            for (std::map<String, String>::const_iterator it = req.query.begin(); it != req.query.end(); ++it) {
                this->logger->println("    " + it->first + ": " + it->second);
            }
            this->logger->println("  Headers: ");
            for (std::map<String, String>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it) {
                this->logger->println("    " + it->first + ": " + it->second);
            }
            if (req.body.length() > 0)
                this->logger->println("  Body: " + req.body);
            this->logger->println("---");
        }

        // Look for a request handler for the path
        if (this->httpHandlers.find(req.path) != this->httpHandlers.end()) {
            // Pass the request to the handler
            HttpResponse response = this->httpHandlers[req.path](&newClient, req);
            respondToClient(newClient, response);
        } else if (req.path.length() == 1 && req.path[0] == '/') {
            // Respond with a default response
            String response_text = "<html><head><title>Innovation Hub - HTTP Server</title></head><body><h1>Hello!</h1><h3>You're connected to " + this->serverName + "!</h3></body></html>";
            newClient.println("HTTP/1.1 200 OK");
            newClient.println("Content-Type: text/html");
            newClient.println("Content-Length: " + String(utf8ByteLength(response_text)));
            newClient.println();
            newClient.print(response_text.c_str());  // send the response body
        } else if (req.path == "/log") {
            // Respond with the log buffer
            if (this->logger == nullptr) {
                // No logger set, so respond with a 404 - this method is not provided on this device
                newClient.println("HTTP/1.1 404 Not Found");
                newClient.println("Content-Type: text/plain");
                newClient.println("Content-Length: 0");
                newClient.println();
            } else {
                size_t num_lines = 20;
                if (req.query.find("lines") != req.query.end()) {
                    num_lines = req.query["lines"].toInt();
                }

                String log_tail = this->logger->tail(num_lines).c_str();
                newClient.println("HTTP/1.1 200 OK");
                newClient.println("Content-Type: text/plain");
                newClient.println("Content-Length: " + String(utf8ByteLength(log_tail)));
                newClient.println();
                newClient.print(log_tail.c_str());
            }
        } else {
            // No Handler for this request, respond with a 404 Not Found
            newClient.println("HTTP/1.1 404 Not Found");
            newClient.println("Content-Type: text/plain");
            newClient.println("Content-Length: 0");
            newClient.println();
        }

        // Stop the client (clearing up resources and closing the connection - we're not supporting keep-alive)
        newClient.stop();
    }
}

void HttpServer::respondToClient(WiFiClient& newClient, HttpResponse& response) {
    // Write the response to the client
    newClient.println("HTTP/1.1 " + String(response.status));
    for (std::map<String, String>::const_iterator it = response.headers.begin(); it != response.headers.end(); ++it) {
        newClient.println(it->first + ": " + it->second);
    }

    size_t bytes_written = 0;
    size_t total_bytes = utf8ByteLength(response.body);
    newClient.println("Content-Length: " + String(total_bytes));
    newClient.println();
    long start = millis();
    size_t write_batch_length = 512;
    while (bytes_written < total_bytes) {
        size_t chunk_size = min(write_batch_length, total_bytes - bytes_written);
        size_t written = newClient.write(response.body.c_str() + bytes_written, chunk_size);
        if (written == 0) {
            // Error writing to the socket - stop the client
            if (this->logger) {
                this->logger->println("Error writing response to HTTP client - will stop client");
            }
            break;
        }
        bytes_written += written;

        if (bytes_written == total_bytes) {
            break;
        }

        if (millis() - start > 1000) { // Too long - give up - we don't want to waste time here writing to the socket - and 1s is already too long!!
            if (this->logger) {
                this->logger->println("Timeout writing response to HTTP client - will stop client");
            }
            break;
        }
    }
}

HttpResponse::HttpResponse() {
    this->status = 200;
}

HttpRequest::HttpRequest() {
    this->method = "GET";
    this->path = "/";
    this->body = "";
}

bool HttpRequest::jsonRequested() {
    if (this->headers.find("accept") != this->headers.end()) {
        String accepts = this->headers["accept"];
        if (accepts.indexOf("application/json") != -1 || accepts.indexOf("json") != -1) {
            return true;
        }
    }
    if (this->query.find("json") != this->query.end()) {
        String json = this->query["json"];
        if (json == "true" || json == "1" || json == "json") {
            return true;
        }
    }
    return false;
}


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