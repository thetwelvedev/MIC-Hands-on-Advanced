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
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// Variável de calibração da temperatura
float calibrationOffset = 37.0;

// Funções auxiliares
float readTemperature();
void calibrateSensor();
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm);
void setupHeartSensor();
void readHeartRate();

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
  float calibratedTemp = rawTemp + calibrationOffset;
  bool overTemp = calibratedTemp >= 38.0; // Febre se >= 38 °C

  readHeartRate(); // Atualiza BPM e média

  Serial.print("Temp (raw): ");
  Serial.print(rawTemp, 2);
  Serial.print(" C | Calibrada: ");
  Serial.print(calibratedTemp, 2);
  Serial.print(" C | ");
  Serial.println(overTemp ? "ALERTA: FEBRE!" : "Normal");

  Serial.print("BPM: ");
  Serial.print((int)beatsPerMinute);
  Serial.print(" | Média: ");
  Serial.println(beatAvg);
  Serial.println("------------------------");

  updateDisplay(calibratedTemp, overTemp, (int)beatsPerMinute, beatAvg);

  delay(1000); // Atualização a cada 1s
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
  calibrationOffset = 25.0 - avg;

  Serial.print("Offset calculado: ");
  Serial.print(calibrationOffset, 2);
  Serial.println(" C");
}

void setupHeartSensor() {
  Serial.println("Iniciando sensor MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Erro: MAX30102 não encontrado.");
    while (true);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // LED vermelho
  particleSensor.setPulseAmplitudeGreen(0);  // Desliga LED verde

  Serial.println("Aproxime o dedo do sensor.");
}

void readHeartRate() {
  long irValue = particleSensor.getIR();

  if (irValue > 50000) {
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute > 20 && beatsPerMinute < 255) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        int total = 0;
        for (byte i = 0; i < RATE_SIZE; i++) total += rates[i];
        beatAvg = total / RATE_SIZE;
      }
    }
  } else {
    beatsPerMinute = 0;
    beatAvg = 0;
    rateSpot = 0;
    lastBeat = 0;
    for (byte i = 0; i < RATE_SIZE; i++) rates[i] = 0;
  }
}

void updateDisplay(float temperature, bool alert, int bpm, int avgBpm) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Dados do Paciente");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperatura
  display.setCursor(0, 15);
  display.setTextSize(1);
  display.print("Temp: ");
  display.setTextSize(2);
  display.print(temperature, 1);
  display.print((char)247); // ° símbolo
  display.println("C");

  // BPM
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("BPM: ");
  display.setTextSize(2);
  display.print(bpm);

  display.setTextSize(1);
  display.setCursor(70, 35);
  display.print("Avg: ");
  display.setTextSize(2);
  display.print(avgBpm);

  // Alerta de temperatura alta
  if (alert) {
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Texto invertido
    display.print(" Febre detectada! ");
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();
}
