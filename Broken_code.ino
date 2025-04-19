#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Firebase_ESP_Client.h>
#include <WiFi.h>

// Configurações do Firebase
#define API_KEY "YUqCN9J30XHtgZIQAb4PdcHMck1rV08HlGDC8LvW"
#define DATABASE_URL "https://monitoramento-bpm-e-temp-default-rtdb.firebaseio.com/"

// Configurações de rede
const char* ssid = "Starlink_CIT";
const char* password = "Ufrr@2024Cit";

// Configurações de hardware
#define MAX30205_ADDRESS 0x48  // Tente também 0x49 se não funcionar
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define RATE_SIZE 4

// Objetos globais
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Variáveis de estado
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;
float calibrationOffset = 0.0;
bool sensorsInitialized = false;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
float currentTemperature = 0.0;
bool fingerDetected = false;

// Declarações de funções (PROTÓTIPOS)
void showSplashScreen(const char* message);
void connectToWiFi();
void setupFirebase();
bool initializeHardware();
void calibrateTemperatureSensor();  // Adicionado protótipo da função
float readTemperature();
void updateSensorData();
void updateHeartRate();
void resetHeartRate();
void updateDisplay();
void sendToFirebase();
void scanI2CDevices();

void setup() {
  Serial.begin(115200);
  while (!Serial); // Aguarda a porta serial estar pronta
  
  // Inicializar I2C com pull-ups
  Wire.begin(21, 22); // SDA, SCL para ESP32
  Wire.setClock(100000); // 100kHz
  
  // Verificar dispositivos I2C
  scanI2CDevices();
  
  // Inicializar display primeiro
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do OLED"));
    while (true);
  }
  
  showSplashScreen("Inicializando...");
  
  // Conectar ao Wi-Fi
  connectToWiFi();
  
  // Inicializar Firebase
  setupFirebase();
  
  // Inicializar hardware
  if (!initializeHardware()) {
    showSplashScreen("Falha nos sensores!");
    while (true);
  }
  
  calibrateTemperatureSensor();
  sensorsInitialized = true;
  
  // Primeira atualização do display com dados reais
  updateSensorData();
  updateDisplay();
}


void loop() {
  // Verificar conexão WiFi periodicamente
  if (millis() % 5000 == 0 && WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  // Atualizar sensores a cada 100ms (mais frequente para melhor detecção)
  if (millis() - lastSensorRead >= 100) {
    lastSensorRead = millis();
    updateSensorData();
  }

  // Atualizar display a cada 250ms para maior responsividade
  if (millis() - lastDisplayUpdate >= 250) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }

  // Enviar para Firebase a cada 5 segundos
  if (millis() - lastFirebaseUpdate >= 5000) {
    sendToFirebase();
    lastFirebaseUpdate = millis();
  }
}

void scanI2CDevices() {
  Serial.println("Scanning I2C devices...");
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println();
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  }
}

void showSplashScreen(const char* message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(message);
  display.display();
  delay(1000); // Mostra por 1 segundo
}

void connectToWiFi() {
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar ao WiFi!");
  }
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "";
  auth.user.password = "";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

bool initializeHardware() {
  // Inicializar sensor de batimento
  Serial.println("Inicializando MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Falha no sensor MAX30102. Verifique conexões.");
    return false;
  }
  
  // Configurar sensor MAX30102
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // Ajuste este valor conforme necessário
  particleSensor.setPulseAmplitudeGreen(0); // Desligar LED verde
  
  // Configurar sensibilidade
  particleSensor.enableDIETEMPRDY();
  
  // Verificar sensor de temperatura
  Serial.println("Verificando MAX30205...");
  Wire.beginTransmission(MAX30205_ADDRESS);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.print("Falha no sensor MAX30205 (Erro: ");
    Serial.print(error);
    Serial.println("). Tentando endereço alternativo 0x49...");
    
    // Tentar endereço alternativo comum para MAX30205
    Wire.beginTransmission(0x49);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.println("MAX30205 encontrado em 0x49");
      #undef MAX30205_ADDRESS
      #define MAX30205_ADDRESS 0x49
    } else {
      Serial.println("MAX30205 não encontrado em nenhum endereço comum");
      return false;
    }
  }

  return true;
}

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00); // Registro de temperatura
  byte error = Wire.endTransmission(false);
  
  if (error != 0) {
    Serial.print("Erro na comunicação com MAX30205: ");
    Serial.println(error);
    return -999;
  }

  delay(10); // Tempo para o sensor preparar os dados

  if (Wire.requestFrom(MAX30205_ADDRESS, 2) != 2) {
    Serial.println("Dados insuficientes do MAX30205");
    return -999;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  int16_t raw = (msb << 8) | lsb;
  float temp = raw * 0.00390625; // Conversão para Celsius

  // Verificação de valores plausíveis
  if (temp < 20.0 || temp > 45.0) {
    Serial.print("Temperatura fora do range esperado: ");
    Serial.println(temp);
    return -999;
  }

  return temp;
}

