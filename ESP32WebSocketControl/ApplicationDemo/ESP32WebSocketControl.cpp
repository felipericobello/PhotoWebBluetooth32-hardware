/**
 * @file ESP32WebSocketControl.cpp
 * @brief Implementation file for the ESP32WebSocketControl library.
 */
#include "ESP32WebSocketControl.h"

// --- Library-Internal Objects and State ---

// Async Web Server object listening on standard HTTP port 80
static AsyncWebServer server(80); 
// Async WebSocket object handling connections on the "/ws" endpoint
static AsyncWebSocket ws("/ws");   

// Pointers to the application's variable array and its size (provided during init)
static VariableConfig* _variables = nullptr; 
static int _numVariables = 0;

// Pointers to the application's stream control callback functions
static StreamControlCallback _onStreamStartCallback = nullptr;
static StreamControlCallback _onStreamStopCallback = nullptr;
// Internal flag to track if data streaming is currently active
static bool _isStreaming = false; 

// --- Internal Helper Function Prototypes ---

/** @brief Finds the index of a variable in the _variables array by its name. Returns -1 if not found. */
static int findVariableIndexInternal(const char* name);
/** @brief Attempts to set a variable's value after type and limit validation. */
static bool setVariableValueInternal(int index, JsonVariant newValueVariant);
/** @brief Sends the current value of a variable to a specific client. */
static void sendVariableValueInternal(uint32_t clientId, int variableIndex);
/** @brief Sends a status/error message (JSON) to a specific client. */
static void sendStatusInternal(uint32_t clientId, const char* status, const char* message);
/** @brief The main callback function handling all WebSocket events. */
static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);


// --- Internal Helper Function Implementations ---

/**
 * @brief Finds the index of a variable by its name.
 * @param name The name of the variable to find.
 * @return The index in the _variables array, or -1 if not found or not initialized.
 */
static int findVariableIndexInternal(const char* name) {
  // Basic check: Ensure the variables array pointer is valid.
  if (!_variables || !name) return -1; 
  
  // Iterate through the application's variable array.
  for (int i = 0; i < _numVariables; i++) {
    // Compare the requested name with the current variable's name.
    if (strcmp(name, _variables[i].name) == 0) {
      return i; // Found it, return the index.
    }
  }
  return -1; // Not found after checking all variables.
}

/**
 * @brief Attempts to set a variable's value, performing type and range checks.
 * @param index The index of the variable in the _variables array.
 * @param newValueVariant A JsonVariant containing the value received from the client.
 * @return True if the value was set successfully, false otherwise.
 */
