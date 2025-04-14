#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; // Média de 4 leituras
byte rates[RATE_SIZE];     // Array de leituras de batimentos
byte rateSpot = 0;
long lastBeat = 0;        // Tempo do último batimento

float beatsPerMinute;
int beatAvg;

void setup() {
  Serial.begin(115200);
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

void loop() {
  long irValue = particleSensor.getIR();

  // Verificar se há dedo no sensor
  if (irValue > 50000) {
    // Detectando batimento cardíaco
    if (checkForBeat(irValue) {
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

    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.println(beatAvg);
  } else {
    // Sem dedo no sensor
    beatAvg = 0;
    beatsPerMinute = 0;
    rateSpot = 0;
    lastBeat = 0;
    
    for (byte x = 0; x < RATE_SIZE; x++)
      rates[x] = 0;
      
    Serial.println("Sem dedo no sensor...");
  }

  delay(20); // Pequeno atraso para estabilidade
}
/*
Pré-requisitos:
Biblioteca "MAX30105 by SparkFun" (instale via Arduino Library Manager)

Conexões corretas entre o ESP32 e o MAX30102:

VIN: 3.3V do ESP32

GND: GND do ESP32

SDA: Pino GPIO21 (padrão)

SCL: Pino GPIO22 (padrão)
*/