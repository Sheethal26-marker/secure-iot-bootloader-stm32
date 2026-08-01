#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "YOUR_WIFI_NAME";
const char *password = "YOUR_WIFI_PASSWORD";

const char *firmwareURL =
  "http://192.168.1.10:8000/firmware.bin";

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");

  HTTPClient http;

  http.begin(firmwareURL);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);

    http.end();
    return;
  }

  int firmwareSize =
    http.getSize();

  Serial.print("Firmware Size: ");
  Serial.println(firmwareSize);

  WiFiClient *stream =
    http.getStreamPtr();

  uint8_t buffer[1024];

  int totalReceived = 0;

  Serial.println("Downloading Firmware...");

  while (http.connected() &&
         (firmwareSize < 0 ||
          totalReceived < firmwareSize))
  {
    size_t available =
      stream->available();

    if (available)
    {
      int bytesToRead =
        min((int)available,
            (int)sizeof(buffer));

      int bytesRead =
        stream->readBytes(
          buffer,
          bytesToRead);

      totalReceived += bytesRead;

      Serial.print("Downloaded: ");
      Serial.print(totalReceived);

      if (firmwareSize > 0)
      {
        Serial.print("/");
        Serial.print(firmwareSize);
      }

      Serial.println(" bytes");

      /*
       * Later:
       *
       * buffer
       * bytesRead
       *
       * will be passed to the UART
       * packet transmission function.
       */
    }