static bool setVariableValueInternal(int index, JsonVariant newValueVariant) {
  // Basic checks: Ensure the variables array and index are valid.
  if (!_variables || index < 0 || index >= _numVariables) {
    Serial.println("setVariableValueInternal: Invalid index or uninitialized variables.");
    return false; 
  }

  // Get a reference to the specific variable configuration for easier access.
  VariableConfig& var = _variables[index]; 

  // Process based on the expected type defined in the VariableConfig.
  switch (var.type) {
    case TYPE_INT:
      // Check if the received JSON value is compatible with an integer.
      if (!newValueVariant.is<int>()) { 
        Serial.printf("Set Error: Value for '%s' is not an integer.\n", var.name);
        return false;
      }
      { // Use block scope for newValue
        int newValue = newValueVariant.as<int>();
        // Perform range validation if limits are enabled for this variable.
        if (var.hasLimits && (static_cast<double>(newValue) < var.minVal || static_cast<double>(newValue) > var.maxVal)) {
          Serial.printf("Set Error: Value %d for '%s' is outside limits [%.2f, %.2f].\n",
                        newValue, var.name, var.minVal, var.maxVal);
          return false;
        }
        // Validation passed, update the variable's integer value.
        var.intValue = newValue;
        Serial.printf("Set OK: Variable '%s' (int) updated to %d.\n", var.name, var.intValue);
      }
      break; // Success for integer type.

    case TYPE_FLOAT:
      // Check if the received JSON value is compatible with a float (or int, which can be promoted).
      if (!newValueVariant.is<float>() && !newValueVariant.is<int>()) {
        Serial.printf("Set Error: Value for '%s' is not a float/number.\n", var.name);
        return false;
      }
      { // Use block scope for newValue
        float newValue = newValueVariant.as<float>();
         // Perform range validation if limits are enabled.
        if (var.hasLimits && (static_cast<double>(newValue) < var.minVal || static_cast<double>(newValue) > var.maxVal)) {
           Serial.printf("Set Error: Value %.3f for '%s' is outside limits [%.2f, %.2f].\n",
                         newValue, var.name, var.minVal, var.maxVal);
           return false;
        }
        // Validation passed, update the variable's float value.
        var.floatValue = newValue;
        Serial.printf("Set OK: Variable '%s' (float) updated to %.3f.\n", var.name, var.floatValue);
      }
      break; // Success for float type.

    case TYPE_STRING:
      // Check if the received JSON value is compatible with a string.
      if (!newValueVariant.is<const char*>()) { 
        Serial.printf("Set Error: Value for '%s' is not a string.\n", var.name);
        return false;
      }
      // No range limits check for strings (could add length check if needed).
      // Update the variable's String value (Arduino String handles memory).
      var.stringValue = newValueVariant.as<String>(); 
      Serial.printf("Set OK: Variable '%s' (string) updated to '%s'.\n", var.name, var.stringValue.c_str());
      break; // Success for string type.

    default:
      // Should not happen if VarType enum is handled correctly.
      Serial.printf("Set Error: Unknown internal type for variable '%s'.\n", var.name);
      return false; 
  }

  return true; // If we reached here, the value was successfully set.
}

/**
 * @brief Sends the current value of a variable as JSON to a specific client.
 * @param clientId The ID of the target WebSocket client.
 * @param variableIndex The index of the variable in the _variables array.
 */
static void sendVariableValueInternal(uint32_t clientId, int variableIndex) {
   // Basic checks: Ensure variables array and index are valid.
   if (!_variables || variableIndex < 0 || variableIndex >= _numVariables) return;
  
    // Create a JSON document for the response. Adjust size if needed for long names/strings.
    StaticJsonDocument<256> jsonDoc; 
    // Get a reference to the variable.
    VariableConfig& var = _variables[variableIndex];

    // Populate the JSON document.
    jsonDoc["variable"] = var.name;
    // Add the 'value' field based on the variable's type.
    switch (var.type) {
      case TYPE_INT:    jsonDoc["value"] = var.intValue;    break;
      case TYPE_FLOAT:  jsonDoc["value"] = var.floatValue;  break;
      case TYPE_STRING: jsonDoc["value"] = var.stringValue; break;
      default:          jsonDoc["value"] = nullptr; // Indicate an issue if type is unknown
                        jsonDoc["error"] = "Unknown internal type"; 
                        break; 
    }

    // Serialize the JSON document to a string.
    String response;
    serializeJson(jsonDoc, response);
    
    // Send the JSON string as a text message to the specified client.
    ws.text(clientId, response);
    // Serial.printf("Sent Value to #%u: %s\n", clientId, response.c_str()); // Optional: Log sent message
}

/**
 * @brief Sends a status or error message as JSON to a specific client.
 * @param clientId The ID of the target WebSocket client.
 * @param status A string indicating the status (e.g., "ok", "error", "info").
 * @param message A descriptive message detailing the status or error.
 */
static void sendStatusInternal(uint32_t clientId, const char* status, const char* message) {
    // Create a JSON document. Adjust size if status messages can be long.
    StaticJsonDocument<128> jsonDoc; 
    jsonDoc["status"] = status;
    jsonDoc["message"] = message;
    
    // Serialize to string.
    String response;
    serializeJson(jsonDoc, response);
    
    // Send as text message.
    ws.text(clientId, response);
    Serial.printf("Sent Status to #%u: %s\n", clientId, response.c_str()); // Log sent status
}

// --- Main WebSocket Event Handler ---

/**
 * @brief Callback function invoked by the AsyncWebSocket library for various events.
 */
