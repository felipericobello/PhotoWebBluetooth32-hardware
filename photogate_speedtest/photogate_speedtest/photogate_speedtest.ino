/*
Bibliotecas:

- ESP Async WebServer by ESP32Async (v3.7.6)
- Async TCP by ESP32Async (v3.3.8)

Placa:

- Board manager: esp32 by Expressif Systems (v3.2.0)
- Placa: ESP32-WROOM-DA Module 
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// --- Configurações ---
const char *ssid = "ESP32_Photogate";
const char *password = "12345678";

// --- Configurações de Buffering e Amostragem ---
const int NUM_SAMPLES_PER_CHUNK = 20; // (N) Quantas leituras agrupar por envio.
const int SAMPLE_INTERVAL_US = 200; // Intervalo de amostragem em microssegundos.

// Pinos dos sensores
const int SENSOR_PIN_1 = 32;
const int SENSOR_PIN_2 = 33;
const int SENSOR_PIN_3 = 34;
const int SENSOR_PIN_4 = 35;
const int SENSOR_PIN_5 = 36;
const int SENSOR_PIN_6 = 37;

// Estrutura de dados
#pragma pack(push, 1)
struct SensorDataPacket {
  uint16_t gate1; uint16_t gate2; uint16_t gate3;
  uint16_t gate4; uint16_t gate5; uint16_t gate6;
  uint32_t time; // Timestamp em ms relativo ao início
}; // Tamanho = 16 bytes
#pragma pack(pop)

const int SINGLE_PACKET_SIZE = sizeof(SensorDataPacket); // 16 bytes
const int BUFFER_SIZE_BYTES = NUM_SAMPLES_PER_CHUNK * SINGLE_PACKET_SIZE; // Tamanho total do buffer de envio

// Buffer para acumular as leituras
SensorDataPacket sensorBuffer[NUM_SAMPLES_PER_CHUNK];
int bufferIndex = 0; // Índice atual no buffer

uint32_t t0; // Timestamp inicial

// Objetos do Servidor
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- Funções Auxiliares ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      bufferIndex = 0; // Reseta o buffer se um novo cliente conectar (opcional)
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    // ... outros casos ...
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN_1, INPUT); pinMode(SENSOR_PIN_2, INPUT); pinMode(SENSOR_PIN_3, INPUT);
  pinMode(SENSOR_PIN_4, INPUT); pinMode(SENSOR_PIN_5, INPUT); pinMode(SENSOR_PIN_6, INPUT);

  Serial.println("\n\n--- Starting Setup ---");
  WiFi.persistent(false); WiFi.disconnect(true); WiFi.mode(WIFI_OFF); delay(100);
  WiFi.mode(WIFI_AP);
  Serial.printf("Attempting to start AP with SSID: [%s]\n", ssid);
   if (password != NULL && strlen(password) > 0) { Serial.printf("Using password.\n"); }
   else { Serial.println("Configuring OPEN network."); }

  Serial.println("Configuring Access Point...");
  bool apConfigured = WiFi.softAP(ssid, password);
  if (apConfigured) {
    Serial.println("WiFi.softAP() call succeeded.");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: "); Serial.println(IP);
  } else {
    Serial.println("!!!!!!!! ERROR: WiFi.softAP() call FAILED! !!!!!!!!");
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "ESP32 Photogate Server (Buffered). Connect client to ws://192.168.4.1/ws");
  });
  server.begin();
  Serial.println("HTTP and WebSocket server started.");
  Serial.printf("Buffering %d samples per chunk.\n", NUM_SAMPLES_PER_CHUNK);
  Serial.printf("Target sample rate: %.2f Hz (%d us interval).\n", 1000000.0/SAMPLE_INTERVAL_US, SAMPLE_INTERVAL_US);
  Serial.printf("Chunk size: %d bytes.\n", BUFFER_SIZE_BYTES);
  Serial.println("--- Setup Complete ---");
  t0 = millis(); // Pega o tempo inicial após a configuração
}

// Função para simular oscilação
uint16_t osc(float deg0, float T) {
  return (uint16_t)(2000.0+2000.0*cos(6.28*(deg0/360.0 + double(millis()-t0)/(1000*T))));
}

void loop() {

  // Só processa se houver algum cliente conectado
  if (ws.count() > 0) {

    // 1. Ler sensores e preencher o buffer atual
    sensorBuffer[bufferIndex].gate1 = (uint16_t)analogRead(SENSOR_PIN_1) + osc(0.0, 1.0);
    sensorBuffer[bufferIndex].gate2 = (uint16_t)analogRead(SENSOR_PIN_2) + osc(30.0, 1.0);
    sensorBuffer[bufferIndex].gate3 = (uint16_t)analogRead(SENSOR_PIN_3) + osc(60.0, 1.0);
    sensorBuffer[bufferIndex].gate4 = (uint16_t)analogRead(SENSOR_PIN_4) + osc(90.0, 1.0);
    sensorBuffer[bufferIndex].gate5 = (uint16_t)analogRead(SENSOR_PIN_5) + osc(120.0, 1.0);
    sensorBuffer[bufferIndex].gate6 = (uint16_t)analogRead(SENSOR_PIN_6) + osc(150.0, 1.0);
    sensorBuffer[bufferIndex].time = millis() - t0; // Timestamp relativo

    bufferIndex++; // Avança para a próxima posição no buffer

    // 2. Verificar se o buffer está cheio
    if (bufferIndex >= NUM_SAMPLES_PER_CHUNK) {
      // Envia o buffer completo
      ws.binaryAll((uint8_t*)sensorBuffer, BUFFER_SIZE_BYTES);
      bufferIndex = 0; // Reseta o índice para começar a preencher o próximo chunk      
    }

    // 3. Esperar o intervalo de amostragem    
    delayMicroseconds(SAMPLE_INTERVAL_US);

  } else { // Se não houver clientes, resetar o buffer    
    bufferIndex = 0;
    delay(100); // Espera um pouco se não houver clientes
  }

  ws.cleanupClients(); // Limpa clientes desconectados
}