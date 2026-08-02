#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// ======================================================
// USER SETTINGS
// ======================================================

const char *ssid = "WIFI_NAME";
const char *password = "WIFI_PASSWORD";

// Windows PC running Python HTTP server
const char *metadataURL =
    "http://wifi_ip_address:8000/firmware_info.json";

const char *firmwareURL =
    "http://wifi_ip_:8000/firmware.bin";

// Mosquitto broker address
// Configure this separately because Mosquitto is in WSL.
const char *mqttServer = "MQTT_BROKER_IP";

const int mqttPort = 1883;

const char *mqttTopic =
    "firmware/update";

// ======================================================
// MQTT
// ======================================================

WiFiClient mqttWiFiClient;
PubSubClient mqttClient(mqttWiFiClient);

// ======================================================
// FIRMWARE METADATA
// ======================================================

int firmwareVersion = 0;
long firmwareSize = 0;

String firmwareName;
String expectedCRC;
String expectedSHA;
String signatureAlgorithm;
String digitalSignature;

bool metadataReady = false;
bool updateRequested = false;

// ======================================================
// 01 SERIAL
// ======================================================

void startSerial()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.println(" Secure IoT Bootloader ESP32");
    Serial.println("==============================");
}

// ======================================================
// 02 WIFI
// ======================================================

bool connectWiFi()
{
    Serial.println();
    Serial.println("[02] WiFi");

    Serial.print("Connecting");

    WiFi.begin(ssid, password);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);

        Serial.print(".");

        if (millis() - start > 20000)
        {
            Serial.println();
            Serial.println("WiFi connection timeout");

            return false;
        }
    }

    Serial.println();
    Serial.println("WiFi Connected");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    return true;
}

// ======================================================
// 03 HTTP TEST
// ======================================================

bool testHTTPServer()
{
    Serial.println();
    Serial.println("[03] HTTP Server Test");

    HTTPClient http;

    if (!http.begin(metadataURL))
    {
        Serial.println("HTTP begin failed");
        return false;
    }

    int code = http.GET();

    Serial.print("HTTP Code: ");
    Serial.println(code);

    if (code != HTTP_CODE_OK)
    {
        Serial.println("HTTP Server Test FAILED");

        if (code < 0)
        {
            Serial.print("Error: ");
            Serial.println(
                http.errorToString(code));
        }

        http.end();

        return false;
    }

    Serial.println("HTTP Server Test PASSED");

    http.end();

    return true;
}

// ======================================================
// 04 JSON
// ======================================================

bool downloadMetadata()
{
    Serial.println();
    Serial.println("[04] Firmware Metadata");

    metadataReady = false;

    HTTPClient http;

    if (!http.begin(metadataURL))
    {
        Serial.println("HTTP begin failed");
        return false;
    }

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        Serial.print("Metadata HTTP Error: ");
        Serial.println(code);

        http.end();

        return false;
    }

    String json = http.getString();

    Serial.println("JSON downloaded:");

    Serial.println(json);

    JsonDocument doc;

    DeserializationError error =
        deserializeJson(doc, json);

    if (error)
    {
        Serial.print("JSON Parse Failed: ");

        Serial.println(
            error.c_str());

        http.end();

        return false;
    }

    // Check required fields

    if (!doc["version"].is<int>() ||
        !doc["firmware"].is<const char *>() ||
        !doc["size"].is<long>() ||
        !doc["crc32"].is<const char *>() ||
        !doc["sha256"].is<const char *>() ||
        !doc["signature_algorithm"].is<const char *>() ||
        !doc["signature"].is<const char *>())
    {
        Serial.println(
            "Required metadata missing");

        http.end();

        return false;
    }

    firmwareVersion =
        doc["version"];

    firmwareName =
        doc["firmware"].as<String>();

    firmwareSize =
        doc["size"];

    expectedCRC =
        doc["crc32"].as<String>();

    expectedSHA =
        doc["sha256"].as<String>();

    signatureAlgorithm =
        doc["signature_algorithm"].as<String>();

    digitalSignature =
        doc["signature"].as<String>();

    Serial.println();
    Serial.println("----- Metadata -----");

    Serial.print("Version: ");
    Serial.println(firmwareVersion);

    Serial.print("Firmware: ");
    Serial.println(firmwareName);

    Serial.print("Size: ");
    Serial.println(firmwareSize);

    Serial.print("CRC32: ");
    Serial.println(expectedCRC);

    Serial.print("SHA256: ");
    Serial.println(expectedSHA);

    Serial.print("Signature algorithm: ");
    Serial.println(signatureAlgorithm);

    Serial.print("Signature: ");
    Serial.println(digitalSignature);

    Serial.println("--------------------");

    metadataReady = true;

    http.end();

    return true;
}

// ======================================================
// 05 MQTT CALLBACK
// ======================================================

void mqttCallback(
    char *topic,
    byte *payload,
    unsigned int length)
{
    Serial.println();
    Serial.println("[MQTT MESSAGE]");

    Serial.print("Topic: ");
    Serial.println(topic);

    String message;

    for (unsigned int i = 0;
         i < length;
         i++)
    {
        message += (char)payload[i];
    }

    message.trim();

    Serial.print("Message: ");
    Serial.println(message);

    if (message == "UPDATE_AVAILABLE")
    {
        Serial.println(
            "Firmware update requested");

        updateRequested = true;
    }
}

// ======================================================
// 05 MQTT CONNECTION
// ======================================================

