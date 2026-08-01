#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "username";
const char *password = "password";

const char *url =
  "http://:8000/firmware_info.json";

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");

  HTTPClient http;

  Serial.println("Connecting to OTA Server...");

  http.begin(url);

  int httpCode = http.GET();

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK)
  {
    String response = http.getString();

    Serial.println("Server Connection Successful");
    Serial.println("Response:");

    Serial.println(response);
  }
  else
  {
    Serial.println("Server Connection Failed");

    Serial.print("Error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

void loop()
{
}
