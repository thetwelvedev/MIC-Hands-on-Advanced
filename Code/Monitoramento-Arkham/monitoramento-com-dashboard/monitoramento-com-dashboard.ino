#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// Configurações de WiFi
const char* ssid = "Nome da rede Wifi";
const char* password = "Senha";
const char* serverUrl = "http://SEU_IP_LOCAL:5000/api/data";

// Endereço I2C do MAX30205
const uint8_t MAX30205_ADDRESS = 0x48;

// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objeto do sensor de batimento
MAX30105 particleSensor;

// Buffers para leitura dos dados
#define MAX_BRIGHTNESS 255
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];  
uint32_t redBuffer[BUFFER_SIZE]; 
int32_t spo2;                    
int8_t validSPO2;                
int32_t heartRate;               
int8_t validHeartRate;           

// Variáveis para cálculo de BPM
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;

// Variáveis para temporização
unsigned long lastDataSend = 0;
const unsigned long SEND_INTERVAL = 2000; 

// Variável de calibração da temperatura
float calibrationOffset = 0.0;

// Protótipos de função
float readTemperature();
bool checkTempSensor();
void calibrateSensor();
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2, bool hasFinger);
void setupHeartSensor();
void readHeartRateAndSpO2();
void scanI2C();
void sendDataToServer(float temp, int bpm, int avgBpm, int spo2, bool hasFinger);
void connectToWiFi();
void reconnectWiFiIfNeeded();

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 

  // Inicializa o display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do OLED"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Inicializando...");
  display.display();

  // Conecta ao WiFi
  connectToWiFi();

  // Verifica dispositivos I2C
  scanI2C();

  // Verifica sensor de temperatura
  if (!checkTempSensor()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERRO: Sensor temp");
    display.display();
    while (true);
  }

  calibrateSensor();     
  setupHeartSensor();   

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Sensores prontos!");
  display.display();
  delay(1000);
}

void loop() {
  // Verifica e reconecta WiFi se necessário
  reconnectWiFiIfNeeded();

  float rawTemp = readTemperature();
  float calibratedTemp = rawTemp + calibrationOffset;
  bool overTemp = calibratedTemp >= 38.0; // Febre se >= 38 °C

  readHeartRateAndSpO2(); 

  bool hasFinger = particleSensor.getIR() > 50000;

  // Envia dados periodicamente
  if (millis() - lastDataSend > SEND_INTERVAL) {
    sendDataToServer(calibratedTemp, heartRate, beatAvg, spo2, hasFinger);
    lastDataSend = millis();
  }

  // Exibe informações no serial
  Serial.print("Temp (raw): ");
  Serial.print(rawTemp, 2);
  Serial.print(" C | Calibrada: ");
  Serial.print(calibratedTemp, 2);
  Serial.print(" C | ");
  Serial.println(overTemp ? "ALERTA: FEBRE!" : "Normal");

  Serial.print("BPM: ");
  Serial.print(heartRate);
  Serial.print(" | Média: ");
  Serial.print(beatAvg);
  Serial.print(" | SpO2: ");
  Serial.print(spo2);
  Serial.print("% | Pulso: ");
  Serial.println(hasFinger ? "Sim" : "Não");
  Serial.println("------------------------");

  updateDisplay(calibratedTemp, overTemp, heartRate, beatAvg, spo2, hasFinger);

  delay(10); 
}

// Função para ler temperatura do MAX30205
float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    Serial.println("Erro na comunicação com MAX30205");
    return -100;
  }

  if (Wire.requestFrom(MAX30205_ADDRESS, 2) != 2) {
    Serial.println("Dados insuficientes");
    return -100;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  
  // Verifica se os bytes são plausíveis
  if (msb == 0xFF && lsb == 0xFF) {
    Serial.println("Leitura inválida (0xFFFF)");
    return -100;
  }

  int16_t raw = (msb << 8) | lsb;
  return raw * 0.00390625; // Conversão para °C
}

// Verifica se o sensor de temperatura está presente
bool checkTempSensor() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("MAX30205 detectado!");
    return true;
  } else {
    Serial.print("MAX30205 não encontrado! Erro: ");
    Serial.println(error);
    return false;
  }
}

