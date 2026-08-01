#include <WiFi.h>

const char *ssid = "wifi_username";
const char *password = "password";

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");

  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
