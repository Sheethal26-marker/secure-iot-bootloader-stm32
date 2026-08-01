#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "CDAC";
const char *password = "s";

const char *url =
    "http://192.168.4.154:8000/firmware_info.json";

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

    http.begin(url);

    int code = http.GET();

    if (code != 200)
    {
        Serial.print("HTTP Error: ");
        Serial.println(code);

        http.end();
        return;
    }

    String json = http.getString();

    Serial.println("JSON received:");
    Serial.println(json);

    JsonDocument doc;

    DeserializationError error =
        deserializeJson(doc, json);

    if (error)
    {
        Serial.print("JSON Error: ");
        Serial.println(error.c_str());

        http.end();
        return;
    }

    int version =
        doc["version"];

    const char *firmware =
        doc["firmware"];

    int size =
        doc["size"];

    const char *crc =
        doc["crc32"];

    const char *sha =
        doc["sha256"];

    const char *algorithm =
        doc["signature_algorithm"];

    const char *signature =
        doc["signature"];

    Serial.println();
    Serial.println("===== METADATA =====");

    Serial.print("Version: ");
    Serial.println(version);

    Serial.print("Firmware: ");
    Serial.println(firmware);

    Serial.print("Size: ");
    Serial.println(size);

    Serial.print("CRC32: ");
    Serial.println(crc);

    Serial.print("SHA256: ");
    Serial.println(sha);

    Serial.print("Signature Algorithm: ");
    Serial.println(algorithm);

    Serial.print("Signature: ");
    Serial.println(signature);

    Serial.println("====================");

    http.end();
}

void loop()
{
}
