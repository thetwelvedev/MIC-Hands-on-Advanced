#include <Wire.h>

// Configuração dos pinos do MAX30205
const int OS_PIN = 23; // Pino conectado ao OS (opcional)
const uint8_t MAX30205_ADDRESS = 0x48; // Altere se A0/A1/A2 estão em HIGH

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA (GPIO21), SCL (GPIO22) no ESP32

  // Configura o pino OS como entrada (para alerta de temperatura)
  pinMode(OS_PIN, INPUT);

  Serial.println("Iniciando sensor MAX30205...");
}

void loop() {
  float temperature = readTemperature();
  bool overTemp = digitalRead(OS_PIN); // Lê o estado do pino OS (HIGH = temperatura crítica)

  Serial.print("Temperatura: ");
  Serial.print(temperature, 2);
  Serial.print(" °C | Alerta (OS): ");
  Serial.println(overTemp ? "CRÍTICO!" : "Normal");

  delay(1000);
}

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00); // Registro da temperatura
  Wire.endTransmission(false);
  Wire.requestFrom(MAX30205_ADDRESS, 2); // Lê 2 bytes

  if (Wire.available() >= 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    int16_t raw = (msb << 8) | lsb;
    return raw * 0.00390625; // Converte para °C (16 bits, 0.00390625 °C/LSB)
  }
  return -1; // Retorna -1 em caso de erro
}
/*

Pino MAX30205	Conexão no ESP32
VCC	3.3V
GND	GND
SDA	GPIO21
SCL	GPIO22
OS	GPIO23 (opcional - para alerta de temperatura)
A0, A1, A2	GND ou VCC (para definir endereço I2C)
*/