#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid =
    "CDAC";

const char *password =
    "321";

const char *mqttServer =
    "172.24.184.226";



WiFiClient espClient;

PubSubClient client(espClient);

void callback(
    char *topic,
    byte *payload,
    unsigned int length)
{
    Serial.print("Topic: ");
    Serial.println(topic);

    Serial.print("Message: ");

    for (unsigned int i = 0;
         i < length;
         i++)
    {
        Serial.print((char)payload[i]);
    }

    Serial.println();
}

void connectMQTT()
{
    while (!client.connected())
    {
        Serial.println(
            "Connecting MQTT...");

        String clientId =
            "ESP32-" +
            String((uint32_t)ESP.getEfuseMac(), HEX);

        if (client.connect(clientId.c_str()))
        {
            Serial.println(
                "MQTT Connected");

            client.subscribe(
                "firmware/update");

            Serial.println(
                "Subscribed firmware/update");
        }
        else
        {
            Serial.print("Failed: ");
            Serial.println(client.state());

            delay(2000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    Serial.println("WiFi Connected");

    client.setServer(
        mqttServer,
        1883);

    client.setCallback(callback);

    connectMQTT();
}

void loop()
{
    if (!client.connected())
    {
        connectMQTT();
    }

    client.loop();
}