static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                             void *arg, uint8_t *data, size_t len) {
  
  // Handle events based on their type.
  switch (type) {
    // Client Connected Event
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      // Optional: Send initial state of variables or stream status to the new client here.
      // for(int i=0; i<_numVariables; ++i) { sendVariableValueInternal(client->id(), i); }
      // sendStatusInternal(client->id(), "info", _isStreaming ? "Stream active" : "Stream inactive");
      break;

    // Client Disconnected Event
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket Client #%u disconnected\n", client->id());
      // Auto-stop streaming if the last client disconnects.
      if (_isStreaming && ws.count() == 0 && _onStreamStopCallback != nullptr) {
           Serial.println("Last client disconnected. Auto-stopping stream.");
           _onStreamStopCallback(); // Call application's stop function.
           _isStreaming = false;    // Update internal state.
      }
      break;

    // Data Received Event
    case WS_EVT_DATA:
      { // Block scope for AwsFrameInfo and potentially large JSON docs
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        
        // --- Handle Text Data (JSON Commands) ---
        if (info->opcode == WS_TEXT && info->final && info->index == 0 && info->len == len) {
          // Ensure data is null-terminated to treat as a C-string.
          data[len] = 0; 
          Serial.printf("Received Text from #%u: %s\n", client->id(), (char*)data);

          // Attempt to parse the received text data as JSON. Adjust size if needed.
          StaticJsonDocument<256> jsonDoc; 
          DeserializationError error = deserializeJson(jsonDoc, (char*)data);

          // Handle JSON parsing errors.
          if (error) {
            Serial.printf("JSON Parse Error: %s\n", error.c_str());
            sendStatusInternal(client->id(), "error", "Invalid JSON format received.");
            return; // Stop processing this message.
          }

          // --- Process Valid JSON Commands ---
          // Extract the "action" field, which is mandatory.
          const char* action = jsonDoc["action"];
          if (!action) {
             sendStatusInternal(client->id(), "error", "JSON missing 'action' field.");
             return;
          }

          // --- Action: "get" or "set" a variable ---
          if (strcmp(action, "get") == 0 || strcmp(action, "set") == 0) {
              // These actions require a "variable" field.
              const char* variableName = jsonDoc["variable"];
              if (!variableName) {
                  sendStatusInternal(client->id(), "error", "Missing 'variable' field for get/set action."); 
                  return;
              }
              // Find the variable's index.
              int varIndex = findVariableIndexInternal(variableName);
              if (varIndex == -1) {
                  sendStatusInternal(client->id(), "error", "Variable name not found."); 
                  return;
              }

              // Handle "get" action.
              if (strcmp(action, "get") == 0) {
                  sendVariableValueInternal(client->id(), varIndex); // Send current value.
              } 
              // Handle "set" action.
              else { // action == "set"
                  // Requires a "value" field. Check it exists and is not JSON null.
                  if (!jsonDoc.containsKey("value") || jsonDoc["value"].isNull()) {
                      sendStatusInternal(client->id(), "error", "Missing or null 'value' field for set action."); 
                      return;
                  }
                  // Attempt to set the variable value (includes validation).
                  JsonVariant newValueVariant = jsonDoc["value"];
                  if (setVariableValueInternal(varIndex, newValueVariant)) {
                      // Success: Send the updated value back as confirmation.
                      sendVariableValueInternal(client->id(), varIndex); 
                      // Optional: broadcastVariableUpdate(variableName); // Notify all clients?
                  } else {
                      // Failure (type mismatch or out of limits). Error already logged by setVariableValueInternal.
                      sendStatusInternal(client->id(), "error", "Failed to set value (invalid type or out of limits).");
                  }
              }
          } 
          // --- Action: "start_stream" ---
          else if (strcmp(action, "start_stream") == 0) {
              // Check if the application provided a start callback.
              if (_onStreamStartCallback != nullptr) {
                  if (!_isStreaming) {
                      Serial.println("Action: start_stream - Calling app callback.");
                      _onStreamStartCallback(); // Execute application's start logic.
                      _isStreaming = true;      // Update internal state.
                      sendStatusInternal(client->id(), "ok", "Stream started.");
                  } else {
                      sendStatusInternal(client->id(), "info", "Stream was already active.");
                  }
              } else {
                  // No callback registered means streaming is not supported.
                  Serial.println("Action: start_stream - No callback registered.");
                  sendStatusInternal(client->id(), "error", "Streaming feature not implemented/configured.");
              }
          } 
          // --- Action: "stop_stream" ---
          else if (strcmp(action, "stop_stream") == 0) {
               // Check if the application provided a stop callback.
               if (_onStreamStopCallback != nullptr) {
                  if (_isStreaming) {
                      Serial.println("Action: stop_stream - Calling app callback.");
                      _onStreamStopCallback(); // Execute application's stop logic.
                      _isStreaming = false;     // Update internal state.
                      sendStatusInternal(client->id(), "ok", "Stream stopped.");
                  } else {
                       sendStatusInternal(client->id(), "info", "Stream was already stopped.");
                  }
              } else {
                  // No callback registered.
                  Serial.println("Action: stop_stream - No callback registered.");
                  sendStatusInternal(client->id(), "error", "Streaming feature not implemented/configured.");
              }
          }
          // --- Unknown Action ---
           else {
              Serial.printf("Unknown action received: %s\n", action);
              sendStatusInternal(client->id(), "error", "Unknown 'action' command.");
          }

        } 
        // --- Handle Binary Data (Optional: Currently just logs) ---
        else if (info->opcode == WS_BINARY) {
             // This library currently focuses on sending binary, not receiving.
             Serial.printf("Received Binary from #%u: %u bytes (ignored by library)\n", client->id(), len);
        }
      } // End block scope for WS_EVT_DATA
      break;

    // Pong Received Event (Response to library's internal Ping)
    case WS_EVT_PONG:
      // Serial.printf("WebSocket Pong received from #%u\n", client->id()); 
      break;

    // Error Event
    case WS_EVT_ERROR:
      Serial.printf("WebSocket Client #%u error #%u: %s\n", client->id(), *((uint16_t*)arg), (char*)data);
      break;
  } // End switch (type)
}