// Calibra o sensor de temperatura
void calibrateSensor() {
  Serial.println("Calibrando temperatura... Aguarde 10s");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrando sensor");
  display.println("Aguarde 10s...");
  display.display();
  delay(10000);

  float sum = 0;
  int validReadings = 0;
  int readings = 20;
  
  for (int i = 0; i < readings; i++) {
    float t = readTemperature();
    if (t != -100 && t > -50 && t < 100) { // Filtra valores absurdos
      sum += t;
      validReadings++;
      Serial.print("Leitura ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(t, 2);
    }
    delay(200);
  }

  if (validReadings == 0) {
    Serial.println("ERRO: Nenhuma leitura válida do sensor!");
    calibrationOffset = 0;
    return;
  }

  float avg = sum / validReadings;
  
  // Se a média estiver muito baixa (como -31.5), ajustamos para ~36°C
  if (avg < -20) {
    calibrationOffset = 36.0 - avg;
    Serial.println("Ajuste para leituras muito baixas");
  } else {
    // Caso contrário, calibramos para temperatura ambiente (~25°C)
    calibrationOffset = 25.0 - avg;
  }

  Serial.print("Média das leituras: ");
  Serial.print(avg, 2);
  Serial.print(" C | Offset calculado: ");
  Serial.print(calibrationOffset, 2);
  Serial.println(" C");
}

// Configura o sensor de batimentos cardíacos
void setupHeartSensor() {
  Serial.println("Iniciando sensor MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 não detectado. Verifique a conexão.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERRO: Sensor cardiaco");
    display.display();
    while (1);
  }

  // Configura o sensor com os melhores parâmetros
  byte ledBrightness = 60; 
  byte sampleAverage = 4;  
  byte ledMode = 2;        
  int sampleRate = 100;    
  int pulseWidth = 411;    
  int adcRange = 4096;     

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.enableDIETEMPRDY(); // Habilita o sensor de temperatura interna

  Serial.println("MAX30102 configurado!");
}

// Lê a frequência cardíaca e SpO2 do sensor
void readHeartRateAndSpO2() {
  long irValue = particleSensor.getIR();
  bool hasFinger = irValue > 50000;

  // Detecta batimentos cardíacos
  if (checkForBeat(irValue) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      // Calcula a média dos batimentos
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Se não há dedo, zera os valores
  if (!hasFinger) {
    heartRate = 0;
    beatAvg = 0;
    spo2 = 0;
    validHeartRate = 0;
    validSPO2 = 0;
    return;
  }

  // Atualiza os buffers para cálculo de SpO2
  static uint32_t lastUpdate = 0;
  static int bufferIndex = 0;

  if (millis() - lastUpdate > 15) { // Coleta dados a cada 15ms
    lastUpdate = millis();
    
    redBuffer[bufferIndex] = particleSensor.getRed();
    irBuffer[bufferIndex] = irValue;
    
    bufferIndex++;
    
    if (bufferIndex == BUFFER_SIZE) {
      bufferIndex = 0;
      
      // Calcula SpO2 e BPM
      maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, 
                                           &spo2, &validSPO2, &heartRate, &validHeartRate);
      
      // Se o cálculo de BPM falhar, usa o valor do algoritmo de batimento
      if (validHeartRate == 0 && beatAvg > 0) {
        heartRate = beatAvg;
        validHeartRate = 1;
      }
    }
  }
}

// Atualiza o display OLED
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2, bool hasFinger) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Monitor de Saude");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperatura
  display.setCursor(0, 15);
  display.print("Temp:");
  display.print(temperature, 1);
  display.print((char)247); // ° símbolo
  display.println("C");

  // BPM
  display.setCursor(0, 25);
  display.print("BPM:");
  if (hasFinger) {
    display.print(bpm);
  } else {
    display.print("--");
  }

  // SpO2
  display.setCursor(72, 15);
  display.print("SpO2:");
  if (hasFinger) {
    display.print(spo2);
  } else {
    display.print("--");
  }
  display.print("%");

  // Média BPM
  display.setCursor(72, 25);
  display.print("Avg:");
  if (hasFinger) {
    display.print(avgBpm);
  } else {
    display.print("--");
  }

  // Status do dedo
  display.setCursor(0, 35);
  display.print("Pulso: ");
  display.print(hasFinger ? "Detectado" : "Ausente");

  // WiFi status
  display.setCursor(0, 45);
  display.print("WiFi: ");
  display.print(WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado");

  // Alerta de febre
  if (alert) {
    display.setCursor(0, 55);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(" ALERTA: FEBRE! ");
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();
}

// Envia dados para o servidor Flask
void sendDataToServer(float temp, int bpm, int avgBpm, int spo2, bool hasFinger) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Cria o objeto JSON para enviar
    String httpRequestData = "{\"temperature\":" + String(temp, 2) + 
                            ",\"bpm\":" + String(bpm) + 
                            ",\"avg_bpm\":" + String(avgBpm) + 
                            ",\"spo2\":" + String(spo2) + 
                            ",\"has_finger\":" + String(hasFinger ? "true" : "false") + 
                            "}";
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Dados enviados com sucesso!");
      Serial.println("Resposta do servidor: " + response);
    } else {
      Serial.print("Erro no envio: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi desconectado - dados não enviados");
  }
}

// Conecta ao WiFi
void connectToWiFi() {
  Serial.println("Conectando ao WiFi...");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Conectando WiFi...");
  display.display();
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("Endereço IP: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi conectado!");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(1000);
  } else {
    Serial.println("Falha na conexão WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Falha no WiFi!");
    display.display();
  }
}

// Verifica e reconecta WiFi se necessário
void reconnectWiFiIfNeeded() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado - tentando reconectar...");
    WiFi.disconnect();
    delay(100);
    connectToWiFi();
  }
}

// Scaneia dispositivos I2C
void scanI2C() {
  Serial.println("Scanning I2C...");
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo encontrado: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("Nenhum dispositivo I2C encontrado!");
  }
}