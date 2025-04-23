#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// Endereço I2C do MAX30205
const uint8_t MAX30205_ADDRESS = 0x48;

// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objeto do sensor MAX30102
MAX30105 particleSensor;

// Buffer para cálculo de média dos batimentos
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// Variáveis para SpO2
#define MAX_BRIGHTNESS 255
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

// Variável de calibração da temperatura
float calibrationOffset = 37.0;

// Funções auxiliares
float readTemperature();
void calibrateSensor();
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2);
void setupHeartSensor();
void readHeartRate();
void readSpO2();

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
  readSpO2();     // Atualiza SpO2

  Serial.print("Temp (raw): ");
  Serial.print(rawTemp, 2);
  Serial.print(" C | Calibrada: ");
  Serial.print(calibratedTemp, 2);
  Serial.print(" C | ");
  Serial.println(overTemp ? "ALERTA: FEBRE!" : "Normal");

  Serial.print("BPM: ");
  Serial.print((int)beatsPerMinute);
  Serial.print(" | Média: ");
  Serial.print(beatAvg);
  Serial.print(" | SpO2: ");
  Serial.print(spo2);
  Serial.println("%");
  Serial.println("------------------------");

  updateDisplay(calibratedTemp, overTemp, (int)beatsPerMinute, beatAvg, spo2);

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

  // Configuração para SpO2
  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.enableDIETEMPRDY();

  Serial.println("Aproxime o dedo do sensor.");
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

void readSpO2() {
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++) {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1) {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++) {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++) {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    
    // Se não temos um valor válido, saímos do loop
    if (validSPO2 && validHeartRate) {
      break;
    }
  }
}

void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Dados do Paciente");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperatura
  display.setCursor(0, 15);
  display.setTextSize(1);
  display.print("Temp:");
  display.setTextSize(1);
  display.print(temperature, 1);
  display.print((char)247); // ° símbolo
  display.println("C");

  // SpO2
  display.setTextSize(1);
  display.setCursor(70, 15);
  display.print("SpO2:");
  display.setTextSize(1);
  display.print(spo2);
  display.println("%");

  // BPM
  display.setTextSize(1);
  display.setCursor(1, 30);
  display.print("BPM:");
  display.setTextSize(0);
  display.print(bpm);

  // Média BPM
  display.setTextSize(1);
  display.setCursor(70, 30);
  display.print("Avg BPM:");
  display.print(avgBpm);

  // Alerta de temperatura alta
  if (alert) {
    display.setTextSize(1);
    display.setCursor(70, 50);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Texto invertido
    display.print(" FEBRE ");
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();
}