// --- Public Function Implementations ---

/**
 * @brief Initializes WiFi AP, WebSocket server, and related components.
 */
void initWiFiWebSocketServer(const char *ssid, const char *password, 
                             VariableConfig *appVariables, int appNumVariables,
                             AsyncWebServerRequest_Callback defaultRouteHandler) {

  // Store references to the application's variables.
  _variables = appVariables;
  _numVariables = appNumVariables;
  // Basic validation of the provided variable array.
  if (!_variables || _numVariables <= 0) { 
      Serial.println("CRITICAL ERROR: Invalid variable array provided to initWiFiWebSocketServer.");
      // Consider halting or restarting if this occurs.
      return; 
  }

  Serial.println("\nInitializing ESP32 WebSocket Control Library...");

  // --- Configure Static IP for the Access Point ---
  // Using a non-default IP can prevent conflicts if the ESP32 is also a client elsewhere.
  IPAddress apIP(192, 168, 5, 1);      // The desired static IP for the ESP32 AP.
  IPAddress gatewayIP(192, 168, 5, 1); // Gateway is the ESP32 itself in AP mode.
  IPAddress subnetMask(255, 255, 255, 0); // Standard subnet mask.

  Serial.printf("Attempting to configure static AP IP: %s\n", apIP.toString().c_str());
  // Apply the static IP configuration *before* starting the AP.
  if (!WiFi.softAPConfig(apIP, gatewayIP, subnetMask)) {
    Serial.println("ERROR: Failed to configure static AP IP address!");
    // Continue with default IP or halt? For now, just log the error.
  } else {
      Serial.println("Static AP IP configuration successful.");
  }

  // --- Start the WiFi Access Point ---
  Serial.printf("Starting WiFi Access Point (SSID: %s)...\n", ssid);
  // Start the AP using the provided SSID and password. It will use the static IP if configured.
  bool apStarted = WiFi.softAP(ssid, password); 

  if (apStarted) {
      Serial.println("Access Point started successfully.");
      // Verify and print the actual IP address obtained.
      IPAddress currentAPIP = WiFi.softAPIP();
      Serial.print("--> ESP32 Access Point IP Address: "); 
      Serial.println(currentAPIP); 
      // Warn if the actual IP doesn't match the configured static IP.
      if (currentAPIP != apIP && WiFi.softAPIP() != IPAddress(192,168,4,1) /* Check against default too */ ) {
          Serial.println("Warning: AP IP does not match configured static IP. Check for conflicts or config errors.");
      }
  } else {
      Serial.println("CRITICAL ERROR: Failed to start Access Point!");
      // Halt or restart might be necessary here.
      return; 
  }

  // --- Configure WebSocket Server ---
  // Register our event handler function to manage WebSocket events.
  ws.onEvent(onWebSocketEvent); 
  // Add the WebSocket handler to the main web server.
  server.addHandler(&ws);
  Serial.println("WebSocket handler attached to /ws endpoint.");

  // --- Configure HTTP Server Root Route ---
  // Use the provided handler or a default one for requests to "/".
  if (defaultRouteHandler) { 
      server.on("/", HTTP_GET, defaultRouteHandler);
      Serial.println("Registered custom HTTP root route handler.");
  } else { 
      // Default handler sends a simple text response.
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
          request->send(200, "text/plain", "ESP32 WebSocket Server Active. Connect to /ws"); 
      });
      Serial.println("Registered default HTTP root route handler.");
  }

  // --- Start the Web Server ---
  server.begin();
  Serial.println("HTTP & WebSocket Server started.");
}

