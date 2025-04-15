#include <Wire.h>

const uint8_t MAX30205_ADDRESS = 0x48; // Altere se necessário
const int OS_PIN = 23; // Pino OS (opcional)

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
  
  pinMode(OS_PIN, INPUT);
  Serial.println("Testando MAX30205...");
}

void loop() {
  float temp = readTemperature();
  bool overTemp = digitalRead(OS_PIN);

  if (temp == -1) {
    Serial.println("Erro na leitura do sensor!");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(temp, 2);
    Serial.print(" °C | Alerta OS: ");
    Serial.println(overTemp ? "CRÍTICO!" : "Normal");
  }
  delay(1000);
}

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00); // Registro da temperatura
  if (Wire.endTransmission(false) != 0) {
    return -1; // Erro na comunicação
  }
  
  Wire.requestFrom(MAX30205_ADDRESS, 2);
  if (Wire.available() >= 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    int16_t raw = (msb << 8) | lsb;
    return raw * 0.00390625; // Conversão para °C
  }
  return -1; // Erro na leitura
}