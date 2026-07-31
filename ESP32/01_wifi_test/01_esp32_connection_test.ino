void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Started");
  Serial.println("Secure IoT Bootloader Project");
}

void loop()
{
  Serial.println("ESP32 Running...");
  delay(2000);
}