void updateSensorData() {
  // Atualizar temperatura
  float rawTemp = readTemperature();
  
  if (rawTemp > -900) { // Se leitura válida
    currentTemperature = rawTemp + calibrationOffset;
  } else {
    Serial.println("Leitura inválida de temperatura");
  }
  
  // Atualizar batimentos
  updateHeartRate();
  
  // Exibir no serial monitor
  Serial.print("Temp: ");
  Serial.print(currentTemperature, 1);
  Serial.print("°C | BPM: ");
  Serial.print((int)beatsPerMinute);
  Serial.print(" | Avg: ");
  Serial.print(beatAvg);
  Serial.print(" | IR: ");
  Serial.println(particleSensor.getIR());
}

void updateHeartRate() {
  long irValue = particleSensor.getIR();
  
  // Exibir valor IR para debug
  static unsigned long lastIRPrint = 0;
  if (millis() - lastIRPrint > 1000) {
    lastIRPrint = millis();
    Serial.print("IR Value: ");
    Serial.println(irValue);
  }

  if (irValue > 50000) { // Dedo detectado
    fingerDetected = true;
    
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      if (delta > 0) {
        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute > 20 && beatsPerMinute < 255) {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;

          // Calcular média
          beatAvg = 0;
          for (byte x = 0; x < RATE_SIZE; x++) {
            beatAvg += rates[x];
          }
          beatAvg /= RATE_SIZE;
        }
      }
    }
  } else { // Sem dedo
    fingerDetected = false;
    resetHeartRate();
  }
}

void resetHeartRate() {
  beatsPerMinute = 0;
  beatAvg = 0;
  rateSpot = 0;
  lastBeat = 0;
  for (byte x = 0; x < RATE_SIZE; x++) {
    rates[x] = 0;
  }
}
void calibrateTemperatureSensor() {
  Serial.println("Calibrando sensor de temperatura...");
  
  float sum = 0;
  int validReadings = 0;
  
  for (int i = 0; i < 20; i++) {
    float temp = readTemperature();
    if (temp > -900) { // Ignorar erros
      sum += temp;
      validReadings++;
      showSplashScreen("Calibrando...");
    }
    delay(250);
  }

  if (validReadings > 0) {
    calibrationOffset = 25.0 - (sum / validReadings);
    Serial.print("Offset calculado: ");
    Serial.print(calibrationOffset, 2);
    Serial.println(" °C");
  } else {
    Serial.println("Falha na calibração - usando offset padrão");
    calibrationOffset = 0.0;
  }
}
void updateDisplay() {
  display.clearDisplay();
  
  // Cabeçalho
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Monitor Cardiaco");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperatura
  display.setCursor(0, 15);
  display.print("Temp: ");
  display.setTextSize(2);
  display.print(currentTemperature, 1);
  display.print("°C");

  // BPM
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("BPM: ");
  display.setTextSize(2);
  display.print((int)beatsPerMinute);

  // Média BPM
  display.setTextSize(1);
  display.setCursor(70, 35);
  display.print("Avg: ");
  display.setTextSize(2);
  display.print(beatAvg);

  // Status
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("WiFi: ");
  display.print(WiFi.status() == WL_CONNECTED ? "OK " : "Off ");
  display.print("FB: ");
  display.print(Firebase.ready() ? "OK" : "Err");

  // Indicador de dedo detectado
  if (!fingerDetected) {
    display.setTextSize(1);
    display.setCursor(70, 55);
    display.print("Coloque o dedo");
  }

  // Alerta de febre
  if (currentTemperature >= 38.0) {
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(" FEBRE DETECTADA ");
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();
}

void sendToFirebase() {
  if (!Firebase.ready() || WiFi.status() != WL_CONNECTED) {
    Serial.println("Firebase não está pronto");
    return;
  }

  if (currentTemperature < -900) return; // Ignorar se leitura inválida

  FirebaseJson json;
  json.set("temperatura", currentTemperature);
  json.set("alerta_febre", currentTemperature >= 38.0);
  json.set("bpm", (int)beatsPerMinute);
  json.set("media_bpm", beatAvg);
  json.set("timestamp", millis() / 1000);

  Serial.println("Enviando dados para Firebase...");
  
  if (!Firebase.RTDB.updateNode(&firebaseData, "/paciente", &json)) {
    Serial.println("Falha no Firebase: " + firebaseData.errorReason());
  } else {
    Serial.println("Dados enviados com sucesso!");
  }
}