/**
 * @brief Registers application callbacks for stream control commands.
 */
void setStreamCallbacks(StreamControlCallback onStart, StreamControlCallback onStop) {
    _onStreamStartCallback = onStart;
    _onStreamStopCallback = onStop;
    Serial.println("Stream control callbacks registered.");
}

/**
 * @brief Broadcasts a variable update (JSON) to all connected clients.
 */
void broadcastVariableUpdate(const char* variableName) {
    // Don't proceed if no clients are connected.
    if (ws.count() == 0) return; 

    // Find the variable index.
    int index = findVariableIndexInternal(variableName);
    if (index < 0) {
        Serial.printf("Broadcast Error: Variable '%s' not found.\n", variableName);
        return;
    }

    // Create the JSON message. Adjust size if needed.
    StaticJsonDocument<256> jsonDoc; 
    VariableConfig& var = _variables[index];
    jsonDoc["variable"] = var.name;
    // Add the value based on type.
    switch (var.type) {
        case TYPE_INT:    jsonDoc["value"] = var.intValue;    break;
        case TYPE_FLOAT:  jsonDoc["value"] = var.floatValue;  break;
        case TYPE_STRING: jsonDoc["value"] = var.stringValue; break;
        default: 
            Serial.printf("Broadcast Error: Unknown type for var '%s'\n", variableName);
            return; // Don't send if type is unknown.
    }

    // Serialize the JSON to a string.
    String response;
    serializeJson(jsonDoc, response);

    // Send the text message to ALL connected clients.
    ws.textAll(response);
    // Serial.printf("Broadcast Sent: %s\n", response.c_str()); // Optional: Log broadcast
}

/**
 * @brief Broadcasts binary data to all connected clients.
 */
void broadcastBinaryData(const uint8_t* data, size_t len) {
    // Only send if clients are connected and data is valid.
    if (ws.count() > 0 && data != nullptr && len > 0) {
        // Send the binary data to ALL connected clients.
        ws.binaryAll(data, len);
        // Avoid Serial printing here as it can be called very frequently
        // and significantly impact performance.
    } 
    // Optional: Log if attempting to send with no clients or bad data.
    // else { if(ws.count() == 0) { /* No clients */ } else { /* Bad data */ } }
}

/**
 * @brief Cleans up disconnected clients.
 */
void cleanupWebSocketClients() {
    ws.cleanupClients();
}