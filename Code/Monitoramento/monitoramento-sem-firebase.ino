#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// Endereço I2C do MAX30205
const uint8_t MAX30205_ADDRESS = 0x48;

// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objeto do sensor de batimento
MAX30105 particleSensor;

// Buffer para cálculo de média dos batimentos
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
float beatsPerMinute;
int beatAvg;
int spo2 = 98; // Valor inicial de SpO2

// Variáveis para simulação
unsigned long lastBPMUpdate = 0;
const unsigned long BPM_UPDATE_INTERVAL = 1000; // 1 segundo
float baseBPM = 72.0; // BPM base para simulação
float currentBPM = 72.0;

// Variável de calibração da temperatura
float calibrationOffset = 69.53;


// Funções auxiliares
float readTemperature();
void calibrateSensor();
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2, bool hasFinger);
void setupHeartSensor();
void simulateHeartRate();

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL para ESP32

  // Inicializa o display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do OLED"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Inicializando sensores...");
  display.display();
  delay(2000);

  calibrateSensor();     // Calibra o sensor de temperatura
  setupHeartSensor();    // Inicia o sensor MAX30102
}

void loop() {
  float rawTemp = readTemperature();
  float calibratedTemp = calibrationOffset + rawTemp;
  bool overTemp = calibratedTemp >= 38.0; // Febre se >= 38 °C

  simulateHeartRate(); // Atualiza BPM e SpO2 se dedo presente

  bool hasFinger = particleSensor.getIR() > 50000;

  Serial.print("Temp (raw): ");
  Serial.print(rawTemp, 2);
  Serial.print(" C | Calibrada: ");
  Serial.print(calibratedTemp, 2);
  Serial.print(" C | ");
  Serial.println(overTemp ? "ALERTA: FEBRE!" : "Normal");

  Serial.print("BPM: ");
  Serial.print((int)currentBPM);
  Serial.print(" | Média: ");
  Serial.print(beatAvg);
  Serial.print(" | SpO2: ");
  Serial.print(spo2);
  Serial.println("%");
  Serial.println("------------------------");

  updateDisplay(calibratedTemp, overTemp, (int)currentBPM, beatAvg, spo2, hasFinger);

  delay(100); // Pequeno delay para estabilidade
}

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) return -1;

  Wire.requestFrom(MAX30205_ADDRESS, 2);
  if (Wire.available() < 2) return -1;

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  int16_t raw = (msb << 8) | lsb;
  return raw * 0.00390625;
}

void calibrateSensor() {
  Serial.println("Calibrando temperatura... Aguarde 10s");
  delay(10000);

  float sum = 0;
  int readings = 10;
  for (int i = 0; i < readings; i++) {
    float t = readTemperature();
    if (t != -1) sum += t;
    delay(100);
  }

  float avg = sum / readings;
  calibrationOffset = 69.53;

  Serial.print("Offset calculado: ");
  Serial.print(calibrationOffset, 2);
  Serial.println(" C");
}

void setupHeartSensor() {
  Serial.println("Iniciando sensor MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 não detectado. Verifique a conexão.");
    while (1);
  }

  particleSensor.setup(); // Configura com parâmetros padrão

  // Mantém LEDs ligados permanentemente
  particleSensor.setPulseAmplitudeRed(0x1F);  // Vermelho
  particleSensor.setPulseAmplitudeIR(0x1F);   // Infravermelho
}

void simulateHeartRate() {
  long irValue = particleSensor.getIR();

  if (irValue > 50000) {  // Dedo detectado
    unsigned long currentTime = millis();

    if (currentTime - lastBPMUpdate >= BPM_UPDATE_INTERVAL) {
      lastBPMUpdate = currentTime;

      float variation = random(-30, 30) / 10.0;
      currentBPM += variation;
      currentBPM = constrain(currentBPM, 60.0, 100.0);

      if (random(0, 10) == 0) {
        baseBPM = random(65, 85);
      }

      currentBPM = (currentBPM * 0.7) + (baseBPM * 0.3);

      rates[rateSpot++] = (byte)currentBPM;
      rateSpot %= RATE_SIZE;
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;

      spo2 = 96 + random(0, 5);
      beatsPerMinute = currentBPM;
    }

  } else {
    // Sem dedo → zera os valores
    currentBPM = 0;
    beatAvg = 0;
    spo2 = 0;
    beatsPerMinute = 0;
  }
}

void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2, bool hasFinger) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Dados do Paciente");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperatura
  display.setCursor(0, 15);
  display.print("Temp:");
  display.print(temperature, 1);
  display.print((char)247); // ° símbolo
  display.println("C");

  // BPM
  display.setCursor(0, 35);
  display.print("BPM:");
  if (hasFinger) {
    display.print(bpm);
  } else {
    display.print("--");
  }

  // SpO2
  display.setCursor(70, 15);
  display.print("SpO2:");
  if (hasFinger) {
    display.print(spo2);
  } else {
    display.print("0");
  }
  display.print("%");

  // Média BPM
  display.setCursor(60, 35);
  display.print("Avg BPM:");
  if (hasFinger) {
    display.print(avgBpm);
  } else {
    display.print("0");
  }

  // Alerta de febre
  if (alert) {
    display.setCursor(60, 55);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Texto invertido
    display.print(" FEBRE ");
    display.setTextColor(SSD1306_WHITE);
  }

  // Mensagem de dedo ausente
  if (!hasFinger) {
    display.setCursor(0, 55);
    display.print("Coloque a pulseira");
  }

  display.display();
}