bool connectMQTT()
{
    Serial.println();
    Serial.println("[05] MQTT");

    if (mqttClient.connected())
    {
        return true;
    }

    String clientId =
        "ESP32-" +
        String(
            (uint32_t)ESP.getEfuseMac(),
            HEX);

    Serial.print("Connecting to broker: ");
    Serial.print(mqttServer);
    Serial.print(":");
    Serial.println(mqttPort);

    if (!mqttClient.connect(
            clientId.c_str()))
    {
        Serial.print("MQTT Failed. State = ");

        Serial.println(
            mqttClient.state());

        return false;
    }

    Serial.println("MQTT Connected");

    if (mqttClient.subscribe(mqttTopic))
    {
        Serial.print("Subscribed: ");
        Serial.println(mqttTopic);
    }
    else
    {
        Serial.println(
            "MQTT subscribe failed");

        return false;
    }

    return true;
}

// ======================================================
// 06 FIRMWARE DOWNLOAD
// ======================================================

bool downloadFirmware()
{
    Serial.println();
    Serial.println("[06] Firmware Download");

    if (!metadataReady)
    {
        Serial.println(
            "Metadata is not available");

        return false;
    }

    HTTPClient http;

    if (!http.begin(firmwareURL))
    {
        Serial.println(
            "Firmware HTTP begin failed");

        return false;
    }

    int code = http.GET();

    Serial.print("HTTP Code: ");
    Serial.println(code);

    if (code != HTTP_CODE_OK)
    {
        Serial.println(
            "Firmware Download Failed");

        http.end();

        return false;
    }

    int httpSize =
        http.getSize();

    Serial.print("HTTP firmware size: ");
    Serial.println(httpSize);

    Serial.print("JSON firmware size: ");
    Serial.println(firmwareSize);

    WiFiClient *stream =
        http.getStreamPtr();

    uint8_t buffer[1024];

    long totalReceived = 0;

    unsigned long lastDataTime =
        millis();

    while (http.connected() &&
           (httpSize < 0 ||
            totalReceived < httpSize))
    {
        size_t available =
            stream->available();

        if (available > 0)
        {
            size_t bytesToRead =
                min(
                    available,
                    sizeof(buffer));

            int bytesRead =
                stream->readBytes(
                    buffer,
                    bytesToRead);

            if (bytesRead > 0)
            {
                totalReceived +=
                    bytesRead;

                lastDataTime =
                    millis();

                // ---------------------------------
                // IMPORTANT:
                //
                // buffer = firmware data
                // bytesRead = valid bytes
                //
                // Later:
                // CRC
                // SHA256
                // UART
                //
                // will be added here.
                // ---------------------------------

                Serial.print("\rDownloaded: ");
                Serial.print(totalReceived);

                if (httpSize > 0)
                {
                    Serial.print("/");
                    Serial.print(httpSize);
                }
            }
        }
        else
        {
            // Don't wait forever if server stops.

            if (millis() - lastDataTime >
                10000)
            {
                Serial.println();
                Serial.println(
                    "Firmware download timeout");

                http.end();

                return false;
            }

            delay(1);
        }
    }

    Serial.println();

    Serial.print("Received: ");
    Serial.println(totalReceived);

    bool success = true;

    if (httpSize >= 0 &&
        totalReceived != httpSize)
    {
        Serial.println(
            "HTTP size mismatch");

        success = false;
    }

    if (firmwareSize > 0 &&
        totalReceived != firmwareSize)
    {
        Serial.println(
            "JSON size mismatch");

        success = false;
    }

    if (success)
    {
        Serial.println(
            "Firmware Download SUCCESS");
    }
    else
    {
        Serial.println(
            "Firmware Download INVALID");
    }

    http.end();

    return success;
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
    // 01
    startSerial();

    // 02
    if (!connectWiFi())
    {
        Serial.println(
            "STOP: WiFi failed");

        return;
    }

    // 03
    if (!testHTTPServer())
    {
        Serial.println(
            "STOP: HTTP failed");

        return;
    }

    // 04
    if (!downloadMetadata())
    {
        Serial.println(
            "STOP: Metadata failed");

        return;
    }

    // 05

    mqttClient.setServer(
        mqttServer,
        mqttPort);

    mqttClient.setCallback(
        mqttCallback);

    if (!connectMQTT())
    {
        Serial.println();
        Serial.println(
            "MQTT unavailable.");

        Serial.println(
            "HTTP + metadata are working.");

        /*
         * We do NOT stop the ESP32 here.
         * loop() will try MQTT again.
         */
    }

    Serial.println();
    Serial.println("==============================");
    Serial.println(" ESP32 READY");
    Serial.println("==============================");

    Serial.println(
        "Waiting for UPDATE_AVAILABLE...");
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
    // Keep WiFi alive

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(
            "WiFi disconnected");

        connectWiFi();
    }

    // Keep MQTT alive

    if (!mqttClient.connected())
    {
        static unsigned long lastRetry = 0;

        if (millis() - lastRetry > 5000)
        {
            lastRetry = millis();

            connectMQTT();
        }
    }
    else
    {
        mqttClient.loop();
    }

    // MQTT requested an update

    if (updateRequested)
    {
        updateRequested = false;

        Serial.println();
        Serial.println(
            "===== UPDATE START =====");

        // Download latest metadata again.
        // Don't use old metadata.

        if (!downloadMetadata())
        {
            Serial.println(
                "Update cancelled: metadata failed");

            return;
        }

        if (!downloadFirmware())
        {
            Serial.println(
                "Update cancelled: download failed");

            return;
        }

        Serial.println();
        Serial.println(
            "Firmware received successfully.");

        Serial.println(
            "Next stage: CRC + SHA256 + UART");

        Serial.println(
            "===== UPDATE END =====");
    }
}
