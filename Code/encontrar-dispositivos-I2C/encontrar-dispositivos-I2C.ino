#include <Wire.h>

void setup() {
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22
  Serial.begin(115200);
  Serial.println("Scanner I2C...");
}

void loop() {
  byte error, address;
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Dispositivo encontrado: 0x");
      Serial.println(address, HEX);
    }
  }
  delay(5000);
}