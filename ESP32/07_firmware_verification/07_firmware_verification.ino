#include <Arduino.h>

/* Firmware Buffer */
#define FIRMWARE_SIZE 1024

uint8_t firmwareBuffer[FIRMWARE_SIZE];

/* Expected CRC from firmware_info.json */
uint32_t expectedCRC = 0xA1B2C3D4;

/* Function Prototypes */
uint32_t calculateCRC32(uint8_t *data, uint32_t length);
bool verifyFirmware();

/* Setup Function */
void setup()
{
    Serial.begin(115200);

    Serial.println("--------------------------------");
    Serial.println("Firmware Verification Started");
    Serial.println("--------------------------------");

    /* Example firmware data */
    for (int i = 0; i < FIRMWARE_SIZE; i++)
    {
        firmwareBuffer[i] = i;
    }

    if (verifyFirmware())
    {
        Serial.println("Firmware Verification SUCCESS");
    }
    else
    {
        Serial.println("Firmware Verification FAILED");
    }
}

void loop()
{
}

/* Verify Firmware */
bool verifyFirmware()
{
    uint32_t calculatedCRC;

    calculatedCRC = calculateCRC32(firmwareBuffer, FIRMWARE_SIZE);

    Serial.print("Expected CRC32   : 0x");
    Serial.println(expectedCRC, HEX);

    Serial.print("Calculated CRC32 : 0x");
    Serial.println(calculatedCRC, HEX);

    if (calculatedCRC == expectedCRC)
    {
        return true;
    }

    return false;
}

/* CRC32 Calculation */
uint32_t calculateCRC32(uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return ~crc;
}
