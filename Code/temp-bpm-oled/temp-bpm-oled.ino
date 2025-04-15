#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// Configurações do MAX30205
const uint8_t MAX30205_ADDRESS = 0x48;
const int OS_PIN = 23;

// Configurações do OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Configurações do MAX30102
MAX30105 particleSensor;
const byte RATE_SIZE = 4; // Média de 4 leituras
byte rates[RATE_SIZE];     // Array de leituras de batimentos
byte rateSpot = 0;
long lastBeat = 0;        // Tempo do último batimento
float beatsPerMinute;
int beatAvg;

// Variáveis de calibração
float calibrationOffset = 37.0;

// Declarações antecipadas das funções
float readTemperature();
void calibrateSensor();
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm);
void setupHeartSensor();
void readHeartRate();

void setup() {
  Serial.begin(115200);
  
  // Inicializa I2C para os sensores
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
  
  // Inicializa o pino OS do MAX30205
  pinMode(OS_PIN, INPUT);
  
  // Inicializa o display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na alocação do SSD1306"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Inicializando...");
  display.display();
  delay(2000);
  
  calibrateSensor();
  setupHeartSensor();
}

void loop() {
  // Ler temperatura
  float rawTemp = readTemperature();
  float calibratedTemp = rawTemp + calibrationOffset;
  bool overTemp = digitalRead(OS_PIN);
  
  // Ler batimento cardíaco
  readHeartRate();
  
  // Atualizar display
  updateDisplay(calibratedTemp, overTemp, (int)beatsPerMinute, beatAvg);
  
  delay(100);
}

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    return -1;
  }
  
  Wire.requestFrom(MAX30205_ADDRESS, 2);
  if (Wire.available() >= 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    int16_t raw = (msb << 8) | lsb;
    return raw * 0.00390625;
  }
  return -1;
}

void calibrateSensor() {
  Serial.println("Calibrando sensor... aguarde 10 segundos");
  delay(10000);
  
  float sum = 0;
  int readings = 10;
  
  for (int i = 0; i < readings; i++) {
    float temp = readTemperature();
    if (temp != -1) {
      sum += temp;
    }
    delay(100);
  }
  
  float avgRawTemp = sum / readings;
  calibrationOffset = 25.0 - avgRawTemp;
  
  Serial.print("Offset de calibração calculado: ");
  Serial.print(calibrationOffset, 2);
  Serial.println(" °C");
}

void setupHeartSensor() {
  Serial.println("Inicializando sensor MAX30102...");

  // Inicializar o sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 não encontrado. Verifique as conexões!");
    while (1);
  }

  // Configurar o sensor
  particleSensor.setup(); // Padrão: LED brilho=50, amostra média=4, amostras/s=100
  particleSensor.setPulseAmplitudeRed(0x0A); // Reduzir o brilho do LED vermelho
  particleSensor.setPulseAmplitudeGreen(0);  // Desligar LED verde (não usado)

  Serial.println("Coloque seu dedo no sensor com leve pressão...");
}

void readHeartRate() {
  long irValue = particleSensor.getIR();

  // Verificar se há dedo no sensor
  if (irValue > 50000) {
    // Detectando batimento cardíaco
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        // Calcular média
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  } else {
    // Sem dedo no sensor
    beatAvg = 0;
    beatsPerMinute = 0;
    rateSpot = 0;
    lastBeat = 0;
    
    for (byte x = 0; x < RATE_SIZE; x++)
      rates[x] = 0;
  }
}

void updateDisplay(float temperature, bool alert, int bpm, int avgBpm) {
  display.clearDisplay();
  
  // Cabeçalho
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Monitor de Saude");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  
  // Temperatura
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.print("Temp: ");
  display.setTextSize(2);
  display.print(temperature, 1);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.println("C");
  
  // Batimento cardíaco
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
  
  // Status
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("Status: ");
  if (alert) {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.print("ALERTA!");
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.print("Normal");
  }
  
  display.display();
}