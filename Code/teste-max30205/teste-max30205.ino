#include <Wire.h>

const uint8_t MAX30205_ADDRESS = 0x48;
const int OS_PIN = 23;

// Variáveis de calibração
float calibrationOffset = 37.0; // Valor inicial para compensar os -37°C

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
  
  pinMode(OS_PIN, INPUT);
  Serial.println("Testando MAX30205...");
  
  // Calibração inicial (opcional - pode ser ajustado manualmente)
  calibrateSensor();
}

void loop() {
  float rawTemp = readTemperature();
  float calibratedTemp = rawTemp + calibrationOffset;
  bool overTemp = digitalRead(OS_PIN);

  if (rawTemp == -1) {
    Serial.println("Erro na leitura do sensor!");
  } else {
    Serial.print("Temperatura Bruta: ");
    Serial.print(rawTemp, 2);
    Serial.print(" °C | Temperatura Calibrada: ");
    Serial.print(calibratedTemp, 2);
    Serial.print(" °C | Alerta OS: ");
    Serial.println(overTemp ? "CRÍTICO!" : "Normal");
  }
  delay(1000);
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
  // Método 1: Calibração manual com valor conhecido
  // Peça ao usuário para inserir a temperatura ambiente real
  // e calcule o offset
  
  // Método 2: Auto-calibração (assumindo que o sensor está em ambiente estável)
  Serial.println("Calibrando sensor... aguarde 10 segundos");
  delay(10000); // Espera para estabilização
  
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
  calibrationOffset = 25.0 - avgRawTemp; // Assumindo 25°C como temperatura ambiente
  
  Serial.print("Offset de calibração calculado: ");
  Serial.print(calibrationOffset, 2);
  Serial.println(" °C");
}
/*
Conexões do CJMU-30205:

VCC: 3.3V do ESP32
GND: GND do ESP32
SDA: Pino GPIO21
SCL: Pino GPIO22 
A0: GND do ESP32
A1: GND do ESP32
A2: GND do ESP32
*/