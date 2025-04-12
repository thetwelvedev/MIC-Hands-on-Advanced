#include <Wire.h>

void setup() {
  Wire.begin(21, 22); // SDA = 21, SCL = 22
  Serial.begin(115200);
  delay(1000);
  Serial.println("Escaneando dispositivos I2C...");

  byte count = 0;
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Dispositivo I2C encontrado no endereço 0x");
      Serial.println(address, HEX);
      count++;
      delay(10);
    }
  }
  if (count == 0)
    Serial.println("Nenhum dispositivo I2C encontrado.");
  else
    Serial.println("Scan completo.");
}

void loop() {
}